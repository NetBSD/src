/*	$NetBSD: h_initfini_align.S,v 1.2 2017/02/27 20:22:32 chs Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.2 2017/02/27 20:22:32 chs Exp $")

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
