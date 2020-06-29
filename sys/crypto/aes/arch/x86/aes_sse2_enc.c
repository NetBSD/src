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
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_enc.c,v 1.1 2020/06/29 23:47:54 riastradh Exp $");

#include <sys/types.h>

#include "aes_sse2_impl.h"

static inline void
add_round_key(__m128i q[static 4], const uint64_t sk[static 8])
{
	q[0] ^= _mm_set_epi64x(sk[4], sk[0]);
	q[1] ^= _mm_set_epi64x(sk[5], sk[1]);
	q[2] ^= _mm_set_epi64x(sk[6], sk[2]);
	q[3] ^= _mm_set_epi64x(sk[7], sk[3]);
}

static inline __m128i
shift_row(__m128i q)
{
	__m128i x, y0, y1, y2, y3, y4, y5, y6;

	x = q;
	y0 = x & _mm_set1_epi64x(0x000000000000FFFF);
	y1 = x & _mm_set1_epi64x(0x00000000FFF00000);
	y2 = x & _mm_set1_epi64x(0x00000000000F0000);
	y3 = x & _mm_set1_epi64x(0x0000FF0000000000);
	y4 = x & _mm_set1_epi64x(0x000000FF00000000);
	y5 = x & _mm_set1_epi64x(0xF000000000000000);
	y6 = x & _mm_set1_epi64x(0x0FFF000000000000);
	y1 = _mm_srli_epi64(y1, 4);
	y2 = _mm_slli_epi64(y2, 12);
	y3 = _mm_srli_epi64(y3, 8);
	y4 = _mm_slli_epi64(y4, 8);
	y5 = _mm_srli_epi64(y5, 12);
	y6 = _mm_slli_epi64(y6, 4);
	return y0 | y1 | y2 | y3 | y4 | y5 | y6;
}

static inline void
shift_rows(__m128i q[static 4])
{

	q[0] = shift_row(q[0]);
	q[1] = shift_row(q[1]);
	q[2] = shift_row(q[2]);
	q[3] = shift_row(q[3]);
}

static inline __m128i
rotr32(__m128i x)
{
	return _mm_slli_epi64(x, 32) | _mm_srli_epi64(x, 32);
}

static inline void
mix_columns(__m128i q[static 4])
{
	__m128i q0, q1, q2, q3, q4, q5, q6, q7;
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	__m128i s0, s1, s2, s3, s4, s5, s6, s7;

	q0 = q[0];
	q1 = q[1];
	q2 = q[2];
	q3 = q[3];
	r0 = _mm_srli_epi64(q0, 16) | _mm_slli_epi64(q0, 48);
	r1 = _mm_srli_epi64(q1, 16) | _mm_slli_epi64(q1, 48);
	r2 = _mm_srli_epi64(q2, 16) | _mm_slli_epi64(q2, 48);
	r3 = _mm_srli_epi64(q3, 16) | _mm_slli_epi64(q3, 48);

	q7 = _mm_shuffle_epi32(q3, 0x0e);
	q6 = _mm_shuffle_epi32(q2, 0x0e);
	q5 = _mm_shuffle_epi32(q1, 0x0e);
	q4 = _mm_shuffle_epi32(q0, 0x0e);

	r7 = _mm_shuffle_epi32(r3, 0x0e);
	r6 = _mm_shuffle_epi32(r2, 0x0e);
	r5 = _mm_shuffle_epi32(r1, 0x0e);
	r4 = _mm_shuffle_epi32(r0, 0x0e);

	s0 = q7 ^ r7 ^ r0 ^ rotr32(q0 ^ r0);
	s1 = q0 ^ r0 ^ q7 ^ r7 ^ r1 ^ rotr32(q1 ^ r1);
	s2 = q1 ^ r1 ^ r2 ^ rotr32(q2 ^ r2);
	s3 = q2 ^ r2 ^ q7 ^ r7 ^ r3 ^ rotr32(q3 ^ r3);
	s4 = q3 ^ r3 ^ q7 ^ r7 ^ r4 ^ rotr32(q4 ^ r4);
	s5 = q4 ^ r4 ^ r5 ^ rotr32(q5 ^ r5);
	s6 = q5 ^ r5 ^ r6 ^ rotr32(q6 ^ r6);
	s7 = q6 ^ r6 ^ r7 ^ rotr32(q7 ^ r7);

	q[0] = _mm_unpacklo_epi64(s0, s4);
	q[1] = _mm_unpacklo_epi64(s1, s5);
	q[2] = _mm_unpacklo_epi64(s2, s6);
	q[3] = _mm_unpacklo_epi64(s3, s7);
}

void
aes_sse2_bitslice_encrypt(unsigned num_rounds,
	const uint64_t *skey, __m128i q[static 4])
{
	unsigned u;

	add_round_key(q, skey);
	for (u = 1; u < num_rounds; u ++) {
		aes_sse2_bitslice_Sbox(q);
		shift_rows(q);
		mix_columns(q);
		add_round_key(q, skey + (u << 3));
	}
	aes_sse2_bitslice_Sbox(q);
	shift_rows(q);
	add_round_key(q, skey + (num_rounds << 3));
}
