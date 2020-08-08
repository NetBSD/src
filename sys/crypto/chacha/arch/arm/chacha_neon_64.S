/*	$NetBSD: chacha_neon_64.S,v 1.6 2020/08/08 14:47:01 riastradh Exp $	*/

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

#include <aarch64/asm.h>

RCSID("$NetBSD: chacha_neon_64.S,v 1.6 2020/08/08 14:47:01 riastradh Exp $")

#define	ROUND(a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r) \
STEP(STEP0,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP1,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP2,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP3,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP4,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP5,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP6,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP7,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP8,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP9,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);   \
STEP(STEP10,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP11,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP12,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP13,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP14,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP15,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP16,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP17,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP18,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
STEP(STEP19,a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,t0,t1,t2,t3, r);  \
/* end ROUND */

#define	STEP(f,a0,a1,a2,a3,b0,b1,b2,b3,c0,c1,c2,c3,d0,d1,d2,d3,t0,t1,t2,t3,r) \
	f(a0,b0,c0,d0, t0, r);						      \
	f(a1,b1,c1,d1, t1, r);						      \
	f(a2,b2,c2,d2, t2, r);						      \
	f(a3,b3,c3,d3, t3, r);						      \
	/* end of STEP */

/*
 * Each step of the ChaCha quarterround, split up so we can interleave
 * the quarterrounds on independent rows/diagonals to maximize pipeline
 * efficiency.  Reference:
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
 *	<<< 16		REV32 Vn.8h for 16,
 *	<<< 12		SHL/SRI/ORR (shift left, shift right and insert, OR)
 *	<<< 8		TBL (general permutation; rot8 below stored in r)
 *	<<< 7		SHL/SRI/ORR
 */
#define	STEP0(a,b,c,d, t, r)	add	a##.4s, a##.4s, b##.4s
#define	STEP1(a,b,c,d, t, r)	eor	d##.16b, d##.16b, a##.16b
#if 0
#define	STEP2(a,b,c,d, t, r)	shl	t##.4s, d##.4s, #16
#define	STEP3(a,b,c,d, t, r)	ushr	d##.4s, d##.4s, #(32 - 16)
#define	STEP4(a,b,c,d, t, r)	orr	d##.16b, d##.16b, t##.16b
#else
#define	STEP2(a,b,c,d, t, r)	rev32	d##.8h, d##.8h
#define	STEP3(a,b,c,d, t, r)	/* nothing */
#define	STEP4(a,b,c,d, t, r)	/* nothing */
#endif

#define	STEP5(a,b,c,d, t, r)	add	c##.4s, c##.4s, d##.4s
#if 0
#define	STEP6(a,b,c,d, t, r)	eor	b##.16b, b##.16b, c##.16b
#define	STEP7(a,b,c,d, t, r)	shl	t##.4s, b##.4s, #12
#define	STEP8(a,b,c,d, t, r)	ushr	b##.4s, b##.4s, #(32 - 12)
#define	STEP9(a,b,c,d, t, r)	orr	b##.16b, b##.16b, t##.16b
#else
#define	STEP6(a,b,c,d, t, r)	eor	t##.16b, b##.16b, c##.16b
#define	STEP7(a,b,c,d, t, r)	shl	b##.4s, t##.4s, #12
#define	STEP8(a,b,c,d, t, r)	sri	b##.4s, t##.4s, #(32 - 12)
#define	STEP9(a,b,c,d, t, r)	/* nothing */
#endif

#define	STEP10(a,b,c,d, t, r)	add	a##.4s, a##.4s, b##.4s
#define	STEP11(a,b,c,d, t, r)	eor	d##.16b, d##.16b, a##.16b
#if 0
#define	STEP12(a,b,c,d, t, r)	shl	t##.4s, d##.4s, #8
#define	STEP13(a,b,c,d, t, r)	ushr	d##.4s, d##.4s, #(32 - 8)
#define	STEP14(a,b,c,d, t, r)	orr	d##.16b, d##.16b, t##.16b
#else
#define	STEP12(a,b,c,d, t, r)	tbl	d##.16b, {d##.16b}, r##.16b
#define	STEP13(a,b,c,d, t, r)	/* nothing */
#define	STEP14(a,b,c,d, t, r)	/* nothing */
#endif

#define	STEP15(a,b,c,d, t, r)	add	c##.4s, c##.4s, d##.4s
#if 0
#define	STEP16(a,b,c,d, t, r)	eor	b##.16b, b##.16b, c##.16b
#define	STEP17(a,b,c,d, t, r)	shl	t##.4s, b##.4s, #7
#define	STEP18(a,b,c,d, t, r)	ushr	b##.4s, b##.4s, #(32 - 7)
#define	STEP19(a,b,c,d, t, r)	orr	b##.16b, b##.16b, t##.16b
#else
#define	STEP16(a,b,c,d, t, r)	eor	t##.16b, b##.16b, c##.16b
#define	STEP17(a,b,c,d, t, r)	shl	b##.4s, t##.4s, #7
#define	STEP18(a,b,c,d, t, r)	sri	b##.4s, t##.4s, #(32 - 7)
#define	STEP19(a,b,c,d, t, r)	/* nothing */
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	HTOLE32(x)
#define	LE32TOH(x)
#elif _BYTE_ORDER == _BIG_ENDIAN
#define	HTOLE32(x)	rev32	x, x
#define	LE32TOH(x)	rev32	x, x
#endif

/*
 * chacha_stream256_neon(uint8_t s[256]@x0,
 *     uint32_t blkno@w1,
 *     const uint8_t nonce[12]@x2,
 *     const uint8_t key[32]@x3,
 *     const uint8_t const[16]@x4,
 *     unsigned nr@w5)
 */
ENTRY(chacha_stream256_neon)
	stp	fp, lr, [sp, #-0x50]!	/* push stack frame with uint64[8] */
	mov	fp, sp

	stp	d8, d9, [sp, #0x10]	/* save callee-saves vectors */
	stp	d10, d11, [sp, #0x20]
	stp	d12, d13, [sp, #0x30]
	stp	d14, d15, [sp, #0x40]

	adrl	x9, v0123	/* x9 := &v0123 */
	mov	x10, x4		/* r10 := c */
	mov	x11, x3		/* r11 := k */
	add	x12, x3, #16	/* r12 := k+4 */
	mov	x13, x2		/* r13 := nonce */

	ld1	{v26.4s-v27.4s}, [x9]	/* v26 := v0123, v27 := rot8 */
	dup	v12.4s, w1	/* v12 := (blkno, blkno, blkno, blkno) */
	ld4r	{v0.4s-v3.4s}, [x10]	/* (v0,v1,v2,v3) := constant */
	ld4r	{v4.4s-v7.4s}, [x11]	/* (v4,v5,v6,v7) := key[0:16) */
	ld4r	{v8.4s-v11.4s}, [x12]	/* (v8,v9,v10,v11) := key[16:32) */
	ld3r	{v13.4s-v15.4s}, [x13]	/* (v13,v14,v15) := nonce */
	add	v12.4s, v12.4s, v26.4s	/* v12 := blkno + (0,1,2,3) */

	LE32TOH(v0.16b)
	LE32TOH(v1.16b)
	LE32TOH(v2.16b)
	LE32TOH(v3.16b)
	LE32TOH(v4.16b)
	LE32TOH(v5.16b)
	LE32TOH(v6.16b)
	LE32TOH(v7.16b)
	LE32TOH(v8.16b)
	LE32TOH(v9.16b)
	LE32TOH(v10.16b)
	LE32TOH(v11.16b)
	/* LE32TOH(v12.16b) -- blkno, already host order */
	LE32TOH(v13.16b)
	LE32TOH(v14.16b)
	LE32TOH(v15.16b)

	mov	v16.16b, v0.16b
	mov	v17.16b, v1.16b
	mov	v18.16b, v2.16b
	mov	v19.16b, v3.16b
	mov	v20.16b, v4.16b
	mov	v21.16b, v5.16b
	mov	v22.16b, v6.16b
	mov	v23.16b, v7.16b
	mov	v24.16b, v8.16b
	mov	v25.16b, v9.16b
	mov	v26.16b, v12.16b	/* reordered since v12 isn't dup */
	mov	w8, v10.s[0]		/* v27-31 needed as temporaries */
	mov	w9, v11.s[0]
	mov	w10, v13.s[0]
	mov	w11, v14.s[0]
	mov	w12, v15.s[0]

	_ALIGN_TEXT
1:	subs	w5, w5, #2
	ROUND(v0,v1,v2,v3, v4,v5,v6,v7, v8,v9,v10,v11, v12,v13,v14,v15,
	    v28,v29,v30,v31, v27)
	ROUND(v0,v1,v2,v3, v5,v6,v7,v4, v10,v11,v8,v9, v15,v12,v13,v14,
	    v28,v29,v30,v31, v27)
	b.ne	1b

	dup	v27.4s, w8
	dup	v28.4s, w9
	dup	v29.4s, w10
	dup	v30.4s, w11
	dup	v31.4s, w12

	add	v0.4s, v0.4s, v16.4s
	add	v1.4s, v1.4s, v17.4s
	add	v2.4s, v2.4s, v18.4s
	add	v3.4s, v3.4s, v19.4s
	add	v4.4s, v4.4s, v20.4s
	add	v5.4s, v5.4s, v21.4s
	add	v6.4s, v6.4s, v22.4s
	add	v7.4s, v7.4s, v23.4s
	add	v8.4s, v8.4s, v24.4s
	add	v9.4s, v9.4s, v25.4s
	add	v10.4s, v10.4s, v27.4s	/* reordered since v12 isn't dup */
	add	v11.4s, v11.4s, v28.4s
	add	v12.4s, v12.4s, v26.4s
	add	v13.4s, v13.4s, v29.4s
	add	v14.4s, v14.4s, v30.4s
	add	v15.4s, v15.4s, v31.4s

	HTOLE32(v0.16b)
	HTOLE32(v1.16b)
	HTOLE32(v2.16b)
	HTOLE32(v3.16b)
	HTOLE32(v4.16b)
	HTOLE32(v5.16b)
	HTOLE32(v6.16b)
	HTOLE32(v7.16b)
	HTOLE32(v8.16b)
	HTOLE32(v9.16b)
	HTOLE32(v10.16b)
	HTOLE32(v11.16b)
	HTOLE32(v12.16b)
	HTOLE32(v13.16b)
	HTOLE32(v14.16b)
	HTOLE32(v15.16b)

	st4	{ v0.s, v1.s, v2.s, v3.s}[0], [x0], #16
	st4	{ v4.s, v5.s, v6.s, v7.s}[0], [x0], #16
	st4	{ v8.s, v9.s,v10.s,v11.s}[0], [x0], #16
	st4	{v12.s,v13.s,v14.s,v15.s}[0], [x0], #16
	st4	{ v0.s, v1.s, v2.s, v3.s}[1], [x0], #16
	st4	{ v4.s, v5.s, v6.s, v7.s}[1], [x0], #16
	st4	{ v8.s, v9.s,v10.s,v11.s}[1], [x0], #16
	st4	{v12.s,v13.s,v14.s,v15.s}[1], [x0], #16
	st4	{ v0.s, v1.s, v2.s, v3.s}[2], [x0], #16
	st4	{ v4.s, v5.s, v6.s, v7.s}[2], [x0], #16
	st4	{ v8.s, v9.s,v10.s,v11.s}[2], [x0], #16
	st4	{v12.s,v13.s,v14.s,v15.s}[2], [x0], #16
	st4	{ v0.s, v1.s, v2.s, v3.s}[3], [x0], #16
	st4	{ v4.s, v5.s, v6.s, v7.s}[3], [x0], #16
	st4	{ v8.s, v9.s,v10.s,v11.s}[3], [x0], #16
	st4	{v12.s,v13.s,v14.s,v15.s}[3], [x0], #16

	ldp	d8, d9, [sp, #0x10]	/* restore callee-saves vectors */
	ldp	d10, d11, [sp, #0x20]
	ldp	d12, d13, [sp, #0x30]
	ldp	d14, d15, [sp, #0x40]

	ldp	fp, lr, [sp], #0x50	/* pop stack frame with uint64[8] */
	ret
END(chacha_stream256_neon)

/*
 * chacha_stream_xor256_neon(uint8_t s[256]@x0, const uint8_t p[256]@x1,
 *     uint32_t blkno@w2,
 *     const uint8_t nonce[12]@x3,
 *     const uint8_t key[32]@x4,
 *     const uint8_t const[16]@x5,
 *     unsigned nr@w6)
 */
ENTRY(chacha_stream_xor256_neon)
	stp	fp, lr, [sp, #-0x50]!	/* push stack frame with uint64[8] */
	mov	fp, sp

	stp	d8, d9, [sp, #0x10]	/* save callee-saves vectors */
	stp	d10, d11, [sp, #0x20]
	stp	d12, d13, [sp, #0x30]
	stp	d14, d15, [sp, #0x40]

	adrl	x9, v0123	/* x9 := &v0123 */
	mov	x10, x5		/* r10 := c */
	mov	x11, x4		/* r11 := k */
	add	x12, x4, #16	/* r12 := k+4 */
	mov	x13, x3		/* r13 := nonce */

	ld1	{v26.4s-v27.4s}, [x9]	/* v26 := v0123, v27 := rot8 */
	dup	v12.4s, w2	/* v12 := (blkno, blkno, blkno, blkno) */
	ld4r	{v0.4s-v3.4s}, [x10]	/* (v0,v1,v2,v3) := constant */
	ld4r	{v4.4s-v7.4s}, [x11]	/* (v4,v5,v6,v7) := key[0:16) */
	ld4r	{v8.4s-v11.4s}, [x12]	/* (v8,v9,v10,v11) := key[16:32) */
	ld3r	{v13.4s-v15.4s}, [x13]	/* (v13,v14,v15) := nonce */
	add	v12.4s, v12.4s, v26.4s	/* v12 := blkno + (0,1,2,3) */

	LE32TOH(v0.16b)
	LE32TOH(v1.16b)
	LE32TOH(v2.16b)
	LE32TOH(v3.16b)
	LE32TOH(v4.16b)
	LE32TOH(v5.16b)
	LE32TOH(v6.16b)
	LE32TOH(v7.16b)
	LE32TOH(v8.16b)
	LE32TOH(v9.16b)
	LE32TOH(v10.16b)
	LE32TOH(v11.16b)
	/* LE32TOH(v12.16b) -- blkno, already host order */
	LE32TOH(v13.16b)
	LE32TOH(v14.16b)
	LE32TOH(v15.16b)

	mov	v16.16b, v0.16b
	mov	v17.16b, v1.16b
	mov	v18.16b, v2.16b
	mov	v19.16b, v3.16b
	mov	v20.16b, v4.16b
	mov	v21.16b, v5.16b
	mov	v22.16b, v6.16b
	mov	v23.16b, v7.16b
	mov	v24.16b, v8.16b
	mov	v25.16b, v9.16b
	mov	v26.16b, v12.16b	/* reordered since v12 isn't dup */
	mov	w8, v10.s[0]		/* v27-31 needed as temporaries */
	mov	w9, v11.s[0]
	mov	w10, v13.s[0]
	mov	w11, v14.s[0]
	mov	w12, v15.s[0]

        _ALIGN_TEXT
1:	subs	w6, w6, #2
	ROUND(v0,v1,v2,v3, v4,v5,v6,v7, v8,v9,v10,v11, v12,v13,v14,v15,
	    v28,v29,v30,v31, v27)
	ROUND(v0,v1,v2,v3, v5,v6,v7,v4, v10,v11,v8,v9, v15,v12,v13,v14,
	    v28,v29,v30,v31, v27)
	b.ne	1b

	dup	v27.4s, w8
	dup	v28.4s, w9
	dup	v29.4s, w10
	dup	v30.4s, w11
	dup	v31.4s, w12

	add	v0.4s, v0.4s, v16.4s
	add	v1.4s, v1.4s, v17.4s
	add	v2.4s, v2.4s, v18.4s
	add	v3.4s, v3.4s, v19.4s
	add	v4.4s, v4.4s, v20.4s
	add	v5.4s, v5.4s, v21.4s
	add	v6.4s, v6.4s, v22.4s
	add	v7.4s, v7.4s, v23.4s
	add	v8.4s, v8.4s, v24.4s
	add	v9.4s, v9.4s, v25.4s
	add	v10.4s, v10.4s, v27.4s	/* reordered since v12 isn't dup */
	add	v11.4s, v11.4s, v28.4s
	add	v12.4s, v12.4s, v26.4s
	add	v13.4s, v13.4s, v29.4s
	add	v14.4s, v14.4s, v30.4s
	add	v15.4s, v15.4s, v31.4s

	/*
	 * We could do these sixteen LD4-into-lane instructions instead
	 * by four LD1-into-register instructions, but we would need to
	 * permute the elements in v0-v15 to put them in the right
	 * order.  We can do that by a series of ZIP1/ZIP2 on 4s-sized
	 * elements, and then ZIP1/ZIP2 on 2d-sized elements, but the
	 * net cost of the thirty-two ZIP1/ZIP2 instructions seems to
	 * exceed the savings in cost from four LD1 instructions rather
	 * than sixteen LD4 instructions, even if we interleave the LD1
	 * instructions with the ZIPs.
	 */
	ld4	{v16.s,v17.s,v18.s,v19.s}[0], [x1], #16
	ld4	{v20.s,v21.s,v22.s,v23.s}[0], [x1], #16
	ld4	{v24.s,v25.s,v26.s,v27.s}[0], [x1], #16
	ld4	{v28.s,v29.s,v30.s,v31.s}[0], [x1], #16
	ld4	{v16.s,v17.s,v18.s,v19.s}[1], [x1], #16
	ld4	{v20.s,v21.s,v22.s,v23.s}[1], [x1], #16
	ld4	{v24.s,v25.s,v26.s,v27.s}[1], [x1], #16
	ld4	{v28.s,v29.s,v30.s,v31.s}[1], [x1], #16
	ld4	{v16.s,v17.s,v18.s,v19.s}[2], [x1], #16
	ld4	{v20.s,v21.s,v22.s,v23.s}[2], [x1], #16
	ld4	{v24.s,v25.s,v26.s,v27.s}[2], [x1], #16
	ld4	{v28.s,v29.s,v30.s,v31.s}[2], [x1], #16
	ld4	{v16.s,v17.s,v18.s,v19.s}[3], [x1], #16
	ld4	{v20.s,v21.s,v22.s,v23.s}[3], [x1], #16
	ld4	{v24.s,v25.s,v26.s,v27.s}[3], [x1], #16
	ld4	{v28.s,v29.s,v30.s,v31.s}[3], [x1], #16

	HTOLE32(v0.16b)
	HTOLE32(v1.16b)
	HTOLE32(v2.16b)
	HTOLE32(v3.16b)
	HTOLE32(v4.16b)
	HTOLE32(v5.16b)
	HTOLE32(v6.16b)
	HTOLE32(v7.16b)
	HTOLE32(v8.16b)
	HTOLE32(v9.16b)
	HTOLE32(v10.16b)
	HTOLE32(v11.16b)
	HTOLE32(v12.16b)
	HTOLE32(v13.16b)
	HTOLE32(v14.16b)
	HTOLE32(v15.16b)

	eor	v16.16b, v16.16b, v0.16b
	eor	v17.16b, v17.16b, v1.16b
	eor	v18.16b, v18.16b, v2.16b
	eor	v19.16b, v19.16b, v3.16b
	eor	v20.16b, v20.16b, v4.16b
	eor	v21.16b, v21.16b, v5.16b
	eor	v22.16b, v22.16b, v6.16b
	eor	v23.16b, v23.16b, v7.16b
	eor	v24.16b, v24.16b, v8.16b
	eor	v25.16b, v25.16b, v9.16b
	eor	v26.16b, v26.16b, v10.16b
	eor	v27.16b, v27.16b, v11.16b
	eor	v28.16b, v28.16b, v12.16b
	eor	v29.16b, v29.16b, v13.16b
	eor	v30.16b, v30.16b, v14.16b
	eor	v31.16b, v31.16b, v15.16b

	st4	{v16.s,v17.s,v18.s,v19.s}[0], [x0], #16
	st4	{v20.s,v21.s,v22.s,v23.s}[0], [x0], #16
	st4	{v24.s,v25.s,v26.s,v27.s}[0], [x0], #16
	st4	{v28.s,v29.s,v30.s,v31.s}[0], [x0], #16
	st4	{v16.s,v17.s,v18.s,v19.s}[1], [x0], #16
	st4	{v20.s,v21.s,v22.s,v23.s}[1], [x0], #16
	st4	{v24.s,v25.s,v26.s,v27.s}[1], [x0], #16
	st4	{v28.s,v29.s,v30.s,v31.s}[1], [x0], #16
	st4	{v16.s,v17.s,v18.s,v19.s}[2], [x0], #16
	st4	{v20.s,v21.s,v22.s,v23.s}[2], [x0], #16
	st4	{v24.s,v25.s,v26.s,v27.s}[2], [x0], #16
	st4	{v28.s,v29.s,v30.s,v31.s}[2], [x0], #16
	st4	{v16.s,v17.s,v18.s,v19.s}[3], [x0], #16
	st4	{v20.s,v21.s,v22.s,v23.s}[3], [x0], #16
	st4	{v24.s,v25.s,v26.s,v27.s}[3], [x0], #16
	st4	{v28.s,v29.s,v30.s,v31.s}[3], [x0], #16

	ldp	d8, d9, [sp, #0x10]	/* restore callee-saves vectors */
	ldp	d10, d11, [sp, #0x20]
	ldp	d12, d13, [sp, #0x30]
	ldp	d14, d15, [sp, #0x40]

	ldp	fp, lr, [sp], #0x50	/* pop stack frame with uint64[8] */
	ret
END(chacha_stream_xor256_neon)

	.section .rodata
	.p2align 4

	.type	v0123,@object
v0123:
	.long	0, 1, 2, 3
END(v0123)

	/*
	 * Must be immediately after v0123 -- we load them in a single
	 * ld1 instruction.
	 */
	.type	rot8,@object
rot8:
	.long	0x02010003, 0x06050407, 0x0a09080b, 0x0e0d0c0f
END(rot8)
