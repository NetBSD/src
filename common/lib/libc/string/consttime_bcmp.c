/* $NetBSD: consttime_bcmp.c,v 1.1.4.2 2012/10/30 18:46:14 yamt Exp $ */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <string.h>
#define consttime_bcmp __consttime_bcmp
#else
#include <lib/libkern/libkern.h>
#endif

int
consttime_bcmp(const void *b1, const void *b2, size_t len)
{
	const char *c1 = b1, *c2 = b2;
	int res = 0;

	while (len --)
		res |= *c1++ ^ *c2++;
	return res;
}
