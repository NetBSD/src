/*	$NetBSD: spl.S,v 1.3 2001/04/17 16:07:13 toshii Exp $	*/

/*
 * Copyright (c) 1996-1998 Mark Brinicombe.
 * Copyright (c) Brini.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * spl routines
 *
 * Created      : 01/03/96
 */

#include "assym.h"
#include <machine/psl.h>
#include <machine/asm.h>
#include <machine/cpu.h>

	.text
	.align	0

Lcurrent_spl_level:
	.word	_C_LABEL(current_spl_level)

Lspl_masks:
	.word	_C_LABEL(spl_masks)

Lspl_mask:
	.word	_C_LABEL(spl_mask)

ENTRY(raisespl)
	stmfd	sp!, {r4}
	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r1, r4,  #(I32_bit)
	msr	cpsr_all, r1

	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	ble	raisespl_exit

	str	r3, [r1]		/* Store the new spl level */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r2, [r2, r3, lsl #2]

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r2, [r1]

	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	ldmfd	sp!, {r0, lr}
raisespl_exit:
	msr	cpsr_all, r4
	ldmfd	sp!, {r4}
	mov	pc, lr

ENTRY(lowerspl)
	stmfd	sp!, {r4}
	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r1, r4,  #(I32_bit)
	msr	cpsr_all, r1

	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	bge	lowerspl_exit

	str	r3, [r1]		/* Store the new spl level */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r2, [r2, r3, lsl #2]

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r2, [r1]

	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	msr	cpsr_all, r4
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
	ldmfd	sp!, {r0, lr}
	ldmfd	sp!, {r4}
	mov	pc, lr
lowerspl_exit:
	msr	cpsr_all, r4
	ldmfd	sp!, {r4}
	mov	pc, lr

ENTRY(splx)
	stmfd	sp!, {r4}
	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r1, r4,  #(I32_bit)
	msr	cpsr_all, r1

	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	beq	splx_exit

	str	r3, [r1]		/* Store the new spl level */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r2, [r2, r3, lsl #2]	/* Use r4 so available later */

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r2, [r1]

	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	msr	cpsr_all, r4
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
	ldmfd	sp!, {r0, lr}
	ldmfd	sp!, {r4}
	mov	pc, lr
splx_exit:
	msr	cpsr_all, r4
	ldmfd	sp!, {r4}
	mov	pc, lr
