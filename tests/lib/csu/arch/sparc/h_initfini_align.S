/*	$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:20 yamt Exp $")

_ENTRY(check_stack_alignment)
	and	%sp, 7, %o1
	retl
	 not	%o1, %o0
