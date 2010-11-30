/*	$NetBSD: h_initfini_align.S,v 1.1 2010/11/30 18:38:53 joerg Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1 2010/11/30 18:38:53 joerg Exp $")

_ENTRY(check_stack_alignment)
	movl	%esp, %eax
	andl	$3, %eax
	jz 1f
	xorl	%eax, %eax
	ret
1:
	incl	%eax
	ret
