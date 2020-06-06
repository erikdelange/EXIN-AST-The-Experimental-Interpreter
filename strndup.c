/* strndup.c
 *
 * Not every C compiler has this function in its standard library so here
 * is a private version.
 *
 * 1994	K.W.E. de Lange
 */
#include <stdlib.h>
#include <string.h>


char *strndup(const char *s, size_t n)
{
	char *result;
	size_t len = strlen(s);

	if (n < len)
		len = n;

	if ((result = malloc(len + 1)) == NULL)
		return NULL;

	result[len] = '\0';

	return (char *)memcpy(result, s, len);
}
