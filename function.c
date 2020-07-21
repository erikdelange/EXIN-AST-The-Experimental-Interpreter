/* function.c
 *
 * Builtin (aka intrinsic) functions.
 *
 * 2019	K.W.E. de Lange
 */
#include <assert.h>
#include <string.h>

#include "list.h"
#include "error.h"
#include "object.h"
#include "function.h"


/* Builtin: determine the type of an expression
 *
 * Syntax: type(expression)
 */
static void type(Array *arguments, Stack *s)
{
	Object *obj = arguments->element[0];

	Object *result = isListNode(obj) ? obj_type(obj_from_listnode(obj)) : obj_type(obj);

	obj_decref(obj);

	push(s, result);
}


/* Builtin: return ASCII character (as string) representation of integer
 *
 * Syntax: chr(integer expression)
 */
static void chr(Array *arguments, Stack *s)
{
	char buffer[BUFSIZE+1];

	Object *obj = arguments->element[0];

	snprintf(buffer, BUFSIZE, "%c", obj_as_char(obj));
	Object *result = obj_create(STR_T, buffer);

	obj_decref(obj);

	push(s, result);
}


/* Builtin: return integer representation of ASCII character (in string)
 *
 * Syntax: ord(string expression)
 */
static void ord(Array *arguments, Stack *s)
{
	Object *obj = arguments->element[0];

	if (TYPE(obj) != STR_T)
		raise(TypeError, "expected string but found %s", TYPENAME(obj));

	Object *result = obj_create(INT_T, (int_t)obj_as_char(obj));

	obj_decref(obj);

	push(s, result);
}


/* Table containing all builtin function names, the expected
 * number of arguments (will be passed as an array of objects)
 * and the function addresses.
 *
 * The function signature is 'void function(Array *, Stack *)' where
 * Array contains the argument objects and the stack is where
 * the return value must be pushed.
 *
 */
static struct {
	char *functionname;
	size_t argc;
	void (*functionaddr)(Array *, Stack *);
} builtinTable[] = {  /* Note: function names *must* be sorted alphabetically */
	{"chr", 1, chr},
	{"ord", 1, ord},
	{"type", 1, type}
};


/* Search for a builtin function.
 *
 * functionname	name of builtin function to search
 * return		-1 if not found else index in table builtinTable
 */
static int search_builtin(const char *functionname)
{
	int l, h, m = 0, d = 0;

	assert(functionname != NULL);

	l = 0, h = (int)(sizeof builtinTable / sizeof builtinTable[0]) - 1;

	while (l <= h) {
		m = (l + h) / 2;
		d = strcmp(&functionname[0], builtinTable[m].functionname);
		if (d < 0)
			h = m - 1;
		if (d > 0)
			l = m + 1;
		if (d == 0)
			break;
	};

	if (d == 0)
		return m;

	return -1;
}


/* Execute a builtin function.
 *
 * functionname	name of builtin function
 * arguments	array with function arguments (Object *)
 * s			stack where to place results
 */
void visit_builtin(const char *functionname, Array *arguments, Stack *s)
{
	assert(functionname != NULL);
	assert(is_builtin(functionname) == true);

	int index = search_builtin(functionname);

	if (index >= 0)
		builtinTable[index].functionaddr(arguments, s);
}


/* Check if a function is a builtin.
 *
 * functionname	name of builtin function
 * return		true if functionname was a builtin function else false
 */
bool is_builtin(const char *functionname)
{
	assert(functionname != NULL);

	return search_builtin(functionname) == -1 ? false : true;
}


/* Return the number of arguments a builtin function expects.
 *
 * functionname	name of builtin function
 * return		true if functionname was a builtin function else false
 */
size_t builtin_argc(const char *functionname)
{
	assert(functionname != NULL);
	assert(is_builtin(functionname) == true);

	return builtinTable[search_builtin(functionname)].argc;
}
