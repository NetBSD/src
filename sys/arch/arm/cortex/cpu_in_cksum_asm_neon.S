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

RCSID("$NetBSD: cpu_in_cksum_asm_neon.S,v 1.3 2012/12/22 18:58:29 matt Exp $")

/*
 * uint32_t
 * cpu_in_cksum_neon(const void *dptr, size_t dlen)
 *
 *	r0 = dptr
 *	r1 = dlen
 */
ENTRY(cpu_in_cksum_neon)
	mov		ip, r0		/* leave r0 as temp */
	add		r3, r1, ip	/* get end pointer */
	and		r1, ip, #7	/* get start offset (leading btyes) */
	and		r2, r3, #7	/* get end offset (trailing bytes) */
	bic		ip, ip, #7	/* start on a dword boundary */
	add		r3, r3, #7	/* round up to a dword boundary */
	bic		r3, r3, #7	/* end on a dword boundary */
	veor		q2, q2, q2	/* clear accumulator */
	vmvn.u64	q1, q2		/* create leading/trailing masks */
	/*
	 * Normally the lower addressed is in d6 but in this case we want to
	 * reverse it since we might only have a single dword and the final
	 * fold will want the dword to trim in d7 so put the first dword in
	 * d7 until we know we are going to read more than one. 
	 */
	veor		d6, d6, d6	/* clear second dword */
	vld1.64		{d7}, [ip:64]!	/* load first dword */
	orrs		r0, r1, r2	/* do we have any offsets */
	beq		.Lpre_main_loop	/*   no, proceed to main loop. */
	mov		r1, r1, lsl #3	/* leading bytes -> bits */
	movs		r2, r2, lsl #3	/* trailing bytes -> bits */
#ifdef __ARMEL__
	subne		r2, r2, #64	/* trim trailing MSBs */
#else
	rsb		r1, r1, #0	/* trim leading MSBs */
	rsbne		r2, r2, #64	/* trim trailing LSBs */
#endif
	vmov		d0, r1, r2	/* move shifts */
	vmovl.u32	q0, d0		/* 2 U32 -> 2 U64 */
	vshl.u64	q1, q1, q0	/* apply shifts to masks */
	vand.u32	d7, d7, d2	/* apply leading mask to 1st dword */
	tst		r1, #8		/* was the starting address odd? */
	beq		.Lpre_main_loop	/*   no, go to pre_main_loop */
	veor		d2, d2, d2	/* clear d2 (indicate odd addr) */

.Lpre_main_loop:
	cmp		ip, r3		/* do we just have a single dword? */
	beq		.Lfinish_up	/*   yes, let finish up! */
	vmov		d6, d7		/* move 1st dword to loaddr reg */
	vld1.64		{d7}, [ip:64]!	/* read rest of initial qword */

.Lmain_loop:
	subs		r1, r3, ip	/* how much left to do? */
	beq		.Lfinish_up	/*   = 0? we are done. */

	bics		r0, r1, #31	/* we deal with octawords only */
	beq		.Lloop_end	/*   no octawords? exit loop */
	rsbs		r0, r0, #128	/* subtract from 128 */
	ble		.Lloop128	/*   <= 0?, do 128 at a time. */
	add		r0, r0, r0, lsr #2 /* multiple by 1.25 */
	add		pc, pc, r0	/* and jump! */
	nop

.Lloop128:
	vld1.64		{d8-d9}, [ip:64]!	/* 128 left */
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6-d7}, [ip:64]!
	vmovl.u16	q0, d8		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d9		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */

	vld1.64		{d8-d9}, [ip:64]!	/* 96 left */
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6-d7}, [ip:64]!
	vmovl.u16	q0, d8		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d9		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */

	vld1.64		{d8-d9}, [ip:64]!	/* 64 left */
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6-d7}, [ip:64]!
	vmovl.u16	q0, d8		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d9		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */

	vld1.64		{d8-d9}, [ip:64]!	/* 32 left */
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6-d7}, [ip:64]!
	vmovl.u16	q0, d8		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vmovl.u16	q0, d9		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */

	b		.Lmain_loop

.Lloop_end:
	/*
	 * We have one to 3 more dwords to process
	 */
	rsb		r0, r1, #24
	add		r0, r0, r0, lsr #1
	add		pc, pc, r0
	nop
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6}, [ip:64]!
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d6}, [ip:64]!
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vld1.64		{d7}, [ip:64]!

.Lfinish_up:
	/*
	 * Apply remaining data in d6 and d7
	 */
	vmovl.u16	q0, d6		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */
	vand		d7, d7, d3	/* apply trailing mask */
	vmovl.u16	q0, d7		/* 4 U16 -> 4 U32 */
	vadd.u32	q2, q2, q0	/* add 4 U32 to accumulator */

	/*
	 * We now have 4 32-bit sums in q2 (each is 20-bits or less).
	 * Now to get to 1 I32 bit sum.
	 */
	vadd.u32	d4, d4, d5	/* 4 I32 -> 2 I32 */
	vmov		r2, s4		/* get flag for odd start */
	teq		r2, #0		/* was start addr even? */
	vmov		r0, r1, d4	/* extract two I32 */
	rev16eq		r0, r0		/* byte swap if start was odd */
	rev16eq		r1, r1		/* byte swap if start was odd */
	adds		ip, r0, r1	/* add them producing carry */
#include "arm/arm/cpu_in_cksum_fold.S"
END(cpu_in_cksum_neon)
