/*	$NetBSD: ccrand_gen16.c,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dennis Ferguson.
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

/*
 * ccrand_gen16.c
 *
 * Implementation of the core keystream generator from * D. J. Bernstein's
 * chacha stream cypher.  Much stolen from his implementation.
 */
#include <crypto/ccrand/ccrand_var.h>

/*
 * Complain if it isn't one of the standard variants.  If you want
 * to try something else just remove this.
 */
#if	(CHACHA_R != 8) && (CHACHA_R != 12) && (CHACHA_R != 20)
#error	"Non-standard ChaCha variant configured, you're on your own"
#endif

/*
 * Macros matching the reference source (I might not do it this way...).
 * PLUS() and ROTATE() moved to the header file.
 */
#define XOR(v, w)	((v) ^ (w))

#define QUARTERROUND(a, b, c, d)				\
	do {							\
		a = PLUS(a, b); d = ROTATE(XOR(d, a), 16);	\
		c = PLUS(c, d); b = ROTATE(XOR(b, c), 12);	\
		a = PLUS(a, b); d = ROTATE(XOR(d, a),  8);	\
		c = PLUS(c, d); b = ROTATE(XOR(b, c),  7);	\
	} while (0)

/*
 * __ccrand_gen16()
 *
 * Generate 16 words of random output from our current state.  Increment
 * the generated blocks count.
 *
 * This should be a whole lot like chacha's salsa20_wordtobyte()
 * but with the following changes:
 *
 * - We increment the counter in here after generating a block (this
 *   means 'in' has to be non-const) since this is called from a number
 *   of places and we only want the code that knows about the counter
 *   in one spot.
 *
 * - We write a constant value of 1 to out[0] and return the first
 *   word of the block (which chacha would normally want to write in
 *   that location) as the function's return value instead.  This
 *   is what is required when using this to refill the block buffer
 *   in the context structure, while not precluding its use to fill
 *   a user buffer directly (the caller will need to rewrite out[0]
 *   with the return value).
 *
 * - We deal in 32 bit words in chacha's word order.  This means that
 *   contexts seeded with identical words will return the same sequence
 *   of words on both little-endian and big-endian machines, leaving it
 *   up to someone else whether this extends to bytes or not.
 *
 * We take as arguments pointers to two 16 word arrays that are normally
 * halves of the 32 word context structure in case the caller wants to
 * directly fill a user buffer instead.
 */
uint32_t
__ccrand_gen16(uint32_t *out, uint32_t *in)
{
	uint32_t x0, x1, x2, x3, x4, x5, x6, x7;
	uint32_t x8, x9, x10, x11, x12, x13, x14, x15;
	int i;

	x0 = in[0];
	x1 = in[1];
	x2 = in[2];
	x3 = in[3];
	x4 = in[4];
	x5 = in[5];
	x6 = in[6];
	x7 = in[7];
	x8 = in[8];
	x9 = in[9];
	x10 = in[10];
	x11 = in[11];
	x12 = in[12];
	x13 = in[13];
	x14 = in[14];
	x15 = in[15];

	for (i = CHACHA_R; i > 0; i -= 2) {
		QUARTERROUND(x0, x4,  x8, x12);
		QUARTERROUND(x1, x5,  x9, x13);
		QUARTERROUND(x2, x6, x10, x14);
		QUARTERROUND(x3, x7, x11, x15);
		QUARTERROUND(x0, x5, x10, x15);
		QUARTERROUND(x1, x6, x11, x12);
		QUARTERROUND(x2, x7,  x8, x13);
		QUARTERROUND(x3, x4,  x9, x14);
	}

	out[0] = 1;
	out[1] = PLUS(x1, in[1]);
	out[2] = PLUS(x2, in[2]);
	out[3] = PLUS(x3, in[3]);
	out[4] = PLUS(x4, in[4]);
	out[5] = PLUS(x5, in[5]);
	out[6] = PLUS(x6, in[6]);
	out[7] = PLUS(x7, in[7]);
	out[8] = PLUS(x8, in[8]);
	out[9] = PLUS(x9, in[9]);
	out[10] = PLUS(x10, in[10]);
	out[11] = PLUS(x11, in[11]);
	out[12] = PLUS(x12, in[12]);
	out[13] = PLUS(x13, in[13]);
	out[14] = PLUS(x14, in[14]);
	out[15] = PLUS(x15, in[15]);
	if (++in[V_CNT] == 0) {
		++in[V_CNTHI];
	}

	return (PLUS(x0, in[0]));
}
