/*	$NetBSD: spl.S,v 1.5.2.2 2008/01/09 01:45:13 matt Exp $	*/

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
#include <arm/arm32/psl.h>
#include <machine/asm.h>
#include <machine/cpu.h>

	.text
	.align	0

.Lcpu_info_store:
	.word	_C_LABEL(cpu_info_store)

ENTRY(raisespl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, .Lcpu_info_store	/* Get the current spl level */
	ldr	r0, [r1, #CI_CPL]
	cmp	r3, r0
	RETc(le)

	stmfd	sp!, {r0, r1, r4, lr}	/* Preserve registers */

	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r2, r4, #(I32_bit)
	msr	cpsr_all, r2

	str	r3, [r1, #CI_CPL]	/* Store the new spl level */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	msr	cpsr_all, r4		/* Restore interrupts */

	ldmfd	sp!, {r0, r1, r4, pc}	/* Restore registers */

ENTRY(lowerspl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, .Lcpu_info_store	/* Get the current spl level */
	ldr	r0, [r1, #CI_CPL]
	cmp	r3, r0
	RETc(ge)

	stmfd	sp!, {r0, r1, r4, lr}	/* Preserve registers */

	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r2, r4,  #(I32_bit)
	msr	cpsr_all, r2

	str	r3, [r1, #CI_CPL]	/* Store the new spl level */

	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	msr	cpsr_all, r4
#ifdef __HAVE_FAST_SOFTINTS
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
#endif
	ldmfd	sp!, {r0, r1, r4, pc}	/* restore registers */

ENTRY(splx)
	mov	r3, r0			/* Save the new value */
	ldr	r1, .Lcpu_info_store	/* Get the current spl level */
	ldr	r0, [r1, #CI_CPL]
	cmp	r3, r0
	RETc(eq)

	stmfd	sp!, {r0, r1, r4, lr}
	/* Disable interrupts */
	mrs	r4, cpsr_all
	orr	r2, r4,  #(I32_bit)
	msr	cpsr_all, r2

	str	r3, [r1, #CI_CPL]	/* Store the new spl level */

	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	msr	cpsr_all, r4
#ifdef __HAVE_FAST_SOFTINTS
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
#endif
	ldmfd	sp!, {r0, r1, r4, pc}
