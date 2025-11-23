/*	$NetBSD: aes_sse2_4x32_enc.c,v 1.1 2025/11/23 22:48:26 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_4x32_enc.c,v 1.1 2025/11/23 22:48:26 riastradh Exp $");

#include <sys/types.h>

#include "aes_sse2_4x32_impl.h"

static inline void
add_round_key(__m128i q[static 8], const uint32_t sk[static 8])
{

	q[0] ^= _mm_set1_epi32(sk[0]);
	q[1] ^= _mm_set1_epi32(sk[1]);
	q[2] ^= _mm_set1_epi32(sk[2]);
	q[3] ^= _mm_set1_epi32(sk[3]);
	q[4] ^= _mm_set1_epi32(sk[4]);
	q[5] ^= _mm_set1_epi32(sk[5]);
	q[6] ^= _mm_set1_epi32(sk[6]);
	q[7] ^= _mm_set1_epi32(sk[7]);
}

static inline __m128i
shift_row(__m128i q)
{
	__m128i x, y0, y1, y2, y3, y4, y5, y6;

	x = q;
	y0 = x & _mm_set1_epi32(0x000000FF);
	y1 = _mm_srli_epi32(x & _mm_set1_epi32(0x0000FC00), 2);
	y2 = _mm_slli_epi32(x & _mm_set1_epi32(0x00000300), 6);
	y3 = _mm_srli_epi32(x & _mm_set1_epi32(0x00F00000), 4);
	y4 = _mm_slli_epi32(x & _mm_set1_epi32(0x000F0000), 4);
	y5 = _mm_srli_epi32(x & _mm_set1_epi32(0xC0000000), 6);
	y6 = _mm_slli_epi32(x & _mm_set1_epi32(0x3F000000), 2);
	return y0 | y1 | y2 | y3 | y4 | y5 | y6;
}

static inline void
shift_rows(__m128i q[static 8])
{

	q[0] = shift_row(q[0]);
	q[1] = shift_row(q[1]);
	q[2] = shift_row(q[2]);
	q[3] = shift_row(q[3]);
	q[4] = shift_row(q[4]);
	q[5] = shift_row(q[5]);
	q[6] = shift_row(q[6]);
	q[7] = shift_row(q[7]);
}

static inline __m128i
rotr16(__m128i x)
{
	return _mm_slli_epi32(x, 16) | _mm_srli_epi32(x, 16);
}

static inline void
mix_columns(__m128i q[static 8])
{
	__m128i q0, q1, q2, q3, q4, q5, q6, q7;
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;

	q0 = q[0];
	q1 = q[1];
	q2 = q[2];
	q3 = q[3];
	q4 = q[4];
	q5 = q[5];
	q6 = q[6];
	q7 = q[7];
	r0 = _mm_srli_epi32(q0, 8) | _mm_slli_epi32(q0, 24);
	r1 = _mm_srli_epi32(q1, 8) | _mm_slli_epi32(q1, 24);
	r2 = _mm_srli_epi32(q2, 8) | _mm_slli_epi32(q2, 24);
	r3 = _mm_srli_epi32(q3, 8) | _mm_slli_epi32(q3, 24);
	r4 = _mm_srli_epi32(q4, 8) | _mm_slli_epi32(q4, 24);
	r5 = _mm_srli_epi32(q5, 8) | _mm_slli_epi32(q5, 24);
	r6 = _mm_srli_epi32(q6, 8) | _mm_slli_epi32(q6, 24);
	r7 = _mm_srli_epi32(q7, 8) | _mm_slli_epi32(q7, 24);

	q[0] = q7 ^ r7 ^ r0 ^ rotr16(q0 ^ r0);
	q[1] = q0 ^ r0 ^ q7 ^ r7 ^ r1 ^ rotr16(q1 ^ r1);
	q[2] = q1 ^ r1 ^ r2 ^ rotr16(q2 ^ r2);
	q[3] = q2 ^ r2 ^ q7 ^ r7 ^ r3 ^ rotr16(q3 ^ r3);
	q[4] = q3 ^ r3 ^ q7 ^ r7 ^ r4 ^ rotr16(q4 ^ r4);
	q[5] = q4 ^ r4 ^ r5 ^ rotr16(q5 ^ r5);
	q[6] = q5 ^ r5 ^ r6 ^ rotr16(q6 ^ r6);
	q[7] = q6 ^ r6 ^ r7 ^ rotr16(q7 ^ r7);
}

/* see inner.h */
void
aes_sse2_4x32_bitslice_encrypt(unsigned num_rounds,
	const uint32_t skey[static 120], __m128i q[static 8])
{
	unsigned u;

	add_round_key(q, skey);
	for (u = 1; u < num_rounds; u ++) {
		aes_sse2_4x32_bitslice_Sbox(q);
		shift_rows(q);
		mix_columns(q);
		add_round_key(q, skey + (u << 3));
	}
	aes_sse2_4x32_bitslice_Sbox(q);
	shift_rows(q);
	add_round_key(q, skey + (num_rounds << 3));
}
