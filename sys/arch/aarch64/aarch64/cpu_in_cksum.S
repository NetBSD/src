/* $NetBSD: cpu_in_cksum.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include <aarch64/asm.h>
#include "assym.h"

RCSID("$NetBSD: cpu_in_cksum.S,v 1.1 2014/08/10 05:47:37 matt Exp $")
/*
 * int cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t initial_sum)
 *
 * Entry:
 *	r0	m
 *	r1	len
 *	r2	off
 *	r3	initial_sum
 *
 * Function wide register usage
 *	r	8accumulated sum
 *	r9	remaining length to parse
 *	ip	pointer to next mbuf
 */
/* LINTSTUB: Func: int cpu_in_cksum(struct mbuf *, int, int, uint32_t) */
ENTRY(cpu_in_cksum)
	mov	x13, #0
	mov	x14, x0
	mov	x15, x1
	mov	x16, x3
	mov	x17, x30

	ldr	w1, [x14, #M_LEN]
	sub	x7, x1, x2
	cmp	x7, x16
	b.le	.Lsingle_mbuf

.Lskip_offset:
	cmp	x2, x1
	b.lt	.Lnonzero_offset
	sub	x2, x2, x1
	ldr	x14, [x14, #M_NEXT]
	cbnz	x2, .Lskip_offset
	ldr	w1, [x14, #M_LEN]

.Lnonzero_offset:
	ldr	x0, [x14, #M_DATA]
	add	x0, x0, x2
	sub	x1, x1, x2

.Lget_cksum:
	eor	x13, x13, x0		// was start not halfword aligned?
	bfi	x13, x1, #1, #1		// remember if length was odd
	sub	x15, x15, x1		// subtract length

	bl	cpu_in_cksum_range
	tst	x13, #1
	lsl	x12, x0, #8
	csel	x0, x0, x12, eq
	add	x16, x16, x0
	cbz	x15, .Lfinalize		// nothing left, finalize
	eor	x13, x13, x13, lsr #1	// merge length into misaligned
.Lnext_mbuf:
	ldr	x14, [x14, #M_NEXT]
	ldr	w1, [x14, #M_LEN]
	cbz	x1, .Lnext_mbuf
	cmp	x1, x15
	csel	x1, x1, x15, lt
	ldr	x0, [x14, #M_DATA]
	b	.Lget_cksum

.Lsingle_mbuf:
	ldr	x0, [x14, #M_DATA]
	add	x0, x0, x2
	and	x13, x0, #1
	cmp	x15, #20
	b.eq	.Ljust_ipv4_hdr
	mov	x1, x15
	bl	cpu_in_cksum
	tst	x13, #1
	lsl	x12, x0, #8
	csel	x0, x0, x12, eq
	add	x16, x16, x0
	b	.Lfinalize

.Ljust_ipv4_hdr:
	tbz	x0, #2, 2f
	ldr	w6, [x0], #4
	ldp	x4, x5, [x0]
	b	.Lipv4_hdr_add
2:	ldp	x4, x5, [x0]
	ldr	w6, [x0, #16]
.Lipv4_hdr_add:
	adds	w16, w16, w6
	adcs	x16, x16, x4
	adcs	x16, x16, x5
	adcs	x16, x16, xzr

.Lfinalize:
	/* we have a 64-bit (no carry) result */
	lsr	x1, x16, #32
	adds	w0, w16, w1
	/* we reduced that to a 33-bit result */
	lsr	w1, w0, #16
	uxth	w0, w0
	add	w0, w0, w1
	/* now a 17-bit result (0 .. 0x1fffe) */
	add	w0, w0, w0, lsr #16
	/* now a 16-bit result (0 .. 0xffff) */
	mvn	w0, w0			// invert
	uxth	w0, w0			// mask to a halfword
	ret	x17
END(cpu_in_cksum)

/*
 *	X0 = ptr
 *	X1 = length
 * returns
 *	x0 = 32-bit checksum
 * clobbers:
 *	x0-x9
 */
ENTRY(cpu_in_cksum_range)
	mov	x6, x0
	mov	x7, x1
	mov	x10, #0

	mov	x0, #0
	msr	nzcv, xzr		// clear carry
	lsr	x8, x7, #4		// get qword count
	lsr	x9, x8, #3
	cbz	x8, .Llast_subqword	// no qwords, deal with the remainder

	/*
	 * We hope for alignment but if it's a small number of qwords
	 * don't worry about.
	 */
	cbz	x9, .Lqword_loop

	/*
	 * Since we are aligning, we have do adjust the length to remove
	 * amount of data needed to align.  Then we need recalculate the
	 * qword count.
	 */
	neg	x9, x6			// get negative of address
	and	x9, x6, #15		// mask off upper bits
	sub	x7, x7, x9		// shrink length.
	lsr	x8, x7, #4		// get qword count
	lsr	x9, x8, #3

	tbz	x6, #0, .Lhword_aligned
	ldrb	w4, [x6], #1		// zero extending load
	add	x0, x0, x4, lsr #8	// carry not possible
	mov	x10, #8			// final rotate
.Lhword_aligned:
	tbz	x6, #1, .Lword_aligned
	ldrh	w4, [x6], #2		// zero extending load
	add	x0, x0, x4		// carry not possible
.Lword_aligned:
	tbz	x6, #2, .Ldword_aligned
	ldr	w4, [x6], #4		// zero extending load
	add	x0, x0, x4		// carry not possible
.Ldword_aligned:
	tbz	x6, #3, .Lqword_aligned
	ldr	x4, [x6], #8
	adds	x0, x0, x4
.Lqword_aligned:
	cbz	x9, .Lqword_loop
	/*
	 * This loop does it 128 bytes (8 * 16) at a time.
	 */
.Lbig_loop:
	ldp	x2, x3, [x6], #16	// 1
	adcs	x0, x0, x2
	adcs	x0, x0, x3
	ldp	x4, x5, [x6], #16	// 2
	adcs	x0, x0, x4
	adcs	x0, x0, x5
	ldp	x2, x3, [x6], #16	// 3
	adcs	x0, x0, x2
	adcs	x0, x0, x3
	ldp	x4, x5, [x6], #16	// 4
	adcs	x0, x0, x4
	adcs	x0, x0, x5
	ldp	x2, x3, [x6], #16	// 5
	adcs	x0, x0, x2
	adcs	x0, x0, x3
	ldp	x4, x5, [x6], #16	// 6
	adcs	x0, x0, x4
	adcs	x0, x0, x5
	ldp	x2, x3, [x6], #16	// 7
	adcs	x0, x0, x2
	adcs	x0, x0, x3
	ldp	x4, x5, [x6], #16	// 8
	adcs	x0, x0, x4
	adcs	x0, x0, x5
	sub	x9, x9, #1
	cbnz	x9, .Lbig_loop
.Lqword_loop:
	ldp	x4, x5, [x6], #16
	adcs	x0, x0, x4
	adcs	x0, x0, x5
	sub	x8, x8, #1
	cbnz	x8, .Lqword_loop
	and	x7, x7, #15		// keep leftover amount.
	cbz	x7, .Lcollapse

.Llast_subqword:
	tbz	x7, #3, .Llast_subdword
	ldr	x4, [x6], #8
	adcs	x0, x0, x4
.Llast_subdword:
	tbz	x7, #2, .Llast_subword
	ldr	w4, [x6], #4
	adcs	x0, x0, x4
.Llast_subword:
	tbz	x7, #1, .Llast_subhalfword
	ldrh	w4, [x6], #2
	adcs	x0, x0, x4
.Llast_subhalfword:
	tbz	x7, #0, .Lcollapse
	ldrb	w4, [x6]
	adcs	x0, x0, x4
.Lcollapse:
	ror	x0, x0, x10
	lsr	x1, x0, #32
	adcs	w0, w0, w1
	/*
	 * We now have a number from 0 to 0xfffffffe + carry.
	 * add zero with carry and we have a 32-bit number.
	 */
	adcs	w0, w0, wzr
	ret
END(cpu_in_cksum_range)
