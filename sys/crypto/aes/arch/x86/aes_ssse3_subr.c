/*	$NetBSD: aes_ssse3_subr.c,v 1.3 2020/07/25 22:31:04 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_ssse3_subr.c,v 1.3 2020/07/25 22:31:04 riastradh Exp $");

#ifdef _KERNEL
#include <sys/systm.h>
#include <lib/libkern/libkern.h>
#else
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#define	KASSERT			assert
#endif

#include "aes_ssse3_impl.h"

static inline __m128i
loadblock(const void *in)
{
	return _mm_loadu_epi8(in);
}

static inline void
storeblock(void *out, __m128i block)
{
	_mm_storeu_epi8(out, block);
}

void
aes_ssse3_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	__m128i block;

	block = loadblock(in);
	block = aes_ssse3_enc1(enc, block, nrounds);
	storeblock(out, block);
}

void
aes_ssse3_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	__m128i block;

	block = loadblock(in);
	block = aes_ssse3_dec1(dec, block, nrounds);
	storeblock(out, block);
}

void
aes_ssse3_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	__m128i cv;

	KASSERT(nbytes);

	cv = loadblock(iv);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		cv ^= loadblock(in);
		cv = aes_ssse3_enc1(enc, cv, nrounds);
		storeblock(out, cv);
	}
	storeblock(iv, cv);
}

void
aes_ssse3_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	__m128i iv0, cv, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	iv0 = loadblock(iv);
	cv = loadblock(in + nbytes - 16);
	storeblock(iv, cv);

	for (;;) {
		b = aes_ssse3_dec1(dec, cv, nrounds);
		if ((nbytes -= 16) == 0)
			break;
		cv = loadblock(in + nbytes - 16);
		storeblock(out + nbytes, b ^ cv);
	}
	storeblock(out, b ^ iv0);
}

static inline __m128i
aes_ssse3_xts_update(__m128i t)
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
aes_ssse3_xts_update_selftest(void)
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
		storeblock(t, aes_ssse3_xts_update(loadblock(t)));
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
aes_ssse3_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	__m128i t, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	t = loadblock(tweak);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		b = t ^ loadblock(in);
		b = aes_ssse3_enc1(enc, b, nrounds);
		storeblock(out, t ^ b);
		t = aes_ssse3_xts_update(t);
	}
	storeblock(tweak, t);
}

void
aes_ssse3_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	__m128i t, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	t = loadblock(tweak);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		b = t ^ loadblock(in);
		b = aes_ssse3_dec1(dec, b, nrounds);
		storeblock(out, t ^ b);
		t = aes_ssse3_xts_update(t);
	}
	storeblock(tweak, t);
}

void
aes_ssse3_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth0[static 16], uint32_t nrounds)
{
	__m128i auth;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(auth0);
	for (; nbytes; nbytes -= 16, in += 16)
		auth = aes_ssse3_enc1(enc, auth ^ loadblock(in), nrounds);
	storeblock(auth0, auth);
}

void
aes_ssse3_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	const __m128i ctr32_inc = _mm_set_epi32(1, 0, 0, 0);
	const __m128i bs32 =
	    _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
	__m128i auth, ctr_be, ctr, ptxt;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(authctr);
	ctr_be = loadblock(authctr + 16);
	ctr = _mm_shuffle_epi8(ctr_be, bs32);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		ptxt = loadblock(in);
		auth = aes_ssse3_enc1(enc, auth ^ ptxt, nrounds);
		ctr = _mm_add_epi32(ctr, ctr32_inc);
		ctr_be = _mm_shuffle_epi8(ctr, bs32);
		storeblock(out, ptxt ^ aes_ssse3_enc1(enc, ctr_be, nrounds));
	}
	storeblock(authctr, auth);
	storeblock(authctr + 16, ctr_be);
}

void
aes_ssse3_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	const __m128i ctr32_inc = _mm_set_epi32(1, 0, 0, 0);
	const __m128i bs32 =
	    _mm_set_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);
	__m128i auth, ctr_be, ctr, ptxt;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(authctr);
	ctr_be = loadblock(authctr + 16);
	ctr = _mm_shuffle_epi8(ctr_be, bs32);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		ctr = _mm_add_epi32(ctr, ctr32_inc);
		ctr_be = _mm_shuffle_epi8(ctr, bs32);
		ptxt = loadblock(in) ^ aes_ssse3_enc1(enc, ctr_be, nrounds);
		storeblock(out, ptxt);
		auth = aes_ssse3_enc1(enc, auth ^ ptxt, nrounds);
	}
	storeblock(authctr, auth);
	storeblock(authctr + 16, ctr_be);
}

int
aes_ssse3_selftest(void)
{

	if (aes_ssse3_xts_update_selftest())
		return -1;

	return 0;
}
