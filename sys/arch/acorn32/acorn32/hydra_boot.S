/*	$NetBSD: hydra_boot.S,v 1.2 2002/10/05 23:30:03 bjh21 Exp $	*/

/*-
 * Copyright (c) 2002 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hydra_boot.S - Code to run on a Hydra slave CPU when it comes out of reset.
 */

#include "assym.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arch/acorn32/acorn32/hydrareg.h>
#include <arch/acorn32/acorn32/hydravar.h>

RCSID("$NetBSD: hydra_boot.S,v 1.2 2002/10/05 23:30:03 bjh21 Exp $")

ENTRY_NP(hydra_probecode)
	/*
	 * This code is mapped in at physical address zero when a CPU
	 * is probed.  It just halts the CPU as quickly as possible.
	 */
	mov	r0, #HYDRA_PHYS_BASE
	ldr	r1, [r0, #(HYDRA_ID_STATUS << 2)]
	and	r1, r1, #3		/* Mask off slave ID */
	mov	r2, #1
	mov	r2, r2, lsl r1		/* Get the bit for this CPU */
	str	r2, [r0, #(HYDRA_HALT_SET << 2)] /* Halt ourselves */
	b	.			/* Get here if we're released */

	.align	0
        .global _C_LABEL(hydra_eprobecode)
_C_LABEL(hydra_eprobecode):

	/* Code called when spinning up a slave CPU for real. */
ENTRY_NP(hydra_hatchcode)
	/*
	 * r0-r3: scratch
	 * r4: &hydraboot_vars
	 * r5: control reg
	 */

	mov	r4, #HYDRABOOT_VARS

	/* Enable 32-bit program and data space. */
	mov	r5, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr	p15, 0, r5, c1, c0, 0

	/* Switch to SVC32 mode. */
	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #PSR_SVC32_MODE
	msr	cpsr_c, r0

	/* Switch to using the real physical address for this page. */
	ldr	r0, [r4, #HB_BOOTPAGE_PA]
	add	r4, r4, r0
	add	pc, pc, r0
	mov	r0, r0
	mov	r0, r0

	/* Disable Hydra MMU for this processor. */
	mov	r0, #HYDRA_PHYS_BASE
	ldr	r1, [r0, #(HYDRA_ID_STATUS << 2)]
	and	r1, r1, #3		/* Mask off slave ID */
	mov	r2, #1
	mov	r2, r2, lsl r1		/* Get the bit for this CPU */
	str	r2, [r0, #(HYDRA_MMU_CLR << 2)] /* Disable MMU */

	/* Set TTB */
	ldr	r0, [r4, #HB_TTB]
	mcr	p15, 0, r0, c2, c0, 0
	/* Flush TLB */
	mov	r0, #0
	mcr	p15, 0, r0, c5, c0, 0
	/* Get start address */
	ldr	r0, [r4, #HB_ENTRY]
	/* Set up stack */
	ldr	sp, [r4, #HB_SP]
	mov	fp, #0
	mov	lr, #0
	/* Enable MMU */
	orr	r5, r5, #CPU_CONTROL_MMU_ENABLE
	orr	r5, r5, #CPU_CONTROL_SYST_ENABLE
	mcr	p15, 0, r5, c1, c0, 0
	/* Run away before the pipeline runs out */
	mov	pc, r0
	b	.
	.align	0
	.global	_C_LABEL(hydra_ehatchcode)
_C_LABEL(hydra_ehatchcode):
