/* config.h
 *
 * Configuration constants.
 *
 * Copyright (c) 2018 K.W.E. de Lange
 */
#ifndef _CONFIG_
#define _CONFIG_

#include <stdio.h>

#define LANGUAGE	"EXIN"
#define VERSION		"2.05"
#define TABSIZE		4		/* default spaces per tab */

/* Constants which are used to define the size of
 * arrays which are created at compile time
 */
#define BUFSIZE		128		/* maximum length of identifier name incl '\0' */
#define LINESIZE	128		/* maximum length of input line incl '\0' */
#define MAXNUMBER	64		/* maximum length of number printed as string incl '\0' */
#define MAXINDENT	132		/* maximum number of indents */

#if BUFSIZE < 9
#error "BUFSIZE must at least be 1 greater than the longest keyword (= continue)"
#endif

/* C representation of EXIN's basic variable types
 */
typedef char char_t;		/* basic type for CHAR_T */
typedef long int_t;			/* basic type for INT_T */
typedef double float_t;		/* basic type for FLOAT_T */

/* Container which holds all global configuration variables
 * whose value can be changed during run time.
 */
typedef struct {
	int debug;      /* debug logging level */
	int tabsize;    /* spaces per tab */
} Config;

extern Config config;

/* Define preprocessor macro DEBUG in the compiler options
 * to enable debug logging.
 */
#ifdef DEBUG
	#define debug_printf(level, fmt, ...) \
				do { \
					if (config.debug & (level)) { \
						fprintf(stdout, fmt, __VA_ARGS__); \
						fflush(stdout); \
					} \
				} while (0)
#else  /* not DEBUG */
	#define debug_printf(level, fmt, ...) \
				do { } while (0)
#endif

/* Debug logging detail levels
 *
 * The level numbers (except 0 :) are ascending powers of 2
 */
#define NODEBUG         0	/* no debug output */
#define DEBUGTOKEN      1	/* show tokens during parsing */
#define DEBUGALLOC      2   /* show object alloc(), free() and (un)bind() */
#define DEBUGASTSTOP    4	/* print AST and stop */
#define DEBUGASTEXEC    8	/* print AST and execute */
#define DEBUGDUMP       16	/* dump identifiers and objects to stdout */
#define DEBUGDUMPFILE   32	/* dump identifiers and objects to file */

/* This macro is used to suppress 'unused argument' warnings during compilation.
 */
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#endif
