/* array.h
 *
 * Data structures and definitions for a universal array holding pointers to void.
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#ifndef _ARRAY_
#define _ARRAY_

#include <stdlib.h>
#include <stdbool.h>

#define ARRAYINCREMENT 10	/* number of elements to add when the array needs to grow */
#define ARRAYDECREMENT 10	/* when array has this many unused elements it will shrink */

/* Array with pointers to 0 or more elements
 *
 * Utility struct used when a variable number of elements must be stored.
 * The elements stored are void pointers (so can point to anything).
 */
typedef struct array {
	size_t capacity;/* maximum number of pointer which can be stored in array 'element' */
	size_t size;	/* number of pointers currently stored in array 'element' */
	void **element;	/* pointer to array of pointers to elements, NULL is case array capacity == 0 */

	struct array *(*alloc)(void);
	void (*free)(struct array *);
	bool (*append_child)(struct array *, void *);
	bool (*insert_child)(struct array *, size_t, void *);
	bool (*remove_child)(struct array *, size_t);
} Array;

extern Array array;

#endif
