/* $NetBSD: vectors.S,v 1.1.4.3 2017/12/03 11:35:43 jdolecek Exp $ */

#include <aarch64/asm.h>
#include "assym.h"

lr	.req	x30	/* link register */

#define BAD_SYNC	1
#define BAD_IRQ		2
#define BAD_FIQ		3
#define BAD_ERROR	4

	.macro	VECT_ENTRY, label
	.align	7
	b	\label
	.endm

	.macro	VECT_INVAL, el, cause, regsize = 64
	.align	7
	/* small enough to fit 32 instrunction slot */
	.if \el == 1
	sub	sp, sp, #TF_SIZE
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
	.if \el == 0
	mrs	x20, sp_el0
	.else
	mov	x20, sp
	.endif
	mrs	x21, elr_el1
	mrs	x22, spsr_el1
	str	x20, [sp, #TF_SP]
	str	x21, [sp, #TF_PC]
	str	x22, [sp, #TF_SPSR]
	mrs	x23, esr_el1
	mrs	x24, far_el1
	str	x23, [sp, #TF_ESR]
	str	x24, [sp, #TF_FAR]
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	mov	x1, #\cause
	b	trap
	/* !!! beware about remaining slot !!! */
	.endm

	.macro	exception_entry, el, regsize = 64
	/* !!! will grow beyond 32 instruction vector slot size !!! */
	.if \regsize == 32
	mov	w0, w0			/* fill 0 in upper half of x0 */
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
	.if \el == 0
	mov	x29, xzr		/* fp pointed to user-space */
	mrs	x20, sp_el0
	.else
	mov	x20, sp
	.endif
	mrs	x21, elr_el1
	mrs	x22, spsr_el1
	str	x20, [sp, #TF_SP]
	str	x21, [sp, #TF_PC]
	str	x22, [sp, #TF_SPSR]
	mrs	x23, esr_el1
	mrs	x24, far_el1
	str	x23, [sp, #TF_ESR]
	str	x24, [sp, #TF_FAR]
	.endm

	.pushsection ".vectors", "ax"
	.global _C_LABEL(el1_vectors)
	.p2align 11
_C_LABEL(el1_vectors):
/*
 * Exception taken from current Exception Level with SP_EL1.
 * (These shouldn't happen)
 */
	VECT_INVAL	1, BAD_SYNC		/* Synchronous EL1t */
	VECT_INVAL	1, BAD_IRQ		/* IRQ EL1t */
	VECT_INVAL	1, BAD_FIQ		/* FIQ EL1t */
	VECT_INVAL	1, BAD_ERROR		/* Error EL1t */
/*
 * Exception taken from current Exception Level with SP.
 * There are entries for exceptions caused in EL1 (kernel exceptions).
 */
	VECT_ENTRY	el1_sync		/* Synchronous EL1h */
	VECT_ENTRY	el1_irq			/* IRQ EL1h */
	VECT_INVAL	1, BAD_FIQ		/* FIQ EL1h */
	VECT_INVAL	1, BAD_ERROR		/* Error EL1h */
/*
 * Exception taken from lower Exception Level which is using AArch64
 * There are entries for exceptions caused in EL0 (native user exceptions).
 */
	VECT_ENTRY	el0_sync		/* Synchronous 64bit EL0 */
	VECT_ENTRY	el0_irq			/* IRQ 64bit EL0 */
	VECT_INVAL	0, BAD_FIQ		/* FIQ 64bit EL0 */
	VECT_INVAL	0, BAD_ERROR		/* Error 64bit EL0 */
/*
 * Exception taken from lower Exception Level which is using AArch32
 * There are entries for exceptions caused in EL0 (compat user exceptions).
 */
	VECT_ENTRY	el0_32sync		/* Synchronous 32bit EL0 */
	VECT_ENTRY	el0_32irq		/* IRQ 32bit EL0 */
	VECT_INVAL	0, BAD_FIQ, 32		/* FIQ 32bit EL0 */
	VECT_INVAL	0, BAD_ERROR, 32	/* Error 32bit EL0 */

_C_LABEL(el1_sync):
	sub	sp, sp, #TF_SIZE
	exception_entry 1
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	mov	x1, xzr
	b	trap
END(el1_sync)

_C_LABEL(el1_irq):
	sub	sp, sp, #TF_SIZE
	exception_entry 1
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	b	interrupt
END(el1_irq)

_C_LABEL(el0_sync):
	exception_entry 0
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	mov	x1, xzr
	b	trap
END(el0_sync)

_C_LABEL(el0_irq):
	exception_entry 0
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	b	interrupt
END(el0_irq)

_C_LABEL(el0_32sync):
	exception_entry 0, 32
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	mov	x1, xzr
	b	trap
END(el0_32sync)

_C_LABEL(el0_32irq):
	exception_entry 0, 32
	ldr	lr, =exception_trap_exit
	mov	x0, sp
	b	interrupt
END(el0_32irq)
