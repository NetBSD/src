/*	$NetBSD: footbridge_irq.S,v 1.2.6.3 2002/11/11 21:56:42 nathanw Exp $	*/

/*
 * Copyright (c) 1998 Mark Brinicombe.
 * Copyright (c) 1998 Causality Limited
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
 */

#include "opt_irqstats.h"
#include "assym.h"

#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/dc21285reg.h>

/*
 * irq_entry:
 *
 *	Main entry point for the IRQ vector on dc21285 CPUs.  Calls
 *	external interrupt dispatch routine.
 */

	.text
	.align	0

.Lcurrent_intr_depth: 
	.word	_C_LABEL(current_intr_depth)
	.word	_C_LABEL(prev_intr_depth)

.Lastpending:
	.word	_C_LABEL(astpending)

ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */

	/*
	 * Note that we have entered the IRQ handler.
	 * We are in SVC mode so we cannot use the processor mode
	 * to determine if we are in an IRQ. Instead we will count the
	 * each time the interrupt handler is nested.
	 */
	ldr	r0, .Lcurrent_intr_depth
	ldr	r2, .Lcurrent_intr_depth+4
	ldr	r1, [r0]
	str	r1, [r2]
	add	r1, r1, #1
	str	r1, [r0]
	
	/* branch off into the external handler */
	mov	r0, sp
	bl	_C_LABEL(footbridge_intr_dispatch)
 	 
	/* Decremement the nest count. */
	ldr	r0, .Lcurrent_intr_depth 
	ldr	r2, .Lcurrent_intr_depth+4
	ldr	r1, [r0]
	str	r1, [r2]
	sub	r1, r1, #1
	str	r1, [r0]

	/*
	 * If we're returning to user mode, check for pending ASTs.
	 */
	ldr	r0, [sp]		/* Get the SPSR from stack */
	and	r0, r0, #(PSR_MODE)	/* Test for USR32 mode before the IRQ */
	teq	r0, #(PSR_USR32_MODE)
	bne	.Lirqout		/* Nope, get out now */

.Lastloop:
	ldr	r0, .Lastpending	/* Do we have an AST pending? */
	ldr	r1, [r0]
	teq	r1, #0x00000000
	beq	.Lirqout		/* Nope, get out now */

	mov	r1, #0x00000000
	str	r1, [r0]		/* Clear astpending */

	mrs	r4, cpsr		/* save CPSR */
	bic	r0, r4, #(I32_bit)	/* Enable IRQs */
	msr	cpsr_c, r0

	mov	r0, sp
	bl	_C_LABEL(ast)		/* ast(frame) */

	msr	cpsr_c, r4		/* Disable IRQs */
	b	.Lastloop		/* Check for more ASTs */

.Lirqout:
	PULLFRAMEFROMSVCANDEXIT
	movs	pc, lr			/* Exit */

	.bss
	.align	0

	.global _C_LABEL(astpending)
_C_LABEL(astpending):
	.word	0

	.global	_C_LABEL(current_intr_depth)
_C_LABEL(current_intr_depth):
	.word	0

	.global	_C_LABEL(prev_intr_depth)
_C_LABEL(prev_intr_depth):
	.word	0

	/*
	 * XXX Provide intrnames/intrcnt for legacy code, but
	 * don't actually use them.
	 */

	.global _C_LABEL(intrnames), _C_LABEL(eintrnames)
	.global _C_LABEL(intrcnt), _C_LABEL(eintrcnt)
_C_LABEL(intrnames): 
_C_LABEL(eintrnames):

	.global _C_LABEL(intrcnt), _C_LABEL(sintrcnt), _C_LABEL(eintrcnt)
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):

/* End of footbridge_irq.S */
