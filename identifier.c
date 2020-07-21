/* identifier.c
 *
 * Identifier management.
 *
 * Identifiers are names which refer to variables or functions. An identifier
 * has a certain scope. At any moment only two scope levels are relevant; the
 * global scope at program level, and the local scope within the currently
 * executed function. When entering a function a new (lowest) scope level
 * must be created.
 *
 * For example identifier 'alpha' may occur in all levels in the scope
 * hierarchy. However the name is only searched at local, and if not
 * found there, at global level.
 *
 * Identifiers are stored in a singly linked list. 'Next' points to the next
 * identifier. Every list starts at a header (of type 'Scope'). These headers
 * are also stored in a singly linked list. 'Parent' always points to the next
 * higher scope level. The highest (= global) level always has a header, called
 * 'top'. Lower levels are created when needed. Global variables 'global' and
 * 'local' provide quick access to respectively the highest and lowest
 * levels in the scope hierarchy.
 *
 *	1994 K.W.E. de Lange
 */
#include <stdlib.h>
#include <string.h>

#include "identifier.h"
#include "error.h"


static Scope top = SCOPE_INIT;	/* head of global identifier list */

static Scope *global = &top;	/* initially global ... */
static Scope *local = &top;		/* ... and local scope are the same */


/* Search an identifier in a specific scope list.
 *
 * level	list to search
 * name		identifier name
 * return	Identifier* object or NULL if not found
 */
static Identifier *searchIdentifierInScope(const Scope *level, const char *name)
{
	Identifier *id;

	for (id = level->first; id; id = id->next)
		if (strcmp(name, id->name) == 0)
			break;

	return id;
}


/* API: Search an identifier, first at local then at global scope level.
 *
 * name		identifier name
 * return	Identifier* object or NULL if not found
 */
static Identifier *search(const char *name)
{
	Identifier *id;

	if ((id = searchIdentifierInScope(local, name)) == NULL)
		id = searchIdentifierInScope(global, name);

	return id;
}


/* Create a new identifier in a specific scope list.
 *
 * type		identifier type (VARIABLE or FUNCTION)
 * level	list in which to add the identifier
 * name		identifier name
 * return	Identifier object or NULL if the identifier already exists
 */
static Identifier *addIdentifier(identifiertype_t type, Scope *level, const char *name)
{
	Identifier *id = NULL;

	if ((searchIdentifierInScope(level, name)) == NULL) {
		if ((id = calloc(1, sizeof(Identifier))) == NULL)
			raise(OutOfMemoryError);

		*id = identifier;

		if ((id->name = strdup(name)) == NULL)
			raise(OutOfMemoryError);

		id->type = type;
		id->node = NULL;
		id->object = NULL;
		id->next = level->first;
		level->first = id;
	}
	return id;
}


/* API: Add an identifier to the local scope.
 *
 * name		identifier name
 * return	identifier object
 */
static Identifier *add(identifiertype_t type ,const char *name)
{
	return addIdentifier(type, local, name);
}


/* API: Unbind an object and an identifier.
 *
 * id		identifier to unbind
 *
 * Unbinding means there is one less reference to the object so
 * the objects reference counter is decremented. For identifiers
 * which point to functions only the pointer to the function
 * is removed.
 */
static void unbind(Identifier *id)
{
	debug_printf(DEBUGALLOC, "\nunbind: %s%s, %-p", id->name, id->type == FUNCTION ? "()" : "", \
							 id->type == FUNCTION ? (void *)0 : (void *)id->object);

	if (id->type == VARIABLE) {
		if (id->object) {
			obj_decref(id->object);
			id->object = NULL;
		}
	} else if (id->type == FUNCTION)
		id->node = NULL;
}


/* API: Bind an object to an identifier. First remove an existing binding (if any).
 *
 * id		identifier to bind object to
 * obj		object to bind to identifier
 *
 * Binding does *not* increment an objects reference counter. This must be
 * done by the function supplying or using the object.
 */
static void bind(Identifier *id, void *obj)
{
	debug_printf(DEBUGALLOC, "\nbind  : %s%s, %-p", id->name, id->type == FUNCTION ? "()" : "" , \
							 id->type == FUNCTION ? (void *)0 : (void *)obj);

	if (id->type == VARIABLE) {
		if (id->object)
			unbind(id);
		id->object = obj;
	} else if (id->type == FUNCTION)
		id->node = obj;
}


/* Remove an identifier and free the allocated memory.
 *
 * id		identifier to remove
 *
 * Also removes the link with the object.
 */
static void removeIdentifier(Identifier *id)
{
	unbind(id);
	free(id->name);

	*id = (const Identifier) { 0 };  /* clear the identifier struct, facilitates debugging */

	free(id);
}


/* API: Append a new lowest level to the scope hierarchy.
 */
static void appendScopeLevel(void)
{
	Scope *level;

	if ((level = calloc(1, sizeof(Scope))) == NULL)
		raise(OutOfMemoryError);

	*level = scope;

	level->parent = local;
	level->first = NULL;

	local = level;
}


/* API: Remove the lowest level from the scope hierarchy.
 *
 * Also releases all identifiers and their link to objects.
 */
static void removeScopeLevel(void)
{
	Identifier *id;
	Scope *level;

	level = local;

	while ((id = level->first)) {
		level->first = id->next;
		removeIdentifier(id);
	}

	if (local != global) {
		local = level->parent;
		free(level);
	} else
		global->first = NULL;
}


#ifdef DEBUG
/*  Print identifiers per level to a semi-colon separated file.
 *
 */
void dump_identifiers_to_file(FILE *fp)
{
	int n;
	Scope *level;
	Identifier *id;

	for (level = local, n = 0; level; level = level->parent, n++)
		;

	fprintf(fp, "%s;%s;%s;%s\n", "level", "name", "type", "object");

	for (level = local; level; level = level->parent, n--) {
		for (id = level->first; id; id = id->next) {
			fprintf(fp, "%d;%s;%s;", n, id->name, identifiertypeName(id->type));
			if (id->object != NULL)
				fprintf(fp, "%-p", (void *)id->object);
			fprintf(fp, "\n");
		}
	}
}


void dump_identifiers(void)
{
	FILE *fp;

	/* file extension .dsv stands for delimiter separated file. */
	if ((fp = fopen("identifier.dsv", "w")) != NULL) {
		dump_identifiers_to_file(fp);
		fclose(fp);
	}
}
#endif


/* The identifier API.
 */
Identifier identifier = {
	.name = NULL,
	.next = NULL,
	.object = NULL,

	.add = add,
	.search = search,
	.bind = bind,
	.unbind = unbind
	};


/* The scope API.
 */
Scope scope = {
	.parent = NULL,
	.first = NULL,

	.append_level = appendScopeLevel,
	.remove_level = removeScopeLevel
	};
