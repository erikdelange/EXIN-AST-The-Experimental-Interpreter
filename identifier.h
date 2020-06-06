/* identifier.h
 *
 * 1994	K.W.E. de Lange
 */
#ifndef _IDENTIFIER_
#define _IDENTIFIER_

#include "object.h"

typedef enum { VARIABLE=1, FUNCTION } identifiertype_t;

static inline char *identifiertypeName(identifiertype_t t)
{
	static char *string[] = {
		"?", "VARIABLE", "FUNCTION"
	};

	if (t < 0 || t > (sizeof(string) / sizeof(string[0]) - 1))
		t = 0;  /* out of bound values revert to 0 */

	return string[t];
}

typedef struct identifier {
	identifiertype_t type;
	char *name;					/* points to a private copy of identifier name */
	struct identifier *next;	/* NULL for last identifier in list */

	struct object *object;
	struct node *node;

	struct identifier *(*add)(const identifiertype_t type, const char *name);
	struct identifier *(*search)(const char *name);
	void (*bind)(struct identifier *self, void *ptr);
	void (*unbind)(struct identifier *self);
} Identifier;

extern Identifier identifier;

typedef struct scope {
	struct scope *parent;
	Identifier *first;

	void (*append_level)(void);
	void (*remove_level)(void);
} Scope;

extern Scope scope;

#define SCOPE_INIT { .parent = NULL, \
                     .first = NULL }


#ifdef DEBUG
void dump_identifiers_to_file(FILE *fp);
void dump_identifiers(void);
#endif  /* DEBUG */

#endif

