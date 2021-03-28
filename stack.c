/* stack.c
 *
 * Functions for creating and using a stack which holds void pointers.
 *
 * Pointers are added on top of the stack by using push(), and removed
 * from the stack via pop(). The stack gets an initial size when created
 * but automatically expands or shrinks dependent on the number of
 * elements it contains.
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#include <assert.h>
#include <stdlib.h>

#include "stack.h"


/* Check if stack is full.
 *
 * A full stack has top equal to the capacity - 1 because
 * top is zero-based.
 *
 */
static bool is_full(Stack *stack)
{
	assert(stack != NULL);

	return stack->top == stack->capacity - 1 ? true : false;
}


/* Check if stack is empty.
 *
 * A stack is empty when top is equal to -1.
 *
 */
bool is_empty(Stack *stack)
{
	assert(stack != NULL);

	return stack->top == -1 ? true : false;
}


/* Add an item to the stack. Increase top by 1.
 *
 */
void push(Stack *stack, void *item)
{
	assert(stack != NULL);

	if (is_full(stack)) {  /* stack overflow so expand the stack */
		stack->capacity += STACKINCREMENT;
		stack->array = realloc(stack->array, stack->capacity * sizeof(void *));
	}
	stack->array[++stack->top] = item;
}


/* Remove the item at the top of the stack. Decrease top by 1.
 *
 * Shrink the stack of the unused space is more then STACKDECREMENT
 * elements. If shrinking fails then the stack remains unchanged.
 *
 * return	item at top of stack or NULL in case of empty stack
 */
void *pop(Stack *stack)
{
	assert(stack != NULL);

	void *p, *mem;

	if (is_empty(stack))  /* stack underflow */
		return NULL;

	p = stack->array[stack->top--];

	if (stack->top + 1 - stack->capacity >= STACKDECREMENT)
		/* reclaim unused stack space */
		if ((mem = realloc(stack->array, stack->top + 1)) != NULL)
			stack->array = mem;

	return p;
}


/* Return the item at the top of the stack without removing it.
 *
 * return	item at top of stack or NULL in case of empty stack
 */
void *peek(Stack *stack)
{
	assert(stack != NULL);

	if (is_empty(stack))  /* stack underflow */
		return NULL;

	return stack->array[stack->top];
}


/* Allocate a new stack with an initial size of 'capacity'.
 *
 * When -Wfanalyzer-malloc-leak is enabled GCC will report a memory leak here.
 * This is a false positive which can be discarded as the allocated memory
 * is assigned to 'stack' and 'stack->array'.
 *
 */
Stack *stack_alloc(long capacity)
{
	Stack *stack;

	/* allocate the stack data struct */
	if ((stack = (Stack *)malloc(sizeof(Stack))) != NULL) {
		stack->capacity = capacity < 0 ? 0 : capacity;  /* minimum stack size is 0 */
		stack->top = -1;  /* -1 indicates an empty stack */

		/* allocate the array which will contain the pointers */
		if ((stack->array = calloc(stack->capacity, sizeof(void *))) == NULL) {
			free(stack);
			stack = NULL;
		}
	}
	return stack;
}


/* Free the memory allocated by a stack.
 *
 * Note that freeing whatever the stack contains must be done separately.
 *
 */
void stack_free(Stack *stack)
{
	assert(stack != NULL);

	if (stack->array)
		free(stack->array);

	free(stack);
}
