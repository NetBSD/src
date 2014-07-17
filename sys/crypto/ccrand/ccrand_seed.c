/*	$NetBSD: ccrand_seed.c,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

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
 * ccrand_seed.c
 *
 * Seed the PRNG.
 */
#include <crypto/ccrand/ccrand_var.h>

/*
 * This is complicated by the fact that we accept any amount of
 * seed that he offers but try to make the cases where he gives
 * us 4-6 words (128 bit key with default or specified iv) or
 * 8 or more words (256 bit key with default or specified iv)
 * match the keystream output of the reference implementation.
 * For other lengths of seed we make do.  When given more than 10
 * words we ignore the excess.
 *
 * It would be easier (and maybe even better in general) if we
 * just filled in all 16 words with seed material.  The issues
 * with this are that I would also like to use the counter to track
 * the use of the PRNG since the last (re)seed, which we can do
 * by setting it to 0 when (re)seeding, and I suspect that
 * the 4 non-zero constant words of key, or at least some amount
 * of pre-selected data, are necessary to avoid someone being able
 * to directly initialize the hash into a weak state (i.e. the
 * ISAAC "problem").  We'll hence stick with the 8+2 word keying
 * of the original.
 */

/*
 * These constants come from
 *
 * static const char sigma[16] = "expand 32-byte k";
 * static const char tau[16] = "expand 16-byte k";
 *
 * in the original code, formed into word in little endian order.
 * For amounts of seed which can't be made to look like a 16- or
 * 32-byte key we use
 *
 * "expand XX-byte k"
 *
 * instead.
 */
#define	KEY_CONST_0	0x61707865	/* "expa", little end order */
#define	KEY_CONST_3	0x6b206574	/* "te k", little end order */

/* These are used when the seed is 8 words or longer */
#define	KEY_32_CONST_1	0x3320646e	/* "nd 3", little end order */
#define	KEY_32_CONST_2	0x79622d32	/* "2-by", little end order */

/* These are used when the seed is 4, 5 or 6 words */
#define	KEY_16_CONST_1	0x3120646e	/* "nd 1", little end order */
#define	KEY_16_CONST_2	0x79622d36	/* "6-by", little end order */

/* These are used when the seed is 0-3 or 7 words */
#define	KEY_XX_CONST_1	0x5820646e	/* "nd X", little end order */
#define	KEY_XX_CONST_2	0x79622d58	/* "X-by", little end order */

/* Default key and iv to use when we have nothing else */
#define	KEY_DEFAULT_N	8

static const uint32_t key_default[KEY_DEFAULT_N] = {
	0x60206136, 0x8dcf2e90, 0xdbf2177a, 0x36e509cd,
	0xf39b27a8, 0x76afb98d, 0x37cafca6, 0xf9e78764
};

#define	KEY_DEFAULT_IV_0	0xea982180
#define	KEY_DEFAULT_IV_1	0x45a1db3b


/*
 * ccrand_seed()
 *
 * Initialize the random number generator with an arbitrary number
 * of words of seed.
 */
void
ccrand_seed(ccrand_t *x, const uint32_t *seed, unsigned int n)
{
	unsigned int n_iv, i, i_seed;
	uint32_t *key = &x->v[V_CTX];
	const uint32_t *iv;

	/*
	 * Set the index to 16 to ensure that the next call for
	 * random numbers generates some.
	 */
	x->v[V_INDEX] = 16;

	/*
	 * Figure out the constants first.  At the same time
	 * figure out how much goes into the iv and how much
	 * goes into the key.
	 */
	key[V_CONST + 0] = KEY_CONST_0;
	if (n >= 8) {
		key[V_CONST + 1] = KEY_32_CONST_1;
		key[V_CONST + 2] = KEY_32_CONST_2;
		iv = &seed[8];
		n_iv = n - 8;
		n -= n_iv;
	} else if (n >= 4 && n <= 6) {
		key[V_CONST + 1] = KEY_16_CONST_1;
		key[V_CONST + 2] = KEY_16_CONST_2;
		iv = &seed[4];
		n_iv = n - 4;
		n -= n_iv;
	} else {
		key[V_CONST + 1] = KEY_XX_CONST_1;
		key[V_CONST + 2] = KEY_XX_CONST_2;
		iv = NULL;
		n_iv = 0;
		if (n == 0) {
			seed = key_default;
			n = KEY_DEFAULT_N;
		}
	}
	key[V_CONST + 3] = KEY_CONST_3;

	/*
	 * Fill in the key.  Wrap around and repeat the seed
	 * if we haven't got enough.
	 */
	for (i = V_KEY, i_seed = 0; i < V_CNT; i++) {
		key[i] = seed[i_seed++];
		if (i_seed >= n) {
			i_seed = 0;
		}
	}

	/*
	 * Set the counter to 0.
	 */
	key[V_CNT] = key[V_CNTHI] = 0;

	/*
	 * Now fill in the iv.  If we have less then two words we fill in
	 * the remainder with a default value.
	 */
	switch (n_iv) {
	case 0:
		key[V_IV + 0] = KEY_DEFAULT_IV_0;
		key[V_IV + 1] = KEY_DEFAULT_IV_1;
		break;

	case 1:
		key[V_IV + 0] = iv[0];
		key[V_IV + 1] = KEY_DEFAULT_IV_1;
		break;

	default:
		key[V_IV + 0] = iv[0];
		key[V_IV + 1] = iv[1];
		break;
	}
}
