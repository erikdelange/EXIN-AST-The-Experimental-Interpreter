/* main.c
 *
 * The interpreters main program. Handles command line arguments
 * and starts execution of the code of the module specified
 * on the command line.
 *
 * Copyright (c) 2018 K.W.E. de Lange
 */
#include <ctype.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>

#include "identifier.h"
#include "object.h"
#include "config.h"
#include "visit.h"
#include "parse.h"


Config config = {				/* global configuration variables */
	.debug = NODEBUG,
	.tabsize = TABSIZE
};


/* Print the usage message to stream (normally stdout or stderr).
 */
static void usage(char *executable, FILE *stream)
{
	fprintf(stream, "\n%s version %s\n", LANGUAGE, VERSION);
	fprintf(stream, "usage: %s [options] module\n", executable);
	fprintf(stream, "module: name of file containing code to execute\n");
	fprintf(stream, "options\n");
	#ifdef DEBUG
	fprintf(stream, "-d[detail] = show debug info\n");
	fprintf(stream, "    detail = sum of options (default = %d)\n", DEBUGASTEXEC);
	fprintf(stream, "    option %2d: no debug output\n", 0);
	fprintf(stream, "    option %2d: show tokens during parsing\n", DEBUGTOKEN);
	fprintf(stream, "    option %2d: show memory allocation\n", DEBUGALLOC);
	fprintf(stream, "    option %2d: show abstract syntax tree after parsing and stop\n", DEBUGASTSTOP);
	fprintf(stream, "    option %2d: show abstract syntax tree after parsing and execute\n", DEBUGASTEXEC);
	fprintf(stream, "    option %2d: dump identifier and object table to stdout after program end\n", DEBUGDUMP);
	fprintf(stream, "    option %2d: dump identifier and object table to disk after program end\n", DEBUGDUMPFILE);
	#endif  /* DEBUG */
	fprintf(stream, "-h = show usage information\n");
	fprintf(stream, "-t[tabsize] = set tab size in spaces\n");
	fprintf(stream, "    tabsize = >= 1 (default = %d)\n", TABSIZE);
	fprintf(stream, "-v = show version information\n");
}


/* The interpreter starts here.
 *
 * For command line options see function usage().
 */
int	main(int argc, char **argv)
{
	char ch;
	char *executable = basename(*argv);

	setbuf(stdout, NULL);  /* unbuffered output */

	/* decode flags on the command line */
	while (--argc > 0 && (*++argv)[0] == '-') {
		ch = *++argv[0];
		switch (ch) {
			#ifdef DEBUG
			case 'd':
				if (isdigit(*++argv[0]))
					config.debug = (int)str_to_int(&(*argv[0]));
				else
					config.debug = DEBUGASTEXEC;
				break;
			#endif  /* DEBUG */
			case 'h':
				usage(executable, stdout);
				return 0;
			case 't':
				if (isdigit(*++argv[0])) {
					config.tabsize = (int)str_to_int(&(*argv[0]));
					if (config.tabsize < 1) {
						fprintf(stderr, "%s: invalid tabsize %d\n", \
										executable, config.tabsize);
						config.tabsize = TABSIZE;
					}
				} else
					config.tabsize = TABSIZE;
				break;
			case 'v':
				fprintf(stdout, "%s version %s\n", LANGUAGE, VERSION);
				return 0;
			default:
				fprintf(stderr, "%s: unknown option -%c\n", executable, ch);
				usage(executable, stderr);
				return 0;
		}
	}

	/* now process the remaining arguments */
	if (argc == 0) {
		fprintf(stderr, "%s: module name missing\n", executable);
		usage(executable, stderr);
	} else if (argc == 1) {
		Stack *s = stack_alloc(10);
		int r = 0;

		Node *root = parse(module.import(*argv));  /* step 1: parse module(s) */

		if (config.debug & (DEBUGASTEXEC | DEBUGASTSTOP))
			print(root, 0);

		Config tmp;
		tmp.debug = config.debug;
		config.debug = 0;  /* no debug output when checking */

		check(root);  /* step 2: do code checks */
		scope.remove_level();

		config.debug = tmp.debug;

		visit(root, s);  /* step 3: visit = execute the AST */

		if (!is_empty(s)) {  /* check for return value */
			Object *obj = pop(s);
			if (isNumber(obj))
				r = obj_as_int(obj);
			obj_decref(obj);
		}

		#ifdef DEBUG
		if (config.debug & (DEBUGDUMP | DEBUGDUMPFILE)) {
			printf("\nstack content = %ld value(s)\n", s->top + 1);

			while (is_empty(s) == false)
				obj_print(stdout, pop(s));
		}

		if (config.debug & DEBUGDUMP) {
			dump_identifiers_to_file(stdout);
			dump_objects_to_file(stdout);
		}

		if (config.debug & DEBUGDUMPFILE) {
			dump_identifiers();
			dump_objects();
		}
		#endif  /* DEBUG */

		return r;
	} else {  /* more than 1 argument */
		fprintf(stderr, "%s: to many modules\n", executable);
		usage(executable, stderr);
	}
	return 0;
}
