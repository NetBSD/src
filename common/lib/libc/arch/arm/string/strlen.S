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

RCSID("$NetBSD: strlen.S,v 1.1 2013/01/02 15:24:21 matt Exp $")

#ifdef __ARMEL__
#define	BYTE0	0x000000ff
#define	BYTE1	0x0000ff00
#define	BYTE2	0x00ff0000
#define	BYTE3	0xff000000
#else
#define	BYTE0	0xff000000
#define	BYTE1	0x00ff0000
#define	BYTE2	0x0000ff00
#define	BYTE3	0x000000ff
#endif

	.text
ENTRY(strlen)
	add	ip, r0, #4		/* for the final post-inc */
1:	tst	r0, #3			/* test for word alignment */
	beq	.Lpre_main_loop		/*   finally word aligned */
	ldrb	r3, [r0], #1		/* load a byte */
	teq	r3, #0			/* is it 0? */
	bne	1b			/*   no, try next byte */
	sub	ip, ip, #3		/* subtract (4 - the NUL) */
	sub	r0, r0, ip		/* subtract start */
	RET				/* return */
.Lpre_main_loop:
#if defined(_ARM_ARCH_6)
#if defined(_ARM_ARCH_7)
	movw	r1, #0xfefe		/* magic constant; 254 in each byte */
#else
	mov	r1, #0xfe		/* put 254 in low byte */
	orr	r1, r1, r1, lsl #8	/* move to next byte */
#endif
	orr	r1, r1, r1, lsl #16	/* move to next halfword */
#endif /* _ARM_ARCH_6 */
.Lmain_loop:
	ldr	r3, [r0], #4		/* load next word */
#if defined(_ARM_ARCH_6)
	/*
	 * Add 254 to each byte using the UQADD8 (unsigned saturating add 8)
	 * instruction.  For every non-NUL byte, the result for that byte will
	 * become 255.  For NUL, it will be 254.  When we complement the
	 * result, if the result is non-0 then we must have encountered a NUL.
	 */
	uqadd8	r3, r3, r1		/* magic happens here */
	mvns	r3, r3			/* is the complemented result non-0? */
	beq	.Lmain_loop		/*    no, then we encountered no NULs */
#else
	/*
	 * No fancy shortcuts so just test each byte lane for a NUL.
	 * (other tests for NULs in a word take more instructions/cycles).
	 */
	tst	r3, #BYTE0		/* is this byte 0? */
	tstne	r3, #BYTE1		/*   no, is this byte 0? */
	tstne	r3, #BYTE2		/*   no, is this byte 0? */
	tstne	r3, #BYTE3		/*   no, is this byte 0? */
	bne	.Lmain_loop		/*   no, then get next word */
#endif
#if defined(_ARM_ARCH_6)
	/*
	 * We encountered a NUL.  Find out where by doing a CLZ and then
	 * shifting right by 3.  That will be the number of non-NUL bytes.
	 */
#ifdef __ARMEL__
	rev	r3, r3			/* we want this in BE for the CLZ */
#endif
	clz	r3, r3			/* count how many leading zeros */
	add	r0, r0, r3, lsr #3	/* divide that by 8 and add to count */
#else
	/*
	 * We encountered a NUL.
	 */
	tst	r3, #BYTE0		/* 1st byte was NUL? */
	beq	1f			/*   yes, done adding */
	add	r0, r0, #1		/* we have one more non-NUL byte */
	tst	r3, #BYTE1		/* 2nd byte was NUL? */
	beq	1f			/*   yes, done adding */
	add	r0, r0, #1		/* we have one more non-NUL byte */
	tst	r3, #BYTE2		/* 3rd byte was NUL? */
	addne	r0, r0, #1		/* no, we have one more non-NUL byte */
1:
#endif /* _ARM_ARCH_6 */
	/*
	 * r0 now points to 4 past the NUL due to the post-inc.  Subtract the
	 * start of the string (which also has 4 added to it to compensate for
	 * the post-inc.
	 */
	sub	r0, r0, ip		/* subtract start to get length */
	RET
END(strlen)
