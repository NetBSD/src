/* $NetBSD: consttime_memequal.c,v 1.1.2.1 2017/08/15 04:53:01 snj Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include "netbsd_ssh_compat.h"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include "namespace.h"
#include <string.h>
#if 0 /* XXXMRG netbsd-6 ssh */
#ifdef __weak_alias
__weak_alias(consttime_memequal,_consttime_memequal)
#endif
#endif
#else
#include <lib/libkern/libkern.h>
#endif

int
consttime_memequal(const void *b1, const void *b2, size_t len)
{
	const unsigned char *c1 = b1, *c2 = b2;
	unsigned int res = 0;

	while (len--)
		res |= *c1++ ^ *c2++;

	/*
	 * Map 0 to 1 and [1, 256) to 0 using only constant-time
	 * arithmetic.
	 *
	 * This is not simply `!res' because although many CPUs support
	 * branchless conditional moves and many compilers will take
	 * advantage of them, certain compilers generate branches on
	 * certain CPUs for `!res'.
	 */
	return (1 & ((res - 1) >> 8));
}
