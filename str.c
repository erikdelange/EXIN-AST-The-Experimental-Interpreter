/* string.c
 *
 * String object operations
 *
 * Principle is that on function entry any StrObject argument is
 * guaranteed to be a StrObject. This means functions do not need
 * to check this. If there is a return value then this is either
 * an object of some type or a none-object in case of an error.
 * Any deviation from these rules is explicitely stated at the
 * respective function.
 *
 * 2016 K.W.E. de Lange
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "strndup.h"
#include "error.h"
#include "str.h"


/* Create a new empty string-object.
 *
 * return	new string-object or NULL in case of error
 */
static StrObject *str_alloc(void)
{
	StrObject *obj;

	if ((obj = calloc(1, sizeof(StrObject))) != NULL) {
		obj->typeobj = (TypeObject *)&strtype;
		obj->type = STR_T;
		obj->refcount = 0;

		if ((obj->sptr = strdup("")) == NULL) {  /* initial value is empty string */
			obj_free((Object *)obj);
			obj = NULL;
		}
	}
	return obj;  /* returns NULL if alloc failed */
}


/* Free a string-object including the string it contains.
 *
 */
static void str_free(StrObject *obj)
{
	free(obj->sptr);

	*obj = (const StrObject) { 0 };  /* clear the object struct, facilitates debugging */

	free(obj);
}


static void str_print(FILE *fp, StrObject *obj)
{
	fprintf(fp, "%s", obj->sptr);
}


static void str_set(StrObject *obj, const char *s)
{
	if (obj->sptr)
		free(obj->sptr);  /* free current string */

	if ((obj->sptr = strdup(s)) == NULL)  /* always create private copy of s */
		raise(OutOfMemoryError);
}


static void str_vset(StrObject *obj, va_list argp)
{
	str_set(obj, va_arg(argp, char *));
}


/* Execute a method on a string.
 *
 * obj			string-object for which method was called
 * name			method name
 * arguments	method arguments as array with pointers to objects
 * return		object with method results or none-object in case of error
 */
static Object *str_method(StrObject *obj, char *name, Array *arguments)
{
	UNUSED(arguments);  /* suppress unused argument warning during compilation */

	Object *result;

	if (strcmp("len", name) == 0) {
		if (arguments->size != 0) {
			raise(SyntaxError, "method %s takes %d arguments", name, 2);
			result = obj_alloc(NONE_T);
		} else
			result = strtype.length(obj);
	} else {
		raise(SyntaxError, "objecttype %s has no method %s", TYPENAME(obj), name);
		result = obj_alloc(NONE_T);
	}

	return result;
}


/* Return string length as an integer-object.
 *
 * return	integer-object with count or none-object in case of error
 */
static Object *str_length(StrObject *obj)
{
	Object *len;

	if ((len = obj_create(INT_T, strlen(obj->sptr))) == NULL)
		len = obj_alloc(NONE_T);

	return len;
}


/* Create a new string which consits of the strings from op1 and op2.
 *
 * Operand op1 or op2 is a string. The other operand can be anything
 * and will be converted to a string. This conversion is guaranteed
 * to return a string for any operand type.
 *
 * return	new string-object or none-object in case of error
 */
static Object *str_concat(Object *op1, Object *op2)
{
	char *s;
	size_t bytes;
	Object *obj;
	StrObject *s1, *s2, *conv = NULL;

	s1 = TYPE(op1) == STR_T ? (StrObject *)op1 : (conv = (StrObject *)obj_to_strobj(op1));
	s2 = TYPE(op2) == STR_T ? (StrObject *)op2 : (conv = (StrObject *)obj_to_strobj(op2));

	bytes = strlen(s1->sptr) + strlen(s2->sptr) + 1;  /* + 1 because strlen does not count the '\0' character */

	if ((s = calloc(bytes, sizeof(char))) == NULL) {
		raise(OutOfMemoryError);
		return obj_alloc(NONE_T);
	}

	strcpy(s, s1->sptr);
	strcat(s, s2->sptr);

	if ((obj = obj_create(STR_T, s)) == NULL)
		obj = obj_alloc(NONE_T);

	free(s);

	if (conv)
		obj_free((Object *)conv);

	return obj;
}


/* Create a new string object which contains n times an existing string object.
 *
 * Operand op1 or op2 is a string object. The other operand is guaranteed to
 * be number-object and will be converted to an integer. If this is negative
 * it will be silently adjusted to 0, 0 will return an empty string object.
 *
 * return	new string-object or none-object in case of error
 */
static Object *str_repeat(Object *op1, Object *op2)
{
	char *str;
	size_t bytes;
	Object *obj;

	StrObject *s = TYPE(op1) == STR_T ? (StrObject *)op1 : (StrObject *)op2;
	Object *n = TYPE(op1) == STR_T ? op2 : op1;

	int_t times = obj_as_int(n);

	if (times < 0)
		times = 0;

	bytes = strlen(s->sptr) * times + 1;  /* + 1 because strlen() does not count the '\0' character */

	if ((str = calloc(bytes, sizeof(char))) == NULL) {  /* callocs 0's make str a zero-length string */
		raise(OutOfMemoryError);
		return obj_alloc(NONE_T);
	}

	while (times--)
		strcat(str, s->sptr);

	if ((obj = obj_create(STR_T, str)) == NULL)
		obj = obj_alloc(NONE_T);

	free(str);

	return obj;
}


/* Check if content of two strings is equal.
 *
 * return	integer-object with value 1 if equal or value 0 if not equal, none-object in case of error
 */
static Object *str_eql(StrObject *op1, StrObject *op2)
{
	Object *result;

	result = obj_create(INT_T, (int_t)(strcmp(op1->sptr, op2->sptr) == 0 ? 1 : 0));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/* Check if content of two strings is not equal.
 *
 * return	integer-object with value 0 if equal or value 1 if not equal, none-object in case of error
 */
static Object *str_neq(StrObject *op1, StrObject *op2)
{
	Object *result;

	result = obj_create(INT_T, (int_t)!(strcmp(op1->sptr, op2->sptr) == 0 ? 1 : 0));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/* Retrieve a character from a string object by index.
 *
 * obj		string-object to retrieve the object from
 * index	index number of object, negative numbers count from the end
 * return	retrieved character-object or none-object in case of error
 */
static CharObject *str_item(StrObject *obj, int_t index)
{
	CharObject *c;
	int_t len;

	len = strlen(obj->sptr);

	if (index < 0)
		index += len;

	if (index < 0 || index >= len) {
		raise(IndexError);
		return (CharObject *)obj_alloc(NONE_T);
	}

	if ((c = (CharObject *)obj_create(CHAR_T, *(obj->sptr + index))) == NULL)
		c = (CharObject *)obj_alloc(NONE_T);

	return c;
}


/* Create a new string-object by taking a slice of an existing string-object.
 *
 * Start and end are silently adjusted to the nearest possible values.
 *
 * list		string-object to take slice from
 * start	index number to start the slice
 * end		last index number of the slice
 * return	new string-object containing slice or none-object in case of error
 */
static StrObject *str_slice(StrObject *obj, int_t start, int_t end)
{
	StrObject *slice;
	char *str;

	int_t len = strlen(obj->sptr);

	if (start < 0)
		start += len;

	if (end < 0)
		end += len;

	if (start < 0)
		start = 0;

	if (end >= len)
		end = len;

	if ((str = strndup(obj->sptr + start, end - start)) == NULL)
		return (StrObject *)obj_alloc(NONE_T);

	if ((slice = (StrObject *)obj_create(STR_T, str)) == NULL)
		slice = (StrObject *)obj_alloc(NONE_T);

	free(str);

	return slice;
}


/* String object API.
 */
StrType strtype = {
	.name = "str",
	.alloc = (Object *(*)())str_alloc,
	.free = (void (*)(Object *))str_free,
	.print = (void (*)(FILE *, Object *))str_print,
	.set = str_set,
	.vset = (void (*)(Object *, va_list))str_vset,
	.method = (Object *(*)(Object *, char *, Array *))str_method,

	.length = str_length,
	.item = str_item,
	.slice = str_slice,
	.concat = str_concat,
	.repeat = str_repeat,
	.eql = (Object *(*)(Object *, Object *))str_eql,
	.neq = (Object *(*)(Object *, Object *))str_neq
	};
