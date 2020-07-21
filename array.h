/* array.h
 *
 * Universal array consisting of pointers to void.
 *
 * 2020 K.W.E. de Lange
 */
#ifndef _ARRAY_
#define _ARRAY_

#include <stdlib.h>
#include <stdbool.h>

/* Array with pointers to 0 or more elements
 *
 * Utility struct used when a variable number of elements must be stored.
 * The elements stored are void pointers (so can point to anything).
 */
typedef struct array {
	size_t size;	/* number of pointers in array 'element' */
	void **element;	/* pointer to array of pointers to elements, NULL is case array size == 0 */
} Array;

Array *array_alloc(void);
void array_free(Array *array);

extern bool array_append_child(Array *array, void *element);
extern bool array_insert_child(Array *array, size_t before_index, void *element);
extern bool array_remove_child(Array *array, size_t index);

#endif
