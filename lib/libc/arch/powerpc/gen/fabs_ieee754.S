/*	$NetBSD: fabs_ieee754.S,v 1.1.4.2 2014/05/22 11:36:48 yamt Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: fabs_ieee754.S,v 1.1.4.2 2014/05/22 11:36:48 yamt Exp $")
#endif

ENTRY(fabs)
	fabs %f1,%f1
	blr
END(fabs)
