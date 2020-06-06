/* config.h
 *
 * Configuration constants.
 *
 * 2018	K.W.E. de Lange
 */
#ifndef _CONFIG_
#define _CONFIG_

#include <stdio.h>

#define LANGUAGE	"EXIN"
#define VERSION		"2.00"
#define TABSIZE		4		/* default spaces per tab */

/* Constants which are used to define the size of
 * arrays which are created at compile time
 */
#define BUFSIZE		256		/* maximum length of identifier name excl '\0' */
#define LINESIZE	256		/* maximum length of input line excl '\0' */
#define MAXINDENT	132		/* maximum number of indents */

/* C representation of EXIN's basic variable types
 */
typedef char char_t;		/* basic type for CHAR_T */
typedef long int_t;			/* basic type for INT_T */
typedef double float_t;		/* basic type for FLOAT_T */

/* Container for all global configuration variables
 * which can be changed during run time.
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
#define DEBUGALLOC	    2   /* show object alloc() & free() * (un)bind() */
#define DEBUGASTSTOP    4	/* print ast and stop */
#define DEBUGASTEXEC    8	/* print ast and execute */
#define DEBUGDUMP       16	/* dump identifiers and objects to stdout */
#define DEBUGDUMPFILE   32	/* dump identifiers and objects to file */

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#endif
