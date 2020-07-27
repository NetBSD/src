/*	$NetBSD: chacha_sse2.c,v 1.2 2020/07/27 20:48:18 riastradh Exp $	*/

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

#include <sys/types.h>
#include <sys/endian.h>

#include "immintrin.h"

#include "chacha_sse2.h"

static inline __m128i
rol32(__m128i x, uint8_t n)
{

	return _mm_slli_epi32(x, n) | _mm_srli_epi32(x, 32 - n);
}

static inline void
chacha_permute(__m128i *p0, __m128i *p1, __m128i *p2, __m128i *p3,
    unsigned nr)
{
	__m128i r0, r1, r2, r3;
	__m128i c0, c1, c2, c3;

	r0 = *p0;
	r1 = *p1;
	r2 = *p2;
	r3 = *p3;

	for (; nr > 0; nr -= 2) {
		r0 = _mm_add_epi32(r0, r1); r3 ^= r0; r3 = rol32(r3, 16);
		r2 = _mm_add_epi32(r2, r3); r1 ^= r2; r1 = rol32(r1, 12);
		r0 = _mm_add_epi32(r0, r1); r3 ^= r0; r3 = rol32(r3, 8);
		r2 = _mm_add_epi32(r2, r3); r1 ^= r2; r1 = rol32(r1, 7);

		c0 = r0;
		c1 = _mm_shuffle_epi32(r1, 0x39);
		c2 = _mm_shuffle_epi32(r2, 0x4e);
		c3 = _mm_shuffle_epi32(r3, 0x93);

		c0 = _mm_add_epi32(c0, c1); c3 ^= c0; c3 = rol32(c3, 16);
		c2 = _mm_add_epi32(c2, c3); c1 ^= c2; c1 = rol32(c1, 12);
		c0 = _mm_add_epi32(c0, c1); c3 ^= c0; c3 = rol32(c3, 8);
		c2 = _mm_add_epi32(c2, c3); c1 ^= c2; c1 = rol32(c1, 7);

		r0 = c0;
		r1 = _mm_shuffle_epi32(c1, 0x93);
		r2 = _mm_shuffle_epi32(c2, 0x4e);
		r3 = _mm_shuffle_epi32(c3, 0x39);
	}

	*p0 = r0;
	*p1 = r1;
	*p2 = r2;
	*p3 = r3;
}

void
chacha_core_sse2(uint8_t out[restrict static 64],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{
	__m128i in0, in1, in2, in3;
	__m128i r0, r1, r2, r3;

	r0 = in0 = _mm_loadu_si128((const __m128i *)c);
	r1 = in1 = _mm_loadu_si128((const __m128i *)k);
	r2 = in2 = _mm_loadu_si128((const __m128i *)k + 1);
	r3 = in3 = _mm_loadu_si128((const __m128i *)in);

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	_mm_storeu_si128((__m128i *)out + 0, _mm_add_epi32(r0, in0));
	_mm_storeu_si128((__m128i *)out + 1, _mm_add_epi32(r1, in1));
	_mm_storeu_si128((__m128i *)out + 2, _mm_add_epi32(r2, in2));
	_mm_storeu_si128((__m128i *)out + 3, _mm_add_epi32(r3, in3));
}

void
hchacha_sse2(uint8_t out[restrict static 32],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{
	__m128i r0, r1, r2, r3;

	r0 = _mm_loadu_si128((const __m128i *)c);
	r1 = _mm_loadu_si128((const __m128i *)k);
	r2 = _mm_loadu_si128((const __m128i *)k + 1);
	r3 = _mm_loadu_si128((const __m128i *)in);

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	_mm_storeu_si128((__m128i *)out + 0, r0);
	_mm_storeu_si128((__m128i *)out + 1, r3);
}

#define	CHACHA_QUARTERROUND(a, b, c, d) do				      \
{									      \
	(a) = _mm_add_epi32((a), (b)); (d) ^= a; (d) = rol32((d), 16);	      \
	(c) = _mm_add_epi32((c), (d)); (b) ^= c; (b) = rol32((b), 12);	      \
	(a) = _mm_add_epi32((a), (b)); (d) ^= a; (d) = rol32((d), 8);	      \
	(c) = _mm_add_epi32((c), (d)); (b) ^= c; (b) = rol32((b), 7);	      \
} while (/*CONSTCOND*/0)

static inline __m128i
load1_epi32(const void *p)
{
	return (__m128i)_mm_load1_ps(p);
}

static inline __m128i
loadu_epi32(const void *p)
{
	return _mm_loadu_si128(p);
}

static inline void
storeu_epi32(void *p, __m128i v)
{
	return _mm_storeu_si128(p, v);
}

static inline __m128i
unpack0_epi32(__m128i a, __m128i b, __m128i c, __m128i d)
{
	__m128 lo = (__m128)_mm_unpacklo_epi32(a, b); /* (a[0], b[0], ...) */
	__m128 hi = (__m128)_mm_unpacklo_epi32(c, d); /* (c[0], d[0], ...) */

	/* (lo[0]=a[0], lo[1]=b[0], hi[0]=c[0], hi[1]=d[0]) */
	return (__m128i)_mm_movelh_ps(lo, hi);
}

static inline __m128i
unpack1_epi32(__m128i a, __m128i b, __m128i c, __m128i d)
{
	__m128 lo = (__m128)_mm_unpacklo_epi32(a, b); /* (..., a[1], b[1]) */
	__m128 hi = (__m128)_mm_unpacklo_epi32(c, d); /* (..., c[1], d[1]) */

	/* (lo[2]=a[1], lo[3]=b[1], hi[2]=c[1], hi[3]=d[1]) */
	return (__m128i)_mm_movehl_ps(hi, lo);
}

static inline __m128i
unpack2_epi32(__m128i a, __m128i b, __m128i c, __m128i d)
{
	__m128 lo = (__m128)_mm_unpackhi_epi32(a, b); /* (a[2], b[2], ...) */
	__m128 hi = (__m128)_mm_unpackhi_epi32(c, d); /* (c[2], d[2], ...) */

	/* (lo[0]=a[2], lo[1]=b[2], hi[0]=c[2], hi[1]=d[2]) */
	return (__m128i)_mm_movelh_ps(lo, hi);
}

static inline __m128i
unpack3_epi32(__m128i a, __m128i b, __m128i c, __m128i d)
{
	__m128 lo = (__m128)_mm_unpackhi_epi32(a, b); /* (..., a[3], b[3]) */
	__m128 hi = (__m128)_mm_unpackhi_epi32(c, d); /* (..., c[3], d[3]) */

	/* (lo[2]=a[3], lo[3]=b[3], hi[2]=c[3], hi[3]=d[3]) */
	return (__m128i)_mm_movehl_ps(hi, lo);
}

void
chacha_stream_sse2(uint8_t *restrict s, size_t n,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{
	__m128i x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15;
	__m128i y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;
	__m128i z0,z1,z2,z3,z4,z5,z6,z7,z8,z9,z10,z11,z12,z13,z14,z15;
	unsigned r;

	if (n < 256)
		goto out;

	x0 = load1_epi32(chacha_const32 + 0);
	x1 = load1_epi32(chacha_const32 + 4);
	x2 = load1_epi32(chacha_const32 + 8);
	x3 = load1_epi32(chacha_const32 + 12);
	x4 = load1_epi32(k + 0);
	x5 = load1_epi32(k + 4);
	x6 = load1_epi32(k + 8);
	x7 = load1_epi32(k + 12);
	x8 = load1_epi32(k + 16);
	x9 = load1_epi32(k + 20);
	x10 = load1_epi32(k + 24);
	x11 = load1_epi32(k + 28);
	/* x12 set in the loop */
	x13 = load1_epi32(nonce + 0);
	x14 = load1_epi32(nonce + 4);
	x15 = load1_epi32(nonce + 8);

	for (; n >= 256; s += 256, n -= 256, blkno += 4) {
		x12 = _mm_add_epi32(_mm_set1_epi32(blkno),
		    _mm_set_epi32(3,2,1,0));
		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (r = nr; r > 0; r -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		y0 = _mm_add_epi32(y0, x0);
		y1 = _mm_add_epi32(y1, x1);
		y2 = _mm_add_epi32(y2, x2);
		y3 = _mm_add_epi32(y3, x3);
		y4 = _mm_add_epi32(y4, x4);
		y5 = _mm_add_epi32(y5, x5);
		y6 = _mm_add_epi32(y6, x6);
		y7 = _mm_add_epi32(y7, x7);
		y8 = _mm_add_epi32(y8, x8);
		y9 = _mm_add_epi32(y9, x9);
		y10 = _mm_add_epi32(y10, x10);
		y11 = _mm_add_epi32(y11, x11);
		y12 = _mm_add_epi32(y12, x12);
		y13 = _mm_add_epi32(y13, x13);
		y14 = _mm_add_epi32(y14, x14);
		y15 = _mm_add_epi32(y15, x15);

		z0 = unpack0_epi32(y0, y1, y2, y3);
		z1 = unpack0_epi32(y4, y5, y6, y7);
		z2 = unpack0_epi32(y8, y9, y10, y11);
		z3 = unpack0_epi32(y12, y13, y14, y15);
		z4 = unpack1_epi32(y0, y1, y2, y3);
		z5 = unpack1_epi32(y4, y5, y6, y7);
		z6 = unpack1_epi32(y8, y9, y10, y11);
		z7 = unpack1_epi32(y12, y13, y14, y15);
		z8 = unpack2_epi32(y0, y1, y2, y3);
		z9 = unpack2_epi32(y4, y5, y6, y7);
		z10 = unpack2_epi32(y8, y9, y10, y11);
		z11 = unpack2_epi32(y12, y13, y14, y15);
		z12 = unpack3_epi32(y0, y1, y2, y3);
		z13 = unpack3_epi32(y4, y5, y6, y7);
		z14 = unpack3_epi32(y8, y9, y10, y11);
		z15 = unpack3_epi32(y12, y13, y14, y15);

		storeu_epi32(s + 16*0, z0);
		storeu_epi32(s + 16*1, z1);
		storeu_epi32(s + 16*2, z2);
		storeu_epi32(s + 16*3, z3);
		storeu_epi32(s + 16*4, z4);
		storeu_epi32(s + 16*5, z5);
		storeu_epi32(s + 16*6, z6);
		storeu_epi32(s + 16*7, z7);
		storeu_epi32(s + 16*8, z8);
		storeu_epi32(s + 16*9, z9);
		storeu_epi32(s + 16*10, z10);
		storeu_epi32(s + 16*11, z11);
		storeu_epi32(s + 16*12, z12);
		storeu_epi32(s + 16*13, z13);
		storeu_epi32(s + 16*14, z14);
		storeu_epi32(s + 16*15, z15);
	}

out:	if (n) {
		const __m128i blkno_inc = _mm_set_epi32(0,0,0,1);
		__m128i in0, in1, in2, in3;
		__m128i r0, r1, r2, r3;

		in0 = _mm_loadu_si128((const __m128i *)chacha_const32);
		in1 = _mm_loadu_si128((const __m128i *)k);
		in2 = _mm_loadu_si128((const __m128i *)k + 1);
		in3 = _mm_set_epi32(le32dec(nonce + 8), le32dec(nonce + 4),
		    le32dec(nonce), blkno);

		for (; n; s += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = _mm_add_epi32(r0, in0);
			r1 = _mm_add_epi32(r1, in1);
			r2 = _mm_add_epi32(r2, in2);
			r3 = _mm_add_epi32(r3, in3);

			if (n < 64) {
				uint8_t buf[64] __aligned(16);

				_mm_storeu_si128((__m128i *)buf + 0, r0);
				_mm_storeu_si128((__m128i *)buf + 1, r1);
				_mm_storeu_si128((__m128i *)buf + 2, r2);
				_mm_storeu_si128((__m128i *)buf + 3, r3);
				memcpy(s, buf, n);

				break;
			}

			_mm_storeu_si128((__m128i *)s + 0, r0);
			_mm_storeu_si128((__m128i *)s + 1, r1);
			_mm_storeu_si128((__m128i *)s + 2, r2);
			_mm_storeu_si128((__m128i *)s + 3, r3);
			in3 = _mm_add_epi32(in3, blkno_inc);
		}
	}
}

void
chacha_stream_xor_sse2(uint8_t *s, const uint8_t *p, size_t n,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{
	__m128i x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15;
	__m128i y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;
	__m128i z0,z1,z2,z3,z4,z5,z6,z7,z8,z9,z10,z11,z12,z13,z14,z15;
	unsigned r;

	if (n < 256)
		goto out;

	x0 = load1_epi32(chacha_const32 + 0);
	x1 = load1_epi32(chacha_const32 + 4);
	x2 = load1_epi32(chacha_const32 + 8);
	x3 = load1_epi32(chacha_const32 + 12);
	x4 = load1_epi32(k + 0);
	x5 = load1_epi32(k + 4);
	x6 = load1_epi32(k + 8);
	x7 = load1_epi32(k + 12);
	x8 = load1_epi32(k + 16);
	x9 = load1_epi32(k + 20);
	x10 = load1_epi32(k + 24);
	x11 = load1_epi32(k + 28);
	/* x12 set in the loop */
	x13 = load1_epi32(nonce + 0);
	x14 = load1_epi32(nonce + 4);
	x15 = load1_epi32(nonce + 8);

	for (; n >= 256; s += 256, p += 256, n -= 256, blkno += 4) {
		x12 = _mm_add_epi32(_mm_set1_epi32(blkno),
		    _mm_set_epi32(3,2,1,0));
		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (r = nr; r > 0; r -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		y0 = _mm_add_epi32(y0, x0);
		y1 = _mm_add_epi32(y1, x1);
		y2 = _mm_add_epi32(y2, x2);
		y3 = _mm_add_epi32(y3, x3);
		y4 = _mm_add_epi32(y4, x4);
		y5 = _mm_add_epi32(y5, x5);
		y6 = _mm_add_epi32(y6, x6);
		y7 = _mm_add_epi32(y7, x7);
		y8 = _mm_add_epi32(y8, x8);
		y9 = _mm_add_epi32(y9, x9);
		y10 = _mm_add_epi32(y10, x10);
		y11 = _mm_add_epi32(y11, x11);
		y12 = _mm_add_epi32(y12, x12);
		y13 = _mm_add_epi32(y13, x13);
		y14 = _mm_add_epi32(y14, x14);
		y15 = _mm_add_epi32(y15, x15);

		z0 = unpack0_epi32(y0, y1, y2, y3);
		z1 = unpack0_epi32(y4, y5, y6, y7);
		z2 = unpack0_epi32(y8, y9, y10, y11);
		z3 = unpack0_epi32(y12, y13, y14, y15);
		z4 = unpack1_epi32(y0, y1, y2, y3);
		z5 = unpack1_epi32(y4, y5, y6, y7);
		z6 = unpack1_epi32(y8, y9, y10, y11);
		z7 = unpack1_epi32(y12, y13, y14, y15);
		z8 = unpack2_epi32(y0, y1, y2, y3);
		z9 = unpack2_epi32(y4, y5, y6, y7);
		z10 = unpack2_epi32(y8, y9, y10, y11);
		z11 = unpack2_epi32(y12, y13, y14, y15);
		z12 = unpack3_epi32(y0, y1, y2, y3);
		z13 = unpack3_epi32(y4, y5, y6, y7);
		z14 = unpack3_epi32(y8, y9, y10, y11);
		z15 = unpack3_epi32(y12, y13, y14, y15);

		storeu_epi32(s + 16*0, loadu_epi32(p + 16*0) ^ z0);
		storeu_epi32(s + 16*1, loadu_epi32(p + 16*1) ^ z1);
		storeu_epi32(s + 16*2, loadu_epi32(p + 16*2) ^ z2);
		storeu_epi32(s + 16*3, loadu_epi32(p + 16*3) ^ z3);
		storeu_epi32(s + 16*4, loadu_epi32(p + 16*4) ^ z4);
		storeu_epi32(s + 16*5, loadu_epi32(p + 16*5) ^ z5);
		storeu_epi32(s + 16*6, loadu_epi32(p + 16*6) ^ z6);
		storeu_epi32(s + 16*7, loadu_epi32(p + 16*7) ^ z7);
		storeu_epi32(s + 16*8, loadu_epi32(p + 16*8) ^ z8);
		storeu_epi32(s + 16*9, loadu_epi32(p + 16*9) ^ z9);
		storeu_epi32(s + 16*10, loadu_epi32(p + 16*10) ^ z10);
		storeu_epi32(s + 16*11, loadu_epi32(p + 16*11) ^ z11);
		storeu_epi32(s + 16*12, loadu_epi32(p + 16*12) ^ z12);
		storeu_epi32(s + 16*13, loadu_epi32(p + 16*13) ^ z13);
		storeu_epi32(s + 16*14, loadu_epi32(p + 16*14) ^ z14);
		storeu_epi32(s + 16*15, loadu_epi32(p + 16*15) ^ z15);
	}

out:	if (n) {
		const __m128i blkno_inc = _mm_set_epi32(0,0,0,1);
		__m128i in0, in1, in2, in3;
		__m128i r0, r1, r2, r3;

		in0 = _mm_loadu_si128((const __m128i *)chacha_const32);
		in1 = _mm_loadu_si128((const __m128i *)k);
		in2 = _mm_loadu_si128((const __m128i *)k + 1);
		in3 = _mm_set_epi32(le32dec(nonce + 8), le32dec(nonce + 4),
		    le32dec(nonce), blkno);

		for (; n; s += 64, p += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = _mm_add_epi32(r0, in0);
			r1 = _mm_add_epi32(r1, in1);
			r2 = _mm_add_epi32(r2, in2);
			r3 = _mm_add_epi32(r3, in3);

			if (n < 64) {
				uint8_t buf[64] __aligned(16);
				unsigned i;

				_mm_storeu_si128((__m128i *)buf + 0, r0);
				_mm_storeu_si128((__m128i *)buf + 1, r1);
				_mm_storeu_si128((__m128i *)buf + 2, r2);
				_mm_storeu_si128((__m128i *)buf + 3, r3);

				for (i = 0; i < n - n%4; i += 4)
					le32enc(s + i,
					    le32dec(p + i) ^ le32dec(buf + i));
				for (; i < n; i++)
					s[i] = p[i] ^ buf[i];

				break;
			}

			r0 ^= _mm_loadu_si128((const __m128i *)p + 0);
			r1 ^= _mm_loadu_si128((const __m128i *)p + 1);
			r2 ^= _mm_loadu_si128((const __m128i *)p + 2);
			r3 ^= _mm_loadu_si128((const __m128i *)p + 3);
			_mm_storeu_si128((__m128i *)s + 0, r0);
			_mm_storeu_si128((__m128i *)s + 1, r1);
			_mm_storeu_si128((__m128i *)s + 2, r2);
			_mm_storeu_si128((__m128i *)s + 3, r3);
			in3 = _mm_add_epi32(in3, blkno_inc);
		}
	}
}

void
xchacha_stream_sse2(uint8_t *restrict s, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t k[static 32],
    unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_sse2(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_sse2(s, nbytes, blkno, subnonce, subkey, nr);
}

void
xchacha_stream_xor_sse2(uint8_t *restrict c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t k[static 32],
    unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_sse2(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_xor_sse2(c, p, nbytes, blkno, subnonce, subkey, nr);
}
