/* stack.h
 *
 * Data structures and definitions for a stack holding pointers to void.
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#ifndef _STACK_
#define _STACK_

#include <stdbool.h>

#define STACKINCREMENT 10	/* number of elements to add when the stack needs to grow */
#define STACKDECREMENT 10	/* when stack has this many unused elements it will shrink */

typedef struct stack {
	long top;				/* zero-based index of the item on top of the stack */
	long capacity;			/* size of the stack as number of elements */
	void **array;			/* pointer to an array of pointers holding the values */

	struct stack *(*alloc)(long);
	void (*free)(struct stack *);
	void (*push)(struct stack *, void *);
	void *(*pop)(struct stack *);
	void *(*peek)(struct stack *);
	bool (*is_empty)(struct stack *);
} Stack;

extern Stack stack;

#endif
