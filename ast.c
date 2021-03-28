/* ast.c
 *
 * Routines for creating nodes in the abstract syntax tree.
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "error.h"
#include "array.h"
#include "ast.h"


/* For every nodetype a 'create_...' function is defined below.
 * On entry the new node has been created, its type set, it is
 * linked to the position in the source code where it originated
 * from, but any other field is uninitialized. This must be
 * done in the respective 'create_...' function.
 *
 * For any char* argument the 'create_...' function will make
 * a private copy using strdup(). Other pointers are used as is.
 *
 * The generic callable create() function can be found at the
 * bottom of this source file.
 *
 */


static void create_block(Node *n)
{
	void check_block(Node *n);
	void visit_block(Node *, Stack *);
	void print_block(Node *, int);

	n->check = check_block;
	n->visit = visit_block;
	n->print = print_block;

	n->block.statements = array_alloc();
}


static void create_literal(Node *n, va_list argp)
{
	void check_literal(Node *);
	void print_literal(Node *, int);
	void visit_literal(Node *, Stack *);

	n->check = check_literal;
	n->print = print_literal;
	n->visit = visit_literal;

	n->literal.type = va_arg(argp, variabletype_t);
	n->literal.value = strdup(va_arg(argp, char *));
}


static void create_unary(Node *n, va_list argp)
{
	void check_unary(Node *n);
	void print_unary(Node *, int);
	void visit_unary(Node *, Stack *);

	n->check = check_unary;
	n->print = print_unary;
	n->visit = visit_unary;

	n->unary.operator = va_arg(argp, unaryoperator_t);
	n->unary.operand = va_arg(argp, Node *);
}


static void create_binary(Node *n, va_list argp)
{
	void check_binary(Node *n);
	void print_binary(Node *, int);
	void visit_binary(Node *, Stack *);

	n->check = check_binary;
	n->print = print_binary;
	n->visit = visit_binary;

	n->binary.operator = va_arg(argp, binaryoperator_t);
	n->binary.left = va_arg(argp, Node *);
	n->binary.right = va_arg(argp, Node *);
}


static void create_comma_expr(Node *n)
{
	void check_comma_expr(Node *n);
	void visit_comma_expr(Node *, Stack *);
	void print_comma_expr(Node *, int);

	n->check = check_comma_expr;
	n->visit = visit_comma_expr;
	n->print = print_comma_expr;

	n->comma_expr.expressions = array_alloc();
}


static void create_arglist(Node *n)
{
	void check_arglist(Node *n);
	void print_arglist(Node *, int);
	void visit_arglist(Node *, Stack *);

	n->check = check_arglist;
	n->print = print_arglist;
	n->visit = visit_arglist;

	n->arglist.arguments = array_alloc();
}


static void create_index(Node *n, va_list argp)
{
	void check_index(Node *n);
	void visit_index(Node *, Stack *);
	void print_index(Node *, int);

	n->check = check_index;
	n->visit = visit_index;
	n->print = print_index;

	n->index.sequence = va_arg(argp, Node *);
	n->index.index = va_arg(argp, Node *);
}


static void create_slice(Node *n, va_list argp)
{
	void check_slice(Node *n);
	void visit_slice(Node *, Stack *);
	void print_slice(Node *, int);

	n->check = check_slice;
	n->visit = visit_slice;
	n->print = print_slice;

	n->slice.sequence = va_arg(argp, Node *);
	n->slice.start = va_arg(argp, Node *);
	n->slice.end = va_arg(argp, Node *);
}


static void create_assignment(Node *n, va_list argp)
{
	void check_assignment(Node *n);
	void visit_assignment(Node *, Stack *);
	void print_assignment(Node *, int);

	n->check = check_assignment;
	n->visit = visit_assignment;
	n->print = print_assignment;

	n->assignment.operator = va_arg(argp, assignmentoperator_t);
	n->assignment.variable = va_arg(argp, Node *);
	n->assignment.expression = va_arg(argp, Node *);
}


static void create_reference(Node *n, va_list argp)
{
	void check_reference(Node *n);
	void visit_reference(Node *, Stack *);
	void print_reference(Node *, int);

	n->check = check_reference;
	n->visit = visit_reference;
	n->print = print_reference;

	n->reference.name = strdup(va_arg(argp, char *));
}


static void create_function_call(Node *n, va_list argp)
{
	void check_function_call(Node *n);
	void visit_function_call(Node *, Stack *);
	void print_function_call(Node *n, int);

	n->check = check_function_call;
	n->visit = visit_function_call;
	n->print = print_function_call;

	n->function_call.name = strdup(va_arg(argp, char *));
	n->function_call.arguments = array_alloc();
	n->function_call.builtin = va_arg(argp, int);  /* bool is promoted to int */
	n->function_call.checked = false;
}


static void create_expression_stmnt(Node *n, va_list argp)
{
	void check_expression_stmnt(Node *n);
	void visit_expression_stmnt(Node *, Stack *);
	void print_expression_stmnt(Node *, int);

	n->check = check_expression_stmnt;
	n->visit = visit_expression_stmnt;
	n->print = print_expression_stmnt;

	n->expression_stmnt.expression = va_arg(argp, Node *);
}


static void create_function_declaration(Node *n, va_list argp)
{
	void check_function_declaration(Node *n);
	void visit_function_declaration(Node *, Stack *);
	void print_function_declaration(Node *n, int);

	n->check = check_function_declaration;
	n->visit = visit_function_declaration;
	n->print = print_function_declaration;

	n->function_declaration.name = strdup(va_arg(argp, char *));
	n->function_declaration.nested = va_arg(argp, int);  /* bool is promoted to int */
	n->function_declaration.arguments = va_arg(argp, Array *);
}


static void create_variable_declaration(Node *n)
{
	void check_variable_declaration(Node *n);
	void visit_variable_declaration(Node *, Stack *);
	void print_variable_declaration(Node *, int);

	n->check = check_variable_declaration;
	n->visit = visit_variable_declaration;
	n->print = print_variable_declaration;

	n->variable_declaration.defvars = array_alloc();
}


static void create_defvar(Node *n, va_list argp)
{
	void check_defvar(Node *n);
	void visit_defvar(Node *, Stack *);
	void print_defvar(Node *, int);

	n->check = check_defvar;
	n->visit = visit_defvar;
	n->print = print_defvar;

	n->defvar.type = va_arg(argp, variabletype_t);
	n->defvar.name = strdup(va_arg(argp, char *));
	n->defvar.initialvalue = va_arg(argp, Node *);
}


static void create_if_stmnt(Node *n)
{
	void check_if_stmnt(Node *n);
	void visit_if_stmnt(Node *, Stack *);
	void print_if_stmnt(Node *, int);

	n->check = check_if_stmnt;
	n->visit = visit_if_stmnt;
	n->print = print_if_stmnt;

	/* condition, consequent and alternative must be added
	 * later in order to record the correct source code position
	 */
}


static void create_while_stmnt(Node *n)
{
	void check_while_stmnt(Node *n);
	void visit_while_stmnt(Node *, Stack *);
	void print_while_stmnt(Node *, int);

	n->check = check_while_stmnt;
	n->visit = visit_while_stmnt;
	n->print = print_while_stmnt;

	/* condition and block must be added later in order
	 * to record the correct source code position
	 */
}


static void create_do_stmnt(Node *n, va_list argp)
{
	void check_do_stmnt(Node *n);
	void visit_do_stmnt(Node *, Stack *);
	void print_do_stmnt(Node *, int);

	n->check = check_do_stmnt;
	n->visit = visit_do_stmnt;
	n->print = print_do_stmnt;

	n->loop_stmnt.condition = va_arg(argp, Node *);
	n->loop_stmnt.block = va_arg(argp, Node *);
}


static void create_for_stmnt(Node *n, va_list argp)
{
	void check_for_stmnt(Node *n);
	void visit_for_stmnt(Node *, Stack *);
	void print_for_stmnt(Node *, int);

	n->check = check_for_stmnt;
	n->visit = visit_for_stmnt;
	n->print = print_for_stmnt;

	n->for_stmnt.name = strdup(va_arg(argp, char *));
	n->for_stmnt.expression = va_arg(argp, Node *);

	/* block must be added later in order to record
	 * the correct source code position
	 */
}


static void create_print_stmnt(Node *n, va_list argp)
{
	void check_print_stmnt(Node *n);
	void visit_print_stmnt(Node *, Stack *);
	void print_print_stmnt(Node *, int);

	n->check = check_print_stmnt;
	n->visit = visit_print_stmnt;
	n->print = print_print_stmnt;

	n->print_stmnt.raw = va_arg(argp, int);
	n->print_stmnt.expressions = array_alloc();
}


static void create_return_stmnt(Node *n, va_list argp)
{
	void check_return_stmnt(Node *n);
	void visit_return_stmnt(Node *, Stack *);
	void print_return_stmnt(Node *, int);

	n->check = check_return_stmnt;
	n->visit = visit_return_stmnt;
	n->print = print_return_stmnt;

	n->return_stmnt.value = va_arg(argp, Node *);
}


static void create_import_stmnt(Node *n, va_list argp)
{
	void check_import_stmnt(Node *n);
	void visit_import_stmnt(Node *, Stack *);
	void print_import_stmnt(Node *, int);

	n->check = check_import_stmnt;
	n->visit = visit_import_stmnt;
	n->print = print_import_stmnt;

	n->import_stmnt.name = strdup(va_arg(argp, char *));
	n->import_stmnt.code = va_arg(argp, Node *);
}


static void create_input_stmnt(Node *n)
{
	void check_input_stmnt(Node *n);
	void visit_input_stmnt(Node *, Stack *);
	void print_input_stmnt(Node *n, int);

	n->check = check_input_stmnt;
	n->visit = visit_input_stmnt;
	n->print = print_input_stmnt;

	n->input_stmnt.prompts = array_alloc();
	n->input_stmnt.identifiers = array_alloc();
}


static void create_pass_stmnt(Node *n)
{
	void check_pass_stmnt(Node *n);
	void visit_pass_stmnt(Node *, Stack *);
	void print_pass_stmnt(Node *, int);

	n->check = check_pass_stmnt;
	n->visit = visit_pass_stmnt;
	n->print = print_pass_stmnt;
}


static void create_break_stmnt(Node *n)
{
	void check_break_stmnt(Node *n);
	void visit_break_stmnt(Node *, Stack *);
	void print_break_stmnt(Node *, int);

	n->check = check_break_stmnt;
	n->visit = visit_break_stmnt;
	n->print = print_break_stmnt;
}


static void create_continue_stmnt(Node *n)
{
	void check_continue_stmnt(Node *n);
	void visit_continue_stmnt(Node *, Stack *);
	void print_continue_stmnt(Node *, int);

	n->check = check_continue_stmnt;
	n->visit = visit_continue_stmnt;
	n->print = print_continue_stmnt;
}


/* API: Create a new AST node
 *
 * type		type of node to create
 * ...		variable number of arguments, dependent on node type
 * return	pointer to a new AST node
 *
 */
Node *create(nodetype_t type, ...)
{
	va_list argp;
	Node *n;

	if ((n = calloc(1, sizeof(Node))) == NULL)
		raise(OutOfMemoryError);
	else {
		n->type = type;

		/* record where we are in the code to be able to print error
		 * or debug messages.
		 */
		n->source.module = scanner.module;
		n->source.lineno = scanner.module->lineno;
		n->source.bol = scanner.module->bol;

		n->method.valid = false;

		va_start(argp, type);

		switch (type) {
			case BLOCK:
				create_block(n);
				break;
			case LITERAL:
				create_literal(n, argp);
				break;
			case UNARY:
				create_unary(n, argp);
				break;
			case BINARY:
				create_binary(n, argp);
				break;
			case COMMA_EXPR:
				create_comma_expr(n);
				break;
			case ARGLIST:
				create_arglist(n);
				break;
			case INDEX:
				create_index(n, argp);
				break;
			case SLICE:
				create_slice(n, argp);
				break;
			case ASSIGNMENT:
				create_assignment(n, argp);
				break;
			case REFERENCE:
				create_reference(n, argp);
				break;
			case FUNCTION_CALL:
				create_function_call(n, argp);
				break;
			case EXPRESSION_STMNT:
				create_expression_stmnt(n, argp);
				break;
			case FUNCTION_DECLARATION:
				create_function_declaration(n, argp);
				break;
			case VARIABLE_DECLARATION:
				create_variable_declaration(n);
				break;
			case DEF_VAR:
				create_defvar(n, argp);
				break;
			case IF_STMNT:
				create_if_stmnt(n);
				break;
			case WHILE_STMNT:
				create_while_stmnt(n);
				break;
			case DO_STMNT:
				create_do_stmnt(n, argp);
				break;
			case FOR_STMNT:
				create_for_stmnt(n, argp);
				break;
			case PRINT_STMNT:
				create_print_stmnt(n, argp);
				break;
			case RETURN_STMNT:
				create_return_stmnt(n, argp);
				break;
			case IMPORT_STMNT:
				create_import_stmnt(n, argp);
				break;
			case INPUT_STMNT:
				create_input_stmnt(n);
				break;
			case PASS_STMNT:
				create_pass_stmnt(n);
				break;
			case BREAK_STMNT:
				create_break_stmnt(n);
				break;
			case CONTINUE_STMNT:
				create_continue_stmnt(n);
				break;
		}

		va_end(argp);
	}
	return n;
}
