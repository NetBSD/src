/* $NetBSD: pxa2x0_irq.S,v 1.1.2.2 2002/11/11 21:56:59 nathanw Exp $ */

/*
 * This code is derived from i80200_irq.S by Jason R. Thorpe.
 */
/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"

#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>

/*
 * irq_entry:
 *	Main entry point for the IRQ vector.
 */

	.text
	.align	0
Lastpending:
	.word	_C_LABEL(astpending)

ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */

	mov	r0, sp
	bl	_C_LABEL(pxa2x0_irq_handler)
1:	

	/*
	 * If we're returning to user mode, check for pending ASTs.
	 */
	ldr	r0, [sp]		/* Get the SPSR from stack */
	and	r0, r0, #(PSR_MODE)	/* Test for USR32 mode before the IRQ */
	teq	r0, #(PSR_USR32_MODE)
	bne	Lirqout			/* Nope, get out now */

Lastloop:	
	ldr	r0, Lastpending		/* Do we have an AST pending? */
	ldr	r1, [r0]
	teq	r1, #0x00000000
	beq	Lirqout			/* Nope, get out now */

	mov	r1, #0x00000000
	str	r1, [r0]		/* Clear astpending */

	mrs	r4, cpsr_all		/* save CPSR */
	bic	r0, r4, #(I32_bit)	/* Enable IRQs */
	msr	cpsr_all, r0

	mov	r0, sp
	bl	_C_LABEL(ast)		/* ast(frame) */

	msr	cpsr_all, r4		/* Disable IRQs */
	b	Lastloop		/* Check for more ASTs */

Lirqout:
	PULLFRAMEFROMSVCANDEXIT
	movs	pc, lr			/* Exit */

	.bss
	.align	0

	.global _C_LABEL(astpending)
_C_LABEL(astpending):
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
