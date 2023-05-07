/*	$NetBSD: h_initfini_align.S,v 1.1 2023/05/07 12:41:49 skrll Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1 2023/05/07 12:41:49 skrll Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

ENTRY_NP(check_stack_alignment)
        andi    a0, a0, 15
        seqz    a0, a0
        ret
END(check_stack_alignment)
