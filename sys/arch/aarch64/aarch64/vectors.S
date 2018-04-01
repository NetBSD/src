/*	$NetBSD: vectors.S,v 1.5 2018/04/01 04:35:03 ryo Exp $	*/

#include <aarch64/asm.h>
#include "assym.h"

	/*
	 * vector_entry macro must be small enough to fit 0x80 bytes!
	 */
	.macro	vector_entry, el, label
	.align 7	/* aligned 0x80 */

	.if \el == 1
	sub	sp, sp, #TF_SIZE	/* need to allocate stack on el1 */
	.endif

	stp	x0, x1, [sp, #TF_X0]
	stp	x2, x3, [sp, #TF_X2]
	stp	x4, x5, [sp, #TF_X4]
	stp	x6, x7, [sp, #TF_X6]
	stp	x8, x9, [sp, #TF_X8]
	stp	x10, x11, [sp, #TF_X10]
	stp	x12, x13, [sp, #TF_X12]
	stp	x14, x15, [sp, #TF_X14]
	stp	x16, x17, [sp, #TF_X16]
	str	x18, [sp, #TF_X18]
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x30, [sp, #TF_X29]

	/* get sp and elr */
	.if \el == 0
	mrs	x20, sp_el0
	.else
	add	x20, sp, #TF_SIZE	/* sp is already adjusted */
	.endif
	mrs	x21, elr_el1

	/* store sp and elr */
	.if TF_SP + 8 == TF_PC
	stp	x20, x21, [sp, #TF_SP]
	.else
	str	x20, [sp, #TF_SP]
	str	x21, [sp, #TF_PC]
	.endif

	mrs	x22, spsr_el1
	str	x22, [sp, #TF_SPSR]

	mrs	x23, esr_el1
	mrs	x24, far_el1

	.if TF_ESR + 8 == TF_FAR
	stp	x23, x24, [sp, #TF_ESR]
	.else
	str	x23, [sp, #TF_ESR]
	str	x24, [sp, #TF_FAR]
	.endif

	.if \el == 0
	/* curlwp->l_private = tpidr_el0 */
	mrs	x1, tpidr_el1		/* curcpu() */
	ldr	x1, [x1, #CI_CURLWP]	/* x0 = curcpu()->ci_curlwp */
	mrs	x0, tpidr_el0
	str	x0, [x1, #L_PRIVATE]	/* curlwp->l_private = tpidr_el0 */
	.endif

	adr	lr, el\el\()_trap_exit	/* el[01]_trap_exit */
	mov	x0, sp
	b	\label
	.endm


	.align 11	/* vector table must be aligned 2048 */
ENTRY(el1_vectors)
/*
 * Exception taken from current Exception Level with SP_EL0.
 * (These shouldn't happen)
 */
	vector_entry	1, trap_el1t_sync
	vector_entry	1, trap_el1t_irq
	vector_entry	1, trap_el1t_fiq
	vector_entry	1, trap_el1t_error

/*
 * Exception taken from current Exception Level with SP_EL1.
 * There are entries for exceptions caused in EL1 (kernel exceptions).
 */
	vector_entry	1, trap_el1h_sync
	vector_entry	1, interrupt
	vector_entry	1, trap_el1h_fiq
	vector_entry	1, trap_el1h_error

/*
 * Exception taken from lower Exception Level which is using AArch64
 * There are entries for exceptions caused in EL0 (native user exceptions).
 */
	vector_entry	0, trap_el0_sync
	vector_entry	0, interrupt
	vector_entry	0, trap_el0_fiq
	vector_entry	0, trap_el0_error

/*
 * Exception taken from lower Exception Level which is using AArch32
 * There are entries for exceptions caused in EL0 (compat user exceptions).
 */
	vector_entry	0, trap_el0_32sync
	vector_entry	0, interrupt
	vector_entry	0, trap_el0_32fiq
	vector_entry	0, trap_el0_32error
END(el1_vectors)
