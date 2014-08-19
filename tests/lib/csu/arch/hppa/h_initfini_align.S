/*	$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:48 tls Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:48 tls Exp $")

LEAF_ENTRY(check_stack_alignment)
	extru	%sp,31,6,%ret0
	comiclr,<> 0, %ret0, %ret0
	ldi	1,%ret0
	bv,n	%r0(%r2)
EXIT(check_stack_alignment)
