/*	$NetBSD: h_initfini_align.S,v 1.1 2010/12/03 20:51:21 joerg Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1 2010/12/03 20:51:21 joerg Exp $")

_ENTRY(check_stack_alignment)
	movl	%esp, %eax
	andl	$15, %eax
	subl	$8, %eax
	jz 1f
	xorl	%eax, %eax
	ret
1:
	incl	%eax
	ret
