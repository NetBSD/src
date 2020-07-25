/*	$NetBSD: aes_sse2_subr.c,v 1.3 2020/07/25 22:29:56 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_subr.c,v 1.3 2020/07/25 22:29:56 riastradh Exp $");

#ifdef _KERNEL
#include <sys/systm.h>
#include <lib/libkern/libkern.h>
#else
#include <err.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#define	KASSERT			assert
#define	panic(fmt, args...)	err(1, fmt, ##args)
#endif

#include <crypto/aes/aes.h>
#include <crypto/aes/arch/x86/aes_sse2.h>

#include "aes_sse2_impl.h"

void
aes_sse2_setkey(uint64_t rk[static 30], const void *key, uint32_t nrounds)
{
	size_t key_len;

	switch (nrounds) {
	case 10:
		key_len = 16;
		break;
	case 12:
		key_len = 24;
		break;
	case 14:
		key_len = 32;
		break;
	default:
		panic("invalid AES nrounds: %u", nrounds);
	}

	aes_sse2_keysched(rk, key, key_len);
}

void
aes_sse2_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load input block interleaved with garbage blocks.  */
	q[0] = aes_sse2_interleave_in(_mm_loadu_epi8(in));
	q[1] = q[2] = q[3] = _mm_setzero_si128();

	/* Transform to bitslice, decrypt, transform from bitslice.  */
	aes_sse2_ortho(q);
	aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_ortho(q);

	/* Store output block.  */
	_mm_storeu_epi8(out, aes_sse2_interleave_out(q[0]));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load input block interleaved with garbage blocks.  */
	q[0] = aes_sse2_interleave_in(_mm_loadu_epi8(in));
	q[1] = q[2] = q[3] = _mm_setzero_si128();

	/* Transform to bitslice, decrypt, transform from bitslice.  */
	aes_sse2_ortho(q);
	aes_sse2_bitslice_decrypt(nrounds, sk_exp, q);
	aes_sse2_ortho(q);

	/* Store output block.  */
	_mm_storeu_epi8(out, aes_sse2_interleave_out(q[0]));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i cv;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load the IV.  */
	cv = _mm_loadu_epi8(iv);

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Load input block and apply CV.  */
		q[0] = aes_sse2_interleave_in(cv ^ _mm_loadu_epi8(in));

		/* Transform to bitslice, encrypt, transform from bitslice.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Remember ciphertext as CV and store output block.  */
		cv = aes_sse2_interleave_out(q[0]);
		_mm_storeu_epi8(out, cv);
	}

	/* Store updated IV.  */
	_mm_storeu_epi8(iv, cv);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t ivp[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i cv, iv, w;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load the IV.  */
	iv = _mm_loadu_epi8(ivp);

	/* Load the last cipher block.  */
	cv = _mm_loadu_epi8(in + nbytes - 16);

	/* Store the updated IV.  */
	_mm_storeu_epi8(ivp, cv);

	/* Process the last blocks if not an even multiple of four.  */
	if (nbytes % (4*16)) {
		unsigned n = (nbytes/16) % 4;

		KASSERT(n > 0);
		KASSERT(n < 4);

		q[1] = q[2] = q[3] = _mm_setzero_si128();
		q[n - 1] = aes_sse2_interleave_in(cv);
		switch (nbytes % 64) {
		case 48:
			w = _mm_loadu_epi8(in + nbytes - 32);
			q[1] = aes_sse2_interleave_in(w);
			/*FALLTHROUGH*/
		case 32:
			w = _mm_loadu_epi8(in + nbytes - 48);
			q[0] = aes_sse2_interleave_in(w);
			/*FALLTHROUGH*/
		case 16:
			break;
		}

		/* Decrypt.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		do {
			n--;
			w = aes_sse2_interleave_out(q[n]);
			if ((nbytes -= 16) == 0)
				goto out;
			cv = _mm_loadu_epi8(in + nbytes - 16);
			_mm_storeu_epi8(out + nbytes, w ^ cv);
		} while (n);
	}

	for (;;) {
		KASSERT(nbytes >= 64);
		nbytes -= 64;

		/*
		 * 1. Set up upper cipher block from cv.
		 * 2. Load lower cipher block into cv and set it up.
		 * 3. Decrypt.
		 */
		q[3] = aes_sse2_interleave_in(cv);

		w = _mm_loadu_epi8(in + nbytes + 4*8);
		q[2] = aes_sse2_interleave_in(w);

		w = _mm_loadu_epi8(in + nbytes + 4*4);
		q[1] = aes_sse2_interleave_in(w);

		w = _mm_loadu_epi8(in + nbytes + 4*0);
		q[0] = aes_sse2_interleave_in(w);

		aes_sse2_ortho(q);
		aes_sse2_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Store the upper output block.  */
		w = aes_sse2_interleave_out(q[3]);
		cv = _mm_loadu_epi8(in + nbytes + 4*8);
		_mm_storeu_epi8(out + nbytes + 4*12, w ^ cv);

		/* Store the middle output blocks.  */
		w = aes_sse2_interleave_out(q[2]);
		cv = _mm_loadu_epi8(in + nbytes + 4*4);
		_mm_storeu_epi8(out + nbytes + 4*8, w ^ cv);

		w = aes_sse2_interleave_out(q[1]);
		cv = _mm_loadu_epi8(in + nbytes + 4*0);
		_mm_storeu_epi8(out + nbytes + 4*4, w ^ cv);

		/*
		 * Get the first output block, but don't load the CV
		 * yet -- it might be the previous ciphertext block, or
		 * it might be the IV.
		 */
		w = aes_sse2_interleave_out(q[0]);

		/* Stop if we've reached the first output block.  */
		if (nbytes == 0)
			goto out;

		/*
		 * Load the preceding cipher block, and apply it as the
		 * chaining value to this one.
		 */
		cv = _mm_loadu_epi8(in + nbytes - 16);
		_mm_storeu_epi8(out + nbytes, w ^ cv);
	}

out:	/* Store the first output block.  */
	_mm_storeu_epi8(out, w ^ iv);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static inline __m128i
aes_sse2_xts_update(__m128i t)
{
	const __m128i one = _mm_set_epi64x(1, 1);
	__m128i s, m, c;

	s = _mm_srli_epi64(t, 63);	/* 1 if high bit set else 0 */
	m = _mm_sub_epi64(s, one);	/* 0 if high bit set else -1 */
	m = _mm_shuffle_epi32(m, 0x4e);	/* swap halves */
	c = _mm_set_epi64x(1, 0x87);	/* carry */

	return _mm_slli_epi64(t, 1) ^ (c & ~m);
}

static int
aes_sse2_xts_update_selftest(void)
{
	static const struct {
		uint32_t in[4], out[4];
	} cases[] = {
		[0] = { {1}, {2} },
		[1] = { {0x80000000U,0,0,0}, {0,1,0,0} },
		[2] = { {0,0x80000000U,0,0}, {0,0,1,0} },
		[3] = { {0,0,0x80000000U,0}, {0,0,0,1} },
		[4] = { {0,0,0,0x80000000U}, {0x87,0,0,0} },
		[5] = { {0,0x80000000U,0,0x80000000U}, {0x87,0,1,0} },
	};
	unsigned i;
	uint32_t t[4];
	int result = 0;

	for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		t[0] = cases[i].in[0];
		t[1] = cases[i].in[1];
		t[2] = cases[i].in[2];
		t[3] = cases[i].in[3];
		_mm_storeu_epi8(t, aes_sse2_xts_update(_mm_loadu_epi8(t)));
		if (t[0] != cases[i].out[0] ||
		    t[1] != cases[i].out[1] ||
		    t[2] != cases[i].out[2] ||
		    t[3] != cases[i].out[3]) {
			printf("%s %u:"
			    " %"PRIx32" %"PRIx32" %"PRIx32" %"PRIx32"\n",
			    __func__, i, t[0], t[1], t[2], t[3]);
			result = -1;
		}
	}

	return result;
}

void
aes_sse2_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i w;
	__m128i t[5];
	unsigned i;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load tweak.  */
	t[0] = _mm_loadu_epi8(tweak);

	/* Handle the first block separately if odd number.  */
	if (nbytes % (4*16)) {
		/* Load up the tweaked inputs.  */
		for (i = 0; i < (nbytes/16) % 4; i++) {
			w = _mm_loadu_epi8(in + 16*i) ^ t[i];
			q[i] = aes_sse2_interleave_in(w);
			t[i + 1] = aes_sse2_xts_update(t[i]);
		}
		for (; i < 4; i++)
			q[i] = _mm_setzero_si128();

		/* Encrypt up to four blocks.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Store the tweaked outputs.  */
		for (i = 0; i < (nbytes/16) % 4; i++) {
			w = aes_sse2_interleave_out(q[i]);
			_mm_storeu_epi8(out + 16*i, w ^ t[i]);
		}

		/* Advance to the next block.  */
		t[0] = t[i];
		in += nbytes % (4*16);
		out += nbytes % (4*16);
		nbytes -= nbytes % (4*16);
		if (nbytes == 0)
			goto out;
	}

	do {
		KASSERT(nbytes % 64 == 0);
		KASSERT(nbytes >= 64);

		/* Load up the tweaked inputs.  */
		for (i = 0; i < 4; i++) {
			w = _mm_loadu_epi8(in + 16*i) ^ t[i];
			q[i] = aes_sse2_interleave_in(w);
			t[i + 1] = aes_sse2_xts_update(t[i]);
		}

		/* Encrypt four blocks.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Store the tweaked outputs.  */
		for (i = 0; i < 4; i++) {
			w = aes_sse2_interleave_out(q[i]);
			_mm_storeu_epi8(out + 16*i, w ^ t[i]);
		}

		/* Advance to the next block.  */
		t[0] = t[4];
		in += 64;
		out += 64;
		nbytes -= 64;
	} while (nbytes);

out:	/* Store the updated tweak.  */
	_mm_storeu_epi8(tweak, t[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
	explicit_memset(t, 0, sizeof t);
}

void
aes_sse2_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i w;
	__m128i t[5];
	unsigned i;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load tweak.  */
	t[0] = _mm_loadu_epi8(tweak);

	/* Handle the first block separately if odd number.  */
	if (nbytes % (4*16)) {
		/* Load up the tweaked inputs.  */
		for (i = 0; i < (nbytes/16) % 4; i++) {
			w = _mm_loadu_epi8(in + 16*i) ^ t[i];
			q[i] = aes_sse2_interleave_in(w);
			t[i + 1] = aes_sse2_xts_update(t[i]);
		}
		for (; i < 4; i++)
			q[i] = _mm_setzero_si128();

		/* Decrypt up to four blocks.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Store the tweaked outputs.  */
		for (i = 0; i < (nbytes/16) % 4; i++) {
			w = aes_sse2_interleave_out(q[i]);
			_mm_storeu_epi8(out + 16*i, w ^ t[i]);
		}

		/* Advance to the next block.  */
		t[0] = t[i];
		in += nbytes % (4*16);
		out += nbytes % (4*16);
		nbytes -= nbytes % (4*16);
		if (nbytes == 0)
			goto out;
	}

	do {
		KASSERT(nbytes % 64 == 0);
		KASSERT(nbytes >= 64);

		/* Load up the tweaked inputs.  */
		for (i = 0; i < 4; i++) {
			w = _mm_loadu_epi8(in + 16*i) ^ t[i];
			q[i] = aes_sse2_interleave_in(w);
			t[i + 1] = aes_sse2_xts_update(t[i]);
		}

		/* Decrypt four blocks.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Store the tweaked outputs.  */
		for (i = 0; i < 4; i++) {
			w = aes_sse2_interleave_out(q[i]);
			_mm_storeu_epi8(out + 16*i, w ^ t[i]);
		}

		/* Advance to the next block.  */
		t[0] = t[4];
		in += 64;
		out += 64;
		nbytes -= 64;
	} while (nbytes);

out:	/* Store the updated tweak.  */
	_mm_storeu_epi8(tweak, t[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
	explicit_memset(t, 0, sizeof t);
}

void
aes_sse2_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load initial authenticator.  */
	q[0] = aes_sse2_interleave_in(_mm_loadu_epi8(auth));

	for (; nbytes; nbytes -= 16, in += 16) {
		q[0] ^= aes_sse2_interleave_in(_mm_loadu_epi8(in));
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);
	}

	/* Store updated authenticator.  */
	_mm_storeu_epi8(auth, aes_sse2_interleave_out(q[0]));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i ctr;
	uint32_t c0, c1, c2, c3;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Set first block to authenticator.  */
	q[0] = aes_sse2_interleave_in(_mm_loadu_epi8(authctr));

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3 = be32dec(authctr + 16 + 4*3);

	/* Set other blocks to garbage -- can't take advantage.  */
	q[2] = q[3] = _mm_setzero_si128();

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Update authenticator.  */
		q[0] ^= aes_sse2_interleave_in(_mm_loadu_epi8(in));

		/* Increment 32-bit counter.  */
		ctr = _mm_set_epi32(bswap32(++c3), c2, c1, c0);
		q[1] = aes_sse2_interleave_in(ctr);

		/* Encrypt authenticator and counter.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);

		/* Encrypt with CTR output.  */
		_mm_storeu_epi8(out,
		    _mm_loadu_epi8(in) ^ aes_sse2_interleave_out(q[1]));
	}

	/* Update authenticator.  */
	_mm_storeu_epi8(authctr, aes_sse2_interleave_out(q[0]));

	/* Update counter.  */
	be32enc(authctr + 16 + 4*3, c3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	__m128i q[4];
	__m128i ctr, block;
	uint32_t c0, c1, c2, c3;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3 = be32dec(authctr + 16 + 4*3);

	/* Increment 32-bit counter.  */
	ctr = _mm_set_epi32(bswap32(++c3), c2, c1, c0);
	q[0] = aes_sse2_interleave_in(ctr);

	/*
	 * Set the other blocks to garbage -- we don't have any
	 * plaintext to authenticate yet.
	 */
	q[1] = q[2] = q[3] = _mm_setzero_si128();

	/* Encrypt first CTR.  */
	aes_sse2_ortho(q);
	aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_ortho(q);

	/* Load the initial authenticator.  */
	q[1] = aes_sse2_interleave_in(_mm_loadu_epi8(authctr));

	for (;; in += 16, out += 16) {
		/* Decrypt the block.  */
		block = _mm_loadu_epi8(in) ^ aes_sse2_interleave_out(q[0]);

		/* Update authenticator.  */
		q[1] ^= aes_sse2_interleave_in(block);

		/* Store plaintext.  */
		_mm_storeu_epi8(out, block);

		/* If this is the last block, stop.  */
		if ((nbytes -= 16) == 0)
			break;

		/* Increment 32-bit counter.  */
		ctr = _mm_set_epi32(bswap32(++c3), c2, c1, c0);
		q[0] = aes_sse2_interleave_in(ctr);

		/* Authenticate previous plaintext, encrypt next CTR.  */
		aes_sse2_ortho(q);
		aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_ortho(q);
	}

	/*
	 * Authenticate last plaintext.  We're only doing this for the
	 * authenticator, not for the counter, so don't bother to
	 * initialize q[0], q[2], q[3].  (Even for the sake of
	 * sanitizers, they're already initialized to something by
	 * now.)
	 */
	aes_sse2_ortho(q);
	aes_sse2_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_ortho(q);

	/* Update authenticator.  */
	_mm_storeu_epi8(authctr, aes_sse2_interleave_out(q[1]));

	/* Update counter.  */
	be32enc(authctr + 16 + 4*3, c3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

int
aes_sse2_selftest(void)
{

	if (aes_sse2_xts_update_selftest())
		return -1;

	/* XXX test aes_sse2_bitslice_decrypt */
	/* XXX test aes_sse2_bitslice_encrypt */
	/* XXX test aes_sse2_keysched */
	/* XXX test aes_sse2_ortho */
	/* XXX test aes_sse2_skey_expand */

	return 0;
}
