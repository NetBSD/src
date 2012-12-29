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

RCSID("$NetBSD: strlen_armv6.S,v 1.2 2012/12/29 05:36:57 matt Exp $")

	.text
ENTRY(strlen)
	add	ip, r0, #4		/* for the final post-inc */
	ands	r1, r0, #3		/* get misalignment */
	bic	r0, r0, #3		/* align to word boundary */
	ldr	r3, [r0], #4		/* load first word */
	beq	.Lpre_main_loop		/*   misaligned?  no, go to loop */
	/*
	 * For misaligned string, we need to make sure that the bytes before
	 * the start of the string will not cause a false match to a NUL.
	 */
	mvn	r2, #0			/* create a mask */
	mov	r1, r1, lsl #3		/* bytes -> bits */
#ifdef __ARMEL__
	mov	r2, r2, lsl r1		/* clear relavent bytes */
#else
	mov	r2, r2, lsr r1		/* clear relavent bytes */
#endif
	mvn	r2, r2			/* invert mask */
	orr	r3, r3, r2		/* orr in mask for leading bytes */
.Lpre_main_loop:
#ifdef _ARM_ARCH_7
	movw	r1, #0xfefe		/* magic constant; 254 in each byte */
#else
	mov	r1, #0xfe		/* put 254 in low byte */
	orr	r1, r1, r1, lsl #8	/* move to next byte */
#endif
	orr	r1, r1, r1, lsl #16	/* move to next halfword */
.Lmain_loop:
	/*
	 * Add 254 to each byte using the UQADD8 (unsigned saturating add 8)
	 * instruction.  For every non-NUL byte, the result for that byte will
	 * become 255.  For NUL, it will be 254.  When we complement the
	 * result, if the result is non-0 then we must have encountered a NUL.
	 */
	uqadd8	r3, r3, r1		/* magic happens here */
	mvns	r3, r3			/* is the complemented result 0? */
	bne	.Lreturn		/*    no, then we encountered a NUL */
	ldr	r3, [r0], #4		/* load next word */
	b	.Lmain_loop		/* and go */
	/*
	 * We encountered a NUL.  Find out where by doing a CLZ and then
	 * shifting right by 3.  That will be the number of non-NUL bytes.
	 */
.Lreturn:
#ifdef __ARMEL__
	rev	r3, r3			/* we want this in BE for the CLZ */
#endif
	clz	r3, r3			/* count how many leading zeros */
	add	r0, r0, r3, lsr #3	/* divide that by 8 and add to count */
	/*
	 * r0 now points to 4 past the NUL due to the post-inc.  Subtract
	 * the start of the string (which also has 4 added to it to compensate
	 * for the post-inc.
	 */
	sub	r0, r0, ip		/* subtract start to get length */
	RET
END(strlen)
