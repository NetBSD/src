/*	$NetBSD: iterated_hash.c,v 1.6.2.2 2024/02/25 15:47:16 martin Exp $	*/

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

#include <stdio.h>

#include <openssl/err.h>
#include <openssl/opensslv.h>

#include <isc/iterated_hash.h>
#include <isc/util.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000

#include <openssl/sha.h>

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength) {
	REQUIRE(out != NULL);

	int n = 0;
	size_t len;
	const unsigned char *buf;
	SHA_CTX ctx;

	if (hashalg != 1) {
		return (0);
	}

	buf = in;
	len = inlength;

	do {
		if (SHA1_Init(&ctx) != 1) {
			ERR_clear_error();
			return (0);
		}

		if (SHA1_Update(&ctx, buf, len) != 1) {
			ERR_clear_error();
			return (0);
		}

		if (SHA1_Update(&ctx, salt, saltlength) != 1) {
			ERR_clear_error();
			return (0);
		}

		if (SHA1_Final(out, &ctx) != 1) {
			ERR_clear_error();
			return (0);
		}

		buf = out;
		len = SHA_DIGEST_LENGTH;
	} while (n++ < iterations);

	return (SHA_DIGEST_LENGTH);
}

#else

#include <openssl/evp.h>

#include <isc/md.h>

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength) {
	REQUIRE(out != NULL);

	int n = 0;
	size_t len;
	unsigned int outlength = 0;
	const unsigned char *buf;
	EVP_MD_CTX *ctx;
	;
	EVP_MD *md;

	if (hashalg != 1) {
		return (0);
	}

	ctx = EVP_MD_CTX_new();
	RUNTIME_CHECK(ctx != NULL);
	md = EVP_MD_fetch(NULL, "SHA1", NULL);
	RUNTIME_CHECK(md != NULL);

	buf = in;
	len = inlength;

	do {
		if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
			goto fail;
		}

		if (EVP_DigestUpdate(ctx, buf, len) != 1) {
			goto fail;
		}

		if (EVP_DigestUpdate(ctx, salt, saltlength) != 1) {
			goto fail;
		}

		if (EVP_DigestFinal_ex(ctx, out, &outlength) != 1) {
			goto fail;
		}

		buf = out;
		len = outlength;
	} while (n++ < iterations);

	EVP_MD_CTX_free(ctx);
	EVP_MD_free(md);

	return (outlength);

fail:
	EVP_MD_CTX_free(ctx);
	EVP_MD_free(md);
	ERR_clear_error();
	return (0);
}

#endif
