/* $NetBSD: gmac.c,v 1.2 2011/06/08 10:14:16 drochner Exp $ */
/* OpenBSD: gmac.c,v 1.3 2011/01/11 15:44:23 deraadt Exp */

/*
 * Copyright (c) 2010 Mike Belopuhov <mike@vantronix.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This code implements the Message Authentication part of the
 * Galois/Counter Mode (as being described in the RFC 4543) using
 * the AES cipher.  FIPS SP 800-38D describes the algorithm details.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <crypto/rijndael/rijndael.h>
#include <opencrypto/gmac.h>

void	ghash_gfmul(const uint32_t *, const uint32_t *, uint32_t *);
void	ghash_update(GHASH_CTX *, const uint8_t *, size_t);

/* Computes a block multiplication in the GF(2^128) */
void
ghash_gfmul(const uint32_t *X, const uint32_t *Y, uint32_t *product)
{
	uint32_t	v[4];
	uint32_t	mul;
	int		i;

	memcpy(v, Y, GMAC_BLOCK_LEN);
	memset(product, 0, GMAC_BLOCK_LEN);

	for (i = 0; i < GMAC_BLOCK_LEN * 8; i++) {
		/* update Z */
		if (X[i >> 5] & (1 << (~i & 31))) {
			product[0] ^= v[0];
			product[1] ^= v[1];
			product[2] ^= v[2];
			product[3] ^= v[3];
		} /* else: we preserve old values */

		/* update V */
		mul = v[3] & 1;
		v[3] = (v[2] << 31) | (v[3] >> 1);
		v[2] = (v[1] << 31) | (v[2] >> 1);
		v[1] = (v[0] << 31) | (v[1] >> 1);
		v[0] = (v[0] >> 1) ^ (0xe1000000 * mul);
	}
}

void
ghash_update(GHASH_CTX *ctx, const uint8_t *X, size_t len)
{
	uint32_t x;
	uint32_t *s = ctx->S;
	uint32_t *y = ctx->Z;
	int i, j;

	for (i = 0; i < len / GMAC_BLOCK_LEN; i++) {
		for (j = 0; j < GMAC_BLOCK_LEN/4; j++) {
			x = (X[0] << 24) | (X[1] << 16) | (X[2] << 8) | X[3];
			s[j] = y[j] ^ x;
			X += 4;
		}

		ghash_gfmul(ctx->H, ctx->S, ctx->S);

		y = s;
	}

	memcpy(ctx->Z, ctx->S, GMAC_BLOCK_LEN);
}

#define AESCTR_NONCESIZE	4

void
AES_GMAC_Init(AES_GMAC_CTX *ctx)
{

	memset(ctx, 0, sizeof(AES_GMAC_CTX));
}

void
AES_GMAC_Setkey(AES_GMAC_CTX *ctx, const uint8_t *key, uint16_t klen)
{
	int i;

	ctx->rounds = rijndaelKeySetupEnc(ctx->K, (const u_char *)key,
	    (klen - AESCTR_NONCESIZE) * 8);
	/* copy out salt to the counter block */
	memcpy(ctx->J, key + klen - AESCTR_NONCESIZE, AESCTR_NONCESIZE);
	/* prepare a hash subkey */
	rijndaelEncrypt(ctx->K, ctx->rounds, (void *)ctx->ghash.H,
			(void *)ctx->ghash.H);
	for (i = 0; i < 4; i++)
		ctx->ghash.H[i] = be32toh(ctx->ghash.H[i]);
}

void
AES_GMAC_Reinit(AES_GMAC_CTX *ctx, const uint8_t *iv, uint16_t ivlen)
{
	/* copy out IV to the counter block */
	memcpy(ctx->J + AESCTR_NONCESIZE, iv, ivlen);
}

int
AES_GMAC_Update(AES_GMAC_CTX *ctx, const uint8_t *data, uint16_t len)
{
	uint8_t		blk[16] = { 0 };
	int		plen;

	if (len > 0) {
		plen = len % GMAC_BLOCK_LEN;
		if (len >= GMAC_BLOCK_LEN)
			ghash_update(&ctx->ghash, data, len - plen);
		if (plen) {
			memcpy(blk, data + (len - plen), plen);
			ghash_update(&ctx->ghash, blk, GMAC_BLOCK_LEN);
		}
	}
	return (0);
}

void
AES_GMAC_Final(uint8_t digest[GMAC_DIGEST_LEN], AES_GMAC_CTX *ctx)
{
	uint8_t		keystream[GMAC_BLOCK_LEN], *k, *d;
	int		i;

	/* do one round of GCTR */
	ctx->J[GMAC_BLOCK_LEN - 1] = 1;
	rijndaelEncrypt(ctx->K, ctx->rounds, ctx->J, keystream);
	k = keystream;
	d = digest;
	for (i = 0; i < GMAC_DIGEST_LEN/4; i++) {
		d[0] = (uint8_t)(ctx->ghash.S[i] >> 24) ^ k[0];
		d[1] = (uint8_t)(ctx->ghash.S[i] >> 16) ^ k[1];
		d[2] = (uint8_t)(ctx->ghash.S[i] >> 8) ^ k[2];
		d[3] = (uint8_t)ctx->ghash.S[i] ^ k[3];
		d += 4;
		k += 4;
	}
	memset(keystream, 0, sizeof(keystream));
}
