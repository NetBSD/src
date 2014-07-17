/*	$NetBSD: ccrand_bytes.c,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

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
 * ccrand_bytes.c
 */
#include <crypto/ccrand/ccrand_var.h>

/*
 * Macro to output the bytes of a word in little-endian order.
 * We might be able to do better but it takes enough work to
 * generate the words that this may not matter.
 */
#define	PUT32(bp, v)					\
	do {						\
		uint32_t Xv = (v);			\
		*(bp)++ = (uint8_t) Xv; Xv >>= 8;	\
		*(bp)++ = (uint8_t) Xv; Xv >>= 8;	\
		*(bp)++ = (uint8_t) Xv; Xv >>= 8;	\
		*(bp)++ = (uint8_t) Xv;			\
	} while (0)


/*
 * ccrand_bytes()
 *
 * Return the requested number of bytes.
 */
void
ccrand_bytes(ccrand_t * __restrict x, void * __restrict bp_void, size_t n_bytes)
{
	uint8_t *bp = bp_void;
	uint32_t i, rem, r;
	size_t words;

	if (n_bytes == 0) {
		return;		/* what the heck? */
	}
	words = n_bytes / sizeof(uint32_t);
	rem = (uint32_t)(n_bytes % sizeof(uint32_t));

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
	 * Get a word out of memory.  We're sure to need it.  This
	 * takes liberties with its knowledge of what __ccrand_gen16()
	 * does.  Also compute how many words remain in the buffer.
	 */
	if (i == N) {
		r = __ccrand_gen16(&x->v[V_BUF], &x->v[V_CTX]);
		i = 1;
	} else {
		r = x->v[i++];
	}

	/*
	 * Keep going until we work `words' down to zero.  We'll
	 * copy down what we have already at the top of the loop,
	 * then generate more if we need it.
	 */
	if (words > 0) {
		for (;;) {
			uint32_t n;

			PUT32(bp, r);
			words--;
			n = N - i;
			if (n > words) {
				n = (uint32_t) words;
			}
			words -= n;

			while (n) {
				PUT32(bp, x->v[i++]);
				n--;
			}

			if (words == 0) {
				break;
			}

			r = __ccrand_gen16(&x->v[V_BUF], &x->v[V_CTX]);
			i = 1;
		}

		/*
		 * If there's a fraction of a word left to write
		 * we'll need to get one more.  At the end of this
		 * write the index back into its spot.
		 */
		if (rem > 0) {
			if (i < N) {
				r = x->v[i++];
			} else {
				r = __ccrand_gen16(&x->v[V_BUF], &x->v[V_CTX]);
				i = 1;
			}
		}
	}

	/*
	 * Write the index back into the buffer in case we haven't already.
	 * Fill in the fraction of a word, remembering that chacha likes
	 * little-endian order.
	 */
	x->v[V_INDEX] = i;
	while (rem) {
		*bp++ = (uint8_t) r;
		r >>= 8;
		rem--;
	}
}
