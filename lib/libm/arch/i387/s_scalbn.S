/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbn.S,v 1.7.4.1 2006/04/21 21:45:56 tron Exp $")

ENTRY(scalbn)
#ifdef __x86_64__
	movl %edi,-12(%rsp)
	fildl -12(%rsp)
	movsd %xmm0,-8(%rsp)
	fldl -8(%rsp)
	fscale
	fstpl -8(%rsp)
	movsd -8(%rsp),%xmm0
	fstp %st(0)
#else
	fildl 12(%esp)
	fldl 4(%esp)
	fscale
	fstp %st(1)		/* clean up stack */
#endif
	ret
