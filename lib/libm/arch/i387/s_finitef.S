/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_finitef.S,v 1.5 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(finitef)
#ifdef __i386__
	movl	4(%esp),%eax
	andl	$0x7f800000, %eax
	cmpl	$0x7f800000, %eax
	setne	%al
	andl	$0x000000ff, %eax
#else
	xorl	%eax,%eax
	movl	$0x7ff00000,%esi
	movss	%xmm0,-4(%rsp)
	andl	-4(%rsp),%esi
	cmpl	$0x7ff00000,%esi
	setne	%al
#endif
	ret
