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

RCSID("$NetBSD: cpu_in_cksum_buffer.S,v 1.4 2012/12/21 06:35:34 matt Exp $")

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
#ifdef _ARM_ARCH_DWORD_OK
	pld	[r0]			/* prefetch the first data */
#endif
	mov	ip, r2			/* initialize accumulator */
	adds	ip, ip, #0		/* clear carry */
	teq	r1, #0			/* did we get passed a zero length? */
	beq	.Lfold_nopop		/* fold the checksum */
	ands	r2, r0, #7		/* test for dword alignment */
	bne	.Ldword_misaligned	/*   no, fixup non dword aligned */

	push	{r4-r5}			/* save temporaries */
	add	r2, r1, r0		/* point r2 just past end */
	LOAD_DWORD_INTO_R4(r0)		/* load first dword */
	sub	r1, r2, r0		/* we've read one dword */
.Ldword_aligned_noload:
#if !defined(__OPTIMIZE_SIZE__)
	bics	r3, r1, #63		/* at least 64 bytes to do? */
	beq	2f			/*   no, then do final collection */
	push	{r6-r7}
1:	
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
	LOAD_DWORD_INTO_R6(r0)		/* 4 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 3 dwords left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7
	LOAD_DWORD_INTO_R6(r0)		/* 2 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 1 dword left */
	adcs	ip, ip, r6
	adcs	ip, ip, r7

	sub	r1, r2, r0		/* find how much is left */
	bics	r3, r1, #63		/* at least 64 bytes to do? */
	bne	1b			/*   yes, run the loop again */

	pop	{r6-r7}			/* done with these so restore them */
#endif /* __OPTIMIZE_SIZE__ */

2:	teq	r1, #0			/* at the end? */
	beq	.Lfinal_add_one_dword	/*   yes, do the final add */
	bmi	.Lfinal_dword_noload	/*   past it, handle the final dword */
3:
#ifdef _ARM_ARCH_DWORD_OK
	pld	[r0, #32]		/* grab next cache line */
#endif
#ifndef __OPTIMIZE_SIZE__
	bic	r3, r1, #7		/* find out how many dwords to do */
	rsb	r3, r3, #56		/* subtract from 56 */
	add	r3, r3, r3, lsr #1	/* multiply by 1.5 */
	add	pc, pc, r3		/* and jump! */
	nop
	adcs	ip, ip, r4		/* 7 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	adcs	ip, ip, r4		/* 6 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	adcs	ip, ip, r4		/* 5 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	adcs	ip, ip, r4		/* 4 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	adcs	ip, ip, r4		/* 3 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	adcs	ip, ip, r4		/* 2 dwords left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
#endif /* __OPTIMIZE_SIZE__ */
	adcs	ip, ip, r4		/* 1 dword left */
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)
	sub	r1, r2, r0		/* find out much left to do? */
	teq	r1, #0			/* at the end? */
	beq	.Lfinal_add_one_dword	/*   yep, proceed to fold */
#ifdef __OPTIMIZE_SIZE__
	bics	r3, r1, #7		/* exhaust all dwords? */
	bne	3b			/*   not yet, do more */
#endif
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	ldr	RHI, [r0], #4		/* we have at least one word to read */
	sub	r3, r1, #4		/* subtract 4 from length */
	teq	r3, #0			/* is the result positive? */
	beq	.Lfinal_add_one_word	/*   = 0? just add that word. */
	movpl	RLO, RHI		/*   > 0? move from hi to lo word */
	ldrpl	RHI, [r0]		/*   > 0? load new hi word */
	movmi	RLO, #0			/*   < 0? clear lo word */

.Lfinal_dword_noload:
	rsb	r1, r1, #8		/* find out many bytes to discard */
	tst	r1, #2			/* discard at least 2? */
#ifdef __ARMEL__
	movne	RHI, RHI, lsl #16	/*   yes, discard upper halfword */
#else
	movne	RHI, RHI, lsr #16	/*   yes, discard lower halfword */
#endif
	tst	r1, #1			/* discard odd? */
	bicne	RHI, RHI, #BYTE3	/*   yes, discard odd byte */
.Lfinal_add_one_dword:
	adcs	ip, ip, RLO		/* add 1st to accumulator */
.Lfinal_add_one_word:
	adcs	ip, ip, RHI		/* add 2nd to accumulator */

	/*
	 * Fall into fold.
	 */

.Lfold:
	pop	{r4-r5}			/* we don't need these anymore */
.Lfold_nopop:
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
	push	{r4-r5}			/* save temporaries */
	ldr	RHI, [r0], #4		/* load word here in case of partial */
	sub	r1, r1, #4		/* subtract length of one word */
	teq	r1, #0 			/* what is length? */
	beq	.Lfinal_add_one_word	/*   = 0? just do the final add */
	mov	RLO, #0			/*   <= 0? zero this */
	bmi	.Lfinal_dword_noload	/*   < 0? handle final partial dword */
	add	r2, r1, r0		/*   > 0? point r2 just past end */
	b	.Ldword_aligned_noload	/*   > 0? accumulate it and loop */

.Lword_misaligned:
	/*
	 * If we start on an odd boundary, set up our stack frame so we
	 * can fixup the return value to be byteswapped.
	 */
	tst	r0, #1			/* start address odd? */
	strne	lr, [sp, #-8]!		/*   yes, save our return address */
	adrne	lr, .Lmisaligned_fixup	/*   yes, return to fixup code. */
	push	{r4-r5}			/* save temporaries */
	tst	r0, #4			/* do we load 1 or 2 words? */
	bic	r0, r0, #3		/* force word alignment */
	add	r1, r1, r2		/* add initial offset to length */
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
	tst	r2, #2			/* discard at least 2? */
#ifdef __ARMEL__
	movne	RLO, RLO, lsr #16	/*   yes, discard lower halfword */
#else
	movne	RLO, RLO, lsl #16	/*   yes, discard upper halfword */
#endif
	tst	r2, #1			/* start odd? */
	bicne	RLO, RLO, #BYTE0	/*   yes, discard even byte */
	/*
	 * See if we have a least a full dword to process.  If we do, jump
	 * into the main loop as if we just load a single dword.
	 */
	teq	r1, #0 			/* what is length? */
	beq	.Lfinal_add_one_dword	/*   = 0? just do the final add */
	addpl	r2, r1, r0		/*   > 0? point r2 just past end */
	bpl	.Ldword_aligned_noload	/*   > 0? accumulate it and loop */

	/*
	 * Not a full dword so do the final dword processing to find out
	 * bytes to discard.  If we only loaded one word, move it to 2nd
	 * word since that is what final_dword will be discarding from and
	 * clear the 1st word.
	 */
	tst	r2, #4			/* one or two words? */
	movne	RHI, RLO		/*   one, move lo word to hi word */
	movne	RLO, #0			/*        and clear lo word */
	b	.Lfinal_dword_noload	/* handle final dword */

	/*
	 * If we had an odd address, we have byte swap the return value.
	 * instead of testing everywhere, we inserted a fake callframe and
	 * set LR to return to do the fixup and return to the caller.
	 */
.Lmisaligned_fixup:
	ldr	lr, [sp], #8		/* fetch saved LR */
#ifdef _ARM_ARCH_6
	rev16	r0, r0			/* byte swap */
#else
	mov	r0, r0, r0, ror #8	/* move 0:7 to 24:31 and 8:15 to 0:7 */
	orr	r0, r0, r0, lsl #16	/* move 0:7 to 16:23 */
	mov	r0, r0, r0, lsr #16	/* clear 16:31 to 0:15 */
#endif
	RET
END(cpu_in_cksum_buffer)
