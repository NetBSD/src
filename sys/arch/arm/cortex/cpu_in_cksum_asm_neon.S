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

RCSID("$NetBSD: cpu_in_cksum_asm_neon.S,v 1.2 2012/12/18 06:05:56 matt Exp $")

/*
 * uint32_t
 * cpu_in_cksum_neon(const void *dptr, size_t dlen)
 *
 *	r0 = dptr
 *	r1 = dlen
 */
ENTRY(cpu_in_cksum_neon)
	str		lr, [sp, #-8]!	/* save lr */
	mov		ip, r0		/* leave r0 as temp */
	add		r3, r1, ip	/* get end pointer */
	ands		r1, ip, #15	/* get qword offset */
	bic		ip, ip, #15	/* start on a qword boundary */
	veor		q3, q3, q3	/* clear accumulator */
	beq		.Lpre_main_loop	/* ya, qword boundary start */

	sub		r0, r3, ip	/* get length to qword start */
	cmp		r0, #16		/* do we have at least a qword? */
	andlt		r2, r3, #15	/* no, factor in trailing bytes */
	blt		.Ltrailing_bytes /*   and do the last partial qword */
	mov		r2, #0		/* yes, no trailing bytes */
	bl		partial_qword	/* do the partial initial qword */
	mov		r1, #0		/* no more leading bytes */

.Lpre_main_loop:
	and		r2, r3, #15	/* trailing bytes */
	bic		r3, r3, #15	/* last partial or empty qword */
	cmp		ip, r3		/* at or past the end? */
	bge		.Ltrailing_bytes /* yes, deal with any trailing bytes */

.Lmain_loop:
	vld1.64		{d4-d5}, [ip:128]!
	vmovl.u16	q0, d4		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d5		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q0	/* add 4 U32 to accumulator */
	cmp		ip, r3
	blt		.Lmain_loop

.Ltrailing_bytes:
	cmp		r2, #0		/* any trailing bytes? */
	blne		partial_qword	/* yes, do final qword */
	ldr		lr, [sp], #8	/* fetch LR */

.Lfold_csum:
	/*
	 * We now have 4 32-bit sums in q3 (each is 20-bits or less).
	 * Now to get to 1 I32 bit sum.
	 */
	vadd.u32	d6, d6, d7	/* 4 I32 -> 2 I32 */
	vmovl.u32	q3, d6		/* split two I32 into two I64 */
	vadd.u32	d6, d6, d7	/* 2 I32 -> 1 I32 */
	vmovl.u16	q3, d6		/* split two I16 into two I32 */
	vmovl.u32	q3, d6		/* split two I32 into two I64 */
	vadd.u32	d6, d6, d7	/* 2 I16 -> 1 I32 */
	vmov		r0, s12		/* fetch csum from d6/q3 */
	/*
	 * The result could be 0x10000 but we expect the caller to deal
	 * with it
	 */
	RET
END(cpu_in_cksum_neon)

/*
 * Handling partial qwords is tricky.
 */
	.type		partial_qword, %function
partial_qword:
	str		lr, [sp, #-8]!	/* save LR */
	vld1.64		{d4-d5}, [ip:128]!	/* fetch data */
#ifdef __ARMEB__
	vswp		d5, d4		/* on BE, MSW should be in d5 */
#endif
	veor		q0, q0, q0	/* create a null mask */
	movs		r0, r1, lsl #3	/* any leading bytes? */
	blne		_C_LABEL(__neon_leading_qword_bitmask)
	vmvn.u64	q0, q0		/* invert leading mask to trailing */
	vand.u32	q2, q2, q0	/* preserve them */
	vmvn.u64	q0, #0		/* create mask */
	movs		r0, r2, lsl #3	/* if equal, no trailing bytes */
	blne		_C_LABEL(__neon_leading_qword_bitmask)
	vand.u32	q2, q2, q0	/* preserve them */
	ldr		lr, [sp], #8	/* Fetch LR */
	vmovl.u16	q0, d4		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d5		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q0	/* add 4 U32 to accumulator */
	RET
	.size		partial_qword, . - partial_qword

/*
 * uint32_t cpu_in_cksum_neon_v4hdr(void *dptr)
 */
ENTRY(cpu_in_cksum_neon_v4hdr)
	bic		ip, r0, #7
	vld1.32		{d0-d2},[ip]	/* it must be in 24 bytes */
	tst		r0, #4		/* depending on 64-bit alignment */
	beq		1f
	vmov		s0, s5		/* move last U32 to first U32 */
1:	vmovl.u32	q1, d2		/* move s5 to d3 and clear s5 */
	vmovl.u16	q3, d0		/* 4 U16 -> 4 U32 */
	vmovl.u16	q2, d1		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q2	/* add 4 U32 to accumulator */
	vmovl.u16	q2, d2		/* 4 U16 -> 4 U32 */
	vadd.u32	q3, q3, q2	/* add 4 U32 to accumulator */
	b		.Lfold_csum
END(cpu_in_cksum_neon_v4hdr)
