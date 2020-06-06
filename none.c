/* none.c
 *
 * 2016	K.W.E. de Lange
 */
#include "error.h"
#include "none.h"


static NoneObject none = {
	.refcount = 0,
	.type = NONE_T,
	.typeobj = (TypeObject *)&nonetype
	};


static NoneObject *none_alloc(void)
{
	return &none;
}


static void none_free(void)
{
	;
}


static void none_print(void)
{
	printf("none");
}


static void none_set(void)
{
	;
}

static void none_vset(void)
{
	;
}


static NoneObject *none_method(Object *obj, char *name)
{
	raise(SyntaxError, "objecttype %s has no method %s", TYPENAME(obj), name);

	return none_alloc();
}


/*	None object API.
 */
NoneType nonetype = {
	.name = "none",
	.alloc = (Object *(*)())none_alloc,
	.free = (void (*)(Object *))none_free,
	.print = (void (*)(FILE *, Object *))none_print,
	.set = (void (*)())none_set,
	.vset = (void (*)())none_vset,
	.method = (Object *(*)())none_method
	};
