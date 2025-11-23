/*	$NetBSD: aes_sse2_4x32_dec.c,v 1.1 2025/11/23 22:48:26 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_4x32_dec.c,v 1.1 2025/11/23 22:48:26 riastradh Exp $");

#include <sys/types.h>

#include "aes_sse2_4x32_impl.h"

/* see inner.h */
void
aes_sse2_4x32_bitslice_invSbox(__m128i q[static 8])
{
	/*
	 * AES S-box is:
	 *   S(x) = A(I(x)) ^ 0x63
	 * where I() is inversion in GF(256), and A() is a linear
	 * transform (0 is formally defined to be its own inverse).
	 * Since inversion is an involution, the inverse S-box can be
	 * computed from the S-box as:
	 *   iS(x) = B(S(B(x ^ 0x63)) ^ 0x63)
	 * where B() is the inverse of A(). Indeed, for any y in GF(256):
	 *   iS(S(y)) = B(A(I(B(A(I(y)) ^ 0x63 ^ 0x63))) ^ 0x63 ^ 0x63) = y
	 *
	 * Note: we reuse the implementation of the forward S-box,
	 * instead of duplicating it here, so that total code size is
	 * lower. By merging the B() transforms into the S-box circuit
	 * we could make faster CBC decryption, but CBC decryption is
	 * already quite faster than CBC encryption because we can
	 * process two blocks in parallel.
	 */
	__m128i q0, q1, q2, q3, q4, q5, q6, q7;

	q0 = ~q[0];
	q1 = ~q[1];
	q2 = q[2];
	q3 = q[3];
	q4 = q[4];
	q5 = ~q[5];
	q6 = ~q[6];
	q7 = q[7];
	q[7] = q1 ^ q4 ^ q6;
	q[6] = q0 ^ q3 ^ q5;
	q[5] = q7 ^ q2 ^ q4;
	q[4] = q6 ^ q1 ^ q3;
	q[3] = q5 ^ q0 ^ q2;
	q[2] = q4 ^ q7 ^ q1;
	q[1] = q3 ^ q6 ^ q0;
	q[0] = q2 ^ q5 ^ q7;

	aes_sse2_4x32_bitslice_Sbox(q);

	q0 = ~q[0];
	q1 = ~q[1];
	q2 = q[2];
	q3 = q[3];
	q4 = q[4];
	q5 = ~q[5];
	q6 = ~q[6];
	q7 = q[7];
	q[7] = q1 ^ q4 ^ q6;
	q[6] = q0 ^ q3 ^ q5;
	q[5] = q7 ^ q2 ^ q4;
	q[4] = q6 ^ q1 ^ q3;
	q[3] = q5 ^ q0 ^ q2;
	q[2] = q4 ^ q7 ^ q1;
	q[1] = q3 ^ q6 ^ q0;
	q[0] = q2 ^ q5 ^ q7;
}

static void
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
inv_shift_row(__m128i q)
{
	__m128i x, y0, y1, y2, y3, y4, y5, y6;

	x = q;
	y0 = x & _mm_set1_epi32(0x000000FF);
	y1 = _mm_slli_epi32(x & _mm_set1_epi32(0x00003F00), 2);
	y2 = _mm_srli_epi32(x & _mm_set1_epi32(0x0000C000), 6);
	y3 = _mm_slli_epi32(x & _mm_set1_epi32(0x000F0000), 4);
	y4 = _mm_srli_epi32(x & _mm_set1_epi32(0x00F00000), 4);
	y5 = _mm_slli_epi32(x & _mm_set1_epi32(0x03000000), 6);
	y6 = _mm_srli_epi32(x & _mm_set1_epi32(0xFC000000), 2);
	return y0 | y1 | y2 | y3 | y4 | y5 | y6;
}

static void
inv_shift_rows(__m128i *q)
{

	q[0] = inv_shift_row(q[0]);
	q[1] = inv_shift_row(q[1]);
	q[2] = inv_shift_row(q[2]);
	q[3] = inv_shift_row(q[3]);
	q[4] = inv_shift_row(q[4]);
	q[5] = inv_shift_row(q[5]);
	q[6] = inv_shift_row(q[6]);
	q[7] = inv_shift_row(q[7]);
}

static inline __m128i
rotr16(__m128i x)
{
	return _mm_slli_epi32(x, 16) | _mm_srli_epi32(x, 16);
}

static void
inv_mix_columns(__m128i q[static 8])
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

	q[0] = q5 ^ q6 ^ q7 ^ r0 ^ r5 ^ r7 ^ rotr16(q0 ^ q5 ^ q6 ^ r0 ^ r5);
	q[1] = q0 ^ q5 ^ r0 ^ r1 ^ r5 ^ r6 ^ r7 ^ rotr16(q1 ^ q5 ^ q7 ^ r1 ^ r5 ^ r6);
	q[2] = q0 ^ q1 ^ q6 ^ r1 ^ r2 ^ r6 ^ r7 ^ rotr16(q0 ^ q2 ^ q6 ^ r2 ^ r6 ^ r7);
	q[3] = q0 ^ q1 ^ q2 ^ q5 ^ q6 ^ r0 ^ r2 ^ r3 ^ r5 ^ rotr16(q0 ^ q1 ^ q3 ^ q5 ^ q6 ^ q7 ^ r0 ^ r3 ^ r5 ^ r7);
	q[4] = q1 ^ q2 ^ q3 ^ q5 ^ r1 ^ r3 ^ r4 ^ r5 ^ r6 ^ r7 ^ rotr16(q1 ^ q2 ^ q4 ^ q5 ^ q7 ^ r1 ^ r4 ^ r5 ^ r6);
	q[5] = q2 ^ q3 ^ q4 ^ q6 ^ r2 ^ r4 ^ r5 ^ r6 ^ r7 ^ rotr16(q2 ^ q3 ^ q5 ^ q6 ^ r2 ^ r5 ^ r6 ^ r7);
	q[6] = q3 ^ q4 ^ q5 ^ q7 ^ r3 ^ r5 ^ r6 ^ r7 ^ rotr16(q3 ^ q4 ^ q6 ^ q7 ^ r3 ^ r6 ^ r7);
	q[7] = q4 ^ q5 ^ q6 ^ r4 ^ r6 ^ r7 ^ rotr16(q4 ^ q5 ^ q7 ^ r4 ^ r7);
}

/* see inner.h */
void
aes_sse2_4x32_bitslice_decrypt(unsigned num_rounds,
	const uint32_t skey[static 120], __m128i q[static 8])
{
	unsigned u;

	add_round_key(q, skey + (num_rounds << 3));
	for (u = num_rounds - 1; u > 0; u --) {
		inv_shift_rows(q);
		aes_sse2_4x32_bitslice_invSbox(q);
		add_round_key(q, skey + (u << 3));
		inv_mix_columns(q);
	}
	inv_shift_rows(q);
	aes_sse2_4x32_bitslice_invSbox(q);
	add_round_key(q, skey);
}
