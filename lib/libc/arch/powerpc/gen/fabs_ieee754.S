/*	$NetBSD: fabs_ieee754.S,v 1.1.28.1 2018/11/26 01:52:11 pgoyette Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
__RCSID("$NetBSD: fabs_ieee754.S,v 1.1.28.1 2018/11/26 01:52:11 pgoyette Exp $")
#endif

STRONG_ALIAS(fabsl, fabs)

ENTRY(fabs)
	fabs %f1,%f1
	blr
END(fabs)
