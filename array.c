/* array.c
 *
 * Functions to create an empty array and to append, insert and remove elements.
 * The array indices are zero-based. The array size expands and shrinks
 * automatically when adding or removing items.
 *
 * Beware that when using the append, insert and remove functions the location
 * of the array can change. Therefore variables may not hold a reference to an
 * element; elements may only be referenced as 'array->element[index]'.
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "array.h"
#include "error.h"


/* Allocate and initialize an empty array
 *
 */
Array *array_alloc(void)
{
	Array *array;

	if ((array = malloc(sizeof(Array))) == NULL)
		raise(OutOfMemoryError);
	else {
		array->capacity = 0;
		array->size = 0;
		array->element = NULL;
	}
	return array;
}


void array_free(Array *array)
{
	assert(array != NULL);

	if (array->element)
		free(array->element);

	free(array);
}


/* Append an element to an array.
 *
 * If the array has no unused space left the arrays capacity is expanded by
 * ARRAYINCREMENT elements.
 *
 * When -Wfanalyzer-malloc-leak is enabled GCC will report a memory leak here.
 * This is a false positive which can be discarded as the allocated memory
 * is assigned to array->element.
 *
 * array	array to which the element must be appended
 * element	pointer to the element to append
 * return	true is successful else false
 * 			in case of an error the array remains unchanged
 */
bool array_append_child(Array *array, void *element)
{
	assert(array != NULL);

	void *mem;

	if (array->capacity == 0) {
		/* no storage for elements yet, create new storage with size ARRAYINCREMENT */
		if ((array->element = calloc(ARRAYINCREMENT, sizeof(void *))) == NULL) {
			raise(OutOfMemoryError);
			return false;
		}
		array->capacity = ARRAYINCREMENT;
	} else {
		if (array->size == array->capacity) {
			/* all capacity is used, increase the size of array by ARRAYINCREMENT elements */
			if ((mem = realloc(array->element, (array->capacity + ARRAYINCREMENT) * sizeof(void *))) == NULL) {
				raise(OutOfMemoryError);
				return false;
			}
			array->capacity += ARRAYINCREMENT;
			array->element = mem;
		}
	}
	array->size += 1;
	array->element[array->size - 1] = element;  /* - 1 because element array is zero-based */

	return true;
}


/* Remove an element from an array.
 *
 * If the unused space in the array exceeds ARRAYDECREMENT elements it is shrunk.
 *
 * array	array from which to remove an element
 * index	index of the element to remove
 * return	true if successful (= index was valid and realloc OK) else false
 * 			in case of an error the array remains unchanged
 */
bool array_remove_child(Array *array, size_t index)
{
	assert(array != NULL);

	void *mem;

	if (index >= array->size)
		return false;

	/* memmove is safe when source and destination area overlap */
	memmove(array->element + index, array->element + index + 1, (array->size + 1 - index) * sizeof(void *));

	array->
	size -= 1;

	if (array->capacity - array->size >= ARRAYDECREMENT) {
		/* reclaim unused array space */
		if ((mem = realloc(array->element, (array->capacity - ARRAYDECREMENT) * sizeof(void *))) == NULL)
			return false;

		array->capacity -= ARRAYDECREMENT;
		array->element = mem;
	}

	return true;
}


/* Insert an element in an array before a certain index.
 *
 * If the array has no unused space left the arrays capacity is expanded by
 * ARRAYINCREMENT elements.
 *
 * array		array in which to insert the element
 * before_index	element will be inserted before this index
 * element		element to insert
 * return		true if successful (= valid before_index and realloc OK) else false
 * 				in case of an error the array remains unchanged
 */
bool array_insert_child(Array *array, size_t before_index, void *element)
{
	assert(array != NULL);

	void *mem;

	if (before_index >= array->size)
		return false;

	if (array->size == array->capacity) {
		/* all capacity is used, increase the size of array by ARRAYINCREMENT elements */
		if ((mem = realloc(array->element, (array->capacity + ARRAYINCREMENT) * sizeof(void *))) == NULL)
			return false;

		array->capacity += ARRAYINCREMENT;
		array->element = mem;
	}
	array->size += 1;

	/* memmove is safe when source and destination area overlap */
	memmove(array->element + before_index + 1, array->element + before_index, (array->size + 1 - before_index) * sizeof(void *));

	array->element[before_index] = element;

	return true;
}


/* Array API
 *
 */
Array array = {
	.capacity = 0,
	.size = 0,
	.element = NULL,
	.alloc = array_alloc,
	.free = array_free,
	.append_child = array_append_child,
	.insert_child = array_insert_child,
	.remove_child = array_remove_child
};
