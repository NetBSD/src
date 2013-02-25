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

RCSID("$NetBSD: strrchr_arm.S,v 1.3.4.2 2013/02/25 00:23:59 tls Exp $")

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
ENTRY(strrchr)
	teq	r1, #0			/* searching for NUL? */
	bne	1f			/*   no, do it the hard way */
	push	{r0, lr}		/* save pointer and return addr */
	bl	PLT_SYM(strlen)		/* get length */
	pop	{r1, lr}		/* restore pointer and returna addr */
	add	r0, r0, r1		/* add pointer to length */
	RET				/* return */

1:	mov	ip, r0			/* we use r0 at the return value */
	mov	r0, #0			/* return NULL by default */
	and	r2, r1, #0xff		/* restrict to byte value */
2:	tst	ip, #3			/* test for word alignment */
	beq	.Lpre_main_loop		/*   finally word aligned */
	ldrb	r3, [ip], #1		/* load a byte */
	cmp	r3, r2			/* did it match? */
	subeq	r0, ip, #1		/*   yes, remember that it did */
	teq	r3, #0			/* was it NUL? */
	bne	2b			/*   no, try next byte */
	RET				/* return */
.Lpre_main_loop:
	push	{r4, r5}		/* save some registers */
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
	ldr	r3, [ip], #4		/* load next word */
#if defined(_ARM_ARCH_6)
	/*
	 * Add 254 to each byte using the UQADD8 (unsigned saturating add 8)
	 * instruction.  For every non-NUL byte, the result for that byte will
	 * become 255.  For NUL, it will be 254.  When we complement the
	 * result, if the result is non-0 then we must have encountered a NUL.
	 */
	uqadd8	r4, r3, r1		/* NUL detection happens here */
	usub8	r3, r3, r2		/* bias for char looked for? */
	uqadd8	r5, r3, r1		/* char detection happens here */
	and	r3, r4, r5		/* merge results */
	mvns	r3, r3			/* is the complement non-0? */
	beq	.Lmain_loop		/*   no, then keep going */

	mvns	r5, r5			/* get we find any matching bytes? */
	beq	.Ldone			/*   no, then we hit the end, return */
	mvns	r4, r4			/* did we encounter a NUL? */
	beq	.Lfind_match		/*   no, find matching byte */
	/*
	 * Copy the NUL bit to the following byte lanes.  Then clear any match
	 * bits in those byte lanes to prevent false positives in those bytes.
	 */
	bics	r5, r5, r4		/* clear any NUL match bits */
	beq	.Ldone			/*   no remaining matches, we're done */
	movs	r3, r4, lshi #8		/* shift up a byte */
	orrnes	r3, r3, r3, lshi #8	/* if non 0, copy up to next byte */
	orrnes	r3, r3, r3, lshi #8	/* if non 0, copy up to last byte */
	bics	r5, r5, r3		/* clear match bits */
	beq	.Ldone			/*   no remaining matches, we're done */
.Lfind_match:
#ifdef __ARMEL__
	rev	r5, r5			/* we want this in BE for the CLZ */
#endif
	/*
	 * If we have multiple matches, we want to the select the "last" match
	 * in the word which will be the lowest bit set.
	 */
	sub	r3, r5, #1		/* subtract 1 */
	and	r3, r3, r5		/* and with mask */
	eor	r5, r5, r3		/* only have the lowest bit set left */
	clz	r5, r5			/* count how many leading zeros */
	add	r0, ip, r5, lsr #3	/* divide that by 8 and add to count */
	sub	r0, r0, #4		/* compensate for the post-inc */
	teq	r4, #0			/* did we read any NULs? */
	beq	.Lmain_loop		/*   no, get next word */
#else
	/*
	 * No fancy shortcuts so just test each byte lane for a NUL.
	 * (other tests for NULs in a word take more instructions/cycles).
	 */
	eor	r4, r3, r2		/* xor .. */
	tst	r3, #BYTE0		/* is byte 0 a NUL? */
	beq	.Ldone			/*   yes, then we're done */
	tst	r4, #BYTE0		/* is byte 0 a match? */
	subeq	r0, ip, #4		/*   yes, remember its location */
	tst	r3, #BYTE1		/* is byte 1 a NUL? */
	beq	.Ldone			/*   yes, then we're done */
	tst	r4, #BYTE1		/* is byte 1 a match? */
	subeq	r0, ip, #3		/*   yes, remember its location */
	tst	r3, #BYTE2		/* is byte 2 a NUL? */
	beq	.Ldone			/*   yes, then we're done */
	tst	r4, #BYTE2		/* is byte 2 a match? */
	subeq	r0, ip, #2		/*   yes, remember its location */
	tst	r3, #BYTE3		/* is byte 3 a NUL? */
	beq	.Ldone			/*   yes, then we're done */
	tst	r4, #BYTE3		/* is byte 3 a match? */
	subeq	r0, ip, #1		/*   yes, remember its location */
	b	.Lmain_loop
#endif /* _ARM_ARCH_6 */
.Ldone:
	pop	{r4, r5}
	RET
END(strrchr)
