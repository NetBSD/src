/*	$NetBSD: fabs.S,v 1.2 2004/02/26 19:19:06 drochner Exp $	*/

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: fabs.S,v 1.2 2004/02/26 19:19:06 drochner Exp $")
#endif

/*
 * Ok, this sucks. Is there really no way to push an xmm register onto
 * the FP stack directly?
 */

ENTRY(fabs)
	movsd	%xmm0, -8(%rsp)
	fldl	-8(%rsp)
	fabs
	fstpl	-8(%rsp)
	movsd	-8(%rsp),%xmm0
	ret
