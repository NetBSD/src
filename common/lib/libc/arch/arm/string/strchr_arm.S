/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

RCSID("$NetBSD: strchr_arm.S,v 1.2 2013/01/15 16:52:35 matt Exp $")

#ifdef __ARMEL__
#define	BYTE0	0x000000ff
#define	BYTE1	0x0000ff00
#define	BYTE2	0x00ff0000
#define	BYTE3	0xff000000
#define	lshi	lsl
#else
#define	BYTE0	0xff000000
#define	BYTE1	0x00ff0000
#define	BYTE2	0x0000ff00
#define	BYTE3	0x000000ff
#define	lshi	lsr
#endif

	.text
ENTRY(strchr)
	and	r2, r1, #0xff		/* restrict to byte value */
1:	tst	r0, #3			/* test for word alignment */
	beq	.Lpre_main_loop		/*   finally word aligned */
	ldrb	r3, [r0], #1		/* load a byte */
	cmp	r3, r2			/* is it a match? */
	beq	2f			/*   yes, return current ptr - 1 */
	teq	r3, #0			/* no, was it 0? */
	bne	1b			/*   no, try next byte */
	mov	r0, #0			/*   yes, set return value to NULL */
	RET				/* return */
2:	sub	r0, r0, #1		/* back up by one */
	RET				/* return */
.Lpre_main_loop:
#if defined(_ARM_ARCH_7)
	movw	r1, #0xfefe		/* magic constant; 254 in each byte */
	movt	r1, #0xfefe		/* magic constant; 254 in each byte */
#elif defined(_ARM_ARCH_6)
	mov	r1, #0xfe		/* put 254 in low byte */
	orr	r1, r1, r1, lsl #8	/* move to next byte */
	orr	r1, r1, r1, lsl #16	/* move to next halfword */
#endif /* _ARM_ARCH_6 */
	orr	r2, r2, r2, lsl #8	/* move to next byte */
	orr	r2, r2, r2, lsl #16	/* move to next halfword */
.Lmain_loop:
	ldr	r3, [r0], #4		/* load next word */
#if defined(_ARM_ARCH_6)
	/*
	 * Add 254 to each byte using the UQADD8 (unsigned saturating add 8)
	 * instruction.  For every non-NUL byte, the result for that byte will
	 * become 255.  For NUL, it will be 254.  When we complement the
	 * result, if the result is non-0 then we must have encountered a NUL.
	 */
	uqadd8	ip, r3, r1		/* NUL detection happens here */
	eor	r3, r3, r2		/* xor to clear each lane */
	uqadd8	r3, r3, r1		/* char detection happens here */
	and	r3, r3, ip		/* merge results */
	mvns	r3, r3			/* is the complement non-0? */
	beq	.Lmain_loop		/*    no, then keep going */

	/*
	 * We've encountered a NUL or a match but we don't know which happened
	 * first.
	 */
	mvns	ip, ip			/* did we encounter a NUL? */
	beq	.Lfind_match		/*   no, find the match */
	movs	ip, ip, lshi #8		/* replicate NUL bit to other bytes */
	orrne	ip, ip, lshi #8		/* replicate NUL bit to other bytes */
	orrne	ip, ip, lshi #8		/* replicate NUL bit to other bytes */
	bics	r3, r3, ip		/* clear any match bits after the NUL */
	beq	.Lnomatch		/*   any left set? if not, no match */
.Lfind_match:
#ifdef __ARMEL__
	rev	r3, r3			/* we want this in BE for the CLZ */
#endif
	clz	r3, r3			/* count how many leading zeros */
	add	r0, r0, r3, lsr #3	/* divide that by 8 and add to count */
	sub	r0, r0, #4		/* compensate for the post-inc */
	RET
.Lnomatch:
	mov	r0, #0
	RET
#else
	/*
	 * No fancy shortcuts so just test each byte lane for a NUL.
	 * (other tests for NULs in a word take more instructions/cycles).
	 */
	eor	ip, r3, r2		/* xor .. */
	tst	r3, #BYTE0		/* is this byte NUL? */
	tstne	ip, #BYTE0		/*   no, does this byte match? */
	tstne	r3, #BYTE1		/*   no, is this byte NUL? */
	tstne	ip, #BYTE1		/*   no, does this byte match? */
	tstne	r3, #BYTE2		/*   no, is this byte NUL? */
	tstne	ip, #BYTE2		/*   no, does this byte match? */
	tstne	r3, #BYTE3		/*   no, is this byte NUL? */
	tstne	ip, #BYTE3		/*   no, does this byte match? */
	bne	.Lmain_loop

	sub	r2, r0, #4		/* un post-inc */
	mov	r0, #0			/* assume no match */

	tst	ip, #BYTE0		/* does this byte match? */
	moveq	r0, r2			/*   yes, point to it */
	RETc(eq)			/*        and return */
	tst	r3, #BYTE0		/* is this byte NUL? */
	RETc(eq)			/*   yes, return NULL */

	tst	ip, #BYTE1		/* does this byte match? */
	addeq	r0, r2, #1		/*   yes, point to it */
	RETc(eq)			/*        and return */
	tst	r3, #BYTE1		/* is this byte NUL? */
	RETc(eq)			/*   yes, return NULL */

	tst	ip, #BYTE2		/* does this byte match? */
	addeq	r0, r2, #2		/*   yes, point to it */
	RETc(eq)			/*        and return */
	tst	r3, #BYTE2		/* is this byte NUL? */
	RETc(eq)			/*   yes, return NULL */

	tst	ip, #BYTE3		/* does this byte match? */
	addeq	r0, r2, #3		/*   yes, point to it */
	/*
	 * Since no NULs and no matches this must be the only case left.
	 */
	RET				/* return */
#endif /* _ARM_ARCH_6 */
END(strchr)
