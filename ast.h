/* ast.h
 *
 * 2020 K.W.E. de Lange
 */
#ifndef _AST_
#define _AST_

#include <stdbool.h>
#include "module.h"
#include "stack.h"
#include "array.h"


typedef enum { LITERAL=1, ARGLIST, UNARY, BINARY, ASSIGNMENT, BLOCK, REFERENCE, VARIABLE_DECLARATION,
			   DEF_VAR, FUNCTION_DECLARATION, COMMA_EXPR, IF_STMNT, PRINT_STMNT, RETURN_STMNT, EXPRESSION_STMNT,
			   WHILE_STMNT, DO_STMNT, PASS_STMNT, FOR_STMNT, IMPORT_STMNT, INPUT_STMNT,
			   BREAK_STMNT, CONTINUE_STMNT, INDEX, SLICE, FUNCTION_CALL } nodetype_t;

static inline char *nodetypeName(nodetype_t nt)
{
	static char *string[] = {
		"?", "LITERAL", "ARGLIST", "UNARY", "BINARY", "ASSIGNMENT", "BLOCK", "REFERENCE", "VARIABLE_DECLARATION",
		"DEF_VAR", "FUNCTION_DECLARATION", "COMMA_EXPR", "IF_STMNT", "PRINT_STMNT", "RETURN_STMNT", "EXPRESSION_STMNT",
		"WHILE_STMNT", "DO_STMNT", "PASS_STMNT", "FOR_STMNT", "IMPORT_STMNT", "INPUT_STMNT",
		"BREAK_STMNT", "CONTINUE_STMNT", "INDEX", "SLICE", "FUNCTION_CALL"
	};

	if (nt < 0 || nt > (sizeof(string) / sizeof(string[0]) - 1))
		nt = 0;  /* out of bound values revert to 0 */

	return string[nt];
}


typedef enum { UNOT=1, UMINUS, UPLUS } unaryoperator_t;

static inline char *unaryoperatorName(unaryoperator_t op)
{
	static char *string[] = {
		"?", "NOT", "MINUS", "PLUS"
	};

	if (op < 0 || op > (sizeof(string) / sizeof(string[0]) - 1))
		op = 0;  /* out of bound values revert to 0 */

	return string[op];
}


typedef enum { ADD=1, SUB, MUL, DIV, MOD, LOGICAL_AND,
			   LOGICAL_OR, LSS, LEQ, GEQ, GTR, EQ, NEQ, OP_IN } binaryoperator_t;

static inline char *binaryoperatorName(binaryoperator_t op)
{
	static char *string[] = {
		"?", "ADD", "SUB", "MUL", "DIV", "MOD", "LOGICAL_AND",
		"LOGICAL_OR", "LSS", "LEQ", "GEQ", "GTR", "EQ", "NEQ", "IN"
	};

	if (op < 0 || op > (sizeof(string) / sizeof(string[0]) - 1))
		op = 0;  /* out of bound values revert to 0 */

	return string[op];
}


typedef enum { ASSIGN=1, ADDASSIGN, SUBASSIGN, MULASSIGN, DIVASSIGN, MODASSIGN } assignmentoperator_t;

static inline char *assignmentoperatorName(assignmentoperator_t op)
{
	static char *string[] = {
		"?", "ASSIGN", "ADDASSIGN", "SUBASSIGN", "MULASSIGN", "DIVASSIGN", "MODASSIGN"
	};

	if (op < 0 || op > (sizeof(string) / sizeof(string[0]) - 1))
		op = 0;  /* out of bound values revert to 0 */

	return string[op];
}


/* literal variable types
 */
typedef enum { VT_CHAR=1, VT_INT, VT_FLOAT, VT_STR, VT_LIST } variabletype_t;

static inline char *variabletypeName(variabletype_t vt)
{
	static char *string[] = {
		"?", "CHAR", "INT", "FLOAT", "STR", "LIST"
	};

	if (vt < 0 || vt > (sizeof(string) / sizeof(string[0]) - 1))
		vt = 0;  /* out of bound values revert to 0 */

	return string[vt];
}


/* A node is a struct which combines the data of all possible
 * nodetypes in an anonymous union.
 *
 * Every node contains a reference to where it originated
 * from in the source code (struct source).
 *
 * Optionally a method and its arguments can be recorded
 * (struct method).
 *
 * Every node includes pointers to functions to print,
 * check or visit the node.
 *
 */
typedef struct node {
	nodetype_t type;

	struct source {
		Module *module;
		size_t lineno;
		size_t bol;
	} source;

	struct method {
		bool valid;
		char *name;
		struct array *arguments;
	} method;

	union {  /* which struct to use depends on node type */
		struct {
			struct array *statements;
		} block;

		struct {
			variabletype_t type;
			char *value;
		} literal;

		struct {
			unaryoperator_t operator;
			struct node *operand;
		} unary;

		struct {
			binaryoperator_t operator;
			struct node *left;
			struct node *right;
		} binary;

		struct {
			struct array *expressions;
		} comma_expr;

		struct {
			struct array *arguments;
		} arglist;

		struct {
			struct node *sequence;
			struct node *index;
		} index;

		struct {
			struct node *sequence;
			struct node *start;
			struct node *end;
		} slice;

		struct {
			assignmentoperator_t operator;
			struct node *variable;
			struct node *expression;
		} assignment;

		struct {
			char *name;
		} reference;

		struct {
			char *name;
			struct array *arguments;
			bool builtin;
			bool checked;
		} function_call;

		struct {
			struct node *expression;
		} expression_stmnt;

		struct {
			char *name;
			struct array *arguments;
			struct node *block;
		} function_declaration;

		struct {
			struct array *defvars;
		} variable_declaration;

		struct {
			variabletype_t type;
			char *name;
			struct node *initialvalue;
		} defvar;

		struct {
			struct node *condition;
			struct node *consequent;
			struct node *alternative;
		} if_stmnt;

		struct {
			struct node *condition;
			struct node *block;
		} loop_stmnt;

		struct {
			char *name;
			struct node *expression;
			struct node *block;
		} for_stmnt;

		struct {
			bool raw;
			struct array *expressions;
		} print_stmnt;

		struct {
			struct node *value;
		} return_stmnt;

		struct {
			char *name;
			struct node *code;
		} import_stmnt;

		struct {
			struct array *prompts;
			struct array *identifiers;
		} input_stmnt;
	};  /* anonymous unions require C11 */

	void (*check)(struct node *);
	void (*print)(struct node *, int);
	void (*visit)(struct node *, struct stack *);
} Node;

Node *create(nodetype_t nt, ...);

#endif
