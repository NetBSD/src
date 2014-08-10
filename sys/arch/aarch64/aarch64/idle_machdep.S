/* $NetBSD: idle_machdep.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <aarch64/asm.h>
#include "assym.h"

RCSID("$NetBSD: idle_machdep.S,v 1.1 2014/08/10 05:47:37 matt Exp $");

ENTRY(cpu_idle)
	sub	sp, sp, #TF_SIZE		// allocate trapfame
	str	x28, [sp, #TF_X28]
	str	x30, [sp, #TF_X30]
	msr	daifset, #DAIF_I|DAIF_F		// disable interrupts
	wfi

	mrs	x1, tpidr_el1			// get curcpu()
	ldr	w28, [x1, #CI_INTR_DEPTH]
	add	w2, w28, #1
	str	w2, [x1, #CI_INTR_DEPTH]
	ldr	x3, [x1, #CI_CC_NINTR]
	add	x3, x3, #1
	str	x3, [x1, #CI_CC_NINTR]

	mov	x0, sp				// get pointer to trapframe
	mov	x4, #SPSR_M_EL1T		// what our spsr should be
	str	x2, [sp, #TF_SPSR]

	bl	armgic_irq_handler		// process irq

	mrs	x1, tpidr_el1			// get curcpu()
	str	w28, [x1, #CI_INTR_DEPTH]	// restore intr depth

	ldr	x28, [sp, #TF_X28]		// restore registers
	ldr	x30, [sp, #TF_X30]		// restore registers
	add	sp, sp, #TF_SIZE		// pop trapframe

	msr	daifclr, #DAIF_I|DAIF_F		// enable interrupts

	ldr	w3, [x1, #CI_SOFTINTS]		// Get pending softint mask
#if 0
	// CPL should be 0
	ldr	w2, [x1, #CI_CPL]		// Get current priority level
	lsr	w3, w3, w2			// shift mask by cpl
#endif
	cbnz	w3, _C_LABEL(dosoftints)	// tailcall dosoftints(void)

	ret
END(cpu_idle)
