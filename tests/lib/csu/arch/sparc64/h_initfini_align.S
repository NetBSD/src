/*	$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:49 tls Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:49 tls Exp $")

_ENTRY(check_stack_alignment)
	add	%sp, BIAS, %o1
	and	%o1, 15, %o2
	retl
	 not	%o2, %o0
