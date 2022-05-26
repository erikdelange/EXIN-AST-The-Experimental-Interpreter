/* module.c
 *
 * Code is stored in modules. Each module is a file. Modules are loaded via
 * the (global) module.import() function. Every module object contains a
 * reference to the loaded code of that module and is used to read characters
 * from the code.
 * Module objects are stored in a singly linked list starting at 'modulehead'.
 *
 * Copyright (c) 1995 K.W.E. de Lange
 */
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"
#include "error.h"


/* Pointer to the head of the list of loaded modules.
 */
static Module *modulehead = NULL;


/* API: Search a module in the list of loaded modules.
 *
 * name		filename of module (may include path)
 * return	module object or NULL if not found
 */
static Module *search(const char *name)
{
	Module *m;

	assert(name != NULL);
	assert(*name != '\0');

	for (m = modulehead; m != NULL; m = m->next)
		if (strcmp(name, m->name) == 0)
			break;

	return m;
}


/* Load the code for a module. An extra closing newline and '\0' are added
 * at the end of the code.
 *
 * self		pointer to module object
 * name		filename (may include path)
 * return	1 if successful else 0 with errno set
 */
static int load(Module *self, const char *name)
{
	FILE *fp;
	struct stat stat_buffer;

	if (stat(name, &stat_buffer) == 0) {
		self->size = stat_buffer.st_size;
		if ((self->code = calloc(self->size + 2, sizeof(char))) != NULL) {
			if ((fp = fopen(name, "r")) != NULL) {
				self->size = fread(self->code, sizeof(char), self->size, fp);
				self->code[self->size++] = '\n';
				self->code[self->size] = 0;
				fclose(fp);
				return 1;
			} else {
				free(self->code);
				self->code = NULL;
			}
		}
	}
	return 0;
}


/* API: Create a new module object and load the code.
 *
 * name		module's filename (may include path)
 * return	module object if successful else an error is raised and the program exits
 */
static Module *import(const char *name)
{
	Module *m = NULL;

	assert(name != NULL);
	assert(*name != '\0');

	if ((m = calloc(1, sizeof(Module))) == NULL)
		raise(OutOfMemoryError);
	else {
		*m = module;

		if (load(m, name) == 0)
			raise(SystemError, "error importing %s: %s (%d)", name, \
								strerror(errno), errno);

		if ((m->name = strdup(name)) == NULL)
			raise(OutOfMemoryError);

		m->next = modulehead;
		modulehead = m;
	}
	return m;
}


/* API: Read the next character.
 *
 * return   the character read or EOF in case end of code was reached
 */
static int nextch(Module *module)
{
	if (module->pos >= module->size)
		return EOF;
	else {
		if (module->pos > 0 && module->code[module->pos - 1] == '\n') {
			module->bol = module->pos;
			module->lineno++;
		}
		return module->code[module->pos++];
	}
}


/* API: Look ahead to see what the next character is, but don't read it.
 *
 * return   the next character to read
 */
static int peekch(Module *module)
{
	if (module->pos >= module->size)
		return EOF;
	else
		return module->code[module->pos];
}


/* API: Undo the read of a character.
 *
 * ch		the character to push back into the input stream
 * return	the character which was pushed back
 */
static int pushch(Module *module, const char ch)
{
	if (module->pos > 0) {
		module->pos -= 1;
		if (module->pos > 0 && module->code[module->pos - 1] == '\n')
			module->lineno--;
	}

	assert(module->code[module->pos] == (char)ch);

	return ch;
}


/*	The module API.
 */
Module module = {
	.next = NULL,
	.name = "",
	.code = "\n",
	.size = 0,
	.pos = 0,
	.bol = 0,
	.lineno = 1,

	.nextch = nextch,
	.peekch = peekch,
	.pushch = pushch,

	.import = import,
	.search = search
	};
