/* error.c
 *
 * Error handling.
 *
 * Copyright (c) 1995 K.W.E. de Lange
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "scanner.h"
#include "error.h"
#include "visit.h"
#include "ast.h"


/* Table containing all error messages. No sorting needed.
 */
static struct {
	int number;
	char *description;
	bool print_extra_info;
} errors[] = {
	{ NameError, "NameError", true },
	{ TypeError, "TypeError", true },
	{ SyntaxError, "SyntaxError", true },
	{ ValueError, "ValueError", true },
	{ SystemError, "SystemError", true },
	{ IndexError, "IndexError: index out of range", false },
	{ OutOfMemoryError, "Out of memory", false },
	{ ModNotAllowedError, "ModNotAllowedError", true },
	{ DivisionByZeroError, "DivisionByZeroError: division by zero", false },
	{ DesignError, "DesignError", true }
};


/* Search the index of the error in the errors[] table.
 *
 * number	error number to search
 * return	index (>= 0) if error found, else -1
 */
static int error_index(const int number)
{
	int i = 0;

	while (1) {
		if (errors[i].number == number)
			break;
		if (++i <= (int)(sizeof errors / sizeof errors[0]) - 1)
			continue;
		return -1;
	}

	return i;
}


/* Display an error message and stop the interpreter.
 *
 * number	error number (see error.h)
 * ...		optional printf style format string, optionally followed by arguments
 * return	nothing, exits the program
 *
 * Example: raise(TypeError, "%s is not subscriptable", TYPENAME(sequence));
 */
void raise(const int number, ...)
{
	int i;
	char *p;
	char *format;
	va_list argp;

	if ((i = error_index(number)) == -1)
		raise(DesignError, "unknown error number %d", number);

	if (current_node || scanner.module) {  /* busy ( checking, visiting ) or parsing */
		char *modulename = current_node ? current_node->source.module->name : scanner.module->name;
		size_t lineno = current_node ? current_node->source.lineno : scanner.module->lineno;
		char *code = current_node ? current_node->source.module->code : scanner.module->code;
		size_t bol = current_node ? current_node->source.bol : scanner.module->bol;

		fprintf(stderr, "File %s", modulename);
		fprintf(stderr, ", line %lu\n", (unsigned long)lineno);

		for (p = code + bol; *p && isspace(*p); p++)
			;  /* skip leading spaces */

		for (; *p && *p != '\n'; p++)
			fprintf(stderr, "%c", *p);

		fprintf(stderr, "\n");
	}

	fprintf(stderr, "%s", errors[i].description);

	va_start(argp, number);

	if (errors[i].print_extra_info == 1) {
		format = va_arg(argp, char *);
		if (format) {
			fprintf(stderr, ": ");
			vfprintf(stderr, format, argp);
		}
	}
	fprintf(stderr, "\n");

	va_end(argp);

	exit(number);
}
