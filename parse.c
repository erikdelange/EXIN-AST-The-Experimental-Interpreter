/* parse.c
 *
 * Encodes source code into an abstract syntax tree.
 *
 * See https://en.wikipedia.org/wiki/Recursive_descent_parser for
 * an explanation of the basic setup of the parser.
 *
 * Contrary to normal C code the comments preceding every function
 * specify the state of the scanner at the entry and at the exit of
 * the function (instead of function arguments and return value).

 * By creating the AST the parser effectively verifies whether the
 * source code complies to the language grammar.
 *
 * The syntax in comments is specified in EBNF metasyntax.
 *
 * 2020 K.W.E. de Lange
 */
#include <stdlib.h>
#include <string.h>

#include "function.h"
#include "scanner.h"
#include "object.h"
#include "error.h"
#include "parse.h"


/* Minimal forward declarations.
 */
static Node *logical_or_expr(void);
static Node *assignment_expr(void);
static Node *comma_expr(void);
static Node *block(void);


/* Utility: check if the current token matches t. If true then return 1
 * and read the next token, if false then return 0.
 */
static int accept(token_t t)
{
	if (scanner.token == t) {
		scanner.next();
		return 1;
	}
	return 0;
}


/* Utility: check if the current token token t matches t. If it does then
 * read the next token, if it does not then stop the parser with an
 * error message.
 */
static int expect(token_t t)
{
	if (accept(t))
		return 1;

	raise(SyntaxError, "expected %s instead of %s", \
					   tokenName(t), tokenName(scanner.token));
	return 0;
}


/* Encode tokens following an expression.
 *
 * First check for subscripts [index] and [start:end], where index is mandatory,
 * start and end are optional. Encoding continues until no subscripts are left.
 * Then check for methods.
 *
 * in:	first token after expression
 * out:	first token after expression, subscripts or method arguments
 */
static Node *trailer(Node *n)
{
	char buffer[BUFSIZE];
	Node *sequence = NULL;

	if (accept(LSQB)) {  /* there is a subscript */
		Node *index = NULL, *start = NULL, *end = NULL;
		enum { IS_INDEX, IS_SLICE } type = IS_INDEX;

		while (1) {
			type = IS_INDEX;

			if (accept(COLON)) {
				start = create(LITERAL, VT_INT, "0");
				type = IS_SLICE;
			} else
				start = index = logical_or_expr();

			if (accept(COLON))
				type = IS_SLICE;

			if (accept(RSQB)) {
				if (type == IS_SLICE) {
					snprintf(buffer, BUFSIZE, "%ld", (long)INT_MAX);
					end = create(LITERAL, VT_INT, buffer);
				}
			} else {
				end = logical_or_expr();
				expect(RSQB);
			}
			if (type == IS_INDEX)
				sequence = create(INDEX, n, index);
			else
				sequence = create(SLICE, n, start, end);

			if (accept(LSQB))
				n = sequence;
			else
				break;
		}
	}

	n = sequence == NULL ? n : sequence;

	if (accept(DOT)) {  /* there is a method */
		if (scanner.token == IDENTIFIER) {

			n->method.valid = true;
			n->method.name = strdup(scanner.string);
			n->method.arguments = array_alloc();

			expect(IDENTIFIER);
			expect(LPAR);
			while (accept(RPAR) == 0) {
				while (1) {
					array_append_child(n->method.arguments, logical_or_expr());
					if (scanner.token == RPAR)
						break;
					expect(COMMA);
				}
			}
		} else
			raise(SyntaxError, "expected method");
	}

	return n;
}


/* Encode variables, function calls, constants, (expression)
 *
 * Syntax: ( function_call | variable | literal | '(' expression ')' ) subscript? ( '.' method )?
 *
 */
static Node *primary_expr(void)
{
	char name[BUFSIZE];
	Node *n = NULL;

	switch (scanner.token) {
		case CHAR:
			n = create(LITERAL, VT_CHAR, scanner.string);
			expect(CHAR);
			break;
		case INT:
			n = create(LITERAL, VT_INT, scanner.string);
			expect(INT);
			break;
		case FLOAT:
			n = create(LITERAL, VT_FLOAT, scanner.string);
			expect(FLOAT);
			break;
		case STR:
			n = create(LITERAL, VT_STR, scanner.string);
			expect(STR);
			break;
		case LSQB:
			n = create(ARGLIST);  /* not a LITERAL as it can contain non-literals e.g. [x] */
			expect(LSQB);
			while (accept(RSQB) == 0) {
				while (1) {
					array_append_child(n->arglist.arguments, assignment_expr());
					if (scanner.token == RSQB)
						break;
					expect(COMMA);
				}
			}
			break;
		case IDENTIFIER:
			snprintf(name, BUFSIZE, "%s", scanner.string);
			expect(IDENTIFIER);
			if (accept(LPAR)) {
				n = create(FUNCTION_CALL, name, is_builtin(name));
				while (accept(RPAR) == 0) {
					while (1) {
						array_append_child(n->function_call.arguments, assignment_expr());
						if (scanner.token == RPAR)
							break;
						expect(COMMA);
					}
				}
			} else
				n = create(REFERENCE, name);
			break;
		case LPAR:  /* parenthesized expression */
			expect(LPAR);
			n = comma_expr();
			expect(RPAR);
			break;
		default:
			raise(SyntaxError, "expression expected");
	}

	return trailer(n);
}


/* Encode expressions with operators: (unary)-  (unary)+  ! (logical negation, NOT)
 *
 * Syntax: ( '+' | '-' | '!' )? primary_expr
 *
 */
static Node *unary_expr(void)
{
	Node *value;

	if (accept(NOT))
		value = create(UNARY, UNOT, primary_expr());
	else if (accept(MINUS))
		value = create(UNARY, UMINUS, primary_expr());
	else if (accept(PLUS))
		value = create(UNARY, UPLUS, primary_expr());
	else
		value = primary_expr();

	return value;
}


/* Encode expressions with operators: *  /  %
 *
 * Syntax: unary_expr ( ( '*' | '/' | '%' ) multiplication_expr )*
 *
 */
static Node *multiplication_expr(void)
{
	Node *value;

	value = unary_expr();

	while (1) {
		if (accept(STAR))
			value = create(BINARY, MUL, value, unary_expr());
		else if (accept(SLASH))
			value = create(BINARY, DIV, value, unary_expr());
		else if (accept(PERCENT))
			value = create(BINARY, MOD, value, unary_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: +  -
 *
 * Syntax: multiplication_expr ( ( '+' | '-' ) addition_expr )*
 *
 */
static Node *addition_expr(void)
{
	Node *value;

	value = multiplication_expr();

	while (1) {
		if (accept(PLUS))
			value = create(BINARY, ADD, value, multiplication_expr());
		else if (accept(MINUS))
			value = create(BINARY, SUB, value, multiplication_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: <  <=  >  >=
 *
 * Syntax: addition_expr ( ( '<'| '>' | '<=' | '>=' ) relational_expr )*
 *
 */
static Node *relational_expr(void)
{
	Node *value;

	value = addition_expr();

	while (1) {
		if (accept(LESS))
			value = create(BINARY, LSS, value, relational_expr());
		else if (accept(LESSEQUAL))
			value = create(BINARY, LEQ, value, relational_expr());
		else if (accept(GREATER))
			value = create(BINARY, GTR, value, relational_expr());
		else if (accept(GREATEREQUAL))
			value = create(BINARY, GEQ, value, relational_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: ==  !=  <>  in
 *
 * Syntax: relational_expr ( ( '==' | '!=' | '<>' | 'in' ) equality_expr )*
 *
 */
static Node *equality_expr(void)
{
	Node *value;

	value = relational_expr();

	while (1) {
		if (accept(EQEQUAL))
			value = create(BINARY, EQ, value, relational_expr());
		else if (accept(NOTEQUAL))
			value = create(BINARY, NEQ, value, relational_expr());
		else if (accept(IN))
			value = create(BINARY, OP_IN, value, relational_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with perators: logical and
 *
 * Syntax: equality_expr ( 'and' logical_and_expr )*
 *
 */
static Node *logical_and_expr(void)
{
	Node *value;

	value = equality_expr();

	while (1) {
		if (accept(AND))
			value = create(BINARY, LOGICAL_AND, value, logical_and_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: logical or
 *
 * Syntax: logical_and_expr ( 'or' logical_or_expr )*
 *
 */
static Node *logical_or_expr(void)
{
	Node *value;

	value = logical_and_expr();

	while (1) {
		if (accept(OR))
			value = create(BINARY, LOGICAL_OR, value, logical_or_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: =  +=  -=  *=  /=  %=
 *
 * Syntax: logical_or_expr ( ( '=' | '+=' | '-=' | '*=' | '\=' | '%=' ) assignment_expr )*
 *
 */
static Node *assignment_expr(void)
{
	Node *value;

	value = logical_or_expr();

	while (1) {
		if (accept(EQUAL))
			value = create(ASSIGNMENT, ASSIGN, value, assignment_expr());
		else if (accept(PLUSEQUAL))
			value = create(ASSIGNMENT, ADDASSIGN, value, logical_or_expr());
		else if (accept(MINUSEQUAL))
			value = create(ASSIGNMENT, SUBASSIGN, value, logical_or_expr());
		else if (accept(STAREQUAL))
			value = create(ASSIGNMENT, MULASSIGN, value, logical_or_expr());
		else if (accept(SLASHEQUAL))
			value = create(ASSIGNMENT, DIVASSIGN, value, logical_or_expr());
		else if (accept(PERCENTEQUAL))
			value = create(ASSIGNMENT, MODASSIGN, value, logical_or_expr());
		else
			break;
	}

	return value;
}


/* Encode expressions with operators: ,
 *
 * Syntax: assignment_expr ( ',' assignment_expr )+
 *
 */
static Node *comma_expr(void)
{
	Node *c = NULL, *n;

	n = assignment_expr();

	if (scanner.token == COMMA) {  /* only create a comma expression if there actually is a comma */
		c = create(COMMA_EXPR);
		array_append_child(c->comma_expr.expressions, n);

		while (1) {
			if (accept(COMMA))
				array_append_child(c->comma_expr.expressions, assignment_expr());
			else
				break;
		}
	}
	return c != NULL ? c : n;
}


/* Encode expression.
 *
 * in:	token = first token of expression
 * out:	token = first token after NEWLINE
 */
static Node *expression_stmnt(void)
{
	Node *n;

	n = create(EXPRESSION_STMNT, comma_expr());

	expect(NEWLINE);

	return n;
}


/* Encode a statement block.
 *
 * Syntax: NEWLINE INDENT statement+ DEDENT
 *
 */
static Node *indented_block(void)
{
	Node *n;

	expect(NEWLINE);
	expect(INDENT);
	n = block();
	expect(DEDENT);

	return n;
}


/* Encode function declaration.
 *
 * Syntax: 'def' identifier '(' (identifier ( ',' identifier )* )? ')' block
 *
 * in:	identifier of function
 * out:	first token after closing parenthesis
 */
static Node *function_declaration(void)
{
	Node *stmnt;
	char name[BUFSIZE];
	Array *arguments = array_alloc();

	snprintf(name, BUFSIZE, "%s", scanner.string);

	expect(IDENTIFIER);
	expect(LPAR);

	while (accept(RPAR) == 0) {
		while (1) {
			if (scanner.token != IDENTIFIER)
				raise(SyntaxError, "expected identifier instead of %s", \
									tokenName(scanner.token));
			array_append_child(arguments, strdup(scanner.string));
			expect(IDENTIFIER);
			if (scanner.token == RPAR)
				break;
			expect(COMMA);
		}
	}

	stmnt = create(FUNCTION_DECLARATION, name, arguments);

	stmnt->function_declaration.block = indented_block();

	return stmnt;
}


/* Encode declaration of variabele(s) and optionally the initial value(s).
 *
 * vt: variabele(s) type - char, int, float, str, list
 *
 * Syntax: type identifier ( '=' assignment_expr )? ( ',' identifier ( '=' assignment_expr )? )* NEWLINE
 *
 * in:	token = first token after DEFCHAR, DEFINT, DEFFLOAT, DEFSTR, DEFLIST
 * out:	token = first token after NEWLINE
 */
static Node *variable_declaration(variabletype_t vt)
{
	char name[BUFSIZE];
	Node *n, *defvar;

	n = create(VARIABLE_DECLARATION);

	while (1) {
		if (scanner.token != IDENTIFIER)
			raise(SyntaxError, "expected identifier instead of %s", \
								tokenName(scanner.token));

		snprintf(name, BUFSIZE, "%s", scanner.string);

		scanner.next();

		if (accept(EQUAL))
			defvar = create(DEF_VAR, vt, name, assignment_expr());
		else
			defvar = create(DEF_VAR, vt, name, NULL);

		array_append_child(n->variable_declaration.defvars, defvar);

		if (accept(NEWLINE))
			break;

		expect(COMMA);
	}

	return n;
}


/* Syntax: 'if' expression block ( 'else' block )?
 *
 * in:	token = first token after IF
 * out:	token = first token after DEDENT of last statement block
 */
static Node *if_stmnt(void)
{
	Node *stmnt, *alternative = NULL;

	stmnt = create(IF_STMNT);

	stmnt->if_stmnt.condition = comma_expr();

	stmnt->if_stmnt.consequent = indented_block();

	if (accept(ELSE))
		alternative = indented_block();

	stmnt->if_stmnt.alternative = alternative;

	return stmnt;
}


/* Syntax: 'while' expression block
 *
 * in:	token = first token after WHILE
 * out:	token = first token after DEDENT of statement block
 */
static Node *while_stmnt(void)
{
	Node *stmnt;

	stmnt = create(WHILE_STMNT);

	stmnt->loop_stmnt.condition = comma_expr();
	stmnt->loop_stmnt.block = indented_block();

	return stmnt;
}


/* Syntax: 'do' block 'while' expression NEWLINE
 *
 * in:	token = first token after DO
 * out:	token = first token after NEWLINE
 */
static Node *do_stmnt(void)
{
	Node *stmnt, *condition, *consequent;

	consequent = indented_block();

	expect(WHILE);

	condition = comma_expr();

	stmnt = create(DO_STMNT, condition, consequent);

	expect(NEWLINE);

	return stmnt;
}


/* Syntax: 'for' identifier 'in' sequence NEWLINE block
 *
 * in:	token = first token after FOR
 * out:	token = first token after dedent of block
 */
static Node *for_stmnt(void)
{
	char targetname[BUFSIZE];
	Node *stmnt;

	if (scanner.token == IDENTIFIER)
		snprintf(targetname, BUFSIZE, "%s", scanner.string);

	expect(IDENTIFIER);
	expect(IN);

	stmnt = create(FOR_STMNT, targetname, comma_expr());

	if (scanner.token != NEWLINE)
		raise(SyntaxError, "expected newline");

	stmnt->for_stmnt.block = indented_block();

	return stmnt;
}


/* Encode printing of value(s).
 *
 * Syntax: 'print' '-raw'? ( assignment_expr ( ',' assignment_expr )* )? NEWLINE
 *
 * in:	token = first token after PRINT
 * out:	token = first token after NEWLINE
 */
static Node *print_stmnt(void)
{
	Node *n;
	bool raw = false;

	if (scanner.token == MINUS)
		if (scanner.peek() == IDENTIFIER && strcmp(scanner.string, "raw") == 0) {
			scanner.next();
			scanner.next();
			raw = true;
		}

	n = create(PRINT_STMNT, raw);

	while (accept(NEWLINE) == 0)
		while (1) {
			array_append_child(n->print_stmnt.expressions, assignment_expr());
			if (scanner.token == NEWLINE)
				break;
			expect(COMMA);
		}

	return n;
}


/* Encode return statement
 *
 * Syntax: 'return' expression? NEWLINE
 *
 * in:	token = first token after RETURN
 * out:	token = first token after NEWLINE
 */
static Node *return_stmnt(void)
{
	Node *stmnt, *value = NULL;

	if (scanner.token != NEWLINE)
		value = comma_expr();

	stmnt = create(RETURN_STMNT, value);

	expect(NEWLINE);

	return stmnt;
}


/* Encode reading of value(s) from STDIN.
 *
 * Syntax: 'input' string? identifier ( , string? identifier )* NEWLINE
 *
 * in:	token = first token after INPUT
 * out:	token = first token after NEWLINE
 */
static Node *input_stmnt(void)
{
	Node *n;

	n = create(INPUT_STMNT);

	do {
		if (scanner.token == STR) {
			array_append_child(n->input_stmnt.prompts, strdup(scanner.string));
			scanner.next();
		} else
			array_append_child(n->input_stmnt.prompts, NULL);  /* no prompt */

		if (scanner.token == IDENTIFIER)
			array_append_child(n->input_stmnt.identifiers, strdup(scanner.string));
		accept(IDENTIFIER);
	} while (accept(COMMA));

	expect(NEWLINE);

	return n;
}


/* Encode importing a module.
 *
 * Syntax: 'import' string_literal NEWLINE
 *
 * in:	token = first token after IMPORT
 * out:	token = first token after NEWLINE
 */
static Node *import_stmnt(void)
{
	Node *code, *stmnt;

	if (module.search(scanner.string) != NULL)
		raise(SyntaxError, "module %s already loaded", scanner.string);

	code = parse(module.import(scanner.string));

	stmnt = create(IMPORT_STMNT, scanner.string, code);

	expect(STR);
	expect(NEWLINE);

	return stmnt;
}


/* Syntax: 'pass' NEWLINE
 *
 * in:	token = first token after PASS
 * out:	token = first token after NEWLINE
 */
static Node *pass_stmnt(void)
{
	Node *stmnt = create(PASS_STMNT);

	expect(NEWLINE);

	return stmnt;
}


/* Syntax: 'break' NEWLINE
 *
 * in:	token = first token after BREAK
 * out:	token = first token after NEWLINE
 */
static Node *break_stmnt(void)
{
	Node *stmnt = create(BREAK_STMNT);

	expect(NEWLINE);

	return stmnt;
}


/* Syntax: 'continue' NEWLINE
 *
 * in:	token = first token after CONTINUE
 * out:	token = first token after NEWLINE
 */
static Node *continue_stmnt(void)
{
	Node *stmnt = create(CONTINUE_STMNT);

	expect(NEWLINE);

	return stmnt;
}


/* Statement encoder.
 *
 * in:	token = token to encode
 * out:	token = first token after statement
 */
static Node *statement(void)
{
	Node *n;

	if (accept(DEFCHAR))
		n = variable_declaration(VT_CHAR);
	else if (accept(DEFINT))
		n = variable_declaration(VT_INT);
	else if (accept(DEFFLOAT))
		n = variable_declaration(VT_FLOAT);
	else if (accept(DEFSTR))
		n = variable_declaration(VT_STR);
	else if (accept(DEFLIST))
		n = variable_declaration(VT_LIST);
	else if (accept(DEFFUNC))
		n = function_declaration();
	else if (accept(IF))
		n = if_stmnt();
	else if (accept(WHILE))
		n = while_stmnt();
	else if (accept(DO))
		n = do_stmnt();
	else if (accept(PRINT))
		n = print_stmnt();
	else if (accept(RETURN))
		n = return_stmnt();
	else if (accept(PASS))
		n = pass_stmnt();
	else if (accept(FOR))
		n = for_stmnt();
	else if (accept(BREAK))
		n = break_stmnt();
	else if (accept(CONTINUE))
		n = continue_stmnt();
	else if (accept(IMPORT))
		n = import_stmnt();
	else if (accept(INPUT))
		n = input_stmnt();
	else if (accept(ENDMARKER))
		n = NULL;
	else
		n = expression_stmnt();

	return n;
}


/* Encode a statement block.
 *
 * Syntax: NEWLINE INDENT statement+ DEDENT
 *
 * in:	token = NEWLINE
 * out:	token = DEDENT
 */
static Node *block(void)
{
	Node *n, *stmnt;

	n = create(BLOCK);

	while (1) {
		if ((stmnt = statement()))
			array_append_child(n->block.statements, stmnt);
		if (scanner.token == DEDENT || scanner.token == ENDMARKER)
			break;
	}

	return n;
}


/* API: Translate source code into an abstract syntax tree.
 *
 * m		module to parse
 * return	Node* to root node of abstract syntax tree
 */
Node *parse(Module *m)
{
	Node *n;
	Scanner s;

	scanner.save(&s);
	scanner.init(&scanner, m);

	scanner.next();  /* first token must be read before starting the parser */

	n = block();

	scanner.load(&s);

	return n;
}
