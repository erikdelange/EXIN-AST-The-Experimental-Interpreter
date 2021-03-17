/* object.c
 *
 * Operations on objects
 *
 * Values of variables are represented as objects. An object contains data
 * but also a number of methods. Every object has a minimal and thus mandatory
 * set of methods. This set is: alloc(), free(), set(), vset(), print() and
 * method().
 *
 * Which other methods are available depends on the type of the object.
 * See: number.c, str.c, list.c and none.c. Operations on objects are
 * called via obj_...() functions.
 *
 * Objects are created when required, but also automatically removed when
 * no longer needed. For this purpose every object instance has a reference
 * counter. Every time an object is allocated or assigned to an identifier
 * the reference counter is incremented. Once a routine no longer needs an
 * object it must decrement the counter. The moment the reference counter
 * hits zero the object is removed from memory. (Beware that if not programmed
 * properly this can be a source of unexplainable bugs or excessive memory
 * consumption).
 *
 * All operations on - and between objects are defined in object.c and are
 * accessed via function names like obj_...() followed by the operation,
 * e.g. obj_add().
 *
 * There are two basic types of operations: unary and binary. Unary
 * operations require only one operand:
 *
 *  result = operator operand
 *
 * The unary operators are:
 *
 *  -   negation of the operand
 *  +   returns the operand (so does nothing)
 *  !   logical negation of the operand (returns 0 or 1)
 *
 * Binary operators require two operands:
 *
 *  result = operand1 operator operand2
 *
 *  Arithmetic operators are:   +  -  *  /  %
 *  Comparison operators are:   ==  !=  <>  <  <=  >  >=  in
 *  Logical operators are:      and  or
 *
 * Which operations are supported depends on the object type. Numerical object
 * will support almost everything, lists or strings support less operations.
 *
 * Two operations are only meant for use on list or string objects:
 *
 *  item[index]
 *  slice[start:end]
 *
 * As C functions unary- and binary operations look like:
 *
 *  result *operator(*operand1)
 *  result *operator(*operand1, *operand2)
 *
 *  Examples:
 *
 *  Object *obj_negate(Object *op1)
 *  Object *obj_add(Object *op1, Object *op2)
 *
 * Function arguments operand1 and operand2 always remain unchanged. Result is
 * a newly created object. Its type is dependent on operand1 and optionally
 * operand2. The operations always return a usable result, so never NULL as
 * this can be a source of bugs. However the results may not be useful as in
 * case of errors often a NONE_T is returned. Exception are obj_alloc() and
 * obj_create() which return a NULL in case the memory allocation failed.
 * It is up to the calling function to handle this.
 *
 * See function coerce() in number.c for the rules for determining the type
 * of the result for arithmetic operations. For logical and comparison
 * operations the result is always an INT_T with values 0 and 1.
 *
 * Copyright (c) 1994 K.W.E. de Lange
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "error.h"
#include "object.h"
#include "number.h"
#include "none.h"
#include "list.h"
#include "str.h"


#ifdef DEBUG

static Object *head = NULL;    /* head of doubly linked list with objects */
static Object *tail = NULL;    /* tail of doubly linked list with objects */

static void _enqueue(Object *obj);
static void _dequeue(Object *obj);

#define enqueue(o)  _enqueue(o)
#define dequeue(o)  _dequeue(o)

#else  /* not DEBUG */

#define enqueue(o)  ((void)0)
#define dequeue(o)  ((void)0)

# endif


/* Create a new object of type 'type' and assign the default initial value.
 *
 * The initial refcount of the new object is 1.
 *
 * type		type of the new object
 * return	pointer to new object or NULL in case of an error
 */
Object *obj_alloc(objecttype_t type)
{
	Object *obj = NULL;

	switch (type) {
		case CHAR_T:
			obj = chartype.alloc();
			break;
		case INT_T:
			obj = inttype.alloc();
			break;
		case FLOAT_T:
			obj = floattype.alloc();
			break;
		case STR_T:
			obj = strtype.alloc();
			break;
		case LIST_T:
			obj = listtype.alloc();
			break;
		case LISTNODE_T:
			obj = listnodetype.alloc();
			break;
		case NONE_T:
			obj = nonetype.alloc();
			break;
	}

	debug_printf(DEBUGALLOC, "\nalloc : %-p", (void *)obj);

	if (obj == NULL)
		raise(OutOfMemoryError);
	else {
		debug_printf(DEBUGALLOC, " %s", TYPENAME(obj));
		enqueue(obj);
		obj_incref(obj);  /* initial refcount = 1 */
	}
	return obj;
}


/* Create a new object of type 'type' and assign an initial value.
 *
 * type		type of the new object, also expected type of the initial value
 * ...		value to assign (mandatory)
 * return	pointer to new object or NULL in case of an error
 */
Object *obj_create(objecttype_t type, ...)
{
	va_list argp;
	Object *obj;

	va_start(argp, type);

	if ((obj = obj_alloc(type)) != NULL)  /* sets refcount to 1 */
		TYPEOBJ(obj)->vset(obj, argp);

	va_end(argp);

	return obj;
}


/* Free the memory which was reserved for an object.
 */
void obj_free(Object *obj)
{
	assert(obj);

	dequeue(obj);

	debug_printf(DEBUGALLOC, "\nfree  : %-p %s", (void *)obj, TYPENAME(obj));

	TYPEOBJ(obj)->free(obj);
}


/* Execute a method of object obj
 */
Object *obj_method(Object *obj, char *name, Array *arguments)
{
	return TYPEOBJ(obj)->method(obj, name, arguments);
}


/* Print object value.
 *
 * fp		pointer to output stream
 * obj		pointer to object to print
 */
void obj_print(FILE *fp, Object *obj)
{
	assert(obj);

	TYPEOBJ(obj)->print(fp, obj);
}


/* Read object value from input stream.
 *
 * fp		input stream
 * type		type of the value to read
 * return	new object containing value or none-object in case of error
 */
Object *obj_scan(FILE *fp, objecttype_t type)
{
	char buffer[LINESIZE + 1] = "";
	Object *obj;

	fgets(buffer, LINESIZE + 1, fp);

	/* if fgets() encounters an error then buffer will contain an empty string ("") */

	buffer[strcspn(buffer, "\r\n")] = 0;  /* remove trailing newline, if any */

	switch (type) {
		case CHAR_T:
			obj = obj_create(CHAR_T, str_to_char(buffer));
			break;
		case INT_T:
			obj = obj_create(INT_T, str_to_int(buffer));
			break;
		case FLOAT_T:
			obj = obj_create(FLOAT_T, str_to_float(buffer));
			break;
		case STR_T:
			obj = obj_create(STR_T, buffer);
			break;
		default:
			raise(TypeError, "unsupported type for input: %d", type);
			obj = obj_alloc(NONE_T);
	}

	return obj;
}


/* (type op1)result = op1
 *
 * return	object with result or none-object in case of error
 */
Object *obj_copy(Object *op1)
{
	switch (TYPE(op1)) {
		case CHAR_T:
			return obj_create(CHAR_T, obj_as_char(op1));
		case INT_T:
			return obj_create(INT_T, obj_as_int(op1));
		case FLOAT_T:
			return obj_create(FLOAT_T, obj_as_float(op1));
		case STR_T:
			return obj_create(STR_T, obj_as_str(op1));
		case LIST_T:
			return obj_create(LIST_T, obj_as_list(op1));
		case LISTNODE_T:
			return obj_copy(obj_from_listnode(op1));
		default:
			raise(TypeError, "cannot copy type %s", TYPENAME(op1));
			return obj_alloc(NONE_T);
	}
}


/* op1 = (type op1) op2
 *
 * return	object with result or none-object in case of error
 */
void obj_assign(Object *op1, Object *op2)
{
	Object *obj;

	switch (TYPE(op1)) {
		case CHAR_T:
			TYPEOBJ(op1)->set(op1, obj_as_char(op2));
			break;
		case INT_T:
			TYPEOBJ(op1)->set(op1, obj_as_int(op2));
			break;
		case FLOAT_T:
			TYPEOBJ(op1)->set(op1, obj_as_float(op2));
			break;
		case STR_T:
			obj = obj_to_strobj(op2);
			TYPEOBJ(op1)->set(op1, obj_as_str(obj));
			obj_decref(obj);
			break;
		case LIST_T:
			TYPEOBJ(op1)->set(op1, obj_as_list(op2));
			break;
		case LISTNODE_T:
			TYPEOBJ(op1)->set(op1, obj_copy(op2));
			break;
		default:
			raise(TypeError, "unsupported operand type(s) for operation =: %s and %s", \
							  TYPENAME(op1), TYPENAME(op2));
	}
}


/* result = op1 + op2
 *
 * return	object with result or none-object in case of error
 */
Object *obj_add(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.add(op1, op2);
	else if (isString(op1) || isString(op2))
		return strtype.concat(op1, op2);
	else if (isList(op1) && isList(op2))
		return listtype.concat((ListObject *)op1, (ListObject *)op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation +: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = op1 - op2
 *
 * return	object with result or none-object in case of error
 */
Object *obj_sub(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.sub(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation -: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = op1 * op2
 *
 * return	object with result or none-object in case of error
 */
Object *obj_mult(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.mul(op1, op2);
	else if ((isNumber(op1) || isNumber(op2)) && (isString(op1) || isString(op2)))
		return strtype.repeat(op1, op2);
	else if ((isNumber(op1) || isNumber(op2)) && (isList(op1) || isList(op2)))
		return listtype.repeat(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation *: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = op1 / op2
 *
 * return	object with result or none-object in case of error
 */
Object *obj_divs(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.div(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation /: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = op1 % op2
 *
 * return	object with result or none-object in case of error
 */
Object *obj_mod(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.mod(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation %%: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = 0 - op1
 *
 * return	object with result or none-object in case of error
 */
Object *obj_invert(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	if (isNumber(op1))
		return numbertype.inv(op1);
	else {
		raise(TypeError, "unsupported operand type for operation -: %s", \
						  TYPENAME(op1));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 == op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_eql(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.eql(op1, op2);
	else if (isString(op1) && isString(op2))
		return strtype.eql(op1, op2);
	else if (isList(op1) && isList(op2))
		return listtype.eql((ListObject *)op1, (ListObject *)op2);
	else
		/* operands of different types are by definition not equal */
		return obj_create(INT_T, (int_t)0);
}


/* result = (int_t)(op1 != op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_neq(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.neq(op1, op2);
	else if (isString(op1) && isString(op2))
		return strtype.neq(op1, op2);
	else if (isList(op1) && isList(op2))
		return listtype.neq((ListObject *)op1, (ListObject *)op2);
	else
		/* operands of different types are by definition not equal */
		return obj_create(INT_T, (int_t)1);
}


/* result = (int_t)(op1 < op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_lss(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.lss(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation <: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 <= op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_leq(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.leq(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation <=: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 > op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_gtr(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.gtr(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation >: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 >= op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_geq(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.geq(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation >=: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 or op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_or(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.or(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation or: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 and op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_and(Object *op1, Object *op2)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isNumber(op1) && isNumber(op2))
		return numbertype.and(op1, op2);
	else {
		raise(TypeError, "unsupported operand type(s) for operation and: %s and %s", \
						  TYPENAME(op1), TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
}


/* result = (int_t)(op1 in (sequence)op2)
 *
 * return	object with result or none-object in case of error
 */
Object *obj_in(Object *op1, Object *op2)
{
	Object *result = NULL;
	Object *item;
	int_t len;

	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;
	op2 = isListNode(op2) ? obj_from_listnode(op2) : op2;

	if (isSequence(op2) == 0) {
		raise(TypeError, "%s is not subscriptable", TYPENAME(op2));
		return obj_alloc(NONE_T);
	}
	len = obj_length(op2);

	for (int_t i = 0; i < len; i++) {
		if (result != NULL)
			obj_decref(result);
		item = obj_item(op2, i);
		result = obj_eql(op1, item);
		obj_decref(item);
		if (obj_as_int(result) == 1)
			break;
	}
	return result;
}


/* result = (int_t)!op1
 *
 * return	object with negated value or none-object in case of error
 */
Object *obj_negate(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	if (isNumber(op1))
		return numbertype.negate(op1);
	else {
		raise(TypeError, "unsupported operand type for operation !: %s", TYPENAME(op1));
		return obj_alloc(NONE_T);
	}
}


/* item = list[index]
 * item = string[index]
 *
 * return	object with item or none-object in case of error
 */
Object *obj_item(Object *sequence, int_t index)
{
	sequence = isListNode(sequence) ? obj_from_listnode(sequence) : sequence;

	if (TYPE(sequence) == STR_T)
		return (Object *)strtype.item((StrObject *)sequence, index);
	else if (TYPE(sequence) == LIST_T)
		return (Object *)listtype.item((ListObject *)sequence, index);
	else {
		raise(TypeError, "type %s is not subscriptable", TYPENAME(sequence));
		return obj_alloc(NONE_T);
	}
}


/* slice = list[start:end]
 * slice = string[start:end]
 *
 * return	object with slice or none-object in case of error
 */
Object *obj_slice(Object *sequence, int_t start, int_t end)
{
	sequence = isListNode(sequence) ? obj_from_listnode(sequence) : sequence;

	if (TYPE(sequence) == STR_T)
		return (Object *)strtype.slice((StrObject *)sequence, start, end);
	else if (TYPE(sequence) == LIST_T)
		return (Object *)listtype.slice((ListObject *)sequence, start, end);
	else {
		raise(TypeError, "type %s is not subscriptable", TYPENAME(sequence));
		return obj_alloc(NONE_T);
	}
}


/* Count the number of items in a sequence (STR_T or LIST_T).
 *
 * sequence	Object* of STR_T or LIST_T
 * return	item count or 0 in case of error
 */
int_t obj_length(Object *sequence)
{
	int_t len = 0;
	Object *obj = NULL;

	sequence = isListNode(sequence) ? obj_from_listnode(sequence) : sequence;

	if (TYPE(sequence) == STR_T)
		obj = strtype.length((StrObject *)sequence);
	else if (TYPE(sequence) == LIST_T)
		obj = listtype.length((ListObject *)sequence);
	else
		raise(TypeError, "type %s is not subscriptable", TYPENAME(sequence));

	if (obj) {
		len = obj_as_int(obj);
		obj_decref(obj);
	}
	return len;
}


/* Return object type name as string-object.
 *
 * return	type name of op1 as string-object or none-object in case of error
 */
Object *obj_type(Object *op1)
{
	Object *result;

	result = obj_create(STR_T, TYPENAME(op1));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/***********************************************************
 * Various conversions between variable- and object-types.
 *
 * A conversion always returns a value which can be used,
 * (so never NULL) even if errors were encountered.
 ***********************************************************
 */


/* Return the value of op1 as a char_t
 *
 * op1		object who's value to return as a char_t
 * return	value of op1 as char_t or 0 if op1 cannot be converted to char_t
 */
char_t obj_as_char(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch (TYPE(op1)) {
		case CHAR_T:
			return ((CharObject *)op1)->cval;
		case INT_T:
			return ((IntObject *)op1)->ival;
		case FLOAT_T:
			return ((FloatObject *)op1)->fval;
		case STR_T:
			return str_to_char(((StrObject *)op1)->sptr);
		default:
			raise(ValueError, "cannot convert %s to char", TYPENAME(op1));
			return 0;
	}
}


/* Return the value of op1 as an int_t
 *
 * op1		object who's value to return as int_t
 * return	value of op1 as int_t or 0 if op1 cannot be converted to int_t
 */
int_t obj_as_int(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch (TYPE(op1)) {
		case CHAR_T:
			return ((CharObject *)op1)->cval;
		case INT_T:
			return ((IntObject *)op1)->ival;
		case FLOAT_T:
			return ((FloatObject *)op1)->fval;
		case STR_T:
			return str_to_int(((StrObject *)op1)->sptr);
		default:
			raise(ValueError, "cannot convert %s to integer", TYPENAME(op1));
			return 0;
	}
}


/* Return the value of op1 as a float_t
 *
 * op1		object who's value to return as float_t
 * return	value of op1 as float_t or 0 if op1 cannot be converted to float_t
 */
float_t obj_as_float(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch (TYPE(op1)) {
		case CHAR_T:
			return ((CharObject *)op1)->cval;
		case INT_T:
			return ((IntObject *)op1)->ival;
		case FLOAT_T:
			return ((FloatObject *)op1)->fval;
		case STR_T:
			return str_to_float(((StrObject *)op1)->sptr);
		default:
			raise(ValueError, "cannot convert %s to float", TYPENAME(op1));
			return 0;
	}
}


/* Return the value of op1 as string
 *
 * op1		object who's value to return as string
 * return	value of op1 as string or "" if op1 cannot be converted to string
 */
char *obj_as_str(Object *op1)
{
	static char *empty = "";

	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch (TYPE(op1)) {
		case STR_T:
			return ((StrObject *)op1)->sptr;
		default:
			raise(ValueError, "cannot convert %s to string", TYPENAME(op1));
			return empty;
	}
}


/* Return the value of op1 as list-object
 *
 * op1		object who's value to return as list-object
 * return	value of op1 as list-object or an empty list if op1 cannot be converted to list-object
 */
Object *obj_as_list(Object *op1)
{
	Object *obj;

	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch(TYPE(op1)) {
		case LIST_T:
			return op1;
		default:
			raise(ValueError, "cannot convert %s to list", TYPENAME(op1));
			if ((obj = obj_alloc(LIST_T)) != NULL)
				return obj;
			else
				return obj_alloc(NONE_T);  /* obj_alloc failed because of out of memory */
	}
}


/* Return the value of op1 as bool
 *
 * op1		object who's value to return as bool
 * return	value of op1 as bool or false if op1 cannot be converted to bool
 */
bool obj_as_bool(Object *op1)
{
	op1 = isListNode(op1) ? obj_from_listnode(op1) : op1;

	switch (TYPE(op1)) {
		case CHAR_T:
			return obj_as_char(op1) ? true : false;
		case INT_T:
			return obj_as_int(op1) ? true : false;
		case FLOAT_T:
			return obj_as_float(op1) ? true : false;
		default:
			raise(ValueError, "cannot convert %s to bool", TYPENAME(op1));
			return false;
	}
}


/* Convert string to a char_t.
 *
 * s		string containing a single character or an escape sequence
 * return	value of s as char_t or 0 in case s cannot be converted to char_t
 */
char_t str_to_char(const char *s)
{
	char_t c = 0;

	if (*s == '\\') {  /* is an escape sequence */
		switch (*++s) {
			case '0' :	c = '\0'; break;
			case 'b' :	c = '\b'; break;
			case 'f' :	c = '\f'; break;
			case 'n' :	c = '\n'; break;
			case 'r' :	c = '\r'; break;
			case 't' :	c = '\t'; break;
			case 'v' :	c = '\v'; break;
			case '\\':	c = '\\'; break;
			case '\'':	c = '\''; break;
			case '\"':	c = '\"'; break;
			default  :	raise(ValueError, "unknown escape sequence: %c", *s);
						return 0;
		}
	} else {  /* not an escape sequence */
		if (*s == '\0') {
			raise(SyntaxError, "empty character constant");
			return 0;
		} else
			c = *s;
	}
	if (*++s != '\0') {
		raise(SyntaxError, "to many characters in character constant");
		return 0;
	}
	return c;
}


/* Convert string to an int_t.
 *
 * s		string starting with an integer value, optionally followed by other characters
 * return	value of s as int_t or 0 in case s cannot be converted to int_t
 */
int_t str_to_int(const char *s)
{
	char *e;
	int_t i;

	errno = 0;

	i = (int_t)strtol(s, &e, 10);

	/* if *e == 0 then s contains an integer only and was converted completely
	 * if e == s nothing was converted, s does not start with an integer
	 * if e > s and *e != 0 then s does start with an integer but is followed
	 * by other characters and the integer part was converted
	 */

	if (e == s || errno != 0) {
		if (errno != 0)
			raise(ValueError, "cannot convert %s to int; %s", \
							   s, strerror(errno));
		else
			raise(ValueError, "cannot convert %s to int", s);

		return 0;
	} else
		return i;
}


/* Convert string to a float_t.
 *
 * s		string starting with a float value, optionally followed by other characters
 * return	value of s as float_t or 0 in case s cannot be converted to float_t
 */
float_t str_to_float(const char *s)
{
	char *e;
	float_t f;

	errno = 0;

	f = (float_t)strtod(s, &e);

	/* if *e == 0 then s contains a float only and was converted completely
	 * if e == s nothing was converted, s does not start with a float
	 * if e > s and *e != 0 then s does start with a float but is followed
	 * by other characters and the float part was converted
	 */

	if (e == s || errno != 0) {
		if (errno != 0)
			raise(ValueError, "cannot convert %s to float; %s", \
							   s, strerror(errno));
		else
			raise(ValueError, "cannot convert %s to float", s);

		return 0;
	} else
		return f;
}


/* Convert an objects value to a string-object
 *
 * obj		object to convert
 * return	value of obj as StrObject
 */
Object *obj_to_strobj(Object *obj)
{
	char buffer[BUFSIZE + 1];

	switch(TYPE(obj)) {
		case STR_T:
			obj_incref(obj);
			return obj;
		case CHAR_T:
			snprintf(buffer, BUFSIZE, "%c", obj_as_char(obj));
			return obj_create(STR_T, buffer);
		case INT_T:
			snprintf(buffer, BUFSIZE, "%ld", obj_as_int(obj));
			return obj_create(STR_T, buffer);
		case FLOAT_T:
			snprintf(buffer, BUFSIZE, "%.16lG", obj_as_float(obj));
			return obj_create(STR_T, buffer);
		case NONE_T:
			return obj_create(STR_T, "None");
		default:
			return obj_create(STR_T, "");
	}
}


#ifdef DEBUG
/* Add object 'item' to the end of the object queue
 */
static void _enqueue(Object *item)
{
	if (head == NULL) {
		head = item;
		item->prevobj = NULL;
	} else {
		item->prevobj = tail;
		tail->nextobj = item;
	}
	tail = item;
	item->nextobj = NULL;
}
#endif  /* DEBUG */

#ifdef DEBUG
/* Remove object 'item' from the object queue
 */
static void _dequeue(Object *item)
{
	if (item->nextobj == NULL) {  /* last element */
		if (item->prevobj == NULL) {  /* also first element */
			head = tail = NULL;  /* so empty the list */
		} else {  /* not also the first element */
			tail = item->prevobj;
			tail->nextobj = NULL;
		}
	} else {  /* not the last element */
		if (item->prevobj == NULL){  /* but the first element */
			head = item->nextobj;
			head->prevobj = NULL;
		} else {  /* somewhere in the middle of the list */
			item->prevobj->nextobj = item->nextobj;
			item->nextobj->prevobj = item->prevobj;
		}
	}
}
#endif  /* DEBUG */


#ifdef DEBUG
/* Print all objects to semi-colon separated file 'object.dsv'.
 *
 * Note: file extension .dsv stands for delimiter separated file.
 */
void dump_objects_to_file(FILE *fp)
{
	fprintf(fp, "%s;%s;%s;%s\n", "object", "refcount", "type", "value");

	for (Object *obj = head; obj; obj = obj->nextobj) {
		fprintf(fp, "%-p;%d;%s;", (void *)obj, obj->refcount,TYPENAME(obj));
		obj_print(fp, obj);
		fprintf(fp, "\n");
	}
}

void dump_objects(void)
{
	FILE *fp;

	if ((fp = fopen("object.dsv", "w")) != NULL) {
		dump_objects_to_file(fp);
		fclose(fp);
	}
}
#endif  /* DEBUG */
