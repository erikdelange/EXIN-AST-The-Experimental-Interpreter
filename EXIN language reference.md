# EXIN Language Reference

##### General
EXIN is a simple language and contains traces of Basic, C and Python.
##### Keywords
The following keywords are reserved and may not be used as variable or function name.
```
and       break     char      continue  def       do
else      float     for       if        import    in
input     int       list      or        pass      print
return    str       while
```
##### Code format
Code consists of lines of plain text. Lines contain statements but can also be empty. Statements do not span lines but are terminated by a newline character. Indentation is used to group statements in blocks for control structures (if-else, do-while, while-do, for-in). For example
```
int a
while a != 10
    print a
    a += 1
```
will not yield the same result as
```
int a
while a != 10
    print a
a += 1
```
The first code snippet prints all digits whereas the second one is an infinite loop.

To avoid confusion it is best to use only tabs for indentation. However spaces will also work. Tabs and spaces can be mixed as long as the interpreter knows how much spaces represent a tab. The default is 4 but this can modified by command line option -t.

A hash sign (*#*) indicates the start of a comment. All text after the hash sign until the end of the line is discarded by the interpreter.
```
# This is a line containing only a comment

print "Hello"  # a comment at the end of a statement
```
Code execution always starts at the top of a file.
##### Data types
The three primitive data types are *char*, *int* and *float*. They are used for storing characters, integers and floating point numbers and match the C data types char, long and double.

On top of these primitive types two additional data types are constructed: strings and lists. These are sequence data types as they can store multiple values which can be accessed by index. Lists can contain any data type, including other lists. Their data type is *list*. A special variant of the list is the string (data type *str*) which can contain only characters.

EXIN is strongly typed and requires that every variable or function is declared before it can be used.
```
char c
int count
float sales_amount
str s1
list l_2
```
Variable names must begin with a letter and consist of letters, digits and underscores.

Variables receive an implicit default value when declared; 0 for the primitive types or else an empty list (*[]*) or empty string (*""*). It is also possible to assign a value during declaration. This value can be a literal or an expression. Multiple variables of the same type can be declared on a single line.
```
char a = 'A', b = '\n', c
int i = 10
float x = 3.14, y = 1E10
str s = "abcd", t = "\n", u = ""
list l = ['a', 2.1, "xyz"], m = [], n
```
The type of a variable or literal can be retrieved via the builtin *type()* function.
``` c
>>> type("abc")
= str
```
##### Literals
A character literal is surrounded by single quotes, a string literal by double quotes. Both may contain escape sequences starting with a backslash. Number literals without a decimal dot are considered integers, and ones with a dot floats. A floating point number can also be written in scientific notation.
``` python
>>> 1 / 2
= 0
>>> 1 / 2.0
= 0.5
>>> 1.0 / 2
= 0.5
>>> 1 / 2E0
= 0.5
>>> 1E0 / 2
= 0.5
```
##### Strings and lists
###### Indices and slices
Elements in lists and strings are accessed via their index. Index numbers start at 0, or -1 when counting from the end of the list or string.
``` python
>>> "abc"[0]
= a

>>> "abc"[-1]
= c
```
It is also possible to take slices of a list or string using indices. The absence of an index identifies either the start or the end of a list or string. Out of bound indices of slices are silently adjusted to the nearest possible value.
``` python
>>> "abcdef"[:]
= abcdef

>>> "abcdef"[1:]
= bcdef

>>> "abcdef"[1:3]
= bc

>>> "abcdef"[:-5]
= a
```
The number of elements in a list or number of characters in a string is returned by the *.len()* method.
``` c
>>> "abcdef".len()
= 6

>>> "abcdef"[1:3].len()
= 2
```
###### Adding and removing values
Characters, numbers and strings can be added to a string via the *+* operator.
``` c
>>> "ab" + 'c'
= abc

>>> "xy" + 3.14
= xy3.14
```
Literal values can be appended to a list via the *+* operator if the literal is represented as a list constant.
``` c
>>> [3] + ["alfa"]
= [3,"alfa"]
```
The most efficient way to append an element at the end of a list is via the *.append* method.
``` c
>>> list m
>>> m.append(3.14)
>>> print m
[3.14]
```
Inserting an element at any place in a list is done via *.insert(before_index, element)*. Removing an element can only be done via its index. As usual this can be an index from the beginning or from the end of the list.
``` c
>>> list m
>>> m.insert(0, 3.14)  # insert at beginning of list
>>> print m
[3.14]
>>> m.remove(-1)  # remove the last item
>>> print m
[]
>>>
```
##### Operators
###### Arithmetic
The binary operators are +, -, \*, / and the modulo operator %. Modulo can only be used on integers. For usage in assignments the shorthand operators +=, -=, \*=, /= and \%= are available instead of (for example) n = n + 1. In an assignment the source value is converted to the type of the target variable. Using addition on lists or strings will result in list or string concatenation. Multiplication of a list or string by a number results in the repetition of the list or string.
``` python
>>> "abc" + "def"
= abcdef

>>> [1,2] + [3,4]
= [1,2,3,4]

>>> "xyz" * 2
= xyzxyz

>>> [1,2] * 2
= [1,2,1,2]
```
###### Comparison
The comparison operators are *==, !=, in, <>, <, <=, >, >=*. Note that equality comparison uses two equal characters where assignment only uses one. Lists and strings can be only be compared using *==* and *!=*. The *in* operator is used to check if a value can be found in a sequence.
###### Logical
The logical operators are *and*, *or* and *!* (being not). True is represented by a non-zero integer, false being zero.
###### Order of evaluation
Expression evaluation follows the following rules of precedence:
 *  first read variables (including subscripts and slices) and literals
 * 	then execute function calls, methods and evaluate parenthesized expressions
 *	then the unary operators + - and !
 *	then multiplication and division (normal and modulo)
 *	then addition and subtraction
 *	then the comparisons < <= > and >=
 *	then the comparisons == != and *in*
 * 	then logical *and*
 *	then logical *or*
 *	then assignment of values (normal and shorthand)
 * 	and finally comma separated statements

##### Control structures
###### If .. else
The *if* keyword is followed by a conditional expression and when this evaluates to true the statement block following *if* is executed. Optionally an *else* block can be defined. *If* statements can be nested.
```
if i > 0
    print "i is greater then zero"
else
    if i == 0
        print "i equals zero"
    else
        print "i is less then zero"
```
###### Loops
Three types of loops are available: *while*, *do .. while* and *for .. in*. The while loop evaluates the conditional expression before the loop is entered, whereas for the do .. while loop this is only done after the loop has been executed. So the statement block of a do .. while loop is executed at least once.
```
int n = 0
while n
    print "this is never executed"

int m
do
    print m
while (m += 1) < 10
```
According to the rules of precedence the parenthesized part of the conditional expression from the do .. while loop is executed before the comparison is made, so loop counter m is incremented first.

An exit from a loop can be forced at any point by the *break* and *continue* statement. When using *break* the innermost loop is exited and its conditional expression is considered to be false. Via *continue* the rest of the statement block of the loop is skipped, causing the loops conditional expression to be evaluated again.
```
int n
while 1
    if n == 10
        break
    n += 1
```
This loop is executed infinitely because 1 always evaluates to true. However the *if* statement with *break* makes sure the loop is terminated once n equals 10.
##### Looping through lists and strings
The *for .. in sequence* loop cycles through the content of a list of string. As with the other loops *break* and *continue* can be used here.
```
for element in [1, 2.0, "abc", 'c']
    print element, type(element)
```
It is not necessary to define variable *element* upfront because it is just a reference to a variable in the list. In C this would be called a pointer. It can be used to change the value in the list. The types of the values which are assigned can be different for each element of the list. If the sequence used in the *for .. in* loop is a string then of course *element* is only assigned characters. Strings are read-only. *Element* stays in existence after the for loop is finished, and then points to the last read value. If the sequence was empty ("" or []) it points to the *none* object.
##### Function definition
Functions are defined using the *def* keyword followed by a function name and a pair of parenthesis containing the argument names separated by comma's. Even if a function has no arguments the parenthesis are mandatory. All arguments are passed by value. There is no type checking when the function is called. The number of arguments in the function call must match the function declaration.
```
# Prepare Fibonacci sequence for n elements, return as list
#
def fibonacci(n)
    list fib
    int f0 = 0, f1 = 1, fn, i

    while i < n
        if i <= 1
            fn = i
        else
            fn = f0 + f1
            f0 = f1, f1 = fn
        fib.append(fn)
        i += 1

    return fib

int n = 10
print "Fibonacci sequence for", n, "elements: ", fibonacci(n)
```
A function returns when it reaches the end of its statement block or when a *return* statement is encountered. When using the *return* statement a return value can be explicitly specified. Without this statement, or when using just *return* the return value is considered to be integer 0. The return value of a function can be used immediately, so a function can appear everywhere where a variable can appear. Any data type can be returned by a function, including lists and strings.
Variables are defined within the scope of a function. Any variable defined outside of a function is considered global. Function definitions can be nested, where the nested function is only visible for its enclosing parent function. A nested function has access to the variables of all enclosing functions. See the example below.
```
# Nested function example. Sorts a list containing integers.
#
# Taken from https://en.wikipedia.org/wiki/Nested_function

def sort(items)
    def quicksort(first, last)
        def swap(p, q)
            int tmp = items[p]
            items[p] = items[q]
            items[q] = tmp

        def partition()
            int pivot = items[first]
            int index = first

            swap(index, last)

            int i = first
            while i < last
                if items[i] < pivot
                    swap(index, i)
                    index += 1
                i += 1

            swap(index, last)

            return index

        if first < last
            int pivotindex = partition()
            quicksort(first, pivotindex - 1)
            quicksort(pivotindex + 1, last)

    quicksort(0, items.len() - 1)
    return items


list l = [3, 1, 0, 2]

print "unsorted list", l

l = sort(l)

print "sorted list", l
```
This will print:
```
unsorted list [3,1,0,2]
sorted list [0,1,2,3]
```
##### Importing modules
The *import* statement loads program code from other files. The imported code is executed immediately after loading. Its functions are added to the local list and any statement or declaration outside a function definition is executed. A module can only be imported once, repeated imports will raise an error. Imports can be nested.
```
import "file2.ext"
```
##### Input and output
Information can be send to the standard output via the *print* statement. Any number of expressions (also none) separated by comma's can follow *print*.
```
>>> str s = "Hello"
>>> print s, "there", 1 + 2 - 3.14
Hello there -0.14
```
The printed arguments are separated by a space, and after the last argument a newline is printed. This behaviour can be suppressed by adding *-raw* after the *print* statement (e.g *print -raw 3.14*). Note that *-raw* is not followed by a comma.

The *input* statement reads data from the standard input into a variable. Input must be ended by a newline. Optionally a prompt string can be specified which is printed before the input is read. Note there is no comma between the prompt and variable. Multiple variables can be read using a single input statement.
```
input "Please enter your name: " name
input first_name, last_name
```
##### Various
The *pass* keyword is a no-operation statement and can be used as a placeholder during program development.
Statements cannot be used as identifier (for a variable or function) name.
##### Builtin functions
A number of builtin functions are provided. These include type(variable) to return a string with the type of the variable, chr(integer) which returns a string with the ASCII representation of integer and ord(string) which returns the ASCII value (as integer) of the character in the string. The purpose of builtin functions is to facilitate adding new functions to the language.
##### Grammar in EBNF
For a graphical representation of the syntax see [EXIN syntax diagram](EXIN%20syntax%20diagram.pdf).
For an explanation of the EBNF notation used below see [EBNF syntax.txt](EBNF%20syntax.txt).
```
/*	EXIN grammar.
 *
 *	For graphical representation use http://bottlecaps.de/rr/ui
 */

program ::= (statement | NEWLINE)* EOF

/* statements */

statement ::= declaration_stmnt | if_stmnt | while_stmnt | do_stmnt | for_stmnt | print_stmnt | return_stmnt |
			  input_stmnt | import_stmt | pass_stmnt | break_stmnt | continue_stmnt | expression_stmnt

declaration_stmnt ::= function_declaration | variable_declaration

function_declaration ::= 'def' identifier '(' (identifier ( ',' identifier )* )? ')' block

variable_declaration ::= var_type identifier ( '=' assignment_expr )? ( ',' identifier ( '=' assignment_expr )? )* NEWLINE

var_type ::= 'char' | 'int' | 'float' | 'str' | 'list'

if_stmnt ::= 'if' expression block ( 'else' block )?

while_stmnt ::= 'while' expression block

do_stmnt ::= 'do' block 'while' expression NEWLINE

for_stmnt ::= 'for' IDENTIFIER 'in' sequence block

print_stmnt ::= 'print' '-raw'? ( assignment_expr ( ',' assignment_expr )* )?  NEWLINE

return_stmnt ::= 'return' expression? NEWLINE

input_stmnt ::= 'input' string? identifier ( ',' string? identifier )* NEWLINE

import_stmt ::= 'import' string_literal NEWLINE

pass_stmnt ::= 'pass' NEWLINE

break_stmnt ::= 'break' NEWLINE

continue_stmnt ::= 'continue' NEWLINE

block ::= NEWLINE INDENT statement+ DEDENT

expression_stmnt ::= expression NEWLINE

/* expressions */

expression ::= comma_expr | assignment_expr

comma_expr ::= assignment_expr ( ',' assignment_expr )+

assignment_expr ::= logical_or_expr ( ( '=' | '+=' | '-=' | '*=' | '\=' | '%=' ) assignment_expr )*

logical_or_expr ::= logical_and_expr ( 'or' logical_or_expr )*

logical_and_expr ::= equality_expr ( 'and' logical_and_expr )*

equality_expr ::= relational_expr ( ( '==' | '!=' | '<>' | 'in' ) equality_expr )*

relational_expr ::= addition_expr ( ( '<'| '>' | '<=' | '>=' ) relational_expr )*

addition_expr ::= multiplication_expr ( ( '+' | '-' ) addition_expr )*

multiplication_expr ::= unary_expr ( ( '*' | '/' | '%' ) multiplication_expr )*

unary_expr ::= ( '+' | '-' | '!' )? primary_expr

primary_expr ::= ( function_call | variable | literal | '(' expression ')' ) subscript? ( '.' method )?

function_call ::= identifier '(' (assignment_expr ( ',' assignment_expr )* )? ')'

subscript ::= '[' ( index | slice ) ']'

index ::= logical_or_expr

slice ::= logical_or_expr? ':' logical_or_expr?

method ::= identifier '(' ( logical_or_expr ( ',' logical_or_expr )* )? ')'

/* identifiers, variables and literals */

identifier ::= alphabetic ( alphabetic | digit | '_' )*

variable ::= identifier

literal ::= numeric_literal | character_literal | string_literal | list_literal

numeric_literal ::= number

character_literal ::= "'" character "'"

string_literal ::= string

list_literal ::= '[' ( assignment_expr ( ',' assignment_expr )* )? ']'

/* low level definitions */

string ::= '"' character* '"'

character ::= 'any character except single quote' | escape

alphabetic ::= [a-zA-Z]

escape ::= '\' [bnfrtv\'"0]

number ::= integer | float

integer ::= digit+

float ::= integer '.' ( integer )? ( ('e' | 'E') ( '+' | '-' )? integer )?

digit ::= [0-9]
```
