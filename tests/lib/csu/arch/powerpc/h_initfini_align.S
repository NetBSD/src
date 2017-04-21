/*	$NetBSD: h_initfini_align.S,v 1.1.16.1 2017/04/21 16:54:11 bouyer Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.16.1 2017/04/21 16:54:11 bouyer Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

_ENTRY(check_stack_alignment)
	li	%r3,0
	andi.	%r0,%r1,15
	bnelr	%cr0
	li	%r3,1
	blr
END(check_stack_alignment)
