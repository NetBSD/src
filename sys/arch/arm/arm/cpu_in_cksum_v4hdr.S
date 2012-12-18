/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

RCSID("$NetBSD: cpu_in_cksum_v4hdr.S,v 1.2 2012/12/18 14:08:25 matt Exp $")

ENTRY(cpu_in_cksum_v4hdr)
#ifdef _ARM_ARCH_DWORD_OK
	tst	r0, #4			/* 64-bit aligned? */
	ldreqd	r2, [r0], #8		/* load 1st/2nd words */
	ldrne	ip, [r0], #4		/* load 1st word */
	ldreq	ip, [r0, #8]		/* load 5th word */
	ldrned	r2, [r0, #8]		/* load 4th/5th words */
#else
	ldr	ip, [r0]		/* load 1st word */
	ldr	r3, [r0, #4]		/* load 2nd word */
	ldr	r2, [r0, #8]		/* load 3rd word */
#endif
	adds	r3, r3, ip		/* accumulate */
	adcs	r2, r2, r3		/* accumulate */
#ifdef _ARM_ARCH_DWORD_OK
	ldrd	r0, [r0]		/* load remaining words */
#else
	ldr	r1, [r0, #12]		/* load 4th word */
	ldr	r0, [r0, #16]		/* load 5th word */
#endif
	adcs	r1, r1, r2		/* accumulate */
	adcs	r0, r0, r1		/* accumulate */
	/*
	 * We now have a 33-bit (r0 + carry) sum which needs to resolved to a
	 * 16-bit sum.
	 */
	mov	r1, r0, lsr #16		/* get upper halfword */
#ifdef _ARM_ARCH_6
	uxth	r0, r0			/* clear upper halfword (16bit carry) */
#else
	bic	r0, r0, #0x00ff0000	/* clear upper halfword (16bit carry) */
	bic	r0, r0, #0xff000000	/* clear upper halfword */
#endif
	adc	r0, r0, r1		/* add halfwords with leftover carry */
	/*
	 * At this point, we have a sum with a max of 0x1fffe.
	 * If we have a 17-bit value (>= 0x10000) then subtract 0xffff.
	 */
	cmp	r0, #0x10000		/* test 16-bit carry */
#ifdef _ARM_ARCH_7
	movw	r1, #0xffff		/* load 0xffff */
#else
	mov	r1, #0x10000		/* load 0x10000 */
	sub	r1, r1, #1		/* subtract by 1 to get 0xffff */
#endif
	subge	r0, r0, r1		/* subtract 0xffff */
	eor	r0, r0, r1		/* complement lower halfword */
	RET
END(cpu_in_cksum_v4hdr)
