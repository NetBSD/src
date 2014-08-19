/*	$NetBSD: fabs_ieee754.S,v 1.1.8.2 2014/08/20 00:02:11 tls Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: fabs_ieee754.S,v 1.1.8.2 2014/08/20 00:02:11 tls Exp $")
#endif

ENTRY(fabs)
	fabs %f1,%f1
	blr
END(fabs)
