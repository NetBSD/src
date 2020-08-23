/*	$NetBSD: chacha_neon_32.S,v 1.4 2020/08/23 16:39:06 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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

RCSID("$NetBSD: chacha_neon_32.S,v 1.4 2020/08/23 16:39:06 riastradh Exp $")

	.fpu	neon

/*
 * ChaCha round, split up so we can interleave the quarterrounds on
 * independent rows/diagonals to maximize pipeline efficiency, with
 * spills to deal with the scarcity of registers.  Reference:
 *
 *	Daniel J. Bernstein, `ChaCha, a variant of Salsa20', Workshop
 *	Record of the State of the Art in Stream Ciphers -- SASC 2008.
 *	https://cr.yp.to/papers.html#chacha
 *
 *	a += b; d ^= a; d <<<= 16;
 *	c += d; b ^= c; b <<<= 12;
 *	a += b; d ^= a; d <<<= 8;
 *	c += d; b ^= c; b <<<= 7;
 *
 * The rotations are implemented with:
 *	<<< 16		VREV32.16 for 16,
 *	<<< 12		VSHL/VSRI/VORR (shift left, shift right and insert, OR)
 *	<<< 8		TBL (general permutation; rot8 below stored in r)
 *	<<< 7		VSHL/VSRI/VORR
 */

.macro	ROUNDLD	a0,a1,a2,a3, b0,b1,b2,b3, c0,c1,c2,c3, d0,d1,d2,d3
	vld1.8		{\c2-\c3}, [sp, :256]
.endm

.macro	ROUND	a0,a1,a2,a3, b0,b1,b2,b3, c0,c1,c2,c3, d0,d1,d2,d3, c0l, d0l,d0h,d1l,d1h,d2l,d2h,d3l,d3h
	/* a += b; d ^= a; d <<<= 16 */
	vadd.u32	\a0, \a0, \b0
	vadd.u32	\a1, \a1, \b1
	vadd.u32	\a2, \a2, \b2
	vadd.u32	\a3, \a3, \b3

	veor		\d0, \d0, \a0
	veor		\d1, \d1, \a1
	veor		\d2, \d2, \a2
	veor		\d3, \d3, \a3

	vrev32.16	\d0, \d0
	vrev32.16	\d1, \d1
	vrev32.16	\d2, \d2
	vrev32.16	\d3, \d3

	/* c += d; b ^= c; b <<<= 12 */
	vadd.u32	\c0, \c0, \d0
	vadd.u32	\c1, \c1, \d1
	vadd.u32	\c2, \c2, \d2
	vadd.u32	\c3, \c3, \d3

	vst1.8		{\c0-\c1}, [sp, :256]	/* free c0 and c1 as temps */

	veor		\c0, \b0, \c0
	veor		\c1, \b1, \c1
	vshl.u32	\b0, \c0, #12
	vshl.u32	\b1, \c1, #12
	vsri.u32	\b0, \c0, #(32 - 12)
	vsri.u32	\b1, \c1, #(32 - 12)

	veor		\c0, \b2, \c2
	veor		\c1, \b3, \c3
	vshl.u32	\b2, \c0, #12
	vshl.u32	\b3, \c1, #12
	vsri.u32	\b2, \c0, #(32 - 12)
	vsri.u32	\b3, \c1, #(32 - 12)

	vld1.8		{\c0l}, [r7, :64]	/* load rot8 table */

	/* a += b; d ^= a; d <<<= 8 */
	vadd.u32	\a0, \a0, \b0
	vadd.u32	\a1, \a1, \b1
	vadd.u32	\a2, \a2, \b2
	vadd.u32	\a3, \a3, \b3

	veor		\d0, \d0, \a0
	veor		\d1, \d1, \a1
	veor		\d2, \d2, \a2
	veor		\d3, \d3, \a3

	vtbl.8		\d0l, {\d0l}, \c0l	/* <<< 8 */
	vtbl.8		\d0h, {\d0h}, \c0l
	vtbl.8		\d1l, {\d1l}, \c0l
	vtbl.8		\d1h, {\d1h}, \c0l
	vtbl.8		\d2l, {\d2l}, \c0l
	vtbl.8		\d2h, {\d2h}, \c0l
	vtbl.8		\d3l, {\d3l}, \c0l
	vtbl.8		\d3h, {\d3h}, \c0l

	vld1.8		{\c0-\c1}, [sp, :256]	/* restore c0 and c1 */

	/* c += d; b ^= c; b <<<= 7 */
	vadd.u32	\c2, \c2, \d2
	vadd.u32	\c3, \c3, \d3
	vadd.u32	\c0, \c0, \d0
	vadd.u32	\c1, \c1, \d1

	vst1.8		{\c2-\c3}, [sp, :256]	/* free c2 and c3 as temps */

	veor		\c2, \b2, \c2
	veor		\c3, \b3, \c3
	vshl.u32	\b2, \c2, #7
	vshl.u32	\b3, \c3, #7
	vsri.u32	\b2, \c2, #(32 - 7)
	vsri.u32	\b3, \c3, #(32 - 7)

	veor		\c2, \b0, \c0
	veor		\c3, \b1, \c1
	vshl.u32	\b0, \c2, #7
	vshl.u32	\b1, \c3, #7
	vsri.u32	\b0, \c2, #(32 - 7)
	vsri.u32	\b1, \c3, #(32 - 7)
.endm

	.text
	.p2align 2
.Lconstants_addr:
	.long	.Lconstants - .

/*
 * chacha_stream256_neon(uint8_t s[256]@r0,
 *     uint32_t blkno@r1,
 *     const uint8_t nonce[12]@r2,
 *     const uint8_t key[32]@r3,
 *     const uint8_t const[16]@sp[0],
 *     unsigned nr@sp[4])
 */
ENTRY(chacha_stream256_neon)
	/* save callee-saves registers */
	push	{r4, r5, r6, r7, r8, r10, fp, lr}
	vpush	{d8-d15}
	mov	fp, sp

	/* r7 := .Lconstants - .Lconstants_addr, r6 := .Lconstants_addr */
	ldr	r7, .Lconstants_addr
	adr	r6, .Lconstants_addr

	/* reserve space for two 128-bit/16-byte q registers */
	sub	sp, sp, #0x20
	bic	sp, sp, #0x1f	/* align */

	/* get parameters */
	add	ip, fp, #96
	add	r7, r7, r6	/* r7 := .Lconstants (= v0123) */
	ldm	ip, {r4, r5}	/* r4 := const, r5 := nr */
	ldm	r2, {r6, r8, r10}	/* (r6, r8, r10) := nonce[0:12) */

	vld1.8	{q12}, [r4]	/* q12 := constant */
	vld1.8	{q13-q14}, [r3]	/* q13-q14 := key */
	vld1.32	{q15}, [r7, :128]! /* q15 := (0, 1, 2, 3) (128-bit aligned) */

#ifdef __ARM_BIG_ENDIAN
	rev	r6, r6
	rev	r8, r8
	rev	r10, r10
#endif

	vdup.32	q0, d24[0]	/* q0-q3 := constant */
	vdup.32	q1, d24[1]
	vdup.32	q2, d25[0]
	vdup.32	q3, d25[1]
	vdup.32	q12, r1		/* q12 := (blkno, blkno, blkno, blkno) */
	vdup.32	q4, d26[0]	/* q4-q11 := (key, key, key, key) */
	vdup.32	q5, d26[1]
	vdup.32	q6, d27[0]
	vdup.32	q7, d27[1]
	vdup.32	q8, d28[0]
	vdup.32	q9, d28[1]
	vdup.32	q10, d29[0]
	vdup.32	q11, d29[1]
	vadd.u32 q12, q12, q15	/* q12 := (blkno,blkno+1,blkno+2,blkno+3) */
	vdup.32	q13, r6		/* q13-q15 := nonce */
	vdup.32	q14, r8
	vdup.32	q15, r10

	b	2f

	_ALIGN_TEXT
1:	ROUNDLD	q0,q1,q2,q3, q5,q6,q7,q4, q10,q11,q8,q9, q15,q12,q13,q14
2:	subs	r5, r5, #2
	ROUND	q0,q1,q2,q3, q4,q5,q6,q7, q8,q9,q10,q11, q12,q13,q14,q15, \
			d16, d24,d25, d26,d27, d28,d29, d30,d31
	ROUNDLD	q0,q1,q2,q3, q4,q5,q6,q7, q8,q9,q10,q11, q12,q13,q14,q15
	ROUND	q0,q1,q2,q3, q5,q6,q7,q4, q10,q11,q8,q9, q15,q12,q13,q14, \
			d20, d30,d31, d24,d25, d26,d27, d28,d29
	bne	1b

	/*
	 * q8-q9 are free / saved on the stack.  We have:
	 *
	 *	q0 = (x0[0], x1[0]; x2[0], x3[0])
	 *	q1 = (x0[1], x1[1]; x2[1], x3[1])
	 *	q2 = (x0[2], x1[2]; x2[2], x3[2])
	 *	q3 = (x0[3], x1[3]; x2[3], x3[3])
	 *	...
	 *	q15 = (x0[15], x1[15]; x2[15], x3[15])
	 *
	 * where xi[j] is the jth word of the ith 16-word block.  Zip
	 * consecutive pairs with vzip.32, and you get:
	 *
	 *	q0 = (x0[0], x0[1]; x1[0], x1[1])
	 *	q1 = (x2[0], x2[1]; x3[0], x3[1])
	 *	q2 = (x0[2], x0[3]; x1[2], x1[3])
	 *	q3 = (x2[2], x2[3]; x3[2], x3[3])
	 *	...
	 *	q15 = (x2[14], x2[15]; x3[14], x3[15])
	 *
	 * As 64-bit d registers, this is:
	 *
	 *	d0 = (x0[0], x0[1])	d1 = (x1[0], x1[1])
	 *	d2 = (x2[0], x2[1])	d3 = (x3[0], x3[1])
	 *	d4 = (x0[2], x0[3])	d5 = (x1[2], x1[3])
	 *	d6 = (x2[2], x2[3])	d7 = (x3[2], x3[3])
	 *	...
	 *	d30 = (x2[14], x2[15])	d31 = (x3[14], x3[15])
	 *
	 * Swap d1<->d4, d3<->d6, ..., and you get:
	 *
	 *	q0 = (x0[0], x0[1]; x0[2], x0[3])
	 *	q1 = (x2[0], x2[1]; x2[2], x2[3])
	 *	q2 = (x1[0], x1[1]; x1[2], x1[3])
	 *	q3 = (x3[0], x3[1]; x3[2], x3[3])
	 *	...
	 *	q15 = (x15[0], x15[1]; x15[2], x15[3])
	 */

	sub	r7, r7, #0x10
	vdup.32	q8, r1		/* q8 := (blkno, blkno, blkno, blkno) */
	vld1.32	{q9}, [r7, :128] /* q9 := (0, 1, 2, 3) */

	vzip.32	q0, q1
	vzip.32	q2, q3
	vzip.32	q4, q5
	vzip.32	q6, q7

	vadd.u32 q8, q8, q9	/* q8 := (blkno,blkno+1,blkno+2,blkno+3) */
	vld1.8	{q9}, [r4]	/* q9 := constant */
	vadd.u32 q12, q12, q8	/* q12 += (blkno,blkno+1,blkno+2,blkno+3) */
	vld1.8	{q8}, [r3]!	/* q8 := key[0:16) */

	vswp	d1, d4
	vswp	d9, d12
	vswp	d3, d6
	vswp	d11, d14

	/*
	 * At this point, the blocks are:
	 *
	 *	q0 = (x0[0], x0[1]; x0[2], x0[3])
	 *	q1 = (x2[0], x2[1]; x2[2], x2[3])
	 *	q2 = (x1[0], x1[1]; x1[2], x1[3])
	 *	q3 = (x3[0], x3[1]; x3[2], x3[3])
	 *	q4 = (x0[4], x0[5]; x0[6], x0[7])
	 *	q5 = (x2[4], x2[5]; x2[6], x2[7])
	 *	q6 = (x1[4], x1[5]; x1[6], x1[7])
	 *	q7 = (x3[4], x3[5]; x3[6], x3[7])
	 *
	 * The first two rows to write out are q0 = x0[0:4) and q4 =
	 * x0[4:8).  Swapping q1<->q4, q3<->q6, q9<->q12, and q11<->q14
	 * enables us to issue all stores in consecutive pairs:
	 *	x0 in q0-q1
	 *	x1 in q8-q9
	 *	x2 in q2-q3
	 *	x3 in q10-q11
	 *	x4 in q4-q5
	 *	x5 in q12-q3
	 *	x6 in q6-q7
	 *	x7 in q14-q15
	 */

	vswp	q1, q4
	vswp	q3, q6

	vadd.u32 q0, q0, q9
	vadd.u32 q4, q4, q9
	vadd.u32 q2, q2, q9
	vadd.u32 q6, q6, q9

	vadd.u32 q1, q1, q8
	vadd.u32 q5, q5, q8
	vadd.u32 q3, q3, q8
	vadd.u32 q7, q7, q8

	vld1.8	{q8-q9}, [sp, :256]	/* restore q8-q9 */

	vst1.8	{q0-q1}, [r0]!
	vld1.8	{q0}, [r3]	/* q0 := key[16:32) */
	mov	r3, #0		/* q1 = (0, nonce[0:4), ..., nonce[8:12)) */
	vmov	d2, r3, r6
	vmov	d3, r8, r10

	vzip.32	q8, q9
	vzip.32	q10, q11
	vzip.32	q12, q13
	vzip.32	q14, q15

	vswp	d17, d20
	vswp	d25, d28
	vswp	d19, d22
	vswp	d27, d30

	vswp	q9, q12
	vswp	q11, q14

	vadd.u32 q8, q8, q0
	vadd.u32 q12, q12, q0
	vadd.u32 q10, q10, q0
	vadd.u32 q14, q14, q0

	vadd.u32 q9, q9, q1
	vadd.u32 q13, q13, q1
	vadd.u32 q11, q11, q1
	vadd.u32 q15, q15, q1

	/* vst1.8	{q0-q1}, [r0]! */
	vst1.8	{q8-q9}, [r0]!
	vst1.8	{q2-q3}, [r0]!
	vst1.8	{q10-q11}, [r0]!
	vst1.8	{q4-q5}, [r0]!
	vst1.8	{q12-q13}, [r0]!
	vst1.8	{q6-q7}, [r0]!
	vst1.8	{q14-q15}, [r0]

	/* zero temporary space on the stack */
	vmov.i32 q0, #0
	vmov.i32 q1, #0
	vst1.8	{q0-q1}, [sp, :256]

	/* restore callee-saves registers and stack */
	mov	sp, fp
	vpop	{d8-d15}
	pop	{r4, r5, r6, r7, r8, r10, fp, lr}
	bx	lr
END(chacha_stream256_neon)

/*
 * chacha_stream_xor256_neon(uint8_t s[256]@r0, const uint8_t p[256]@r1,
 *     uint32_t blkno@r2,
 *     const uint8_t nonce[12]@r3,
 *     const uint8_t key[32]@sp[0],
 *     const uint8_t const[16]@sp[4],
 *     unsigned nr@sp[8])
 */
ENTRY(chacha_stream_xor256_neon)
	/* save callee-saves registers */
	push	{r4, r5, r6, r7, r8, r10, fp, lr}
	vpush	{d8-d15}
	mov	fp, sp

	/* r7 := .Lconstants - .Lconstants_addr, r6 := .Lconstants_addr */
	ldr	r7, .Lconstants_addr
	adr	r6, .Lconstants_addr

	/* reserve space for two 128-bit/16-byte q registers */
	sub	sp, sp, #0x20
	bic	sp, sp, #0x1f	/* align */

	/* get parameters */
	add	ip, fp, #96
	add	r7, r7, r6	/* r7 := .Lconstants (= v0123) */
	ldm	ip, {r4, r5, ip}	/* r4 := key, r5 := const, ip := nr */
	ldm	r3, {r6, r8, r10}	/* (r6, r8, r10) := nonce[0:12) */

	vld1.8	{q12}, [r5]	/* q12 := constant */
	vld1.8	{q13-q14}, [r4]	/* q13-q14 := key */
	vld1.32	{q15}, [r7, :128]! /* q15 := (0, 1, 2, 3) (128-bit aligned) */

#ifdef __ARM_BIG_ENDIAN
	rev	r6, r6
	rev	r8, r8
	rev	r10, r10
#endif

	vdup.32	q0, d24[0]	/* q0-q3 := constant */
	vdup.32	q1, d24[1]
	vdup.32	q2, d25[0]
	vdup.32	q3, d25[1]
	vdup.32	q12, r2		/* q12 := (blkno, blkno, blkno, blkno) */
	vdup.32	q4, d26[0]	/* q4-q11 := (key, key, key, key) */
	vdup.32	q5, d26[1]
	vdup.32	q6, d27[0]
	vdup.32	q7, d27[1]
	vdup.32	q8, d28[0]
	vdup.32	q9, d28[1]
	vdup.32	q10, d29[0]
	vdup.32	q11, d29[1]
	vadd.u32 q12, q12, q15	/* q12 := (blkno,blkno+1,blkno+2,blkno+3) */
	vdup.32	q13, r6		/* q13-q15 := nonce */
	vdup.32	q14, r8
	vdup.32	q15, r10

	b	2f

	_ALIGN_TEXT
1:	ROUNDLD	q0,q1,q2,q3, q5,q6,q7,q4, q10,q11,q8,q9, q15,q12,q13,q14
2:	subs	ip, ip, #2
	ROUND	q0,q1,q2,q3, q4,q5,q6,q7, q8,q9,q10,q11, q12,q13,q14,q15, \
			d16, d24,d25, d26,d27, d28,d29, d30,d31
	ROUNDLD	q0,q1,q2,q3, q4,q5,q6,q7, q8,q9,q10,q11, q12,q13,q14,q15
	ROUND	q0,q1,q2,q3, q5,q6,q7,q4, q10,q11,q8,q9, q15,q12,q13,q14, \
			d20, d30,d31, d24,d25, d26,d27, d28,d29
	bne	1b

	/*
	 * q8-q9 are free / saved on the stack.  Now for the real fun:
	 * in only 16 registers, compute p[i] ^ (y[i] + x[i]) for i in
	 * {0,1,2,...,15}.  The twist is that the p[i] and the y[i] are
	 * transposed from one another, and the x[i] are in general
	 * registers and memory.  See comments in chacha_stream256_neon
	 * for the layout with swaps.
	 */

	sub	r7, r7, #0x10
	vdup.32	q8, r2		/* q8 := (blkno, blkno, blkno, blkno) */
	vld1.32	{q9}, [r7, :128] /* q9 := (0, 1, 2, 3) */

	vzip.32	q0, q1
	vzip.32	q2, q3
	vzip.32	q4, q5
	vzip.32	q6, q7

	vadd.u32 q8, q8, q9	/* q8 := (blkno,blkno+1,blkno+2,blkno+3) */
	vld1.8	{q9}, [r5]	/* q9 := constant */
	vadd.u32 q12, q12, q8	/* q12 += (blkno,blkno+1,blkno+2,blkno+3) */
	vld1.8	{q8}, [r4]!	/* q8 := key[0:16) */

	vswp	d3, d6
	vswp	d9, d12
	vswp	d1, d4
	vswp	d11, d14

	vswp	q1, q4
	vswp	q3, q6

	vadd.u32 q0, q0, q9
	vadd.u32 q4, q4, q9
	vadd.u32 q2, q2, q9
	vadd.u32 q6, q6, q9

	vadd.u32 q1, q1, q8
	vadd.u32 q5, q5, q8
	vadd.u32 q3, q3, q8
	vadd.u32 q7, q7, q8

	vld1.8	{q8-q9}, [r1]!	/* load plaintext bytes [0:32) */

	veor	q0, q0, q8	/* compute ciphertext bytes [0:32) */
	veor	q1, q1, q9

	vld1.8	{q8-q9}, [sp, :256]	/* restore q8-q9 */

	vst1.8	{q0-q1}, [r0]!	/* store ciphertext bytes [0:32) */
	vld1.8	{q0}, [r4]	/* q0 := key[16:32) */
	mov	r3, #0		/* q1 = (0, nonce[0:4), ..., nonce[8:12)) */
	vmov	d2, r3, r6
	vmov	d3, r8, r10

	vzip.32	q8, q9
	vzip.32	q10, q11
	vzip.32	q12, q13
	vzip.32	q14, q15

	vswp	d19, d22
	vswp	d25, d28
	vswp	d17, d20
	vswp	d27, d30

	vswp	q9, q12		/* free up q9 earlier for consecutive q8-q9 */
	vswp	q11, q14

	vadd.u32 q8, q8, q0
	vadd.u32 q12, q12, q0
	vadd.u32 q10, q10, q0
	vadd.u32 q14, q14, q0

	vadd.u32 q9, q9, q1
	vadd.u32 q13, q13, q1
	vadd.u32 q11, q11, q1
	vadd.u32 q15, q15, q1

	vld1.8	{q0-q1}, [r1]!	/* load plaintext bytes [32:64) */

	veor	q0, q0, q8	/* compute ciphertext bytes [32:64) */
	veor	q1, q1, q9

	vld1.8	{q8-q9}, [r1]!	/* load plaintext bytes [64:96) */
	vst1.8	{q0-q1}, [r0]!	/* store ciphertext bytes [32:64) */
	vld1.8	{q0-q1}, [r1]!	/* load plaintext bytes [96:128) */

	veor	q2, q2, q8	/* compute ciphertext bytes [64:96) */
	veor	q3, q3, q9

	vld1.8	{q8-q9}, [r1]!	/* load plaintext bytes [128:160) */
	vst1.8	{q2-q3}, [r0]!	/* store ciphertext bytes [64:80) */

	veor	q10, q10, q0	/* compute ciphertext bytes [96:128) */
	veor	q11, q11, q1

	vld1.8	{q0-q1}, [r1]!	/* load plaintext bytes [160:192) */
	vst1.8	{q10-q11}, [r0]!	/* store ciphertext bytes [80:96) */

	veor	q4, q4, q8	/* compute ciphertext bytes [128:160) */
	veor	q5, q5, q9

	vld1.8	{q8-q9}, [r1]!	/* load plaintext bytes [192:224) */
	vst1.8	{q4-q5}, [r0]!	/* store ciphertext bytes [96:112) */

	veor	q12, q12, q0	/* compute ciphertext bytes [160:192) */
	veor	q13, q13, q1

	vld1.8	{q0-q1}, [r1]	/* load plaintext bytes [224:256) */
	vst1.8	{q12-q13}, [r0]!	/* store ciphertext bytes [112:128) */

	veor	q6, q6, q8	/* compute ciphertext bytes [192:224) */
	veor	q7, q7, q9

	vst1.8	{q6-q7}, [r0]!	/* store ciphertext bytes [192:224) */

	veor	q14, q14, q0	/* compute ciphertext bytes [224:256) */
	veor	q15, q15, q1

	vst1.8	{q14-q15}, [r0]	/* store ciphertext bytes [224:256) */

	/* zero temporary space on the stack */
	vmov.i32 q0, #0
	vmov.i32 q1, #0
	vst1.8	{q0-q1}, [sp, :256]

	/* restore callee-saves registers and stack */
	mov	sp, fp
	vpop	{d8-d15}
	pop	{r4, r5, r6, r7, r8, r10, fp, lr}
	bx	lr
END(chacha_stream_xor256_neon)

	.section .rodata
	.p2align 4
.Lconstants:

	.type	v0123,%object
v0123:
	.long	0, 1, 2, 3
END(v0123)

	.type	rot8,%object
rot8:
	.byte	3,0,1,2, 7,4,5,6
END(rot8)
