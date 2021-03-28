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
#define STACKDECREMENT 100	/* when stack has this many unused elements it will shrink */

#if (STACKDECREMENT / STACKINCREMENT) < 10
#error "STACKDECREMENT must be at least 10 times as big as STACKINCREMENT"
#endif

typedef struct stack {
	long top;				/* zero-based index of the item on top of the stack */
	long capacity;			/* size of the stack as number of elements */
	void **array;			/* pointer to an array of pointers holding the values */
} Stack;

extern struct stack *stack_alloc(long capacity);
extern void stack_free(Stack *s);

extern void push(Stack *s, void *item);
extern void *pop(Stack *s);
extern void *peek(Stack *s);
extern bool is_empty(Stack *s);

#endif
