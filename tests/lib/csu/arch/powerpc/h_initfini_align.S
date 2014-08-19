/*	$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:49 tls Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:49 tls Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

_ENTRY(check_stack_alignment)
	li	%r3,0
	andis.	%r0,%r1,15
	bnelr	%cr0
	li	%r3,1
	blr
END(check_stack_alignment)
