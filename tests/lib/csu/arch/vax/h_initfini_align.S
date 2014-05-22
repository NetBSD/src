/*	$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

_ENTRY(check_stack_alignment, 0)
	clrl	%r0
	bitl	$3,%sp
	bneq	1f
	incl	%r0
1:	ret
END(check_stack_alignment)
