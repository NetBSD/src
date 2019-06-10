/*	$NetBSD: fabs_ieee754.S,v 1.1.30.1 2019/06/10 22:05:17 christos Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: fabs_ieee754.S,v 1.1.30.1 2019/06/10 22:05:17 christos Exp $")
#endif

STRONG_ALIAS(fabsl, fabs)

ENTRY(fabs)
	fabs %f1,%f1
	blr
END(fabs)
