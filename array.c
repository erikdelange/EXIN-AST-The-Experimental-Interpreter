/* array.c
 *
 * Functions to create an empty array and to append, insert and remove elements.
 * The array indices are zero-based.
 *
 * Beware that when using the append, insert and remove functions the location
 * of the array can change. Therefore variables may not hold a reference to an
 * element; elements must only be referenced as 'array->element[index]'.
 *
 * 2020 K.W.E. de Lange
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

	if (array->size == 0) {
		/* no storage for elements yet, create new storage with size 1 */
		if ((array->element = calloc(++array->size, sizeof(void *))) == NULL) {
			raise(OutOfMemoryError);
			return false;
		}
	} else {
		/* increase the size of the array by 1 element */
		if ((mem = realloc(array->element, ++array->size * sizeof(void *))) == NULL) {
			raise(OutOfMemoryError);
			return false;
		}
		array->element = mem;
	}
	array->element[array->size - 1] = element;  /* - 1 because element array is zero-based */

	return true;
}


/* Remove an element from an array.
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

	/* free the space of the removed element */
	if ((mem = realloc(array->element, --array->size * sizeof(void *))) == NULL)
		return false;

	array->element = mem;

	return true;
}


/* Insert an element in an array before a certain index.
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

	/* add space to the array for the new element */
	if ((mem = realloc(array->element, ++array->size * sizeof(void *))) == NULL)
		return false;

	array->element = mem;

	/* memmove is safe when source and destination area overlap */
	memmove(array->element + before_index + 1, array->element + before_index, (array->size + 1 - before_index) * sizeof(void *));

	array->element[before_index] = element;

	return true;
}
