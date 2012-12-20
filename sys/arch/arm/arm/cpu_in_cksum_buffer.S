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

RCSID("$NetBSD: cpu_in_cksum_buffer.S,v 1.3 2012/12/20 08:03:21 matt Exp $")

/*
 * Special note:
 * The use of cmp is avoided so that APSR.C (carry) is never overwritten.
 */

#ifdef _ARM_ARCH_DWORD_OK
#define	LOAD_DWORD_INTO_R4(r)	ldrd	r4, [r], #8
#else
#define	LOAD_DWORD_INTO_R4(r)	ldr	r4, [r], #4;	ldr	r5, [r], #4
#endif

/*
 * uint16_t cpu_in_cksum_buffer(const void *, size_t, uint32_t initial_csum);
 */

ENTRY(cpu_in_cksum_buffer)
	mov	ip, r2			/* initialize accumulator */
	adds	ip, ip, #0		/* clear carry */
	push	{r4-r5}			/* save temporaries */
	teq	r1, #0			/* did we get passed a zero length? */
	beq	.Lfold			/* fold the checksum */
	ands	r2, r0, #7		/* test for dword alignment */
	bne	.Ldword_misaligned	/*   no, fixup non dword aligned */

	add	r2, r1, r0		/* point r2 just past end */
#ifndef __OPTIMIZE_SIZE__
	bics	r3, r1, #63		/* at least 64 bytes to do? */
	bne	4f			/*   yes, then do them */
#endif /* __OPTIMIZE_SIZE__ */
	bics	r3, r1, #7		/* at least 8 bytes to do? */
	beq	.Lfinal_dword		/*   no, handle the final dword */
3:
#ifndef __OPTIMIZE_SIZE__
	rsb	r3, r3, #64		/* subtract from 64 */
#ifdef _ARM_ARCH_DWORD_OK
	add	r3, r3, r3, lsr #1	/* multiply by 1.5 */
	add	pc, pc, r3		/* and jump! */
#else
	add	pc, pc, r3, lsl #1	/* multiply by 2 and jump! */
#endif
	nop
4:	LOAD_DWORD_INTO_R4(r0)		/* 8 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 7 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 6 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 5 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 4 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 3 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
	LOAD_DWORD_INTO_R4(r0)		/* 2 dwords left */
	adcs	ip, ip, r4
	adcs	ip, ip, r5
#endif /* __OPTIMIZE_SIZE__ */
	LOAD_DWORD_INTO_R4(r0)		/* 1 dword left */
.Ladd_one_dword:
	adcs	ip, ip, r4
.Ladd_one_word:
	adcs	ip, ip, r5
	teq	r2, r0			/* nothing left? */
	beq	.Lfold			/*   yep, proceed to hold */

	sub	r1, r2, r0		/* find out much left to do? */
#ifndef __OPTIMIZE_SIZE__
	bics	r3, r1, #63		/* at least 64 bytes left? */
	bne	4b			/*   yep, do 64 at time */
#endif
	bics	r3, r1, #7		/* at least 8 bytes left? */
	bne	3b			/*   yep, do them */

.Lfinal_dword:
	ldr	r5, [r0], #4		/* load next word */
	tst	r1, #3			/* final amount one word exactly? */
	beq	.Lfinal_add_one_word	/*   yes, and go add it */
	sub	r3, r1, #1		/* 0-3 = 1 word, 4-7 = 2 words */
	tst	r3, #4			/* one more word left? */
	moveq	r4, #0			/*   no, use 0 for 1st word  */
	movne	r4, r5			/*   yes, move from 2nd word to 1st */
	ldrne	r5, [r0]		/*   yes, load last word */
.Lfinal_dword_noload:
	rsb	r1, r1, #4		/* find out many bytes to discard */
#ifdef __ARMEL__
	tst	r1, #2			/* discard at least 2? */
	movne	r5, r5, lsl #16		/*   yes, discard upper halfword */
	tst	r1, #1			/* discard odd? */
	bicne	r5, r5, #0xff000000	/*   yes, discard odd byte */
#else
	tst	r1, #2			/* discard at least 2? */
	movne	r5, r5, lsr #16		/*   yes, discard lower halfword */
	tst	r1, #1			/* discard odd? */
	bicne	r5, r5, #0x000000ff	/*   yes, discard odd byte */
#endif
.Lfinal_add_one_dword:
	adcs	ip, ip, r4		/* add 1st to accumulator */
.Lfinal_add_one_word:
	adcs	ip, ip, r5		/* add 2nd to accumulator */

	/*
	 * Fall into fold.
	 */

.Lfold:
	pop	{r4-r5}			/* we don't need these anymore */
	/*
	 * We now have the 33-bit result in <carry>, ip.  Pull in the
	 * standard folding code.
	 */
#include "cpu_in_cksum_fold.S"

.Ldword_misaligned:
	tst	r0, #3			/* are at least word aligned? */
	bne	.Lword_misaligned	/*   no, do it the hard way */
	ldr	r5, [r0], #4		/* load word here in case of partial */
	sub	r1, r1, #4		/* subtract length of one word */
	teq	r1, #0 			/* what is length? */
	beq	.Lfinal_add_one_word	/*   = 0? just do the final add */
	addgt	r2, r1, r0		/*   > 0? point r2 just past end */
	bgt	.Ladd_one_word		/*   > 0? accumulate it and loop */
	mov	r4, #0			/*   < 0? zero this */
	b	.Lfinal_dword_noload	/*   < 0? handle final partial dword */

.Lword_misaligned:
	tst	r0, #4			/* do we load 1 or 2 words? */
	bic	r0, r0, #3		/* force word alignment */
	add	r1, r1, r2		/* add initial offset to length */
	sub	r1, r1, #8		/* subtract length of one dword */
	ldmeqia	r0!, {r4-r5}		/* load first dword */
	ldrne	r4, [r0], #4		/* load first word */
	movne	r5, #0			/* no second word */
	/*
	 * We are now dword aligned.
	 */
#ifdef __ARMEL__
	tst	r2, #2			/* discard at least 2? */
	movne	r4, r4, lsr #16		/* yes, discard lower halfword */
	tst	r2, #1			/* start odd? */
	bicne	r4, r4, #0x000000ff	/* yes, discard even byte */
#else
	tst	r2, #2			/* discard at least 2? */
	movne	r4, r4, lsl #16		/* yes, discard upper halfword */
	tst	r2, #1			/* start odd? */
	bicne	r4, r4, #0xff000000	/* yes, discard even byte */
#endif
	/*
	 * Since we started on an odd boundary, set up our stack frame so we
	 * fixup the return value to be byteswapped.
	 */
	ldrne	r3, [sp, #4]		/* pop r5 */
	strne	r3, [sp, #-4]!		/* push it again */ 
	ldrne	r3, [sp, #4]		/* pop r4 */
	strne	r3, [sp, #-4]!		/* push it again */
	strne	lr, [sp, #8]		/* save our return address */
	adrne	lr, .Lmisaligned_fixup	/* use new to fixup the return value */
	/*
	 * See if we have a least a full dword to process.  If we do, jump
	 * into the main loop as if we just load a single dword.
	 */
	teq	r1, #0 			/* what is length? */
	beq	.Lfinal_add_one_word	/*   = 0? just do the final add */
	addgt	r2, r1, r0		/*   > 0? point r2 just past end */
	bgt	.Ladd_one_dword		/*   > 0? accumulate it and loop */

	/*
	 * Not a full dword so do the final dword processing to find out
	 * bytes to discard.  If we only loaded one word, move it to 2nd
	 * word since that is what final_dword will be discarding from and
	 * clear the 1st word.
	 */
	tst	r2, #4			/* one or two words? */
	moveq	r5, r4			/*   one, move 1st word to 2nd word */
	moveq	r4, #0			/*        and clear 1st word */
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
