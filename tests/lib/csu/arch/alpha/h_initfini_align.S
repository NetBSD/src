/*	$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:48 tls Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.8.2 2014/08/20 00:04:48 tls Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

LEAF_NOPROFILE(check_stack_alignment, 0)
	ldiq	v0, 0
	and	sp, 7, t0
	cmoveq	t0, 1, v0
	ret
END(check_stack_alignment)
