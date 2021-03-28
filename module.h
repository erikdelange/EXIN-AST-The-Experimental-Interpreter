/* module.h
 *
 * Copyright (c) 2018 K.W.E. de Lange
 */
#ifndef _MODULE_
#define _MODULE_

/* This struct is the API to a module object, containing both data and
 * function addresses.
 *
 * Function import() loads a new module. Function search() looks for a module
 * in the list of loaded modules.
 */
typedef struct module {
	struct module *next;	/* next module in list with loaded modules */
	char *name;				/* module name */
	char *code;  			/* buffer containing module code */
	size_t size;			/* number of bytes in code buffer */
	size_t pos;				/* index in code of the next character to read */
	size_t bol;				/* index in code of beginning of current line */
	size_t lineno;			/* number of current line */

	int (*nextch)(struct module *);			/* read the next character */
	int (*peekch)(struct module *);			/* peek the next character */
	int (*pushch)(struct module *, char);	/* push character back in the input stream */
	void (*tobol)(struct module *);			/* move to beginning of current line */
	void (*reset)(struct module *);			/* reset reader to line 1, character 1 */

	struct module *(*import)(const char *name);	/* import a new module */
	struct module *(*search)(const char *name);	/* search for loaded module */
} Module;

extern Module module;

#endif
