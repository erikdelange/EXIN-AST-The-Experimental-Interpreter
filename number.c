/* number.c
 *
 * Number object (CHAR_T, INT_T, FLOAT_T) operations
 *
 * Principle is that on function entry any Object argument is
 * guaranteed to be either CharObject, IntObject or FloatObject.
 * This means functions do not need to check this. If there is a
 * return value then this is either an Object or none-object in
 * case of an error. Any deviation from these rules is
 * explicitly stated at the respective function.
 *
 * 2016 K.W.E. de Lange
 */
#include <assert.h>
#include <stdlib.h>

#include "number.h"
#include "error.h"


static Object *char_alloc(void)
{
	CharObject *obj;

	if ((obj = calloc(1, sizeof(CharObject))) != NULL) {
		obj->typeobj = (TypeObject *)&chartype;
		obj->type = CHAR_T;
		obj->refcount = 0;

		obj->cval = 0;
	}
	return (Object *)obj;  /* returns NULL if alloc failed */
}


static Object *int_alloc(void)
{
	IntObject *obj;

	if ((obj = calloc(1, sizeof(IntObject))) != NULL) {
		obj->typeobj = (TypeObject *)&inttype;
		obj->type = INT_T;
		obj->refcount = 0;

		obj->ival = 0;
	}
	return (Object *)obj;  /* returns NULL if alloc failed */
}


static Object *float_alloc(void)
{
	FloatObject *obj;

	if ((obj = calloc(1, sizeof(FloatObject))) != NULL) {
		obj->typeobj = (TypeObject *)&floattype;
		obj->type = FLOAT_T;
		obj->refcount = 0;

		obj->fval = 0;
	}
	return (Object *)obj;  /* returns NULL if alloc failed */
}


static void number_free(Object *obj)
{
	*obj = (const Object) { 0 };  /* clear the object struct, facilitates debugging */

	free(obj);
}


static void number_print(FILE *fp, Object *obj)
{
	switch (TYPE(obj)) {
		case CHAR_T:
			fprintf(fp, "%c", obj_as_char(obj));
			break;
		case INT_T:
			fprintf(fp, "%ld", obj_as_int(obj));
			break;
		case FLOAT_T:
			fprintf(fp, "%.*G", 15, obj_as_float(obj));
			break;
		default:
			assert(0);
			break;
	}
}


static void char_set(CharObject *obj, char_t c)
{
	obj->cval = c;
}


static void int_set(IntObject *obj, int_t i)
{
	obj->ival = i;
}


static void float_set(FloatObject *obj, float_t f)
{
	obj->fval = f;
}


static void number_vset(Object *obj, va_list argp)
{
	switch (TYPE(obj)) {
		case CHAR_T:
			char_set((CharObject *)obj, va_arg(argp, int));  /* va_arg requires at least an int */
			break;
		case INT_T:
			int_set((IntObject *)obj, va_arg(argp, int_t));
			break;
		case FLOAT_T:
			float_set((FloatObject *)obj, va_arg(argp, float_t));
			break;
		default:
			assert(0);
			break;
	}
}


static Object *number_method(Object *obj, char *name, Array *arguments)
{
	UNUSED(arguments);  /* suppress unused argument warning during compilation */

	raise(SyntaxError, "objecttype %s has no method %s", TYPENAME(obj), name);

	return obj_alloc(NONE_T);
}


/* Determine the type of the result of an arithmetic operations
 * on two numeric operands according to the following rules:
 *
 * FLOAT_T if at least one operand is FLOAT_T,
 * else INT_T if at least one operand is INT_T
 * else CHAR_T
 */
static objecttype_t coerce(Object *op1, Object *op2)
{
	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		return FLOAT_T;
	else if (TYPE(op1) == INT_T || TYPE(op2) == INT_T)
		return INT_T;
	else
		return CHAR_T;
}


static Object *number_add(Object *op1, Object *op2)
{
	Object *result = NULL;

	switch (coerce(op1, op2)) {
		case CHAR_T:
			result = obj_create(CHAR_T, obj_as_char(op1) + obj_as_char(op2));
			break;
		case INT_T:
			result = obj_create(INT_T, obj_as_int(op1) + obj_as_int(op2));
			break;
		case FLOAT_T:
			result = obj_create(FLOAT_T, obj_as_float(op1) + obj_as_float(op2));
			break;
		default:
			assert(0);
			break;
	}

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_sub(Object *op1, Object *op2)
{
	Object *result = NULL;

	switch (coerce(op1, op2)) {
		case CHAR_T:
			result = obj_create(CHAR_T, obj_as_char(op1) - obj_as_char(op2));
			break;
		case INT_T:
			result = obj_create(INT_T, obj_as_int(op1) - obj_as_int(op2));
			break;
		case FLOAT_T:
			result = obj_create(FLOAT_T, obj_as_float(op1) - obj_as_float(op2));
			break;
		default:
			assert(0);
			break;
	}

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_mul(Object *op1, Object *op2)
{
	Object *result = NULL;

	switch (coerce(op1, op2)) {
		case CHAR_T:
			result = obj_create(CHAR_T, obj_as_char(op1) * obj_as_char(op2));
			break;
		case INT_T:
			result = obj_create(INT_T, obj_as_int(op1) * obj_as_int(op2));
			break;
		case FLOAT_T:
			result = obj_create(FLOAT_T, obj_as_float(op1) * obj_as_float(op2));
			break;
		default:
			assert(0);
			break;
	}

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_div(Object *op1, Object *op2)
{
	Object *result = NULL;

	if (obj_as_float(op2) == 0)
		raise(DivisionByZeroError);
	else
		switch (coerce(op1, op2)) {
			case CHAR_T:
				result = obj_create(CHAR_T, obj_as_char(op1) / obj_as_char(op2));
				break;
			case INT_T:
				result = obj_create(INT_T, obj_as_int(op1) / obj_as_int(op2));
				break;
			case FLOAT_T:
				result = obj_create(FLOAT_T, obj_as_float(op1) / obj_as_float(op2));
				break;
			default:
				assert(0);
				break;
		}

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_mod(Object *op1, Object *op2)
{
	Object *result = NULL;

	if (obj_as_float(op2) == 0)
		raise(DivisionByZeroError);
	else
		switch (coerce(op1, op2)) {
			case CHAR_T:
				result = obj_create(CHAR_T, obj_as_char(op1) % obj_as_char(op2));
				break;
			case INT_T:
				result = obj_create(INT_T, obj_as_int(op1) % obj_as_int(op2));
				break;
			case FLOAT_T:
				raise(ModNotAllowedError, "%% operator only allowed on integers");
				break;
			default:
				assert(0);
				break;
		}

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_inv(Object *op1)
{
	Object *op2 = NULL, *result;

	switch (TYPE(op1)) {
		case CHAR_T:
			op2 = obj_create(CHAR_T, (char_t)0);
			break;
		case INT_T:
			op2 = obj_create(INT_T, (int_t)0);
			break;
		case FLOAT_T:
			op2 = obj_create(FLOAT_T, (float_t)0);
			break;
		default:
			assert(0);
			break;
	}

	if (op2) {
		result = obj_sub(op2, op1);
		obj_decref(op2);
	} else
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_eql(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) == obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) == obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) == obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_neq(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) != obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) != obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) != obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_lss(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) < obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) < obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) < obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_leq(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) <= obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) <= obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) <= obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_gtr(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) > obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) > obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) > obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_geq(Object *op1, Object *op2)
{
	Object *result;

	if (TYPE(op1) == FLOAT_T || TYPE(op2) == FLOAT_T)
		result = obj_create(INT_T, (int_t)(obj_as_float(op1) >= obj_as_float(op2)));
	else if (TYPE(op1) == INT_T || TYPE(op1) == INT_T)
		result = obj_create(INT_T, (int_t)(obj_as_int(op1) >= obj_as_int(op2)));
	else
		result = obj_create(INT_T, (int_t)(obj_as_char(op1) >= obj_as_char(op2)));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_or(Object *op1, Object *op2)
{
	Object *result;

	result = obj_create(INT_T, (int_t)(obj_as_bool(op1) || obj_as_bool(op2) ? 1 : 0));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_and(Object *op1, Object *op2)
{
	Object *result;

	result = obj_create(INT_T, (int_t)(obj_as_bool(op1) && obj_as_bool(op2) ? 1 : 0));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


static Object *number_negate(Object *op1)
{
	Object *result;

	result = obj_create(INT_T, (int_t)!obj_as_bool(op1));

	if (result == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/* Number object API (separate for char_t, int_t, float_t and the generic number).
 */
CharType chartype = {
	.name = "char",
	.alloc = char_alloc,
	.free = number_free,
	.print = number_print,
	.set = (void (*)())char_set,
	.vset = number_vset,
	.method = number_method
	};

IntType inttype = {
	.name = "int",
	.alloc = int_alloc,
	.free = number_free,
	.print = number_print,
	.set = int_set,
	.vset = number_vset,
	.method = number_method
	};

FloatType floattype = {
	.name = "float",
	.alloc = float_alloc,
	.free = number_free,
	.print = number_print,
	.set = float_set,
	.vset = number_vset,
	.method = number_method
	};

NumberType numbertype = {
	.name = "number",
	.alloc = int_alloc,  /* number considered INT_T */
	.free = number_free,
	.print = number_print,
	.set = int_set,  /* number considered INT_T */
	.vset = number_vset,
	.method = number_method,

	.add = number_add,
	.sub = number_sub,
	.mul = number_mul,
	.div = number_div,
	.mod = number_mod,
	.inv = number_inv,
	.eql = number_eql,
	.neq = number_neq,
	.lss = number_lss,
	.leq = number_leq,
	.gtr = number_gtr,
	.geq = number_geq,
	.or = number_or,
	.and = number_and,
	.negate = number_negate
	};
