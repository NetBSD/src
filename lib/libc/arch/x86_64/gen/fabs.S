/*	$NetBSD: fabs.S,v 1.3.74.1 2014/08/10 06:51:49 tls Exp $	*/

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: fabs.S,v 1.3.74.1 2014/08/10 06:51:49 tls Exp $")
#endif

	.section .rodata
	.align 8
__signmask:
	.long 0xffffffff
	.long 0x7fffffff

ENTRY(fabs)
	movsd	__signmask(%rip),%xmm1
	andpd	%xmm1,%xmm0
	ret
END(fabs)
