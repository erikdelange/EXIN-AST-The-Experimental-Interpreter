/* visit.c
 *
 * Functions to print, check or execute an AST.
 *
 * All are implemented using a visitor pattern.
 * See: https://en.wikipedia.org/wiki/Visitor_pattern
 *
 * During execution values (represented by objects) are exchanged between functions via
 * a stack. In this way function prototypes for the visitor functions are uniform and
 * not dependent on the number of arguments.
 *
 * 2020 K.W.E. de Lange
 */
#include <stdlib.h>
#include <string.h>

#include "identifier.h"
#include "function.h"
#include "scanner.h"
#include "object.h"
#include "error.h"
#include "visit.h"
#include "list.h"


static int do_break = 0;	/* If true busy quiting loop because of break */
static int do_continue = 0;	/* If true busy quiting loop because of continue */
static int do_return = 0;	/* If true busy exiting block or module because of return */


Node *current_node = NULL;	/* During check and visit used to keep track of the node
							 * currently executed for easy error reporting. NULL while
							 * still parsing the source code.
							 */


/* Utility to print the format string indented by 'level' indents
 * of 2 characters each.
 */
static void printf_indent(const int level, const char *format, ...)
{
	va_list argp;

	for (int i = 0; i < level; i++)
		printf("| ");

    va_start(argp, format);
	vprintf(format, argp);
	va_end(argp);
}


/* API: Print a node.
 *
 * Prints a node. Each node is responsible to print its child nodes (if any).
 *
 * n		node to print
 * level	how much indentation to use when printing the node content
 */
void print(Node *n, int level)
{
	Node *tmp = current_node;
	current_node = n;

	printf_indent(level, "%s\n", nodetypeName(n->type));

	n->print(n, level);

	if (n->method.valid) {
		printf_indent(level + 1, "METHOD %s\n", n->method.name);

		for (size_t i = 0; i != n->method.arguments->size; i++)
			print(n->method.arguments->element[i], level + 2);
	}

	current_node = tmp;
}


/* API: Static code checks.
 *
 * Perform all checks possible without actually executing the code.
 *
 * Any check done here does not need to be repeated during
 * execution (and thus is not).
 *
 * n		node to check
 */
void check(Node *n)
{
	Node *tmp = current_node;
	current_node = n;

	n->check(n);

	current_node = tmp;
}


/* API: Execute a node.
 *
 * n		node to execute
 * s		stack to use when exchanging values
 */
void visit(Node *n, Stack *s)
{
	Node *tmp = current_node;
	current_node = n;

	n->visit(n, s);

	if (n->method.valid) {
		Object *obj = pop(s);
		push(s, obj_method(isListNode(obj) ? obj_from_listnode(obj) : obj, n->method.name, n->method.arguments));
		obj_decref(obj);
	}

	current_node = tmp;
}


/* For every nodetype a print, check and visit function are defined below.
 * These functions have global scope, whereas static might be expected.
 * This is done because to be able to use a visitor pattern efficiently
 * every node struct contains pointers to the functions to print, check
 * and visit that node. These pointers are assigned their values in
 * another source file, hence the global definitions here.
 */


void print_block(Node *n, int level)
{
	for (size_t i = 0; i != n->block.statements->size; i++)
		print(n->block.statements->element[i], level + 1);
}


void check_block(Node *n)
{
	for (size_t i = 0; i != n->block.statements->size; i++)
		check(n->block.statements->element[i]);
}


void visit_block(Node *n, Stack *s)
{
	for (size_t i = 0; i != n->block.statements->size && !(do_break || do_continue); i++)
		visit(n->block.statements->element[i], s);
}


void print_literal(Node *n, int level)
{
	printf_indent(level + 1, "TYPE %s\n", variabletypeName(n->literal.type));

	switch (n->literal.type) {
		case VT_CHAR:
			printf_indent(level + 1, "VALUE \'%s\'\n", n->literal.value);
			break;
		case VT_STR:
			printf_indent(level + 1, "VALUE \"%s\"\n", n->literal.value);
			break;
		default:
			printf_indent(level + 1, "VALUE %s\n", n->literal.value);
	}
}


void check_literal(Node *n)
{
	switch (n->literal.type) {
		case VT_CHAR:
			str_to_char(n->literal.value);  /* only check if conversion is possible */
			break;
		case VT_INT:
			str_to_int(n->literal.value);
			break;
		case VT_FLOAT:
			str_to_float(n->literal.value);
			break;
		case VT_STR:
			/* nothing to check here n->literal.value is already a string */
			break;
		case VT_LIST:
			raise(DesignError, "literals of type VT_LIST are not implemented");
			break;
		default:
			raise(DesignError, "unknown literal type %d", n->literal.type);
			break;
	}
}


void visit_literal(Node *n, Stack *s)
{
	switch (n->literal.type) {
		case VT_CHAR:
			push(s, obj_create(CHAR_T, str_to_char(n->literal.value)));
			break;
		case VT_INT:
			push(s, obj_create(INT_T, str_to_int(n->literal.value)));
			break;
		case VT_FLOAT:
			push(s, obj_create(FLOAT_T, str_to_float(n->literal.value)));
			break;
		case VT_STR:
			push(s, obj_create(STR_T, n->literal.value));
			break;
		default:
			break;
	}
}


void print_unary(Node *n, int level)
{
	printf_indent(level + 1, "OPERATOR %s\n", unaryoperatorName(n->unary.operator));

	print(n->unary.operand, level + 1);
}


void check_unary(Node *n)
{
	switch (n->unary.operator) {
		case UNOT:
		case UMINUS:
		case UPLUS:
			break;
		default:
 			raise(DesignError, "unknown unary operator %d", n->unary.operator);
			break;
	}

	check(n->unary.operand);
}


void visit_unary(Node *n, Stack *s)
{
	Object *obj;

	visit(n->unary.operand, s);

	switch (n->unary.operator) {
		case UNOT:
			obj = pop(s);
			push(s, obj_negate(obj));
			obj_decref(obj);
			break;
		case UMINUS:
			obj = pop(s);
			push(s, obj_invert(obj));
			obj_decref(obj);
			break;
		case UPLUS:
			break;
	}
}


void print_binary(Node *n, int level)
{
	printf_indent(level + 1, "OPERATOR %s\n", binaryoperatorName(n->binary.operator));

	print(n->binary.left, level + 1);
	print(n->binary.right, level + 1);
}


void check_binary(Node *n)
{
	switch (n->binary.operator) {
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case LSS:
		case LEQ:
		case GTR:
		case GEQ:
		case EQ:
		case NEQ:
		case OP_IN:
		case LOGICAL_AND:
		case LOGICAL_OR:
			break;
		default:
 			raise(DesignError, "unknown binary operator %d", n->binary.operator);
			break;
	}

	check(n->binary.left);
	check(n->binary.right);
}


void visit_binary(Node *n, Stack *s)
{
	Object *left, *right;

	visit(n->binary.left, s);
	left = pop(s);

	visit(n->binary.right, s);
	right = pop(s);

	switch (n->binary.operator) {
		case ADD:
			push(s, obj_add(left, right));
			break;
		case SUB:
			push(s, obj_sub(left, right));
			break;
		case MUL:
			push(s, obj_mult(left, right));
			break;
		case DIV:
			push(s, obj_divs(left, right));
			break;
		case MOD:
			push(s, obj_mod(left, right));
			break;
		case LSS:
			push(s, obj_lss(left, right));
			break;
		case LEQ:
			push(s, obj_leq(left, right));
			break;
		case GTR:
			push(s, obj_gtr(left, right));
			break;
		case GEQ:
			push(s, obj_geq(left, right));
			break;
		case EQ:
			push(s, obj_eql(left, right));
			break;
		case NEQ:
			push(s, obj_neq(left, right));
			break;
		case OP_IN:
			push(s, obj_in(left, right));
			break;
		case LOGICAL_AND:
			push(s, obj_and(left, right));
			break;
		case LOGICAL_OR:
			push(s, obj_or(left, right));
			break;
	}

	obj_decref(left);
	obj_decref(right);
}


void print_comma_expr(Node *n, int level)
{
	for (size_t i = 0; i != n->comma_expr.expressions->size; i++)
		print(n->comma_expr.expressions->element[i], level + 1);
}


void check_comma_expr(Node *n)
{
	for (size_t i = 0; i != n->comma_expr.expressions->size; i++)
		check(n->comma_expr.expressions->element[i]);
}


void visit_comma_expr(Node *n, Stack *s)
{
	for (size_t i = 0; i != n->comma_expr.expressions->size; i++) {
		visit(n->comma_expr.expressions->element[i], s);
		if (i == n->comma_expr.expressions->size - 1)
			break;  /* last expression reached */
		else
			obj_decref(pop(s));  /* only result from last expression is used */
	}
}


void print_arglist(Node *n, int level)
{
	for (size_t i = 0; i != n->arglist.arguments->size; i++)
		print(n->arglist.arguments->element[i], level + 1);
}


void check_arglist(Node *n)
{
	for (size_t i = 0; i != n->arglist.arguments->size; i++)
		check(n->arglist.arguments->element[i]);
}


void visit_arglist(Node *n, Stack *s)
{
	Object *obj, *arg;

	obj = obj_alloc(LIST_T);

	for (size_t i = 0; i != n->arglist.arguments->size; i++) {
		visit(n->arglist.arguments->element[i], s);
		arg = pop(s);
		listtype.append((ListObject *)obj, obj_copy(arg));
		obj_decref(arg);
	}

	push(s, obj);
}


void print_index(Node *n, int level)
{
	print(n->index.sequence, level + 1);
	print(n->index.index, level + 1);
}


void check_index(Node *n)
{
	check(n->index.sequence);
	check(n->index.index);
}


void visit_index(Node *n, Stack *s)
{
	Object *obj, *sequence, *index;

	visit(n->index.sequence, s);
	sequence = pop(s);

	visit(n->index.index, s);
	index = pop(s);

	obj = obj_item(sequence, obj_as_int(index));

	obj_decref(index);
	obj_decref(sequence);

	push(s, obj);
}


void print_slice(Node *n, int level)
{
	print(n->slice.sequence, level + 1);
	print(n->slice.start, level + 1);
	print(n->slice.end, level + 1);
}


void check_slice(Node *n)
{
	check(n->slice.sequence);
	check(n->slice.start);
	check(n->slice.end);
}


void visit_slice(Node *n, Stack *s)
{
	Object *obj, *sequence, *start, *end;

	visit(n->slice.sequence, s);
	sequence = pop(s);

	visit(n->slice.start, s);
	start = pop(s);

	visit(n->slice.end, s);
	end = pop(s);

	obj = obj_slice(sequence, obj_as_int(start), obj_as_int(end));

	obj_decref(end);
	obj_decref(start);
	obj_decref(sequence);

	push(s, obj);
}


void print_assignment(Node *n, int level)
{
	printf_indent(level + 1, "OPERATOR %s\n", assignmentoperatorName(n->assignment.operator));

	print(n->assignment.variable, level + 1);
	print(n->assignment.expression, level + 1);
}


void check_assignment(Node *n)
{
	switch (n->assignment.operator) {
		case ASSIGN:
		case ADDASSIGN:
		case SUBASSIGN:
		case MULASSIGN:
		case DIVASSIGN:
		case MODASSIGN:
			break;
		default:
			raise(DesignError, "unknown assignment operator %d", n->assignment.operator);
			break;
	}

	check(n->assignment.variable);
	check(n->assignment.expression);
}


void visit_assignment(Node *n, Stack *s)
{
	Object *target, *value, *tmp;

	visit(n->assignment.variable, s);
	target = pop(s);

	visit(n->assignment.expression, s);
	value = pop(s);

	switch (n->assignment.operator) {
		case ASSIGN:
			tmp = obj_copy(value);
			break;
		case ADDASSIGN:
			tmp = obj_add(target, value);
			break;
		case SUBASSIGN:
			tmp = obj_sub(target, value);
			break;
		case MULASSIGN:
			tmp = obj_mult(target, value);
			break;
		case DIVASSIGN:
			tmp = obj_divs(target, value);
			break;
		case MODASSIGN:
			tmp = obj_mod(target, value);
			break;
		default:
			tmp = obj_alloc(NONE_T);
	}

	obj_assign(target, tmp);
	obj_decref(tmp);
	obj_decref(value);

	push(s, target);
}


void print_reference(Node *n, int level)
{
	printf_indent(level + 1, "NAME %s\n", n->reference.name);
}


void check_reference(Node *n)
{
	Identifier *id;

	if ((id = identifier.search(n->reference.name)) == NULL)
		raise(NameError, "identifier %s is not defined", n->reference.name);

	if (id->type != VARIABLE)
		raise(TypeError, "identifier %s is not a variable", n->reference.name);
}


void visit_reference(Node *n, Stack *s)
{
	Identifier *id;

	id = identifier.search(n->reference.name);

	obj_incref(id->object);
	push(s, id->object);
}


void print_function_call(Node *n, int level)
{
	printf_indent(level + 1, "NAME %s\n", n->function_call.name);
	printf_indent(level + 1, "BUILTIN = %s\n", n->function_call.builtin == true ? "TRUE" : "FALSE");

	for (size_t i = 0; i != n->function_call.arguments->size; i++)
		print(n->function_call.arguments->element[i], level + 1);
}


void check_function_call(Node *n)
{
	Identifier *id;

	if (n->function_call.builtin == false) {
		if (n->function_call.checked == false) {
			n->function_call.checked = true;  /* avoid recursive checks in case of recursive function calls */

			if ((id = identifier.search(n->function_call.name)) == NULL)
				raise(NameError, "identifier %s is not defined", n->function_call.name);

			if (id->node->type != FUNCTION_DECLARATION)
				raise(TypeError, "identifier %s is not a function", n->function_call.name);

			if (id->node->function_declaration.arguments->size != n->function_call.arguments->size)
				raise(SyntaxError, "%d argument(s) expected, %d found", \
								   id->node->function_declaration.arguments->size, n->function_call.arguments->size);

			scope.append_level();

			/* create the identifier for the formal function arguments */
			for (size_t i = 0; i != id->node->function_declaration.arguments->size; i++)
				identifier.add(VARIABLE, id->node->function_declaration.arguments->element[i]);

			check(id->node->function_declaration.block);

			scope.remove_level();
		}
	} else {  /* builtin == true */
		if (n->function_call.arguments->size != builtin_argc(n->function_call.name))
				raise(SyntaxError, "builtin function %s expects %d argument(s) but %d were given", \
		              n->function_call.name, builtin_argc(n->function_call.name), n->function_call.arguments->size);
	}
}


void visit_function_call(Node *n, Stack *s)
{
	Node *fdecl;
	Identifier *id;
	Array *args = array_alloc();

	/* place the actual arguments objects in an array */
	for (size_t i = 0; i != n->function_call.arguments->size; i++) {
		visit(n->function_call.arguments->element[i], s);
		array_append_child(args, pop(s));
	}

	if (n->function_call.builtin == true)
		visit_builtin(n->function_call.name, args ,s);
	else {  /* builtin == false */
		id = identifier.search(n->function_call.name);
		fdecl = id->node;

		scope.append_level();

		for (size_t i = 0; i != args->size; i++) {
			id = identifier.add(VARIABLE, fdecl->function_declaration.arguments->element[i]);
			identifier.bind(id, obj_copy(args->element[i]));  /* create local copy */
			obj_decref(args->element[i]);  /* release original argument */
		}

		visit(fdecl->function_declaration.block, s);

		scope.remove_level();

		if (do_return == 0)
			push(s, obj_create(INT_T, 0));  /* result if the function did not end with a RETURN statement */

		do_return = 0;
	}

	array_free(args);
}


void print_expression_stmnt(Node *n, int level)
{
	print(n->expression_stmnt.expression, level + 1);
}


void check_expression_stmnt(Node *n)
{
	check(n->expression_stmnt.expression);
}


void visit_expression_stmnt(Node *n, Stack *s)
{
	Object *obj;

	visit(n->expression_stmnt.expression, s);

	obj = pop(s);
	obj_decref(obj);  /* expression statements do not have a result */
}


void print_function_declaration(Node *n, int level)
{
	printf_indent(level + 1, "NAME %s\n", n->function_declaration.name);
	printf_indent(level + 1, "ARGUMENTS ");

	for (size_t i = 0; i != n->function_declaration.arguments->size; i++)
		if (i == 0)
			printf("%s", (char *)n->function_declaration.arguments->element[i]);
		else
			printf(", %s", (char *)n->function_declaration.arguments->element[i]);

	printf("\n");

	print(n->function_declaration.block, level + 1);
}


void check_function_declaration(Node *n)
{
	Identifier *id;

	if (is_builtin(n->function_declaration.name) == true)
		raise(NameError, "builtin function %s cannot be redefined", n->function_declaration.name);

	if ((id = identifier.add(FUNCTION, n->function_declaration.name)) == NULL)
		raise(NameError, "identifier %s already declared", n->function_declaration.name);

	identifier.bind(id, n);

	scope.append_level();

	for (size_t i = 0; i != n->function_declaration.arguments->size; i++)
		identifier.add(VARIABLE, (char *)n->function_declaration.arguments->element[i]);

	check(n->function_declaration.block);

	scope.remove_level();
}


void visit_function_declaration(Node *n, Stack *s)
{
	UNUSED(s);

	Identifier *id;

	id = identifier.add(FUNCTION, n->function_declaration.name);
	identifier.bind(id, n);
}


void print_variable_declaration(Node *n, int level)
{
	for (size_t i = 0; i != n->variable_declaration.defvars->size; i++)
		print(n->variable_declaration.defvars->element[i], level + 1);
}


void check_variable_declaration(Node *n)
{
	for (size_t i = 0; i != n->variable_declaration.defvars->size; i++)
		check(n->variable_declaration.defvars->element[i]);
}


void visit_variable_declaration(Node *n, Stack *s)
{
	for (size_t i = 0; i != n->variable_declaration.defvars->size; i++)
		visit(n->variable_declaration.defvars->element[i], s);
}


void print_defvar(Node *n, int level)
{
	printf_indent(level + 1, "NAME %s\n", n->defvar.name);
	printf_indent(level + 1, "TYPE %s\n", variabletypeName(n->defvar.type));

	if (n->defvar.initialvalue)
		print(n->defvar.initialvalue, level + 1);
}


void check_defvar(Node *n)
{
	if (is_builtin(n->defvar.name) == true)
		raise(NameError, "%s is a builtin function", n->defvar.name);

	if (identifier.add(VARIABLE ,n->defvar.name) == NULL)
		raise(NameError, "identifier %s already declared", n->defvar.name);

	if (n->defvar.initialvalue)
		check(n->defvar.initialvalue);
}


void visit_defvar(Node *n, Stack *s)
{
	Identifier *id;
	Object *obj;

	id = identifier.add(VARIABLE, n->defvar.name);

	switch (n->defvar.type) {
		case VT_CHAR:
			obj = obj_alloc(CHAR_T);
			break;
		case VT_INT:
			obj = obj_alloc(INT_T);
			break;
		case VT_FLOAT:
			obj = obj_alloc(FLOAT_T);
			break;
		case VT_STR:
			obj = obj_alloc(STR_T);
			break;
		case VT_LIST:
			obj = obj_alloc(LIST_T);
			break;
		default:
			obj = obj_alloc(NONE_T);
	}

	identifier.bind(id, obj);

	if (n->defvar.initialvalue) {
		visit(n->defvar.initialvalue, s);
		obj = pop(s);
		obj_assign(id->object, obj);
		obj_decref(obj);
	}
}


void print_if_stmnt(Node *n, int level)
{
	print(n->if_stmnt.condition, level + 1);
	print(n->if_stmnt.consequent, level + 1);

	if (n->if_stmnt.alternative)
		print(n->if_stmnt.alternative, level + 1);
}


void check_if_stmnt(Node *n)
{
	check(n->if_stmnt.condition);
	check(n->if_stmnt.consequent);

	if (n->if_stmnt.alternative)
		check(n->if_stmnt.alternative);
}


void visit_if_stmnt(Node *n, Stack *s)
{
	Object *obj;

	visit(n->if_stmnt.condition, s);
	obj = pop(s);

	if (obj_as_bool(obj) == true)
		visit(n->if_stmnt.consequent, s);
	else if (n->if_stmnt.alternative)
		visit(n->if_stmnt.alternative, s);

	obj_decref(obj);
}


void print_while_stmnt(Node *n, int level)
{
	print(n->loop_stmnt.condition, level + 1);
	print(n->loop_stmnt.block, level + 1);
}


void check_while_stmnt(Node *n)
{
	check(n->loop_stmnt.condition);
	check(n->loop_stmnt.block);
}


void visit_while_stmnt(Node *n, Stack *s)
{
	bool condition;
	Object *obj;

	do_break = do_continue = 0;

	while (1) {
		visit(n->loop_stmnt.condition, s);
		obj = pop(s);
		condition = obj_as_bool(obj);
		obj_decref(obj);

		if (condition == false || do_break || do_return)
			break;

		visit(n->loop_stmnt.block, s);

		do_continue = 0;
	}

	do_break = 0;
}


void print_do_stmnt(Node *n, int level)
{
	print(n->loop_stmnt.block, level + 1);
	print(n->loop_stmnt.condition, level + 1);
}


void check_do_stmnt(Node *n)
{
	check(n->loop_stmnt.block);
	check(n->loop_stmnt.condition);
}


void visit_do_stmnt(Node *n, Stack *s)
{
	bool condition;
	Object *obj;

	do_break = do_continue = 0;

	while (1) {
		visit(n->loop_stmnt.block, s);
		do_continue = 0;

		visit(n->loop_stmnt.condition, s);
		obj = pop(s);
		condition = obj_as_bool(obj);
		obj_decref(obj);

		if (condition == false || do_break || do_return)
			break;
	}

	do_break = 0;
}


void print_for_stmnt(Node *n, int level)
{
	printf_indent(level + 1, "TARGET %s\n", n->for_stmnt.name);

	print(n->for_stmnt.expression, level + 1);
	print(n->for_stmnt.block, level + 1);
}


void check_for_stmnt(Node *n)
{
	if (identifier.search(n->for_stmnt.name) == NULL)
		identifier.add(VARIABLE, n->for_stmnt.name);

	check(n->for_stmnt.expression);
	check(n->for_stmnt.block);
}


void visit_for_stmnt(Node *n, Stack *s)
{
	int_t len;
	Object *seq;
	Identifier *id;

	if ((id = identifier.search(n->for_stmnt.name)) == NULL)
		id = identifier.add(VARIABLE, n->for_stmnt.name);

	identifier.bind(id, obj_alloc(NONE_T));  /* result for empty lists or strings */

	visit(n->for_stmnt.expression, s);

	seq = pop(s);
	len = obj_length(seq);

	do_break = do_continue = 0;

	for (int_t i = 0; i < len && !do_break && !do_return; i++) {
		identifier.bind(id, obj_item(seq, i));  /* bind() implicitly unbinds the previous object */
		visit(n->for_stmnt.block, s);
		do_continue = 0;
	}

	do_break = 0;

	obj_decref(seq);
}


void print_print_stmnt(Node *n, int level)
{
	printf_indent(level + 1, "RAW = %s\n", n->print_stmnt.raw == true ? "TRUE" : "FALSE");

	for (size_t i = 0; i != n->print_stmnt.expressions->size; i++)
		print(n->print_stmnt.expressions->element[i], level + 1);
}


void check_print_stmnt(Node *n)
{
	for (size_t i = 0; i != n->print_stmnt.expressions->size; i++)
		check(n->print_stmnt.expressions->element[i]);
}


void visit_print_stmnt(Node *n, Stack *s)
{
	bool first = true;
	Object *obj;

	for (size_t i = 0; i != n->print_stmnt.expressions->size; i++) {
		if (first == true)
			first = false;
		else  /* first == false */
			if (n->print_stmnt.raw == false)
				printf(" ");

		visit(n->print_stmnt.expressions->element[i], s);

		obj = pop(s);

		#ifdef VT100
		debug_printf(~NODEBUG, "%c[032m", 27);  /* VT100 green foreground */
		#endif  /* VT100 */

		obj_print(stdout, obj);

		#ifdef VT100
		debug_printf(~NODEBUG, "%c[0m", 27);  /* VT100 standard foreground */
		#endif  /* VT100 */

		obj_decref(obj);
	}

	if (n->print_stmnt.raw == false)
		printf("\n");
}


void print_return_stmnt(Node *n, int level)
{
	if (n->return_stmnt.value)
		print(n->return_stmnt.value, level + 1);
}


void check_return_stmnt(Node *n)
{
	if (n->return_stmnt.value)
		check(n->return_stmnt.value);
}


void visit_return_stmnt(Node *n, Stack *s)
{
	if (n->return_stmnt.value == NULL)
		push(s, obj_create(INT_T, 0));
	else
		visit(n->return_stmnt.value, s);

	do_return = 1;
}


void print_import_stmnt(Node *n, int level)
{
	printf_indent(level + 1, "MODULE %s\n", n->import_stmnt.name);

	print(n->import_stmnt.code, level + 1);
}


void check_import_stmnt(Node *n)
{
	check(n->import_stmnt.code);
}


void visit_import_stmnt(Node *n, Stack *s)
{
	visit(n->import_stmnt.code, s);
}


void print_input_stmnt(Node *n, int level)
{
	for (size_t i = 0; i != n->input_stmnt.prompts->size; i++) {
		if (n->input_stmnt.prompts->element[i]) {
			printf_indent(level + 1, "PROMPT %s\n", (char *)n->input_stmnt.prompts->element[i]);
		}
		printf_indent(level + 1, "IDENTIFIER %s\n", (char *)n->input_stmnt.identifiers->element[i]);
	}
}


void check_input_stmnt(Node *n)
{
	Identifier *id;

	for (size_t i = 0; i != n->input_stmnt.identifiers->size; i++) {
		if ((id = identifier.search(n->input_stmnt.identifiers->element[i])) == NULL)
			raise(NameError, "identifier %s is not defined", n->input_stmnt.identifiers->element[i]);
		if (id->type != VARIABLE)
			raise(TypeError, "identifier %s is not a variable", n->input_stmnt.identifiers->element[i]);
	}
}


void visit_input_stmnt(Node *n, Stack *s)
{
	UNUSED(s);

	Identifier *id;
	Object *obj;

	for (size_t i = 0; i != n->input_stmnt.identifiers->size; i++) {
		if (n->input_stmnt.prompts->element[i])
			printf("%s", (char *)n->input_stmnt.prompts->element[i]);

		id = identifier.search(n->input_stmnt.identifiers->element[i]);
		obj = obj_scan(stdin, TYPE(id->object));
		identifier.bind(id, obj);
	}
}


void print_pass_stmnt(Node *n, int level)
{
	UNUSED(n);
	UNUSED(level);
}


void check_pass_stmnt(Node *n)
{
	UNUSED(n);
}


void visit_pass_stmnt(Node *n, Stack *s)
{
	UNUSED(n);
	UNUSED(s);
}


void print_break_stmnt(Node *n, int level)
{
	UNUSED(n);
	UNUSED(level);
}


void check_break_stmnt(Node *n)
{
	UNUSED(n);
}


void visit_break_stmnt(Node *n, Stack *s)
{
	UNUSED(n);
	UNUSED(s);

	do_break = 1;
}


void print_continue_stmnt(Node *n, int level)
{
	UNUSED(n);
	UNUSED(level);
}


void check_continue_stmnt(Node *n)
{
	UNUSED(n);
}


void visit_continue_stmnt(Node *n, Stack *s)
{
	UNUSED(n);
	UNUSED(s);

	do_continue = 1;
}
