/* function.h
 *
 * 2019	K.W.E. de Lange
 */
#ifndef _FUNCTION_
#define _FUNCTION_

#include "array.h"
#include "stack.h"

bool is_builtin(const char *functionname);
size_t builtin_argc(const char *functionname);
void visit_builtin(const char *functionname, Array *arguments, Stack *s);

#endif
