/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbn.S,v 1.9.12.1 2013/02/25 00:27:57 tls Exp $")

#ifdef WEAK_ALIAS
WEAK_ALIAS(scalbn,_scalbln)
WEAK_ALIAS(scalbln,_scalbln)
#endif
#ifdef __x86_64__
ENTRY(_scalbn)
	movl %edi,-12(%rsp)
	fildl -12(%rsp)
	movsd %xmm0,-8(%rsp)
	fldl -8(%rsp)
	fscale
	fstpl -8(%rsp)
	movsd -8(%rsp),%xmm0
	fstp %st(0)
	ret
#elif defined(STRONG_ALIAS)
STRONG_ALIAS(_scalbn, scalbln)
#endif

ENTRY(_scalbln)
#ifdef __x86_64__
	movq %rdx,-16(%rsp)
	fildq -16(%rsp)
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
