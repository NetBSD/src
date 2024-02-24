/*	$NetBSD: aes.c,v 1.1.2.2 2024/02/24 13:07:19 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out) {
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
