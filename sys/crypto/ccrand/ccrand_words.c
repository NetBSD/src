/*	$NetBSD: ccrand_words.c,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

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
 * ccrand_words.c
 */
#include <crypto/ccrand/ccrand_var.h>

/*
 * ccrand_words()
 *
 *
 * Give the caller the number of random words he asks for.  The words
 * are in chacha's preferred order (if you want to write them as bytes
 * you should write them little-endian).
 */
void
ccrand_words(ccrand_t *x, uint32_t *wp, unsigned int n_w)
{
	uint32_t n_words = n_w;
	uint32_t i, n;

	if (n_words == 0) {
		return;		/* what the heck? */
	}

	/*
	 * Get the index into what's currently in the buffer.  If
	 * we're uninitialized we may need to deal with that.
	 */
	i = x->v[V_INDEX];
	if ((i - 1) >= N) {
		ccrand_seed(x, NULL, 0);
		i = x->v[V_INDEX];
	}

	/*
	 * If we have words in the buffer, copy them in.
	 */
	n = N - i;
	if (n > n_words) {
		n = n_words;
	}
	n_words -= n;
	while (n) {
		*wp++ = x->v[i++];
		n--;
	}

	/*
	 * We've exhausted what we had already.  We can now
	 * generate 16 word blocks into his buffer directly.
	 * The first word needs to be overwritten by the return
	 * value, however.
	 */
	while (n_words >= N) {
		*wp = __ccrand_gen16(wp, &x->v[V_CTX]);
		wp += N;
		n_words -= N;
	}

	/*
	 * He may need less than 16 words more.  If he does
	 * generate a new group and copy what he needs out.
	 */
	if (n_words) {
		*wp++ = __ccrand_gen16(&x->v[V_BUF], &x->v[V_CTX]);
		i = 1;
		while (--n_words > 0) {
			*wp++ = x->v[i++];
		}
	}

	/*
	 * Store the index to account for what remains.
	 */
	x->v[V_INDEX] = i;
}
