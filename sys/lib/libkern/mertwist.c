/*	$NetBSD: mertwist.c,v 1.3 2008/01/31 03:34:04 simonb Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifdef _KERNEL
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#else
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#define	KASSERT(x)	assert(x)
#endif

/*
 * Mersenne Twister
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 */

#define	MATRIX_A(a)		(((a) & 1) ? 0x9908b0df : 0)
#define	TEMPERING_MASK_B	0x9d2c5680
#define	TEMPERING_MASK_C	0xefc60000
#define	MIX(a,b)		(((a) & 0x80000000) | ((b) & 0x7fffffff))

#define	KNUTH_MULTIPLIER	0x6c078965

#ifndef MTPRNG_RLEN
#define	MTPRNG_RLEN		624
#endif
#define	MTPRNG_POS1		397

static void mtprng_refresh(struct mtprng_state *);

/*
 * Initialize the generator from a seed
 */
void
mtprng_init(void *v, const uint8_t *key, size_t len)
{
	struct mtprng_state * const mt = v;
	size_t i, j, n;
	size_t xlen, keylen, passes;

	/*
	 * If we have more than half of all the key data, simply copy it in
	 * and refresh/mix.
	 */
	if (len >= MTPRNG_RLEN * sizeof(uint32_t) / 2) {
		if (len > sizeof(mt->mt_elem))
			len = sizeof(mt->mt_elem);
		memcpy(mt->mt_elem, key, len);
		mtprng_refresh(mt);
		return;
	}

	/*
	 * Figure out how many 32bit words of key data we have.
	 */
	xlen = (len + 3) >> 2;
	
	/*
	 * Divide the keydata into equal-sized blobs
	 */
	n = MTPRNG_RLEN / xlen;
	keylen = len / xlen;

	/*
	 * Except for the first which gets and left overs (0 to 3 bytes)
	 */
	n += MTPRNG_RLEN - n * xlen;
	keylen += len - keylen * xlen;

	mt->mt_idx = 0;

	for (passes = 0, j = 0; passes < xlen; passes++) {
		uint32_t seed;

		KASSERT(0 < keylen);
		KASSERT(keylen <= 4);

		/*
		 * We need each key to be 32bit (or so).
		 */
		for (i = 0, seed = 0; i < keylen; i++) {
			seed = (seed << 8) | (seed >> 24);
			seed |= *key++;
		}

		/*
		 * Use Knuth's algorithm for expanding this seed over its
		 * portion of the key space.
		 */
		mt->mt_elem[j++] = seed;
		for (i = 1; i < n; i++, j++) {
			mt->mt_elem[j] = KNUTH_MULTIPLIER
			    * (mt->mt_elem[j-1] ^ (mt->mt_elem[j-1] >> 30)) + i;
		}

		/*
		 * After the first pass, reset keylen and n to their correct
		 * values.
		 */
		if (passes == 0) {
			n = MTPRNG_RLEN / xlen;
			keylen = len / xlen;
		}
	}
	KASSERT(j == MTPRNG_RLEN);
	mtprng_refresh(mt);
}

/*
 * Generate an array of 624 untempered numbers
 */
void
mtprng_refresh(struct mtprng_state *mt)
{
	uint32_t y;
	size_t i, j;
	/*
	 * The following has been refactored to avoid the need for 'mod 624'
	*/
	for (i = 0, j = MTPRNG_POS1; j < MTPRNG_RLEN; i++, j++) {
		y = MIX(mt->mt_elem[i], mt->mt_elem[i+1]);
		mt->mt_elem[i] = mt->mt_elem[j] ^ (y >> 1) ^ MATRIX_A(y);
	}
	for (j = 0; i < MTPRNG_RLEN - 1; i++, j++) {
		y = MIX(mt->mt_elem[i], mt->mt_elem[i+1]);
		mt->mt_elem[i] = mt->mt_elem[j] ^ (y >> 1) ^ MATRIX_A(y);
	}
	y = MIX(mt->mt_elem[MTPRNG_RLEN - 1], mt->mt_elem[0]);
	mt->mt_elem[MTPRNG_RLEN - 1] =
	    mt->mt_elem[MTPRNG_POS1 - 1] ^ (y >> 1) ^ MATRIX_A(y);
}
 
/*
 * Extract a tempered PRN based on the current index.  Then recompute a
 * new value for the index.  This avoids having to regenerate the array
 * every 624 iterations. We can do this since recomputing only the next
 * element and the [(i + 397) % 624] one.
 */
uint32_t
mtprng_rawrandom(void *v)
{
	struct mtprng_state * const mt = v;
	uint32_t x, y;
	const size_t i = mt->mt_idx;
	size_t j;

	/*
	 * First generate the random value for the current position.
	 */
	x = mt->mt_elem[i];
	x ^= x >> 11;
	x ^= (x << 7) & TEMPERING_MASK_B;
	x ^= (x << 15) & TEMPERING_MASK_C;
	x ^= x >> 18;

	/*
	 * Next recalculate the next sequence for the current position.
	 */
	y = mt->mt_elem[i];
	if (__predict_true(i < MTPRNG_RLEN - 1)) {
		/*
		 * Avoid doing % since it can be expensive.
		 * j = (i + MTPRNG_POS1) % MTPRNG_RLEN;
		 */
		j = i + MTPRNG_POS1;
		if (j >= MTPRNG_RLEN)
			j -= MTPRNG_RLEN;
		mt->mt_idx++;
	} else {
		j = MTPRNG_POS1 - 1;
		mt->mt_idx = 0;
	}
	y = MIX(y, mt->mt_elem[mt->mt_idx]);
	mt->mt_elem[i] = mt->mt_elem[j] ^ (y >> 1) ^ MATRIX_A(y);

	/*
	 * Return the value calculated in the first step.
	 */
	return x;
}

uint32_t
mtprng_random(void *v)
{
	uint64_t a;
	size_t n;

	/*
	 * Grab 8 raw 32bit numbers.  Add them together shifting by 4 for
	 * each iteration.
	 */

	for (n = 0, a = 0; n < 9; n++)
		a = (a << 4) + mtprng_random(v);

	/*
	 * Now we have a 64 bit number.  Add the top and bottom 32bit
	 * numbers and return that as the hashed value.
	 */
	return a + (a >> 32);
}
