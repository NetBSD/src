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

RCSID("$NetBSD: cpu_in_cksum_buffer.S,v 1.7 2012/12/23 03:44:24 matt Exp $")

/*
 * Special note:
 * The use of cmp is avoided so that APSR.C (carry) is never overwritten.
 */

#ifdef _ARM_ARCH_DWORD_OK
#define	LOAD_DWORD_INTO_R4(r)	ldrd	r4, [r], #8
#define	LOAD_DWORD_INTO_R6(r)	ldrd	r6, [r], #8
#else
#define	LOAD_DWORD_INTO_R4(r)	ldmia	r!, {r4-r5}
#define	LOAD_DWORD_INTO_R4(r)	ldmia	r!, {r6-r7}
#endif

#if defined(__ARMEL__) || !defined(_ARM_ARCH_DWORD_OK)
#define	RLO	r4
#define	RHI	r5
#else
#define	RLO	r5
#define	RHI	r4
#endif
#if defined(__ARMEL__)
#define	BYTE0	0x000000ff
#define	BYTE3	0xff000000
#else
#define	BYTE0	0xff000000
#define	BYTE3	0x000000ff
#endif

/*
 * uint16_t cpu_in_cksum_buffer(const void *, size_t, uint32_t initial_csum);
 */

ENTRY(cpu_in_cksum_buffer)
#if defined(_ARM_ARCH_DWORD_OK) && !defined(__OPTIMIZE_SIZE__)
	pld	[r0]			/* prefetch the first data */
#endif
	adds	ip, r2, #0		/* initialize accumulator/clear carry */
	teq	r1, #0			/* did we get passed a zero length? */
	beq	.Lfold			/* fold the checksum */
	add	r2, r0, r1		/* point r2 just past end */
	push	{r4-r5,r10-r11}		/* save registers */
	mvn	r11, #0			/* initialize trailing mask */
	ands	r3, r2, #3		/* limit to a word */
	beq	1f			/* no trailing bytes? */
	/*
	 * This buffer doesn't end on a word boundary so create a mask
	 * to discard the unneeded bytes in the last word and then round
	 * up the length and ending address to a word boundary.
	 */
	rsb	r3, r3, #4		/* find out how many bytes to clear */
	add	r2, r2, r3		/* align to word boundary */
	add	r1, r1, r3		/* align to word boundary */
	mov	r3, r3, lsl #3		/* bytes -> bits */
#ifdef __ARMEL__
	mov	r11, r11, lsr r3	/* replace with zero bits */
#else
	mov	r11, r11, lsl r3	/* replace with zero bits */
#endif
1:
	ands	r10, r0, #7		/* test for dword alignment */
	bne	.Ldword_misaligned	/*   no, fixup non dword aligned */
	/*
	 * If the (now rounded up) length is 4, then only bit 2 will be set.
	 * So if we clear that bit and the result is 0, then the length must
	 * have been 4.
	 */
	bics	RLO, r1, #4		/* more than 1 word? */
	beq	.Lfinal_word_load	/*   no, just load final word */
	LOAD_DWORD_INTO_R4(r0)		/* load first dword */
#if defined(_ARM_ARCH_DWORD_OK) && !defined(__OPTIMIZE_SIZE__)
	pld	[r0, #32]		/* prefetch data */
#endif
	.p2align 3
.Ldword_aligned_noload:
	sub	r1, r2, r0		/* how much is remaining? */
	bics	r3, r1, #15		/* at least 16 bytes to do? */
	beq	.Lfinal_words		/*   no, but we have at least 1 word */
	push	{r6-r7}
#if !defined(__OPTIMIZE_SIZE__)
	bics	r3, r1, #63		/* at least 64 bytes to do? */
	bne	.Lloop64		/*   yes, then do them */
	tst	r1, #32			/* what about 32 bytes */
	bne	.Lloop32		/*   yes, then do them */
	b	.Lloop16		/* then we must have 16 bytes */
.Lloop64:
	LOAD_DWORD_INTO_R6(r0)		/* 8 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 7 dwords left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7
	LOAD_DWORD_INTO_R6(r0)		/* 6 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 5 dwords left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7
.Lloop32:
	LOAD_DWORD_INTO_R6(r0)		/* 4 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 3 dwords left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7
#endif /* !__OPTIMIZE_SIZE__ */
.Lloop16:
	LOAD_DWORD_INTO_R6(r0)		/* 2 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 1 dword left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7

	sub	r1, r2, r0		/* find how much is left */
#if !defined(__OPTIMIZE_SIZE__)
	bics	r3, r1, #63		/* at least 64 bytes to do? */
	bne	.Lloop64		/*   yes, run the loop again */
	tst	r1, #32			/* what about 32 bytes? */
	bne	.Lloop32		/*   yes, do 32-bytes */
#endif /* !__OPTIMIZE_SIZE__ */

	bics	r3, r1, #15		/* at least 16 bytes to do? */
	bne	.Lloop16		/*   yes, deal with them. */

	pop	{r6-r7}			/* done with these so restore them */

	teq	r1, #0			/* how much left?? */
	beq	.Ladd_final_dword	/*   = 0? do the final add */
.Lfinal_words:
	/*
	 * We have 1 to 3 words left to load.
	 */
	tst	r1, #8			/* at least one dword (5+ bytes)? */
	beq	.Lfinal_word_load	/*   no, deal with the final word. */
	/*
	 * We have at least 8 bytes left so accumulate the pending dword
	 * and then load the next dword.
	 */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	/*
	 * At this point r1 is either 8 or 12 so we can just clear bit 3
	 * to see if we have one more word to read.
	 */
	bics	r1, r1, #8		/* subtract dword from length */
	beq	.Ladd_final_dword	/*   = 0? do the final add */
.Lfinal_word_load:
	/*
	 * Finally we are at the word to load.
	 */
	adcs	ip, ip, RHI		/* accumulate RHI */
	ldr	RHI, [r0]		/* load last word into RHI */
.Ladd_final_dword:
	adcs	ip, ip, RLO		/* add RLO to accumulator */
.Ladd_final_word:
	and	RHI, RHI, r11		/* apply trailing mask to RHI */
	adcs	ip, ip, RHI		/* add RHI to accumulator */

	/*
	 * Fall into fold.
	 */
	tst	r10, #1			/* was starting address odd? */
	movne	ip, ip, ror #8		/*   yes, compensate */

	pop	{r4-r5,r10-r11}		/* we don't need these anymore */
.Lfold:
	/*
	 * We now have the 33-bit result in <carry>, ip.  Pull in the
	 * standard folding code.
	 */
#include "cpu_in_cksum_fold.S"

.Ldword_misaligned:
#ifdef _ARM_ARCH_DWORD_OK
	pld	[r0, #32]		/* preload next cacheline */
#endif
	tst	r0, #3			/* are at least word aligned? */
	bne	.Lword_misaligned	/*   no, do it the hard way */
	ldr	RHI, [r0], #4		/* load word here in case of partial */
	sub	r1, r1, #4		/* subtract length of one word */
	teq	r1, #0 			/* what is length? */
	beq	.Ladd_final_word	/*  <= 0? just do the final add */
	mov	RLO, #0			/*   > 0? clear RLO */
	b	.Ldword_aligned_noload	/*   > 0? accumulate it and loop */

.Lword_misaligned:
	/*
	 * If we start on an odd boundary, set up our stack frame so we
	 * can fixup the return value to be byteswapped.
	 */
	tst	r0, #4			/* do we load 1 or 2 words? */
	bic	r0, r0, #3		/* force word alignment */
	add	r1, r1, r10		/* add initial offset to length */
	sub	r1, r1, #8		/* subtract length of one dword */
#ifdef _ARM_ARCH_DWORD_OK
	ldreqd	r4, [r0], #8		/* load first dword */
#else
	ldmeqia	r0!, {r4-r5}		/* load first dword */
#endif
	ldrne	RLO, [r0], #4		/* load first word */
	movne	RHI, #0			/* no second word */
	/*
	 * We are now dword aligned.
	 */
	and	r3, r10, #3		/* limit to a single word length */
	mov	r3, r3, lsl #3		/* bytes -> bits */
#ifdef __ARMEL__
	mov	RLO, RLO, lsr r3	/* discard unneeded bits */
	mov	RLO, RLO, lsl r3	/* replace with zero bits */
#else
	mov	RLO, RLO, lsl r3	/* discard unneeded bits */
	mov	RLO, RLO, lsr r3	/* replace with zero bits */
#endif
	/*
	 * See if we have a least a full dword to process.  If we do, jump
	 * into the main loop as if we just load a single dword.
	 */
	teq	r1, #0 			/* what is length? */
	beq	.Ladd_final_dword	/*   = 0? just do the final add */
	bpl	.Ldword_aligned_noload	/*   > 0? accumulate it and loop */
	movne	RHI, RLO		/*   yes? move RLO to RHI */
	b	.Ladd_final_word	/* handle final word */
END(cpu_in_cksum_buffer)
