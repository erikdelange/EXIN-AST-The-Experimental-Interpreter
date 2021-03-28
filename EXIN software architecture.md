# EXIN Software Architecture

The interpreter implements the language grammar as described in [EXIN EBNF Syntax Diagram](EXIN%20syntax%20diagram.pdf). The interpreters design is kept simple so its workings can be easily understood.

##### Running the interpreter
When starting the interpreter with the -h argument - and if it has been compiled with the DEBUG macro (-D DEBUG) - the following message is printed.
```
> .\exin -h
EXIN version 2.00
usage: exin-ast.exe [options] module
module: name of file containing code to execute
options
-d[detail] = show debug info
    detail = sum of options (default = 8)
    option  0: no debug output
    option  1: show tokens during parsing
    option  2: show memory allocation
    option  4: show abstract syntax tree after parsing and stop
    option  8: show abstract syntax tree after parsing and execute
    option 16: dump identifier and object table to stdout after program end
    option 32: dump identifier and object table to disk after program end
-h = show usage information
-t[tabsize] = set tab size in spaces
    tabsize = >= 1 (default = 4)
-v = show version information
```
By specifying a module it is loaded and executed. The module name must include its extension (if any), the interpreter does not guess.

##### Workings
The key element in the interpreter is an abstract syntax tree (AST). This is a tree representation of the source code where every node in the tree describes a language construct. For example the *if .. then .. else ..* statement is represented by a node with three branches; the condition, the branch to execute when the condition is true, and an optional branch in case the condition is false. It is 'abstract' in the sense that syntactic elements such as parenthesis, comma's or indents are not needed here.
The node data structure is central to the AST. It is a struct containing (a.o.) a union to store the statement-dependent data elements; for example *if .. then .. else ..* requires different data then a plain *return* (3 branches versus a single branch with an optional return value).
The first step in the interpretation process is to create the AST. This is done by the parser (function *parse()*). For this purpose the parser uses a scanner (or lexer) to translate the source code into a stream of tokens. A token is a group of characters which have a special meaning in the language. For example the *while* statement or floating point constant *5.1E3*. A one token look-ahead is used (a LL(1) parser). The parser digests these tokens, verifies if they match the language syntax and creates the AST. The next step is to do a number of semantic checks on the AST, for example whether variables are defined before they are used (function check() in visit.c). After these checks the AST is executed (function visit() in visit.c).
Separating between parsing (and syntax checking), semantic checking and execution creates - in my view - cleaner code, as you don't have to combine all three tasks into a single function. Also, any check done upfront does not need to be repeated during execution.

###### Visitor pattern
Printing, checking and executing is done using the visitor pattern. Calling a node, for example to print itself, is done in a universal way. The node itself - remember its content differs per statement - knows what to print and if child nodes must be called for printing. This pattern is used in the print(), check() and visit() functions in the node struct. When visiting a node sometimes there are parameters or a return value and sometimes not. To keep the function signature uniform these values are transferred via a stack (see stack.c).
Using an AST with a visitor pattern also has a disadvantage; implementing statements which do not follow the normal sequence of operations - like *break* or *continue* - is cumbersome as you have to travel back through the stack of calls to visit().

###### Adding new features
New functions can be added easily to the language by creating them in function.c. Adding new language constructs - for example an *elif* statement in *if .. then .. else ..* is a bit more complex and requires changes in ast.h, ast.c, parse.c and visit.c.

###### Testing
An interpreter still is a complex piece of software and an error is easily made. To catch these I've created hundreds of test scripts in the language. After every change to the interpreter all scripts are executed, and their actual output is compared with the expected output. In this way bugs are caught quickly. If the tests missed something I just add a new script. I've created a separate piece of software (in Python, see [here](https://github.com/erikdelange/EXIN-Test-Suite-Management)) to record and execute all the tests.

###### Efficiency
Using an AST make the interpreter fairly efficient as source code only needs to be read and decoded once. Only the retrieval of identifiers could be done more efficient as they are stored as strings. This means variable names are searched in the identifier lists every time. Some interpreters first translate names in shorter (e.g. one- or two-byte) versions before starting interpretation to speeds up things. However the aim for this interpreter was simplicity and not speed.

##### Variables
Function names and variables are stored in linked lists with their identifiers. Globals *global* and *local* in *identifier.c* point to the respective lists with identifiers. An exception are the names of built-in functions, these are defined in *function.c*.
An identifier is just a name (ie. a string). The value which belongs to a variable is stored separately in an object. This allows an identifier to point to any type of value. This feature is used in the *for .. in* statement where a single identifier refers to a different object per iteration. Using a uniform way to store values makes operations on variables easy to code. Because all values are objects they can also be used during expression evaluation. The generic functions to do unary and binary operations on objects can be found in *object.c*. Actually the *obj_...()* functions are wrappers. For each type of variable a separate C file with the supported operations exists. See *number.c*, *str.c* and *list.c* for the details and note that not every object supports all operations. The obj_...() wrappers just call functions in these files.
A special object is *none*. *None* is used as a return value when a function (presumably because of an error) cannot return a value.

###### Code structure
The code is structured along the three steps mentioned above (parse, check, visit). Parsing is supported by parse.c, scanner.c, module.c and ast.c. Semantic checking is supported in visit.c, and execution by visit.c and object.c plus the underlying objects.
![EXIN-software-structure.png](https://github.com/erikdelange/EXIN-AST-The-Experimental-Interpreter/blob/master/EXIN-software-structure.png)

##### Notes on coding

###### Include files
If a source file requires a header (*.h*) file, this has the same basename (*module.c, module.h*). Every header file has a guard (_BASENAME_) to prevent double inclusion. Every source or header file only includes the headers it needs, so no 'include all' approach.

###### Assertions
Certain bugs such as NULL pointers can terminate the interpreter immediately and leave you clueless where things went wrong. To catch these bugs in several functions the validity of input and output values is checked using the assert() macro. In the release version assertions are removed by defining preprocessor macro NDEBUG. Assert() is also used in switch statements where the default branch should never be reached (unless you made a mistake in your code).

###### Directory structure
No complexity, all source and header files reside in the same directory.

###### Function prototypes
Unless required for code readability function definitions are sequenced in such a way in a source file that the use of function prototypes is not needed.

###### Static functions
In principle all functions and any variable defined outside a function are declared as static. This means they can only be accessed from within the source file where they are defined. This ensures that making a function or variable available to other source files is a concious decision. In this way I try to avoid spaghetti code and side-effects by prohibiting changes to variables from all over the code.

###### Struct holding functions
Another way I use to structure the source code is by using structs containing function pointers (and sometimes some data). From within a certain source file only this struct and thus the function pointers it contains is exposed to other source files, so only these functions can be called. Using a struct like this is a very crude way to mimic what in other languages would be an object. For example see the scanner which reads tokens (meaningful sequences of characters) from EXIN modules. It is accessed via globally defined struct *scanner* in file *scanner.c* which contains both the scanners' variables and all exposed functions.

``` C
/* definition of the object */
typedef struct scanner {
	token_t token;
	token_t peeked;
	bool at_bol;
	char string[BUFSIZE + 1];

	token_t (*next)(void);
	token_t (*peek)(void);
	void (*init)(struct scanner *);
	void (*save)(struct scanner *);
	void (*load)(struct scanner *);
} Scanner;

/* create an initialized instance of the object */
Scanner scanner = {
	.token = UNKNOWN,
	.peeked = 0,
	.at_bol = true,
	.string[0] = 0,

	.next = next_token,
	.peek = peek_token,
	.init = scanner_init,
	.save = scanner_save,
	.load = scanner_load
};

/* accessing functions and variables */
token = scanner.next();
printf("%s", token.string);
```
This way of code structuring is used in scanner.c, module.c, number.c, str.c, list.c, none.c and for generic object functions (alloc, free, print, set, vset, method) in object.c. For operations on objects - like copy, add or multiply - global functions like obj_add(object *op1, object *op2) are used instead. I thought this was more readable; just compare obj_add(a,b) with TYPEOBJ(a)->add(a,b). (Ideally you would want to do a->add(b), but this won't work in C as the function add() does not know it is called from object a).

###### Break, Continue, Return
The *break*, *continue* and *return* statements interrupt the flow of execution. Each has a variable attached, its name preceded by do_, which - if true - indicates being busy exiting a block of statements based on one of these conditions. These variables are used to traverse back through the call stack of functions in visit().

###### Reporting errors
If an error occurs within a function it will still return a usable value. So no NULL's which can be a source of unexpected program endings. Note that returning a usable value, like a NONE_T, does not mean it is useful, it just prevents crashes.

###### Learning C
Aside from Stack Overflow I found http://c-faq.com/index.html very helpful.

##### Versions
The interpreter requires C11. For development I used MinGW-w64's GCC C compiler (at time of writing version 10.2.0) and the CodeLite IDE. When compiling I set -Wall, -Wextra and -fanalyzer to report as many errors or questionable constructs as possible.

##### Debug messages
The interpreter can produce all kinds of debugging output. For this add DEBUG to the preprocessor macros when compiling. Search for `debug_printf()` in the code to see where the messages are generated. For example, when running the following program with -d24 as debug level ...
``` python
int n = 3
print n
```
... this debug output is printed.
```
BLOCK
| VARIABLE_DECLARATION
| | DEF_VAR
| | | NAME n
| | | TYPE INT
| | | LITERAL
| | | | TYPE INT
| | | | VALUE 3
| PRINT_STMNT
| | RAW = FALSE
| | REFERENCE
| | | NAME n

3

level;name;object
1;n;00000000007C6CC0

object;refcount;type;value
00000000007C6CC0;1;int;3
```
This shows an example of an abstract syntax tree: the top level has two nodes (VARIABLE DECLARATION and PRINT_STMTNT). The character '3' is the programs output. It is followed by a memory dump of the variables which exists after the program has ended. This is one of the ways to check for memory leaks (the other one is to show the memory allocation and deallocation during execution).

Printing debugging messages can obscure a programs actual output. Therefore when debugging the original output is printed using a green foreground (provided preprocessor macro VT100 is defined). This is not visible here as MarkDown does not allow manually setting colours in code blocks. The foreground colour is set using ANSI/VT100 codes. On Windows 7 these are not automatically recognized by PowerShell and cmd.exe, so the debugging output will just show the actual escape sequences. To enable ANSI/VT100 codes in these shells execute the following command.
``` shell
reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1
```
Restart the shell afterwards. In Windows' bash shell the colours work out of the box. If the shell you are using does not support VT100 codes remove *VT100* from the preprocessor macros and recompile.

##### Preprocessor macros
The following preprocessor macros are used:
  * NDEBUG to remove assert() macros
  * DEBUG to enable various debug functions
  * VT100 to enable coloured output during debugging
