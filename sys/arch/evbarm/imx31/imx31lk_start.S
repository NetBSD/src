/*	$NetBSD: imx31lk_start.S,v 1.4.14.1 2014/08/20 00:02:54 tls Exp $	*/
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

RCSID("$NetBSD: imx31lk_start.S,v 1.4.14.1 2014/08/20 00:02:54 tls Exp $")

#ifndef SDRAM_START
#define SDRAM_START 0x80000000
#endif

/*
 * L1 == "Level One" == "first-level"
 * L2 == "Level Two" == "second-level"
 */

	.text

	.global _C_LABEL(imx31lk_start)
_C_LABEL(imx31lk_start):
	cpsid	if, #PSR_SVC32_MODE

	/*
	 * Firmware already mapped SDRAM VA == PA. at 0x800..
	 * now map SDRAM also at VA 0x800...
	 */
	mrc	p15, 0, r0, c2, c0, 0		/* L1 table addr into r0 */
#ifdef ARM_MMU_EXTENDED
	mcr	p15, 0, r0, c2, c0, 1		/* copy it to TTBR1 */
	mov	r3, #TTBCR_S_N_1
	mcr	p15, 0, r3, c2, c0, 2		/* set TTBCR to enable TTBR1 */
#endif

	mov	r1, #(KERNEL_BASE_EXT >> L1_S_SHIFT)
	add	r2, r1, #0x80			/* 128 1MB entries */
	ldr	r3, .Lsdram_pde
1:
	/* and looplooploop */
	str	r3, [r0, r1, lsl #2]
	add	r3, r3, #L1_S_SIZE
	add	r1, r1, #1
	cmp	r1, r2
	blt	1b

	/*
	 * Map an L1 section for each device to make this easy.
	 */
	/* UART1 */
	mov	r1, #0xfd0			/* offset to 0xfd000000 */

	ldr	r3, .Lio_pde
	str	r3, [r0, r1, lsl #2]

	/* etc, TBD... */

	/*
	 * Make domain control go full art.
	 */
	mov	r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL * 2))|DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Now let's clean the cache again to make sure everything is in place.
	 */
	bl	_C_LABEL(arm11x6_idcache_wbinv_all)

	/* Invalidate TLBs just to be sure */
	mov	r0, #0
	mcr     p15, 0, r0, c8, c7, 0

	/*
	 * You are standing at the gate to NetBSD. --More--
	 * Unspeakable cruelty and harm lurk down there. --More--
	 * Are you sure you want to enter?
	 */
#ifdef KERNEL_BASES_EQUAL
	b	start
#else
	adr	r0, .Lstart
	ldr	ip, [r0]
	bx	ip
#endif

/* symbol to use for address calculation in the right VA */
#ifndef KERNEL_BASES_EQUAL
.Lstart:
	.word	start
#endif

#if L1_S_DOM_KERNEL
#error kernel domain (L1_S_DOM_KERNEL) is not 0
#endif

.Lsdram_pde:
	.word	0x80000000|L1_S_AP_KRW|L1_S_C|L1_S_B|L1_TYPE_S
.Lio_pde:
	.word	0x43f00000|L1_S_AP_KRW|L1_TYPE_S
END(imx31lk_start)
