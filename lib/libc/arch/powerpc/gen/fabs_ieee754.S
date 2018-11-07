/*	$NetBSD: fabs_ieee754.S,v 1.2 2018/11/07 22:21:42 riastradh Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: fabs_ieee754.S,v 1.2 2018/11/07 22:21:42 riastradh Exp $")
#endif

STRONG_ALIAS(fabsl, fabs)

ENTRY(fabs)
	fabs %f1,%f1
	blr
END(fabs)
