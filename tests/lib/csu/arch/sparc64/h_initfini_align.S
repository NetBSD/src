/*	$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $")

_ENTRY(check_stack_alignment)
	add	%sp, BIAS, %o1
	and	%o1, 15, %o2
	retl
	 not	%o2, %o0
