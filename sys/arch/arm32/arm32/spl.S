/*	$NetBSD: spl.S,v 1.4 1998/03/28 00:14:51 mark Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * splasm.S
 *
 * spl routines
 *
 * Created      : 01/03/96
 */

#include "assym.h"
#include <machine/psl.h>
#include <machine/asm.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text
	.align	0

Lcurrent_spl_level:
	.word	_current_spl_level

Lspl_masks:
	.word	_spl_masks

Lspl_mask:
	.word	_spl_mask

ENTRY(raisespl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	movle	pc, lr

	str	r3, [r1]		/* Store the new spl mask */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r2, [r2, r3, lsl #2]

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r2, [r1]

	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_irq_setmasks		/* Update the actual masks */
	ldmfd	sp!, {r0, pc}		/* Exit */


ENTRY(lowerspl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	movge	pc, lr

	str	r3, [r1]		/* Store the new spl mask */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r2, [r2, r3, lsl #2]

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r2, [r1]

	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_irq_setmasks		/* Update the actual masks */
	ldmfd	sp!, {r0, pc}		/* Exit */


/*
 * For the sake of efficiency we will try and avoid making the dosoftints()
 * call unless necessary.
 * I am sure there are better optimisations ...
 */

Lsoft_interrupts:
	.word	_soft_interrupts

ENTRY(splx)
	mov	r3, r0			/* Save the new value */
	ldr	r1, Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	moveq	pc, lr

	str	r3, [r1]		/* Store the new spl mask */

	stmfd	sp!, {r0, r4, lr}	/* Preserve registers */

	ldr	r2, Lspl_masks		/* Get the spl mask */
	ldr	r4, [r2, r3, lsl #2]	/* Use r4 so available later */

	ldr	r1, Lspl_mask		/* Store in the current spl mask */
	str	r4, [r1]

	bl	_irq_setmasks		/* Update the actual masks */
	ldr	r0, Lsoft_interrupts	/* Get the pending soft ints */
	ldr	r0, [r0]
	ands	r0, r0, r4		/* any soft ints to deliver ? */
	blne	_dosoftints		/* Process any pending soft ints */
	ldmfd	sp!, {r0, r4, pc}	/* Exit */
