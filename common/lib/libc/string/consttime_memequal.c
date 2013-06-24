/* $NetBSD: consttime_memequal.c,v 1.1 2013/06/24 04:21:19 riastradh Exp $ */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <string.h>
#define consttime_memequal __consttime_memequal
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
	return res;
}
