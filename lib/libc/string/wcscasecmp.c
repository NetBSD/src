/*	$NetBSD: wcscasecmp.c,v 1.1 2006/08/22 20:50:46 christos Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint) 
__RCSID("$NetBSD: wcscasecmp.c,v 1.1 2006/08/22 20:50:46 christos Exp $"); 
#endif /* LIBC_SCCS and not lint */ 

#include "namespace.h"
#include <assert.h>
#include <wchar.h>
#include <wctype.h>

__weak_alias(wcscasecmp,_wcscasecmp)

int
wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
	int lc1  = 0;
	int lc2  = 0;
	int diff = 0;

	_DIAGASSERT(s1);
	_DIAGASSERT(s2);

	for (;;) {
		lc1 = towlower(*s1);
		lc2 = towlower(*s2);

		diff = lc1 - lc2;
		if (diff)
			return diff;

		if (!lc1)
			return 0;

		++s1;
		++s2;
	}
}
