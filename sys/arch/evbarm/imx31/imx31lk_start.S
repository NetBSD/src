/*	$NetBSD: imx31lk_start.S,v 1.3.6.1 2011/06/06 09:05:25 jruoho Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"

RCSID("$NetBSD: imx31lk_start.S,v 1.3.6.1 2011/06/06 09:05:25 jruoho Exp $")

/*
 */

#define CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0  /* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */


#ifndef SDRAM_START
#define SDRAM_START 0x80000000
#endif

#define IMX31_DCACHE_SIZE 0x4000	/* 16KB L1 */

/*
 * L1 == "Level One" == "first-level"
 * L2 == "Level Two" == "second-level"
 */

	.text

	.global _C_LABEL(imx31lk_start)
_C_LABEL(imx31lk_start):
	/* Figure out where we want to jump to when the time comes */
	adr	r8, .Lstart
	ldr	r8, [r8]

	/*
	 * set up virtual address space mapping
	 * for initial bootstrap.
	 */
	mov     r2, #(L1_S_SIZE)		/* 1MB chunks */

	/*
	 * Firmware already mapped SDRAM VA == PA. at 0x800..
	 * now map SDRAM also at VA 0x800...
	 */
	mrc	p15, 0, r0, c2, c0, 0		/* L1 table addr into r0 */
	add	r0, r0, #(0x800 * 4)		/* offset to 0x80000000 */

	mov	r3, #SDRAM_START		/* map to 0x800.. */
	orr	r3, r3, #(L1_S_AP_KRW)		/* the usual perms & stuff */
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM_KERNEL)

	mov	r1, #0x80			/* 128 1MB entries */
1:
	/* and looplooploop */
	str	r3, [r0], #4
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Map an L1 section for each device to make this easy.
	 */
	/* UART1 */
	mrc	p15, 0, r0, c2, c0, 0		/* L1 table addr into r0 */
	add	r0, r0, #(0xfd0 * 4)		/* offset to 0xfd000000 */

	mov	r3,	#0x43000000	
	orr	r3, r3,	#0x00f00000	
	orr	r3, r3, #(L1_S_AP_KRW)
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM_KERNEL)
	str	r3, [r0], #4			/* note autoinc */

	/* etc, TBD... */

	/*
	 * Make domain control go full art.
	 */
	mov	r0, #0xffffffff
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Now let's clean the cache again to make sure everything
	 * is in place.
	 *
	 * XXX: should this take into account the XScale cache clean bug?
	 */
	mov	r3, #(IMX31_DCACHE_SIZE)
	subs	r3, r3, #32
1:
	mcr	p15, 0, r3, c7, c10, 2
	subs	r3, r3, #32
	bne	1b
	CPWAIT(r3)

	/* Drain write buffer */
	mcr	p15, 0, r6, c7, c10, 4

	/* Invalidate TLBs just to be sure */
	mcr     p15, 0, r0, c8, c7, 0

	/*
	 * You are standing at the gate to NetBSD. --More--
	 * Unspeakable cruelty and harm lurk down there. --More--
	 * Are you sure you want to enter?
	 */
	adr	r8, .Lstart
	ldr	r8, [r8]
	mov	pc, r8				/* So be it */

/* symbol to use for address calculation in the right VA */
.Lstart:
	.word	start


/*
 * Calculate size of kernel to copy.  Don't bother to copy bss,
 * although I guess the CPU could use the warmup exercise ...
 */
.Lcopy_size:
        .word _edata - _C_LABEL(imx31lk_start)

