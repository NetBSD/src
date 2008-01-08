/*	$NetBSD: i80200_irq.S,v 1.14.34.1 2008/01/08 22:09:33 bouyer Exp $	*/

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
#include "opt_perfctrs.h"

#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arm/xscale/i80200reg.h>

/*
 * irq_entry:
 *
 *	Main entry point for the IRQ vector on i80200 CPUs.  Calls
 *	board-specific external interrupt dispatch routine.
 */

	.text
	.align	0

.Lcurrent_intr_depth: 
	.word	_C_LABEL(cpu_info_store) + CI_IDEPTH

.Lintr_dispatch:
	.word	_C_LABEL(i80200_extirq_dispatch)

#if defined(PERFCTRS)
.Lpmc_dispatch:
	.word	_C_LABEL(xscale_pmc_dispatch)
#endif

LOCK_CAS_CHECK_LOCALS

AST_ALIGNMENT_FAULT_LOCALS

ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */
	ENABLE_ALIGNMENT_FAULTS

	/*
	 * Note that we have entered the IRQ handler.  We are
	 * in SVC mode so we cannot use the processor mode to
	 * determine if we are in an IRQ.  Instead, we will
	 * count each time the interrupt handler is nested.
	 */
	ldr	r0, .Lcurrent_intr_depth
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]

	/*
	 * Get the interrupt status into a callee-save register.
	 */
	mrc	p13, 0, r4, c4, c0, 0

#if defined(PERFCTRS)
	/*
	 * Check for PMU interrupts.
	 * If we have one, call the routine to handle it.
	 */
	tst	r4, #(INTSRC_PI)
	beq	.Lpmc_intr_return
	mov	r1, r4
	mov	r0, sp
	mov	lr, pc
	ldr	pc, .Lpmc_dispatch
.Lpmc_intr_return:
#endif

	/*
	 * XXX - any need to handle BMU interrupts?
	 */

	/*
	 * Check for external IRQs.  If we have one, call the
	 * external IRQ dispatcher.  The argument is a pointer
	 * to the stack frame.  This function will be called with
	 * interrupts disabled, and will return with interrupts
	 * disabled.
	 */
	tst	r4, #(INTSRC_II)
	beq	.Lextirq_return		/* no external IRQ pending */
	ldr	r1, .Lintr_dispatch
	mov	r0, sp
	mov	lr, pc
	ldr	pc, [r1]
.Lextirq_return:

	/* Decremement the nest count. */
	ldr	r0, .Lcurrent_intr_depth 
	ldr	r1, [r0]
	sub	r1, r1, #1
	str	r1, [r0]

	LOCK_CAS_CHECK

	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAMEFROMSVCANDEXIT
	movs	pc, lr			/* Exit */

	.bss
	.align	0

	.global _C_LABEL(astpending)
_C_LABEL(astpending):
	.word	0
