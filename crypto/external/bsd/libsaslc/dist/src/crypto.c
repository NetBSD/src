/* $Id: crypto.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mateusz Kocielski.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "crypto.h"

#define HMAC_MD5_KEYSIZE	64
#define HMAC_MD5_IPAD		0x36
#define HMAC_MD5_OPAD		0x5C

/* local header */

static const char saslc__hex[] = "0123456789abcdef";

static char * saslc__digest_to_ascii(const unsigned char *);

/**
 * @brief converts MD5 binary digest into text representation.
 * @param d MD5 digest
 * @return the text representation, note that user is responsible for freeing
 * allocated memory.
 */

static char *
saslc__digest_to_ascii(const unsigned char *d)
{
	char *r;
	size_t i;

	if ((r = calloc((MD5_DIGEST_LENGTH << 1) + 1, sizeof(*r))) == NULL)
		return NULL;

	for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
		size_t j = i << 1;
		r[j] = saslc__hex[(unsigned int)d[i] >> 4];
		r[j + 1] = saslc__hex[d[i] & 0x0F];
	}

	return r;
}

/**
 * @brief encode data to base64.
 * @param in input data
 * @param len input data length
 * @return in encoded using base64. User is responsible for freeing string.
 */

char *
saslc__crypto_base64(const unsigned char *in, size_t len)
{
	BIO *m;
	BIO *base64;
	BUF_MEM *c;
	char *r;
	
	m = BIO_new(BIO_s_mem());
	base64 = BIO_new(BIO_f_base64());

	/* base64 -> mem */
	base64 = BIO_push(base64, m);
	/*LINTED len should be size_t in api*/
	BIO_write(base64, in, len);
	/*LINTED wrong argument, null effect*/
	(void)BIO_flush(base64);
	/*LINTED wrong argument, non-portable cast*/
	BIO_get_mem_ptr(base64, &c);
#if 0
	/* c->length is unsigned and cannot be < 0 */
	if (c->length < 0)
		return NULL;
#endif
	/*LINTED length should be size_t in api*/
	r = calloc(c->length, sizeof(*r));
	if (r == NULL)
		goto end;
	if (c->length != 0) {
		/*LINTED length should be size_t in api*/
		memcpy(r, c->data, c->length - 1);
	}
end:
	BIO_free_all(base64);

	return r;
}

/**
 * @brief generates safe nonce basing on OpenSSL
 * RAND_pseudo_bytes, which should be enough for our purposes.
 * @param b nonce length
 * @return nonce, user is responsible for freeing nonce.
 */

unsigned char *
saslc__crypto_nonce(size_t b)
{
	unsigned char *n;

	if ((n = calloc(b, sizeof(*n))) == NULL)
		return NULL;
	
	/*LINTED b should be size_t in api*/
	if (RAND_pseudo_bytes(n, b) != 1) {
		free(n);
		return NULL;
	}

	return n;
}

/**
 * @brief computes md5(D)
 * @param d data (string)
 * @return the text representation of the computed digest, note that user is
 * responsible for freeing allocated memory.
 */

char *
saslc__crypto_md5(const char *d)
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5_CTX md5c;

	MD5_Init(&md5c);
	MD5_Update(&md5c, d, strlen(d));
	MD5_Final(digest, &md5c);

	return saslc__digest_to_ascii(digest);
}

/**
 * @brief computes hmac_md5(K, I)
 * @param key hmac_md5 key
 * @param keylen hmac_md5 key length
 * @param in hmac_md5 input
 * @param inlen hmac_md5 input length
 * @return the text representation of the computed digest, note that user is
 * responsible for freeing allocated memory.
 */

char *
saslc__crypto_hmac_md5(const unsigned char *key, size_t keylen,
	const unsigned char *in, size_t inlen)
{
	unsigned char ipad_key[HMAC_MD5_KEYSIZE],
	    opad_key[HMAC_MD5_KEYSIZE], digest[MD5_DIGEST_LENGTH];
	const unsigned char *hmac_key;
	size_t i;
	MD5_CTX md5c;

	/* HMAC_MD5(K, T) = MD5(K XOR OPAD, MD5(K XOR IPAD, T)) */

	memset(ipad_key, 0, sizeof(ipad_key));
	memset(opad_key, 0, sizeof(opad_key));

	if (keylen > 64) {
		hmac_key = MD5(key, keylen, NULL);
		keylen = MD5_DIGEST_LENGTH;
	} else
		hmac_key = (const unsigned char *)key;

	for (i = 0; i < HMAC_MD5_KEYSIZE ; i++) {
		unsigned char k = i < keylen ? hmac_key[i] : 0;
		ipad_key[i] = k ^ HMAC_MD5_IPAD;
		opad_key[i] = k ^ HMAC_MD5_OPAD;
	}

	/* d = MD5(K XOR IPAD, T) */
	MD5_Init(&md5c);
	MD5_Update(&md5c, ipad_key, HMAC_MD5_KEYSIZE);
	MD5_Update(&md5c, in, inlen);
	MD5_Final(digest, &md5c);
	/* MD5(K XOR OPAD, d) */
	MD5_Init(&md5c);
	MD5_Update(&md5c, opad_key, HMAC_MD5_KEYSIZE);
	MD5_Update(&md5c, digest, MD5_DIGEST_LENGTH);
	MD5_Final(digest, &md5c);

	return saslc__digest_to_ascii(digest);
}
