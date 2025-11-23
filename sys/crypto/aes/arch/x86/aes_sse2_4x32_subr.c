/*	$NetBSD: aes_sse2_4x32_subr.c,v 1.1 2025/11/23 22:48:27 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_4x32_subr.c,v 1.1 2025/11/23 22:48:27 riastradh Exp $");

#include <crypto/aes/aes.h>

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

#include "aes_sse2_4x32_impl.h"
#include "aes_sse2_4x32_subr.h"

#ifndef _MM_TRANSPOSE4_EPI32
#define	_MM_TRANSPOSE4_EPI32(r0, r1, r2, r3) do				      \
{									      \
	__m128i _mm_tmp0, _mm_tmp1, _mm_tmp2, _mm_tmp3;			      \
									      \
	_mm_tmp0 = _mm_unpacklo_epi32(r0, r1);				      \
	_mm_tmp2 = _mm_unpacklo_epi32(r2, r3);				      \
	_mm_tmp1 = _mm_unpackhi_epi32(r0, r1);				      \
	_mm_tmp3 = _mm_unpackhi_epi32(r2, r3);				      \
	(r0) = (__m128i)_mm_movelh_ps((__m128)_mm_tmp0, (__m128)_mm_tmp2);    \
	(r1) = (__m128i)_mm_movehl_ps((__m128)_mm_tmp2, (__m128)_mm_tmp0);    \
	(r2) = (__m128i)_mm_movelh_ps((__m128)_mm_tmp1, (__m128)_mm_tmp3);    \
	(r3) = (__m128i)_mm_movehl_ps((__m128)_mm_tmp3, (__m128)_mm_tmp1);    \
} while (0)
#endif

void
aes_sse2_4x32_setkey(uint32_t rk[static 60], const void *key, uint32_t nrounds)
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

	aes_sse2_4x32_keysched(rk, key, key_len);
}

void
aes_sse2_4x32_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Load input block interleaved with garbage blocks.  */
	q[0] = _mm_loadu_epi8(in);
	q[2] = q[4] = q[6] = _mm_setzero_si128();
	q[1] = q[3] = q[5] = q[7] = _mm_setzero_si128();

	/* Transform to bitslice, decrypt, transform from bitslice.  */
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	aes_sse2_4x32_ortho(q);
	aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_4x32_ortho(q);
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

	/* Store output block.  */
	_mm_storeu_epi8(out, q[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_4x32_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk);

	/* Load input block interleaved with garbage blocks.  */
	q[0] = _mm_loadu_epi8(in);
	q[2] = q[4] = q[6] = _mm_setzero_si128();
	q[1] = q[3] = q[5] = q[7] = _mm_setzero_si128();

	/* Transform to bitslice, decrypt, transform from bitslice.  */
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	aes_sse2_4x32_ortho(q);
	aes_sse2_4x32_bitslice_decrypt(nrounds, sk_exp, q);
	aes_sse2_4x32_ortho(q);
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

	/* Store output block.  */
	_mm_storeu_epi8(out, q[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_4x32_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	__m128i cv;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Load the IV.  */
	cv = _mm_loadu_epi8(iv);

	/*
	 * Zero the registers we won't be using, since CBC encryption
	 * is inherently sequential so we can only do one block at a
	 * time.
	 */
	q[2] = q[4] = q[6] = _mm_setzero_si128();
	q[1] = q[3] = q[5] = q[7] = _mm_setzero_si128();

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Load input block and apply CV.  */
		q[0] = cv ^ _mm_loadu_epi8(in);

		/* Transform to bitslice, encrypt, transform from bitslice.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

		/* Remember ciphertext as CV and store output block.  */
		cv = q[0];
		_mm_storeu_epi8(out, cv);
	}

	/* Store updated IV.  */
	_mm_storeu_epi8(iv, cv);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_4x32_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t ivp[static 16],
    uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	__m128i cv, iv, w;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk);

	/* Load the IV.  */
	iv = _mm_loadu_epi8(ivp);

	/* Load the last cipher block.  */
	cv = _mm_loadu_epi8(in + nbytes - 16);

	/* Store the updated IV.  */
	_mm_storeu_epi8(ivp, cv);

	/* Process the last blocks if not an even multiple of eight.  */
	if (nbytes % (8*16)) {
		unsigned i, n = (nbytes/16) % 8;

		KASSERT(n > 0);
		KASSERT(n < 8);

		for (i = 8; i --> n;)
			q[i] = _mm_setzero_si128();
		q[i] = cv;
		while (i --> 0)
			q[i] = _mm_loadu_epi8(in + nbytes - 16*n + 16*i);

		/* Decrypt up to seven blocks.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		do {
			n--;
			w = q[n];
			if ((nbytes -= 16) == 0)
				goto out;
			cv = _mm_loadu_epi8(in + nbytes - 16);
			_mm_storeu_epi8(out + nbytes, w ^ cv);
		} while (n);
	}

	for (;;) {
		KASSERT(nbytes >= 128);
		nbytes -= 128;

		/*
		 * 1. Set up upper cipher block from cv.
		 * 2. Load lower cipher blocks from input.
		 */
		q[7] = cv;	/* _mm_loadu_epi8(in + nbytes + 16*7) */
		q[6] = _mm_loadu_epi8(in + nbytes + 16*6);
		q[5] = _mm_loadu_epi8(in + nbytes + 16*5);
		q[4] = _mm_loadu_epi8(in + nbytes + 16*4);
		q[3] = _mm_loadu_epi8(in + nbytes + 16*3);
		q[2] = _mm_loadu_epi8(in + nbytes + 16*2);
		q[1] = _mm_loadu_epi8(in + nbytes + 16*1);
		q[0] = _mm_loadu_epi8(in + nbytes + 16*0);

		/* Decrypt eight blocks at a time.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		/* Store the seven upper output blocks.  */
		cv = _mm_loadu_epi8(in + nbytes + 16*6);
		_mm_storeu_epi8(out + nbytes + 16*7, cv ^ q[7]);
		cv = _mm_loadu_epi8(in + nbytes + 16*5);
		_mm_storeu_epi8(out + nbytes + 16*6, cv ^ q[6]);
		cv = _mm_loadu_epi8(in + nbytes + 16*4);
		_mm_storeu_epi8(out + nbytes + 16*5, cv ^ q[5]);
		cv = _mm_loadu_epi8(in + nbytes + 16*3);
		_mm_storeu_epi8(out + nbytes + 16*4, cv ^ q[4]);
		cv = _mm_loadu_epi8(in + nbytes + 16*2);
		_mm_storeu_epi8(out + nbytes + 16*3, cv ^ q[3]);
		cv = _mm_loadu_epi8(in + nbytes + 16*1);
		_mm_storeu_epi8(out + nbytes + 16*2, cv ^ q[2]);
		cv = _mm_loadu_epi8(in + nbytes + 16*0);
		_mm_storeu_epi8(out + nbytes + 16*1, cv ^ q[1]);

		/*
		 * Get the first output block, but don't load the CV
		 * yet -- it might be the previous ciphertext block, or
		 * it might be the IV.
		 */
		w = q[0];

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
aes_sse2_4x32_xts_update(__m128i t)
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
aes_sse2_4x32_xts_update_selftest(void)
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
		_mm_storeu_epi8(t, aes_sse2_4x32_xts_update(_mm_loadu_epi8(t)));
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
aes_sse2_4x32_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	__m128i t[9];
	unsigned i;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Load tweak.  */
	t[0] = _mm_loadu_epi8(tweak);

	/* Handle the first block separately if odd number.  */
	if (nbytes % (8*16)) {
		/* Load up the tweaked inputs.  */
		for (i = 0; i < (nbytes/16) % 8; i++) {
			q[i] = _mm_loadu_epi8(in + 16*i) ^ t[i];
			t[i + 1] = aes_sse2_4x32_xts_update(t[i]);
		}
		for (; i < 8; i++)
			q[i] = _mm_setzero_si128();

		/* Encrypt up to seven blocks.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		/* Store the tweaked outputs.  */
		for (i = 0; i < (nbytes/16) % 8; i++)
			_mm_storeu_epi8(out + 16*i, q[i] ^ t[i]);

		/* Advance to the next block.  */
		t[0] = t[i];
		in += nbytes % (8*16);
		out += nbytes % (8*16);
		nbytes -= nbytes % (8*16);
		if (nbytes == 0)
			goto out;
	}

	do {
		KASSERT(nbytes % 128 == 0);
		KASSERT(nbytes >= 128);

		/* Load up the tweaked inputs.  */
		for (i = 0; i < 8; i++) {
			q[i] = _mm_loadu_epi8(in + 16*i) ^ t[i];
			t[i + 1] = aes_sse2_4x32_xts_update(t[i]);
		}

		/* Encrypt eight blocks.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		/* Store the tweaked outputs.  */
		for (i = 0; i < 8; i++)
			_mm_storeu_epi8(out + 16*i, q[i] ^ t[i]);

		/* Advance to the next block.  */
		t[0] = t[8];
		in += 128;
		out += 128;
		nbytes -= 128;
	} while (nbytes);

out:	/* Store the updated tweak.  */
	_mm_storeu_epi8(tweak, t[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
	explicit_memset(t, 0, sizeof t);
}

void
aes_sse2_4x32_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	__m128i t[9];
	unsigned i;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk);

	/* Load tweak.  */
	t[0] = _mm_loadu_epi8(tweak);

	/* Handle the first block separately if odd number.  */
	if (nbytes % (8*16)) {
		/* Load up the tweaked inputs.  */
		for (i = 0; i < (nbytes/16) % 8; i++) {
			q[i] = _mm_loadu_epi8(in + 16*i) ^ t[i];
			t[i + 1] = aes_sse2_4x32_xts_update(t[i]);
		}
		for (; i < 8; i++)
			q[i] = _mm_setzero_si128();

		/* Decrypt up to seven blocks.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		/* Store the tweaked outputs.  */
		for (i = 0; i < (nbytes/16) % 8; i++)
			_mm_storeu_epi8(out + 16*i, q[i] ^ t[i]);

		/* Advance to the next block.  */
		t[0] = t[i];
		in += nbytes % (8*16);
		out += nbytes % (8*16);
		nbytes -= nbytes % (8*16);
		if (nbytes == 0)
			goto out;
	}

	do {
		KASSERT(nbytes % 128 == 0);
		KASSERT(nbytes >= 128);

		/* Load up the tweaked inputs.  */
		for (i = 0; i < 8; i++) {
			q[i] = _mm_loadu_epi8(in + 16*i) ^ t[i];
			t[i + 1] = aes_sse2_4x32_xts_update(t[i]);
		}

		/* Decrypt eight blocks.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_decrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		_MM_TRANSPOSE4_EPI32(q[1], q[3], q[5], q[7]);

		/* Store the tweaked outputs.  */
		for (i = 0; i < 8; i++)
			_mm_storeu_epi8(out + 16*i, q[i] ^ t[i]);

		/* Advance to the next block.  */
		t[0] = t[8];
		in += 128;
		out += 128;
		nbytes -= 128;
	} while (nbytes);

out:	/* Store the updated tweak.  */
	_mm_storeu_epi8(tweak, t[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
	explicit_memset(t, 0, sizeof t);
}

void
aes_sse2_4x32_cbcmac_update1(const struct aesenc *enc,
    const uint8_t in[static 16], size_t nbytes,
    uint8_t auth[static 16], uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Initialize garbage blocks.  */
	q[1] = q[2] = q[3] = q[4] = q[5] = q[6] = q[7] = _mm_setzero_si128();

	/* Load initial authenticator.  */
	q[0] = _mm_loadu_epi8(auth);

	for (; nbytes; nbytes -= 16, in += 16) {
		/* Combine input block.  */
		q[0] ^= _mm_loadu_epi8(in);

		/* Transform to bitslice, encrypt, transform from bitslice.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	}

	/* Store updated authenticator.  */
	_mm_storeu_epi8(auth, q[0]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_4x32_ccm_enc1(const struct aesenc *enc,
    const uint8_t in[static 16], uint8_t out[static 16], size_t nbytes,
    uint8_t authctr[static 32], uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	uint32_t c0, c1, c2, c3be;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Set first block to authenticator.  */
	q[0] = _mm_loadu_epi8(authctr);

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3be = bswap32(le32dec(authctr + 16 + 4*3));

	/* Set other blocks to garbage -- can't take advantage.  */
	q[1] = q[3] = q[4] = q[5] = q[6] = q[7] = _mm_setzero_si128();

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Update authenticator.  */
		q[0] ^= _mm_loadu_epi8(in);

		/* Increment 32-bit counter.  */
		q[2] = _mm_set_epi32(bswap32(++c3be), c2, c1, c0);

		/* Encrypt authenticator and counter.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

		/* Encrypt with CTR output.  */
		_mm_storeu_epi8(out, _mm_loadu_epi8(in) ^ q[2]);
	}

	/* Update authenticator.  */
	_mm_storeu_epi8(authctr, q[0]);

	/* Update counter.  */
	le32enc(authctr + 16 + 4*3, bswap32(c3be));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

void
aes_sse2_4x32_ccm_dec1(const struct aesenc *enc,
    const uint8_t in[static 16], uint8_t out[static 16], size_t nbytes,
    uint8_t authctr[static 32], uint32_t nrounds)
{
	uint32_t sk_exp[120];
	__m128i q[8];
	uint32_t c0, c1, c2, c3be;
	__m128i b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	aes_sse2_4x32_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk);

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3be = bswap32(le32dec(authctr + 16 + 4*3));

	/* Increment 32-bit counter.  */
	q[0] = _mm_set_epi32(bswap32(++c3be), c2, c1, c0);

	/*
	 * Set the other blocks to garbage -- we don't have any
	 * plaintext to authenticate yet.
	 */
	q[1] = q[3] = q[4] = q[5] = q[6] = q[7] = _mm_setzero_si128();

	/* Encrypt first CTR.  */
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	aes_sse2_4x32_ortho(q);
	aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_4x32_ortho(q);
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

	/* Load the initial authenticator.  */
	q[2] = _mm_loadu_epi8(authctr);

	for (;; in += 16, out += 16) {
		/* Decrypt the block.  */
		b = _mm_loadu_epi8(in) ^ q[0];

		/* Update authenticator.  */
		q[2] ^= b;

		/* Store plaintext.  */
		_mm_storeu_epi8(out, b);

		/* If this is the last block, stop.  */
		if ((nbytes -= 16) == 0)
			break;

		/* Increment 32-bit counter.  */
		q[0] = _mm_set_epi32(bswap32(++c3be), c2, c1, c0);

		/* Authenticate previous plaintext, encrypt next CTR.  */
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
		aes_sse2_4x32_ortho(q);
		aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
		aes_sse2_4x32_ortho(q);
		_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	}

	/*
	 * Authenticate last plaintext.  We're only doing this for the
	 * authenticator, not for the counter, so don't bother to
	 * initialize q[0].  (Even for the sake of sanitizers, they're
	 * already initialized to something by now.)
	 */
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);
	aes_sse2_4x32_ortho(q);
	aes_sse2_4x32_bitslice_encrypt(nrounds, sk_exp, q);
	aes_sse2_4x32_ortho(q);
	_MM_TRANSPOSE4_EPI32(q[0], q[2], q[4], q[6]);

	/* Update authenticator.  */
	_mm_storeu_epi8(authctr, q[2]);

	/* Update counter.  */
	le32enc(authctr + 16 + 4*3, bswap32(c3be));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

int
aes_sse2_4x32_selftest(void)
{

	if (aes_sse2_4x32_xts_update_selftest())
		return -1;

	/* XXX test aes_sse2_4x32_bitslice_decrypt */
	/* XXX test aes_sse2_4x32_bitslice_encrypt */
	/* XXX test aes_sse2_4x32_keysched */
	/* XXX test aes_sse2_4x32_ortho */
	/* XXX test aes_sse2_4x32_skey_expand */

	return 0;
}
