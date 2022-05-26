/* list.c
 *
 * List object operations
 *
 * See list.h for an explanation on the data structures for lists.
 *
 * 2016 K.W.E. de Lange
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "error.h"
#include "none.h"
#include "list.h"


/* Minimal set of forward declarations.
 *
 */
static int_t length(ListObject *obj);



/* Create a new empty list-object.
 *
 * return	new list-object or NULL in case of error
 */
static ListObject *list_alloc(void)
{
	ListObject *obj;

	if ((obj = calloc(1, sizeof(ListObject))) != NULL) {
		obj->typeobj = (TypeObject *)&listtype;
		obj->type = LIST_T;
		obj->refcount = 0;

		obj->head = NULL;
		obj->tail = NULL;
	}
	return obj;  /* returns NULL if alloc failed */
}


/* Free a list-object, including all listnodes. Remove the
 * reference from the listnodes to the objects they hold.
 *
 */
static void list_free(ListObject *obj)
{
	Object *item;

	while (length(obj) > 0) {
		item = listtype.remove(obj, 0);
		obj_decref(item);
	}

	*obj = (const ListObject) { 0 };  /* clear the object struct, facilitates debugging */

	free(obj);
}


/* Print list content between square brackets.
 *
 */
static void list_print(FILE *fp, ListObject *obj)
{
	printf("[");

	for (ListNode *listnode = obj->head; listnode; listnode = listnode->next) {
		obj_print(fp, listnode->obj);
		if (listnode->next)
			fprintf(fp, ",");
	}
	fprintf(fp, "]");
}


/* Copy the content of list 'src' to list 'dest'.
 *
 * List 'dest' will be emptied, and on return will
 * contain new objects (= deep copy).
 *
 * dest		destination list
 * src		source list
 */
static void list_set(ListObject *dest, ListObject *src)
{
	Object *item;

	while (length(dest) > 0) {
		item = listtype.remove(dest, 0);
		obj_decref(item);
	}

	for (ListNode *listnode = src->head; listnode; listnode = listnode->next)
		listtype.append(dest, obj_copy(listnode->obj));
}


static void list_vset(ListObject *obj, va_list argp)
{
	list_set(obj, va_arg(argp, ListObject *));
}


/* Execute a method on a list.
 *
 * obj			list-object for which method was called
 * name			method name
 * arguments	method arguments as array with pointers to objects
 * return		object with method results or none-object in case of error
 */
static Object *list_method(ListObject *obj, char *name, Array *arguments)
{
	Object *result;

	if (strcmp("len", name) == 0) {
		if (arguments->size != 0) {
			raise(SyntaxError, "method %s takes %d arguments", name, 0);
			result = obj_alloc(NONE_T);
		} else
			result = listtype.length(obj);
	} else if (strcmp("insert", name) == 0) {
		if (arguments->size != 2)
			raise(SyntaxError, "method %s takes %d arguments", name, 2);
		else {
			Object *index = arguments->element[0];
			Object *value = arguments->element[1];

			listtype.insert(obj, obj_as_int(index), obj_copy(value));
		}
		result = obj_alloc(NONE_T);
	} else if (strcmp("append", name) == 0) {
		if (arguments->size != 1)
			raise(SyntaxError, "method %s takes %d argument", name, 1);
		else {
			Object *value = arguments->element[0];

			listtype.append(obj, obj_copy(value));
		}
		result = obj_alloc(NONE_T);
	} else if (strcmp("remove", name) == 0) {
		if (arguments->size != 1) {
			raise(SyntaxError, "method %s takes %d argument", name, 1);
			result = obj_alloc(NONE_T);
		} else {
			Object *index = arguments->element[0];

			result = listtype.remove(obj, obj_as_int(index));
		}
	} else {
		raise(SyntaxError, "objecttype %s has no method %s", TYPENAME(obj), name);
		result = obj_alloc(NONE_T);
	}

	return result;
}


/* Count the number of listnodes in a list.
 *
 * obj		listobject for which to count listnodes
 * return	listnode count
 */
static int_t length(ListObject *obj)
{
	ListNode *listnode;
	int_t i;

	for (i = 0, listnode = obj->head; listnode; i++, listnode = listnode->next)
		;

	return i;
}


/* Return listnode count as an integer-object.
 *
 * return	integer-object with count or none-object in case of error
 */
static Object *list_length(ListObject *obj)
{
	Object *len;

	if ((len = obj_create(INT_T, length(obj))) == NULL)
		len = obj_alloc(NONE_T);

	return len;
}


/* Create a new list which consists of the objects from lists op1 and op2.
 *
 * return	new listobject or none-object in case of error
 */
static Object *list_concat(ListObject *op1, ListObject *op2)
{
	ListObject *list;
	ListNode *item;
	int_t i;

	if ((list = (ListObject *)obj_alloc(LIST_T)) == NULL)
		return obj_alloc(NONE_T);

	for (i = 0; i < length(op1); i++) {
		item = listtype.item(op1, i);
		listtype.append(list, obj_copy(item->obj));
		obj_decref(item);
	}

	for (i = 0; i < length(op2); i++) {
		item = listtype.item(op2, i);
		listtype.append(list, obj_copy(item->obj));
		obj_decref(item);
	}

	return (Object *)list;
}


/* Create a new list which consists of n times an existing list.
 *
 * Either operand op1 or op2 is a list-object. The other operand is
 * guaranteed to be number-object and will be converted to an
 * integer. If this is negative it will be silently adjusted
 * to 0, 0 will return an empty list.
 *
 * return	new listobject or none-object in case of error
 */
static Object *list_repeat(Object *op1, Object *op2)
{
	ListObject *list;
	ListNode *item;
	int_t i, times;

	Object *s = TYPE(op1) == LIST_T ? op1 : op2;
	Object *n = TYPE(op1) == LIST_T ? op2 : op1;

	times = obj_as_int(n);

	if (times < 0)
		times = 0;

	if ((list = (ListObject *)obj_alloc(LIST_T)) == NULL)
		return obj_alloc(NONE_T);

	while (times--)
		for (i = 0; i < length((ListObject *)s); i++) {
			item = listtype.item((ListObject *)s, i);
			listtype.append(list, obj_copy(item->obj));
			obj_decref(item);
		}

	return (Object *)list;
}


/* Compare the content of two lists by index (math: tuple).
 *
 * return	true of content is equal else false
 */
static bool list_cmp(ListObject *op1, ListObject *op2)
{
	bool equal;
	Object *obj;
	int_t i, l1;
	ListNode *item1, *item2;

	l1 = length(op1);

	if (l1 != length(op2))
		return false;  /* the lists should at least be of equal length */

	for (equal = true, i = 0; i < l1; i++) {
		item1 = listtype.item(op1, i);
		item2 = listtype.item(op2, i);
		obj = obj_eql((Object *)item1, (Object *)item2);
		equal = obj_as_bool(obj);
		obj_decref(item1);
		obj_decref(item2);
		obj_decref(obj);
		if (equal == false)
			break;  /* stop compare on first mismatch */
	}
	return i == l1 ? true : false;  /* true (1) = equal, false (0) = not equal */
}


/* Check if content of two lists is equal.
 *
 * return	integer-object with value 1 if equal or value 0 if not equal,
 *			none-object in case of error
 */
static Object *list_eql(ListObject *op1, ListObject *op2)
{
	Object *result;

	if ((result = obj_create(INT_T, list_cmp(op1, op2))) == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/* Check if content of two lists is not equal.
 *
 * return	integer-object with value 0 if equal or value 1 if not equal,
 * 			none-object in case of error
 */
static Object *list_neq(ListObject *op1, ListObject *op2)
{
	Object *result;

	if ((result = obj_create(INT_T, !list_cmp(op1, op2))) == NULL)
		result = obj_alloc(NONE_T);

	return result;
}


/* Retrieve a listnode from a list by its index.
 *
 * Note: The refcount of the listnode is increased by 1.
 *
 * list		list to retrieve the object from
 * index	index number of object, negative numbers count from the end
 * return	retrieved listnode-object or none-object in case of error
 */
static ListNode *list_item(ListObject *list, int_t index)
{
	ListNode *listnode;
	int_t len, i;

	len = length(list);

	if (index < 0)
		index += len;

	if (index < 0 || index >= len) {
		raise(IndexError);
		return (ListNode *)obj_alloc(NONE_T);
	}

	for (i = 0, listnode = list->head; listnode; i++, listnode = listnode->next) {
		if (i == index) {
			break;
		}
	}

	obj_incref(listnode);

	return listnode;
}


/* Create a new list by taking a slice from an existing list.
 *
 * The new list contains new objects (= deep copy). 'Start' and 'end'
 * are silently adjusted to the nearest possible values.
 *
 * list		list to take slice from
 * start	index number to start the slice
 * end		last index number of the slice
 * return	new list-object with slice or none-object in case of error
 */
static ListObject *list_slice(ListObject *list, int_t start, int_t end)
{
	ListObject *slice;
	ListNode *listnode;
	int_t len;

	len = length(list);

	if (start < 0)
		start += len;

	if (end < 0)
		end += len;

	if (start < 0)
		start = 0;

	if (end >= len)
		end = len;

	if ((slice = (ListObject *)obj_alloc(LIST_T)) != NULL) {
		for (int_t i = start; i < end; i++) {
			listnode = listtype.item(list, i);
			listtype.append(slice, obj_copy(listnode->obj));
			obj_decref(listnode);
		}
	} else
		slice = (ListObject *)obj_alloc(NONE_T);

	return slice;
}


/* Append an object to the end of a list.
 *
 * list		list to append object to
 * obj		object to append
 */
static void list_append_object(ListObject *list, Object *obj)
{
	ListNode *listnode, *tail;

	if ((listnode = (ListNode *)obj_create(LISTNODE_T, obj)) == NULL)
		return;

	if (list->head == NULL) {  /* append to empty list */
		list->head = listnode;
		list->tail = listnode;
	} else {  /* append to list which already has one of more listnodes */
		tail = list->tail;
		listnode->prev = tail;
		tail->next = listnode;
		list->tail = listnode;
	}
}


/* Insert an object before the listnode with index number 'index'.
 *
 * Index is silently adjusted to the nearest possible value.
 * A negative index counts back from the end of the list. Index -1
 * points to the last listnode.
 *
 * list		list to insert object into
 * index	insert object before this index number
 * obj		object to insert
 */
static void list_insert_object(ListObject *list, int_t index, Object *obj)
{
	ListNode *listnode, *iptr;
	int_t len;

	if ((listnode = (ListNode *)obj_create(LISTNODE_T, obj)) == NULL)
		return;

	if (list->head == NULL) {  /* insert in empty list */
		list->head = listnode;
		list->tail = listnode;
	} else {  /* insert in list which already has one or more listnodes */
		len = length(list);

		if (index < 0)
			index += len;

		if (index <= 0) {  /* insert before first listnode */
			listnode->next = list->head;
			listnode->next->prev = listnode;
			listnode->prev = NULL;
			list->head = listnode;
		} else if (index >= len) {  /* insert after last listnode */
			iptr = list->tail;
			listnode->prev = iptr;
			iptr->next = listnode;
			list->tail = listnode;
		} else {  /* insert somewhere in the middle */
			for (iptr = list->head; index--; iptr = iptr->next)
				;
			/* insert before iptr */
			listnode->next = iptr;
			listnode->prev = iptr->prev;
			listnode->next->prev = listnode;
			listnode->prev->next = listnode;
		}
	}
}


/* Remove the listnode with index number 'index' from a list.
 *
 * Index must exist (numbering starts at 0). A negative index counts back
 * from the end of the list. Index -1 points to the last listnode.
 *
 * list		list to remove object from
 * index	index number of object to remove
 * return	object which was removed from list, or none-object in case index was out if range
 */
static Object *list_remove_object(ListObject *list, int_t index)
{
	ListNode *listnode;
	Object *obj = NULL;
	int_t len, i;

	len = length(list);

	if (index < 0)
		index += len;  /* negative index */

	if (index < 0 || index >= len)
		return obj_alloc(NONE_T);  /* IndexError: index out of range */

	for (i = 0, listnode = list->head; listnode; i++, listnode = listnode->next) {
		if (i == index) {
			obj = listnode->obj;
			if (list->head == list->tail) {  /* list contains only 1 listnode */
				list->head = NULL;
				list->tail = NULL;
			} else if (listnode->prev == NULL) {  /* at least 2 listnodes, remove first */
				list->head = listnode->next;
				listnode->next->prev = NULL;
			} else if (listnode->next == NULL) {  /* at least 2 listnodes, remove last */
				list->tail = listnode->prev;
				listnode->prev->next = NULL;
			} else {  /* at least 3 nodes, listnode is not first or last */
				listnode->prev->next = listnode->next;
				listnode->next->prev = listnode->prev;
			}
			obj_incref(obj);  /* avoid that obj (= return value) is released */
			obj_decref(listnode);
			break;
		}
	}
	return obj;
}


/* List object API.
*/
ListType listtype = {
	.name = "list",
	.alloc = (Object *(*)())list_alloc,
	.free = (void (*)(Object *))list_free,
	.print = (void (*)(FILE *, Object *))list_print,
	.set = list_set,
	.vset =  (void (*)(Object *, va_list))list_vset,
	.method = (Object *(*)(Object *, char *, Array *))list_method,

	.length = list_length,
	.item = list_item,
	.slice = list_slice,
	.concat = list_concat,
	.repeat = list_repeat,
	.eql = list_eql,
	.neq = list_neq,
	.insert = list_insert_object,
	.append = list_append_object,
	.remove = list_remove_object
	};


/* Create a new empty listnode.
 *
 * return	new listnode-object or none-object in case of error
 */
static ListNode *listnode_alloc(void)
{
	ListNode *obj;

	if ((obj = calloc(1, sizeof(ListNode))) != NULL) {
		obj->typeobj = (TypeObject *)&listnodetype;
		obj->type = LISTNODE_T;
		obj->refcount = 0;

		obj->next = NULL;
		obj->prev = NULL;
		obj->obj = NULL;
	}
	return obj;  /* returns NULL if alloc failed */
}


/* Free a listnode, and release the object it references.
 *
 */
static void listnode_free(ListNode *listnode)
{
	if (listnode->obj)
		obj_decref(listnode->obj);

	*listnode = (const ListNode) { 0 };  /* clear the object struct, facilitates debugging */

	free(listnode);
}


/* Print listnode content.
 *
 */
static void listnode_print(FILE *fp, ListNode *listnode)
{
	obj_print(fp, listnode->obj);
}


static void listnode_set(ListNode *listnode, Object *obj)
{
	if (listnode->obj)
		obj_decref(listnode->obj);

	listnode->obj = obj;
}


static void listnode_vset(ListNode *listnode, va_list argp)
{
	listnode_set(listnode, va_arg(argp, Object *));
}


static ListNode *listnode_method(ListNode *obj, char *name, Array *arguments)
{
	UNUSED(arguments);

	raise(SyntaxError, "objecttype %s has no method %s", TYPENAME(obj), name);

	return (ListNode *)obj_alloc(NONE_T);
}


/* Listnode object API.
 */
ListNodeType listnodetype = {
	.name = "listnode",
	.alloc = (Object *(*)())listnode_alloc,
	.free = (void (*)(Object *))listnode_free,
	.print = (void (*)(FILE *, Object *))listnode_print,
	.set = listnode_set,
	.vset = (void (*)(Object *, va_list))listnode_vset,
	.method = (Object *(*)(Object *, char *, Array *))listnode_method
	};
