/*	$NetBSD: aes.c,v 1.4 2020/05/24 19:46:26 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file isc/aes.c */

#include <openssl/evp.h>
#include <openssl/opensslv.h>

#include <isc/aes.h>
#include <isc/assertions.h>
#include <isc/platform.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define EVP_CIPHER_CTX_new()   &(_context), EVP_CIPHER_CTX_init(&_context)
#define EVP_CIPHER_CTX_free(c) RUNTIME_CHECK(EVP_CIPHER_CTX_cleanup(c) == 1)
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	EVP_CIPHER_CTX _context;
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */
	EVP_CIPHER_CTX *c;
	int len;

	c = EVP_CIPHER_CTX_new();
	RUNTIME_CHECK(c != NULL);
	RUNTIME_CHECK(EVP_EncryptInit(c, EVP_aes_128_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(c, 0);
	RUNTIME_CHECK(
		EVP_EncryptUpdate(c, out, &len, in, ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	EVP_CIPHER_CTX_free(c);
}

void
isc_aes192_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	EVP_CIPHER_CTX _context;
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */
	EVP_CIPHER_CTX *c;
	int len;

	c = EVP_CIPHER_CTX_new();
	RUNTIME_CHECK(c != NULL);
	RUNTIME_CHECK(EVP_EncryptInit(c, EVP_aes_192_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(c, 0);
	RUNTIME_CHECK(
		EVP_EncryptUpdate(c, out, &len, in, ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	EVP_CIPHER_CTX_free(c);
}

void
isc_aes256_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	EVP_CIPHER_CTX _context;
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */
	EVP_CIPHER_CTX *c;
	int len;

	c = EVP_CIPHER_CTX_new();
	RUNTIME_CHECK(c != NULL);
	RUNTIME_CHECK(EVP_EncryptInit(c, EVP_aes_256_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(c, 0);
	RUNTIME_CHECK(
		EVP_EncryptUpdate(c, out, &len, in, ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	EVP_CIPHER_CTX_free(c);
}
