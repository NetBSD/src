/*	$NetBSD: blake2s.c,v 1.1 2020/08/20 21:21:05 riastradh Exp $	*/

/*-
 * Copyright (c) 2015 Taylor R. Campbell
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef _KERNEL

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: blake2s.c,v 1.1 2020/08/20 21:21:05 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>

#include <lib/libkern/libkern.h>

#else

#define	_POSIX_C_SOURCE	200809L

#include <assert.h>
#include <stdint.h>
#include <string.h>

#endif

#include "blake2s.h"

#include <sys/endian.h>

static inline uint32_t
rotr32(uint32_t x, unsigned c)
{

	return ((x >> c) | (x << (32 - c)));
}

#define	BLAKE2S_G(VA, VB, VC, VD, X, Y)	do				      \
{									      \
	(VA) = (VA) + (VB) + (X);					      \
	(VD) = rotr32((VD) ^ (VA), 16);					      \
	(VC) = (VC) + (VD);						      \
	(VB) = rotr32((VB) ^ (VC), 12);					      \
	(VA) = (VA) + (VB) + (Y);					      \
	(VD) = rotr32((VD) ^ (VA), 8);					      \
	(VC) = (VC) + (VD);						      \
	(VB) = rotr32((VB) ^ (VC), 7);					      \
} while (0)

static const uint32_t blake2s_iv[8] = {
	0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
	0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
};

static const uint8_t blake2s_sigma[10][16] = {
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
	{ 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
	{  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
	{  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
	{  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
	{ 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
	{ 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
	{  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
	{ 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 },
};

static void
blake2s_compress(uint32_t h[8], uint64_t c, uint32_t last,
    const uint8_t in[64])
{
	uint32_t v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15;
	uint32_t m[16];
	unsigned i;

	/* Load the variables: first 8 from state, next 8 from IV.  */
	v0 = h[0];
	v1 = h[1];
	v2 = h[2];
	v3 = h[3];
	v4 = h[4];
	v5 = h[5];
	v6 = h[6];
	v7 = h[7];
	v8 = blake2s_iv[0];
	v9 = blake2s_iv[1];
	v10 = blake2s_iv[2];
	v11 = blake2s_iv[3];
	v12 = blake2s_iv[4];
	v13 = blake2s_iv[5];
	v14 = blake2s_iv[6];
	v15 = blake2s_iv[7];

	/* Incorporate the block counter and whether this is last.  */
	v12 ^= c & 0xffffffffU;
	v13 ^= c >> 32;
	v14 ^= last;

	/* Load the message block.  */
	for (i = 0; i < 16; i++)
		m[i] = le32dec(in + 4*i);

	/* Transform the variables.  */
	for (i = 0; i < 10; i++) {
		const uint8_t *sigma = blake2s_sigma[i];

		BLAKE2S_G(v0, v4,  v8, v12, m[sigma[ 0]], m[sigma[ 1]]);
		BLAKE2S_G(v1, v5,  v9, v13, m[sigma[ 2]], m[sigma[ 3]]);
		BLAKE2S_G(v2, v6, v10, v14, m[sigma[ 4]], m[sigma[ 5]]);
		BLAKE2S_G(v3, v7, v11, v15, m[sigma[ 6]], m[sigma[ 7]]);
		BLAKE2S_G(v0, v5, v10, v15, m[sigma[ 8]], m[sigma[ 9]]);
		BLAKE2S_G(v1, v6, v11, v12, m[sigma[10]], m[sigma[11]]);
		BLAKE2S_G(v2, v7,  v8, v13, m[sigma[12]], m[sigma[13]]);
		BLAKE2S_G(v3, v4,  v9, v14, m[sigma[14]], m[sigma[15]]);
	}

	/* Update the state.  */
	h[0] ^= v0 ^ v8;
	h[1] ^= v1 ^ v9;
	h[2] ^= v2 ^ v10;
	h[3] ^= v3 ^ v11;
	h[4] ^= v4 ^ v12;
	h[5] ^= v5 ^ v13;
	h[6] ^= v6 ^ v14;
	h[7] ^= v7 ^ v15;

	(void)explicit_memset(m, 0, sizeof m);
}

void
blake2s_init(struct blake2s *B, size_t dlen, const void *key, size_t keylen)
{
	uint32_t param0;
	unsigned i;

	assert(0 < dlen);
	assert(dlen <= 32);
	assert(keylen <= 32);

	/* Record the digest length.  */
	B->dlen = dlen;

	/* Initialize the buffer.  */
	B->nb = 0;

	/* Initialize the state.  */
	B->c = 0;
	for (i = 0; i < 8; i++)
		B->h[i] = blake2s_iv[i];

	/*
	 * Set the parameters.  We support only variable digest and key
	 * lengths: no tree hashing, no salt, no personalization.
	 */
	param0 = 0;
	param0 |= (uint32_t)dlen << 0;
	param0 |= (uint32_t)keylen << 8;
	param0 |= (uint32_t)1 << 16; /* tree fanout = 1 */
	param0 |= (uint32_t)1 << 24; /* tree depth = 1 */
	B->h[0] ^= param0;

	/* If there's a key, compress it as the first message block.  */
	if (keylen) {
		static const uint8_t zero_block[64];

		blake2s_update(B, key, keylen);
		blake2s_update(B, zero_block, 64 - keylen);
	}
}

void
blake2s_update(struct blake2s *B, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t n = len;

	/* Check the current state of the buffer.  */
	if (n <= 64u - B->nb) {
		/* Can at most exactly fill the buffer.  */
		(void)memcpy(&B->b[B->nb], p, n);
		B->nb += n;
		return;
	} else if (0 < B->nb) {
		/* Can fill the buffer and go on.  */
		(void)memcpy(&B->b[B->nb], p, 64 - B->nb);
		B->c += 64;
		blake2s_compress(B->h, B->c, 0, B->b);
		p += 64 - B->nb;
		n -= 64 - B->nb;
	}

	/* At a block boundary.  Compress straight from the input.  */
	while (64 < n) {
		B->c += 64;
		blake2s_compress(B->h, B->c, 0, p);
		p += 64;
		n -= 64;
	}

	/*
	 * Put whatever's left in the buffer.  We may fill the buffer,
	 * but we can't compress in that case until we know whether we
	 * are compressing the last block or not.
	 */
	(void)memcpy(B->b, p, n);
	B->nb = n;
}

void
blake2s_final(struct blake2s *B, void *digest)
{
	uint8_t *d = digest;
	unsigned dlen = B->dlen;
	unsigned i;

	/* Pad with zeros, and do the last compression.  */
	B->c += B->nb;
	for (i = B->nb; i < 64; i++)
		B->b[i] = 0;
	blake2s_compress(B->h, B->c, ~(uint32_t)0, B->b);

	/* Reveal the first dlen/4 words of the state.  */
	for (i = 0; i < dlen/4; i++)
		le32enc(d + 4*i, B->h[i]);
	d += 4*i;
	dlen -= 4*i;

	/* If the caller wants a partial word, reveal that too.  */
	if (dlen) {
		uint32_t hi = B->h[i];

		do {
			*d++ = hi;
			hi >>= 8;
		} while (--dlen);
	}

	/* Erase the state.  */
	(void)explicit_memset(B, 0, sizeof B);
}

void
blake2s(void *digest, size_t dlen, const void *key, size_t keylen,
    const void *in, size_t inlen)
{
	struct blake2s ctx;

	blake2s_init(&ctx, dlen, key, keylen);
	blake2s_update(&ctx, in, inlen);
	blake2s_final(&ctx, digest);
}

static void
blake2_selftest_prng(void *buf, size_t len, uint32_t seed)
{
	uint8_t *p = buf;
	size_t n = len;
	uint32_t t, a, b;

	a = 0xdead4bad * seed;
	b = 1;

	while (n--) {
		t = a + b;
		*p++ = t >> 24;
		a = b;
		b = t;
	}
}

int
blake2s_selftest(void)
{
	const uint8_t d0[32] = {
		0x6a,0x41,0x1f,0x08,0xce,0x25,0xad,0xcd,
		0xfb,0x02,0xab,0xa6,0x41,0x45,0x1c,0xec,
		0x53,0xc5,0x98,0xb2,0x4f,0x4f,0xc7,0x87,
		0xfb,0xdc,0x88,0x79,0x7f,0x4c,0x1d,0xfe,
	};
	const unsigned dlen[4] = { 16, 20, 28, 32 };
	const unsigned mlen[6] = { 0, 3, 64, 65, 255, 1024 };
	uint8_t m[1024], d[32], k[32];
	struct blake2s ctx;
	unsigned di, mi, i;

	blake2s_init(&ctx, 32, NULL, 0);
	for (di = 0; di < 4; di++) {
		for (mi = 0; mi < 6; mi++) {
			blake2_selftest_prng(m, mlen[mi], mlen[mi]);
			blake2s(d, dlen[di], NULL, 0, m, mlen[mi]);
			blake2s_update(&ctx, d, dlen[di]);

			blake2_selftest_prng(k, dlen[di], dlen[di]);
			blake2s(d, dlen[di], k, dlen[di], m, mlen[mi]);
			blake2s_update(&ctx, d, dlen[di]);
		}
	}
	blake2s_final(&ctx, d);
	for (i = 0; i < 32; i++) {
		if (d[i] != d0[i])
			return -1;
	}

	return 0;
}

#ifdef _KERNEL

MODULE(MODULE_CLASS_MISC, blake2s, NULL);

static int
blake2s_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (blake2s_selftest())
			panic("blake2s: self-test failed");
		aprint_verbose("blake2s: self-test passed\n");
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}

#endif
