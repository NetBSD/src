/*	$NetBSD: srtbegin.S,v 1.6 2003/07/22 04:57:24 briggs Exp $	*/

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

#include <machine/asm.h>
#include <arm/armreg.h>

#define	STACKSIZE	1024

ENTRY(start)
	/*
	 * We assume we've been loaded VA==PA, or that the MMU
	 * is disabled.  Make sure the MMU is disabled so that
	 * we don't have to care about the caches.
	 */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	/*
	 * Check to see if __text_store == &start.  If not, we're most
	 * likely running from flash.  Flash is slow, and we'd
	 * really like to run the code from RAM.  Copy it out.
	 */
	adr	r1, Ltext
	ldmia	r1, {r1-r3}
	cmp	r1, r2			/* __text_store == &start? */
	beq	relocated		/* yes, in RAM */

	/* Copy text segment from ROM to RAM */
1:	ldrb	r0, [r1], #0x01
	strb	r0, [r2], #0x01
	cmp	r2, r3			/* copy done? */
	bne	1b

	/*
	 * Okay, we are finished relocating the text segment.  Now
	 * we need to leap to the next instruction.
	 */
	adr	r1, Lrelocated
	ldr	pc, [r1]

relocated:
	/*
	 * Check to see if __data_store == __data_start.  If not,
	 * we're most likely built for running from flash,
	 * and must copy the data segment out to RAM.
	 */
	adr	r1, Ldata
	ldmia	r1, {r1-r3}
	cmp	r1, r2			/* __data_store == __data_start? */
	beq	2f			/* yes, in RAM */

	cmp	r2, r3			/* __data_store == _edata? */
	beq	2f			/* yes, skip copy */

	/* Copy data segment from ROM to RAM */
1:	ldrb	r0, [r1], #0x01
	strb	r0, [r2], #0x01
	cmp	r2, r3			/* copy done? */
	bne	1b

2:
	/* Clear the BSS. */
	adr	r1, Lbss
	ldmia	r1, {r1, r2}
	sub	r2, r2, r1
	mov	r3, #0

1:	strb	r3, [r1], #0x01
	subs	r2, r2, #0x01
	bgt	1b

	/* Set the stack pointer */
	adr	r1, Lstack
	ldr	r1, [r1]
	add	sp, r1, #STACKSIZE

	b	_C_LABEL(main)

Lrelocated:
	.word	relocated

Ltext:
	.word	_C_LABEL(__text_store)
	.word	_C_LABEL(start)
	.word	_C_LABEL(_etext)

Ldata:
	.word	_C_LABEL(__data_store)
	.word	_C_LABEL(__data_start)

Lbss:
	.word	_C_LABEL(_edata)
	.word	_C_LABEL(_end)

Lstack:
	.word	Lstackspace

	.comm	Lstackspace, STACKSIZE
