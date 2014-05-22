/*	$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:19 yamt Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: h_initfini_align.S,v 1.1.4.2 2014/05/22 11:42:19 yamt Exp $")

/*
 * LINTSTUB: bool check_stack_alignment(void);
 */

LEAF_NOPROFILE(check_stack_alignment)
#ifdef __mips_o32
	andi	v1,sp,3
#else
	andi	v1,sp,7
#endif
	sltiu	v0,v1,1
	jr	ra
END(check_stack_alignment)
