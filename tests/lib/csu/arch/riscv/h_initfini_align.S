/*	$NetBSD: h_initfini_align.S,v 1.2 2023/08/24 05:51:55 rin Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.2 2023/08/24 05:51:55 rin Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

ENTRY_NP(check_stack_alignment)
        andi    a0, sp, 15
        seqz    a0, a0
        ret
END(check_stack_alignment)
