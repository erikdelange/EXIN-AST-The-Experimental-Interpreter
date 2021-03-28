/* visit.h
 *
 * Copyright (c) 2020 K.W.E. de Lange
 */
#ifndef _VISIT_
#define _VISIT_

#include "ast.h"

/* Many functions in visit.c share the same signature so
 * the can be referenced by a function pointer. But
 * sometimes you don't need and use all the arguments
 * in the function definition, and this will cause
 * 'unused parameter' warnings during compilation.
 * This macro avoids these warnings, so they do not
 * blur other - perhaps more relevant - warnings.
 *
 */
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

extern Node *current_node;

extern void check(Node *n);
extern void print(Node *n, int level);
extern void visit(Node *n, Stack *s);

#endif
