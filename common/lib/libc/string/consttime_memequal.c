/* $NetBSD: consttime_memequal.c,v 1.3 2013/08/28 17:47:07 riastradh Exp $ */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include "namespace.h"
#include <string.h>
#ifdef __weak_alias
__weak_alias(consttime_memequal,_consttime_memequal)
#endif
#else
#include <lib/libkern/libkern.h>
#endif

int
consttime_memequal(const void *b1, const void *b2, size_t len)
{
	const char *c1 = b1, *c2 = b2;
	int res = 0;

	while (len --)
		res |= *c1++ ^ *c2++;
	return !res;
}
