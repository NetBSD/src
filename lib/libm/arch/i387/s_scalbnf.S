/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbnf.S,v 1.9 2013/02/11 01:19:33 matt Exp $")

#ifdef WEAK_ALIAS
WEAK_ALIAS(scalbnf,_scalbnf)
WEAK_ALIAS(scalblnf,_scalblnf)
#endif

#ifdef __x86_64__
ENTRY(_scalbnf)
	movl %edi,-8(%rsp)
	fildl -8(%rsp)
	movss %xmm0,-4(%rsp)
	flds -4(%rsp)
	fscale
	fstps -4(%rsp)
	movss -4(%rsp),%xmm0
	fstp %st(0)
	ret
#elif defined(STRONG_ALIAS)
STRONG_ALIAS(_scalbnf,_scalbnf)
#endif

ENTRY(_scalblnf)
#ifdef __x86_64__
	movq %rdx,-16(%rsp)
	fildq -16(%rsp)
	movss %xmm0,-4(%rsp)
	flds -4(%rsp)
	fscale
	fstps -4(%rsp)
	movss -4(%rsp),%xmm0
	fstp %st(0)
#else
	fildl 8(%esp)
	flds 4(%esp)
	fscale
	fstp %st(1)		/* clean up stack */
#endif
	ret
