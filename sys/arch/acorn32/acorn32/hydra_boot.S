/*	$NetBSD: hydra_boot.S,v 1.3.2.2 2002/10/10 18:30:19 jdolecek Exp $	*/

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

#include <machine/asm.h>
#include <arch/acorn32/acorn32/hydrareg.h>

RCSID("$NetBSD: hydra_boot.S,v 1.3.2.2 2002/10/10 18:30:19 jdolecek Exp $")

ENTRY_NP(hydra_bootcode)
	/*
	 * This code is mapped in at physical address zero when a CPU
	 * hatches, so it forms the vector table until the Hydra MMU
	 * is turned off and the CPU MMU is turned on.  Any exception
	 * apart from a reset just puts us in a tight loop.
	 */
	b	Lhydra_reset		/* Reset */
	b	. 			/* Undefined instruction */
	b	.			/* SWI */
	b	.			/* Prefetch abort */
	b	.			/* Data abort */
	b	.			/* Address exception */
	b	.			/* IRQ */
	b	.			/* FIQ */

Lhydra_reset:
	mov	r0, #HYDRA_PHYS_BASE
	ldr	r1, [r0, #(HYDRA_ID_STATUS << 2)]
	and	r1, r1, #3		/* Mask off slave ID */
	mov	r2, #1
	mov	r2, r2, lsl r1		/* Get the bit for this CPU */
	str	r2, [r0, #(HYDRA_HALT_SET << 2)] /* Halt ourselves */
	b	.			/* Get here if we're released */

	.align	0
        .global _C_LABEL(hydra_ebootcode)
_C_LABEL(hydra_ebootcode):
