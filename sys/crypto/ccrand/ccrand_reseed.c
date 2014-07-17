/*	$NetBSD: ccrand_reseed.c,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

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
 * ccrand_reseed.c
 *
 * Incorporate new seed into the PRNG state.
 */
#include <crypto/ccrand/ccrand_var.h>

/*
 * This is meant to be called when the owner of the context
 * has generated enough random stuff with it that he decides
 * it might be prudent to re-key the cipher.
 *
 * The first 2 words of new seed (or 1, if that's all there is)
 * are copied into the iv.  If called with exact 2 words of seed
 * the function is equivalent to ECRYPT_ivsetup() in the reference
 * implementation.  Words beyond 2 are mixed into the existing
 * key.  No more than 10 words are used.
 */

/*
 * ccrand_reseed()
 *
 * Incorporate new seed material into the PRNG state.
 */
void
ccrand_reseed(ccrand_t *x, const uint32_t *seed, unsigned int n)
{
	uint32_t *key = &x->v[V_CTX];
	unsigned int i;

	/*
	 * If the context appears not to have been previously
	 * initialized call the seed function instead.
	 */
	if ((x->v[V_INDEX] - 1) >= 16) {
		ccrand_seed(x, seed, n);
		return;
	}

	if (n == 0) {
		return;
	}
	key[V_CNT] = key[V_CNTHI] = 0;

	/*
	 * If he only gave us enough to fill in the iv, just do
	 * that and return.  Otherwise use the rest to torque the
	 * key.
	 */
	switch (n) {
	case 2:
		key[V_IV + 1] = seed[1];
		/*FALLSTHROUGH*/

	case 1:
		key[V_IV + 0] = seed[0];
		return;

	default:
		key[V_IV + 0] = *seed++;
		key[V_IV + 1] = *seed++;
		n -= 2;
		break;
	}

	/*
	 * We mix the new seed into the existing material rather
	 * than just copying it.  The idea is that this can't make
	 * things worse than the new seed's entropy, even if the
	 * prior state has been revealed, but may improve things
	 * a tiny bit if the prior state hasn't been revealed and
	 * there is some doubt about the excellence of the new
	 * seed.  This is probably ineffective for any useful
	 * purpose but it seems like it can't hurt.
	 */
	i = 0;
	key[V_KEY + 0] = PLUS(ROTATE(key[V_KEY + 0], 3), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 1] = PLUS(ROTATE(key[V_KEY + 1], 11), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 2] = PLUS(ROTATE(key[V_KEY + 2], 19), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 3] = PLUS(ROTATE(key[V_KEY + 3], 29), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 4] = PLUS(ROTATE(key[V_KEY + 4], 7), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 5] = PLUS(ROTATE(key[V_KEY + 5], 13), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 6] = PLUS(ROTATE(key[V_KEY + 6], 23), seed[i++]);
	if (i >= n) i = 0;
	key[V_KEY + 7] = PLUS(ROTATE(key[V_KEY + 7], 5), seed[i]);
}
