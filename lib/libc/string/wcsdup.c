/*	$NetBSD: wcsdup.c,v 1.1 2006/08/22 20:50:46 christos Exp $	*/

#include <sys/cdefs.h>

#if defined(LIBC_SCCS) && !defined(lint) 
__RCSID("$NetBSD: wcsdup.c,v 1.1 2006/08/22 20:50:46 christos Exp $"); 
#endif /* LIBC_SCCS and not lint */ 

#include "namespace.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>

__weak_alias(wcsdup,_wcsdup)

wchar_t *
wcsdup(const wchar_t *str)
{
	wchar_t *copy;
	size_t len;

	_DIAGASSERT(str != NULL);

	len = wcslen(str) + 1;
	copy = malloc(len * sizeof (wchar_t));

	if (!copy)
		return NULL;

	return wmemcpy(copy, str, len);
}
