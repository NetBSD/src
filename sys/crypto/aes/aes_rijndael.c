/*	$NetBSD: aes_rijndael.c,v 1.2 2020/07/25 22:14:35 riastradh Exp $	*/

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

/*
 * Legacy `Rijndael' API
 *
 *	rijndael_set_key
 *	rijndael_encrypt
 *	rijndael_decrypt
 *
 *	rijndaelKeySetupEnc
 *	rijndaelKeySetupDec
 *	rijndaelEncrypt
 *	rijndaelDecrypt
 *	rijndael_makeKey
 *	rijndael_cipherInit
 *	rijndael_blockEncrypt
 *	rijndael_blockDecrypt
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_rijndael.c,v 1.2 2020/07/25 22:14:35 riastradh Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_cbc.h>
#include <crypto/aes/aes_xts.h>
#include <crypto/rijndael/rijndael.h>
#include <crypto/rijndael/rijndael-alg-fst.h>
#include <crypto/rijndael/rijndael-api-fst.h>

void
rijndael_set_key(rijndael_ctx *ctx, const uint8_t *key, int keybits)
{

	ctx->Nr = rijndaelKeySetupEnc(ctx->ek, key, keybits);
	rijndaelKeySetupDec(ctx->dk, key, keybits);
}

void
rijndael_encrypt(const rijndael_ctx *ctx, const uint8_t *in, uint8_t *out)
{

	rijndaelEncrypt(ctx->ek, ctx->Nr, in, out);
}

void
rijndael_decrypt(const rijndael_ctx *ctx, const u_char *in, uint8_t *out)
{

	rijndaelDecrypt(ctx->dk, ctx->Nr, in, out);
}

int
rijndaelKeySetupEnc(uint32_t *rk, const uint8_t *key, int keybits)
{
	struct aesenc enc;
	unsigned nrounds;

	switch (keybits) {
	case 128:
		nrounds = aes_setenckey128(&enc, key);
		break;
	case 192:
		nrounds = aes_setenckey192(&enc, key);
		break;
	case 256:
		nrounds = aes_setenckey256(&enc, key);
		break;
	default:
		panic("invalid AES key bits: %d", keybits);
	}

	memcpy(rk, enc.aese_aes.aes_rk, 4*(nrounds + 1)*sizeof(rk[0]));
	explicit_memset(&enc, 0, sizeof enc);

	return nrounds;
}

int
rijndaelKeySetupDec(uint32_t *rk, const uint8_t *key, int keybits)
{
	struct aesdec dec;
	unsigned nrounds;

	switch (keybits) {
	case 128:
		nrounds = aes_setdeckey128(&dec, key);
		break;
	case 192:
		nrounds = aes_setdeckey192(&dec, key);
		break;
	case 256:
		nrounds = aes_setdeckey256(&dec, key);
		break;
	default:
		panic("invalid AES key bits: %d", keybits);
	}

	memcpy(rk, dec.aesd_aes.aes_rk, 4*(nrounds + 1)*sizeof(rk[0]));
	explicit_memset(&dec, 0, sizeof dec);

	return nrounds;
}

void
rijndaelEncrypt(const uint32_t *rk, int nrounds, const uint8_t in[16],
    uint8_t out[16])
{
	struct aesenc enc;

	memcpy(enc.aese_aes.aes_rk, rk, 4*(nrounds + 1)*sizeof(rk[0]));
	aes_enc(&enc, in, out, nrounds);
	explicit_memset(&enc, 0, sizeof enc);
}

void
rijndaelDecrypt(const uint32_t *rk, int nrounds, const uint8_t in[16],
    uint8_t out[16])
{
	struct aesdec dec;

	memcpy(dec.aesd_aes.aes_rk, rk, 4*(nrounds + 1)*sizeof(rk[0]));
	aes_dec(&dec, in, out, nrounds);
	explicit_memset(&dec, 0, sizeof dec);
}

int
rijndael_makeKey(keyInstance *key, BYTE direction, int keybits,
    const char *keyp)
{

	if (key == NULL)
		return BAD_KEY_INSTANCE;

	memset(key, 0x1a, sizeof(*key));

	switch (direction) {
	case DIR_ENCRYPT:
	case DIR_DECRYPT:
		key->direction = direction;
		break;
	default:
		return BAD_KEY_DIR;
	}

	switch (keybits) {
	case 128:
	case 192:
	case 256:
		key->keyLen = keybits;
		break;
	default:
		return BAD_KEY_MAT;
	}

	if (keyp)
		memcpy(key->keyMaterial, keyp, keybits/8);

	switch (direction) {
	case DIR_ENCRYPT:
		key->Nr = rijndaelKeySetupEnc(key->rk,
		    (const uint8_t *)key->keyMaterial, keybits);
		break;
	case DIR_DECRYPT:
		key->Nr = rijndaelKeySetupDec(key->rk,
		    (const uint8_t *)key->keyMaterial, keybits);
		break;
	default:
		panic("unknown encryption direction %d", direction);
	}
	rijndaelKeySetupEnc(key->ek, (const uint8_t *)key->keyMaterial,
	    keybits);

	return 1;
}

int
rijndael_cipherInit(cipherInstance *cipher, BYTE mode, const char *iv)
{

	switch (mode) {
	case MODE_ECB:		/* used only for encrypting one block */
	case MODE_CBC:
	case MODE_XTS:
		cipher->mode = mode;
		break;
	case MODE_CFB1:		/* unused */
	default:
		return BAD_CIPHER_MODE;
	}

	if (iv)
		memcpy(cipher->IV, iv, RIJNDAEL_MAX_IV_SIZE);
	else
		memset(cipher->IV, 0, RIJNDAEL_MAX_IV_SIZE);

	return 1;
}

int
rijndael_blockEncrypt(cipherInstance *cipher, keyInstance *key,
    const BYTE *in, int nbits, BYTE *out)
{
	struct aesenc enc;

	if (cipher == NULL)
		return BAD_CIPHER_STATE;
	if (key == NULL)
		return BAD_CIPHER_STATE;
	if (key->direction != DIR_ENCRYPT)
		return BAD_CIPHER_STATE;

	if (in == NULL || nbits <= 0)
		return 0;

	memcpy(enc.aese_aes.aes_rk, key->rk,
	    4*(key->Nr + 1)*sizeof(key->rk[0]));
	switch (cipher->mode) {
	case MODE_ECB:
		KASSERT(nbits == 128);
		aes_enc(&enc, in, out, key->Nr);
		break;
	case MODE_CBC:
		KASSERT(nbits % 128 == 0);
		aes_cbc_enc(&enc, in, out, nbits/8, (uint8_t *)cipher->IV,
		    key->Nr);
		break;
	case MODE_XTS:
		KASSERT(nbits % 128 == 0);
		aes_xts_enc(&enc, in, out, nbits/8, (uint8_t *)cipher->IV,
		    key->Nr);
		break;
	default:
		panic("invalid AES mode: %d", cipher->mode);
	}
	explicit_memset(&enc, 0, sizeof enc);

	return nbits;
}

int
rijndael_blockDecrypt(cipherInstance *cipher, keyInstance *key,
    const BYTE *in, int nbits, BYTE *out)
{
	struct aesdec dec;

	if (cipher == NULL)
		return BAD_CIPHER_STATE;
	if (key == NULL)
		return BAD_CIPHER_STATE;
	if (key->direction != DIR_DECRYPT)
		return BAD_CIPHER_STATE;

	if (in == NULL || nbits <= 0)
		return 0;

	memcpy(dec.aesd_aes.aes_rk, key->rk,
	    4*(key->Nr + 1)*sizeof(key->rk[0]));
	switch (cipher->mode) {
	case MODE_ECB:
		KASSERT(nbits == 128);
		aes_dec(&dec, in, out, key->Nr);
		break;
	case MODE_CBC:
		KASSERT(nbits % 128 == 0);
		aes_cbc_dec(&dec, in, out, nbits/8, (uint8_t *)cipher->IV,
		    key->Nr);
		break;
	case MODE_XTS:
		KASSERT(nbits % 128 == 0);
		aes_xts_dec(&dec, in, out, nbits/8, (uint8_t *)cipher->IV,
		    key->Nr);
		break;
	default:
		panic("invalid AES mode: %d", cipher->mode);
	}
	explicit_memset(&dec, 0, sizeof dec);

	return nbits;
}
