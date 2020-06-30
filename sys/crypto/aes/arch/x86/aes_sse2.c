/*
 * Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_sse2.c,v 1.2 2020/06/30 20:32:11 riastradh Exp $");

#include <sys/types.h>

#ifdef _KERNEL
#include <lib/libkern/libkern.h>
#else
#include <stdint.h>
#include <string.h>
#endif

#include "aes_sse2_impl.h"

static void
br_range_dec32le(uint32_t *p32, size_t nwords, const void *v)
{
	const uint8_t *p8 = v;

	while (nwords --> 0) {
		uint32_t x0 = *p8++;
		uint32_t x1 = *p8++;
		uint32_t x2 = *p8++;
		uint32_t x3 = *p8++;

		*p32++ = x0 | (x1 << 8) | (x2 << 16) | (x3 << 24);
	}
}

void
aes_sse2_bitslice_Sbox(__m128i q[static 4])
{
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	__m128i y1, y2, y3, y4, y5, y6, y7, y8, y9;
	__m128i y10, y11, y12, y13, y14, y15, y16, y17, y18, y19;
	__m128i y20, y21;
	__m128i z0, z1, z2, z3, z4, z5, z6, z7, z8, z9;
	__m128i z10, z11, z12, z13, z14, z15, z16, z17;
	__m128i t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
	__m128i t10, t11, t12, t13, t14, t15, t16, t17, t18, t19;
	__m128i t20, t21, t22, t23, t24, t25, t26, t27, t28, t29;
	__m128i t30, t31, t32, t33, t34, t35, t36, t37, t38, t39;
	__m128i t40, t41, t42, t43, t44, t45, t46, t47, t48, t49;
	__m128i t50, t51, t52, t53, t54, t55, t56, t57, t58, t59;
	__m128i t60, t61, t62, t63, t64, t65, t66, t67;
	__m128i s0, s1, s2, s3, s4, s5, s6, s7;

	x0 = _mm_shuffle_epi32(q[3], 0x0e);
	x1 = _mm_shuffle_epi32(q[2], 0x0e);
	x2 = _mm_shuffle_epi32(q[1], 0x0e);
	x3 = _mm_shuffle_epi32(q[0], 0x0e);
	x4 = q[3];
	x5 = q[2];
	x6 = q[1];
	x7 = q[0];

	/*
	 * Top linear transformation.
	 */
	y14 = x3 ^ x5;
	y13 = x0 ^ x6;
	y9 = x0 ^ x3;
	y8 = x0 ^ x5;
	t0 = x1 ^ x2;
	y1 = t0 ^ x7;
	y4 = y1 ^ x3;
	y12 = y13 ^ y14;
	y2 = y1 ^ x0;
	y5 = y1 ^ x6;
	y3 = y5 ^ y8;
	t1 = x4 ^ y12;
	y15 = t1 ^ x5;
	y20 = t1 ^ x1;
	y6 = y15 ^ x7;
	y10 = y15 ^ t0;
	y11 = y20 ^ y9;
	y7 = x7 ^ y11;
	y17 = y10 ^ y11;
	y19 = y10 ^ y8;
	y16 = t0 ^ y11;
	y21 = y13 ^ y16;
	y18 = x0 ^ y16;

	/*
	 * Non-linear section.
	 */
	t2 = y12 & y15;
	t3 = y3 & y6;
	t4 = t3 ^ t2;
	t5 = y4 & x7;
	t6 = t5 ^ t2;
	t7 = y13 & y16;
	t8 = y5 & y1;
	t9 = t8 ^ t7;
	t10 = y2 & y7;
	t11 = t10 ^ t7;
	t12 = y9 & y11;
	t13 = y14 & y17;
	t14 = t13 ^ t12;
	t15 = y8 & y10;
	t16 = t15 ^ t12;
	t17 = t4 ^ t14;
	t18 = t6 ^ t16;
	t19 = t9 ^ t14;
	t20 = t11 ^ t16;
	t21 = t17 ^ y20;
	t22 = t18 ^ y19;
	t23 = t19 ^ y21;
	t24 = t20 ^ y18;

	t25 = t21 ^ t22;
	t26 = t21 & t23;
	t27 = t24 ^ t26;
	t28 = t25 & t27;
	t29 = t28 ^ t22;
	t30 = t23 ^ t24;
	t31 = t22 ^ t26;
	t32 = t31 & t30;
	t33 = t32 ^ t24;
	t34 = t23 ^ t33;
	t35 = t27 ^ t33;
	t36 = t24 & t35;
	t37 = t36 ^ t34;
	t38 = t27 ^ t36;
	t39 = t29 & t38;
	t40 = t25 ^ t39;

	t41 = t40 ^ t37;
	t42 = t29 ^ t33;
	t43 = t29 ^ t40;
	t44 = t33 ^ t37;
	t45 = t42 ^ t41;
	z0 = t44 & y15;
	z1 = t37 & y6;
	z2 = t33 & x7;
	z3 = t43 & y16;
	z4 = t40 & y1;
	z5 = t29 & y7;
	z6 = t42 & y11;
	z7 = t45 & y17;
	z8 = t41 & y10;
	z9 = t44 & y12;
	z10 = t37 & y3;
	z11 = t33 & y4;
	z12 = t43 & y13;
	z13 = t40 & y5;
	z14 = t29 & y2;
	z15 = t42 & y9;
	z16 = t45 & y14;
	z17 = t41 & y8;

	/*
	 * Bottom linear transformation.
	 */
	t46 = z15 ^ z16;
	t47 = z10 ^ z11;
	t48 = z5 ^ z13;
	t49 = z9 ^ z10;
	t50 = z2 ^ z12;
	t51 = z2 ^ z5;
	t52 = z7 ^ z8;
	t53 = z0 ^ z3;
	t54 = z6 ^ z7;
	t55 = z16 ^ z17;
	t56 = z12 ^ t48;
	t57 = t50 ^ t53;
	t58 = z4 ^ t46;
	t59 = z3 ^ t54;
	t60 = t46 ^ t57;
	t61 = z14 ^ t57;
	t62 = t52 ^ t58;
	t63 = t49 ^ t58;
	t64 = z4 ^ t59;
	t65 = t61 ^ t62;
	t66 = z1 ^ t63;
	s0 = t59 ^ t63;
	s6 = t56 ^ ~t62;
	s7 = t48 ^ ~t60;
	t67 = t64 ^ t65;
	s3 = t53 ^ t66;
	s4 = t51 ^ t66;
	s5 = t47 ^ t65;
	s1 = t64 ^ ~s3;
	s2 = t55 ^ ~t67;

	q[3] = _mm_unpacklo_epi64(s4, s0);
	q[2] = _mm_unpacklo_epi64(s5, s1);
	q[1] = _mm_unpacklo_epi64(s6, s2);
	q[0] = _mm_unpacklo_epi64(s7, s3);
}

void
aes_sse2_ortho(__m128i q[static 4])
{
#define SWAPN(cl, ch, s, x, y)   do { \
		__m128i a, b; \
		a = (x); \
		b = (y); \
		(x) = (a & _mm_set1_epi64x(cl)) | \
		    _mm_slli_epi64(b & _mm_set1_epi64x(cl), (s)); \
		(y) = _mm_srli_epi64(a & _mm_set1_epi64x(ch), (s)) | \
		    (b & _mm_set1_epi64x(ch)); \
	} while (0)

#define SWAP2(x, y)    SWAPN(0x5555555555555555, 0xAAAAAAAAAAAAAAAA,  1, x, y)
#define SWAP4(x, y)    SWAPN(0x3333333333333333, 0xCCCCCCCCCCCCCCCC,  2, x, y)
#define SWAP8(x, y)    SWAPN(0x0F0F0F0F0F0F0F0F, 0xF0F0F0F0F0F0F0F0,  4, x, y)

	SWAP2(q[0], q[1]);
	SWAP2(q[2], q[3]);

	SWAP4(q[0], q[2]);
	SWAP4(q[1], q[3]);

	__m128i q0 = q[0];
	__m128i q1 = q[1];
	__m128i q2 = q[2];
	__m128i q3 = q[3];
	__m128i q4 = _mm_shuffle_epi32(q[0], 0x0e);
	__m128i q5 = _mm_shuffle_epi32(q[1], 0x0e);
	__m128i q6 = _mm_shuffle_epi32(q[2], 0x0e);
	__m128i q7 = _mm_shuffle_epi32(q[3], 0x0e);
	SWAP8(q0, q4);
	SWAP8(q1, q5);
	SWAP8(q2, q6);
	SWAP8(q3, q7);
	q[0] = _mm_unpacklo_epi64(q0, q4);
	q[1] = _mm_unpacklo_epi64(q1, q5);
	q[2] = _mm_unpacklo_epi64(q2, q6);
	q[3] = _mm_unpacklo_epi64(q3, q7);
}

__m128i
aes_sse2_interleave_in(__m128i w)
{
	__m128i lo, hi;

	lo = _mm_shuffle_epi32(w, 0x10);
	hi = _mm_shuffle_epi32(w, 0x32);
	lo &= _mm_set1_epi64x(0x00000000FFFFFFFF);
	hi &= _mm_set1_epi64x(0x00000000FFFFFFFF);
	lo |= _mm_slli_epi64(lo, 16);
	hi |= _mm_slli_epi64(hi, 16);
	lo &= _mm_set1_epi32(0x0000FFFF);
	hi &= _mm_set1_epi32(0x0000FFFF);
	lo |= _mm_slli_epi64(lo, 8);
	hi |= _mm_slli_epi64(hi, 8);
	lo &= _mm_set1_epi16(0x00FF);
	hi &= _mm_set1_epi16(0x00FF);
	return lo | _mm_slli_epi64(hi, 8);
}

__m128i
aes_sse2_interleave_out(__m128i q)
{
	__m128i lo, hi;

	lo = q;
	hi = _mm_srli_si128(q, 1);
	lo &= _mm_set1_epi16(0x00FF);
	hi &= _mm_set1_epi16(0x00FF);
	lo |= _mm_srli_epi64(lo, 8);
	hi |= _mm_srli_epi64(hi, 8);
	lo &= _mm_set1_epi32(0x0000FFFF);
	hi &= _mm_set1_epi32(0x0000FFFF);
	lo |= _mm_srli_epi64(lo, 16);
	hi |= _mm_srli_epi64(hi, 16);
	return (__m128i)_mm_shuffle_ps((__m128)lo, (__m128)hi, 0x88);
}

static const unsigned char Rcon[] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36
};

static uint32_t
sub_word(uint32_t x)
{
	__m128i q[4];
	uint32_t y;

	memset(q, 0, sizeof(q));
	q[0] = _mm_loadu_si32(&x);
	aes_sse2_ortho(q);
	aes_sse2_bitslice_Sbox(q);
	aes_sse2_ortho(q);
	_mm_storeu_si32(&y, q[0]);
	return y;
}

unsigned
aes_sse2_keysched(uint64_t *comp_skey, const void *key, size_t key_len)
{
	unsigned num_rounds;
	int i, j, k, nk, nkf;
	uint32_t tmp;
	uint32_t skey[60];

	switch (key_len) {
	case 16:
		num_rounds = 10;
		break;
	case 24:
		num_rounds = 12;
		break;
	case 32:
		num_rounds = 14;
		break;
	default:
		/* abort(); */
		return 0;
	}
	nk = (int)(key_len >> 2);
	nkf = (int)((num_rounds + 1) << 2);
	br_range_dec32le(skey, (key_len >> 2), key);
	tmp = skey[(key_len >> 2) - 1];
	for (i = nk, j = 0, k = 0; i < nkf; i ++) {
		if (j == 0) {
			tmp = (tmp << 24) | (tmp >> 8);
			tmp = sub_word(tmp) ^ Rcon[k];
		} else if (nk > 6 && j == 4) {
			tmp = sub_word(tmp);
		}
		tmp ^= skey[i - nk];
		skey[i] = tmp;
		if (++ j == nk) {
			j = 0;
			k ++;
		}
	}

	for (i = 0, j = 0; i < nkf; i += 4, j += 2) {
		__m128i q[4], q0, q1, q2, q3, q4, q5, q6, q7;
		__m128i w;

		w = _mm_loadu_epi8(skey + i);
		q[0] = q[1] = q[2] = q[3] = aes_sse2_interleave_in(w);
		aes_sse2_ortho(q);
		q0 = q[0] & _mm_set1_epi64x(0x1111111111111111);
		q1 = q[1] & _mm_set1_epi64x(0x2222222222222222);
		q2 = q[2] & _mm_set1_epi64x(0x4444444444444444);
		q3 = q[3] & _mm_set1_epi64x(0x8888888888888888);
		q4 = _mm_shuffle_epi32(q0, 0x0e);
		q5 = _mm_shuffle_epi32(q1, 0x0e);
		q6 = _mm_shuffle_epi32(q2, 0x0e);
		q7 = _mm_shuffle_epi32(q3, 0x0e);
		_mm_storeu_si64(&comp_skey[j + 0], q0 | q1 | q2 | q3);
		_mm_storeu_si64(&comp_skey[j + 1], q4 | q5 | q6 | q7);
	}
	return num_rounds;
}

void
aes_sse2_skey_expand(uint64_t *skey,
	unsigned num_rounds, const uint64_t *comp_skey)
{
	unsigned u, v, n;

	n = (num_rounds + 1) << 1;
	for (u = 0, v = 0; u < n; u ++, v += 4) {
		__m128i x0, x1, x2, x3;

		x0 = x1 = x2 = x3 = _mm_loadu_si64(&comp_skey[u]);
		x0 &= 0x1111111111111111;
		x1 &= 0x2222222222222222;
		x2 &= 0x4444444444444444;
		x3 &= 0x8888888888888888;
		x1 = _mm_srli_epi64(x1, 1);
		x2 = _mm_srli_epi64(x2, 2);
		x3 = _mm_srli_epi64(x3, 3);
		x0 = _mm_sub_epi64(_mm_slli_epi64(x0, 4), x0);
		x1 = _mm_sub_epi64(_mm_slli_epi64(x1, 4), x1);
		x2 = _mm_sub_epi64(_mm_slli_epi64(x2, 4), x2);
		x3 = _mm_sub_epi64(_mm_slli_epi64(x3, 4), x3);
		_mm_storeu_si64(&skey[v + 0], x0);
		_mm_storeu_si64(&skey[v + 1], x1);
		_mm_storeu_si64(&skey[v + 2], x2);
		_mm_storeu_si64(&skey[v + 3], x3);
	}
}
