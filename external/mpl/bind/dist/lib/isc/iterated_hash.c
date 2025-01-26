/*	$NetBSD: iterated_hash.c,v 1.9 2025/01/26 16:25:37 christos Exp $	*/

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

#include <stdbool.h>
#include <stdio.h>

#include <openssl/err.h>
#include <openssl/opensslv.h>

#include <isc/iterated_hash.h>
#include <isc/thread.h>
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
		return 0;
	}

	buf = in;
	len = inlength;

	do {
		if (SHA1_Init(&ctx) != 1) {
			ERR_clear_error();
			return 0;
		}

		if (SHA1_Update(&ctx, buf, len) != 1) {
			ERR_clear_error();
			return 0;
		}

		if (SHA1_Update(&ctx, salt, saltlength) != 1) {
			ERR_clear_error();
			return 0;
		}

		if (SHA1_Final(out, &ctx) != 1) {
			ERR_clear_error();
			return 0;
		}

		buf = out;
		len = SHA_DIGEST_LENGTH;
	} while (n++ < iterations);

	return SHA_DIGEST_LENGTH;
}

void
isc__iterated_hash_initialize(void) {
	/* empty */
}

void
isc__iterated_hash_shutdown(void) {
	/* empty */
}

#else /* HAVE_SHA1_INIT */

#include <openssl/evp.h>

static thread_local bool initialized = false;
static thread_local EVP_MD_CTX *mdctx = NULL;
static thread_local EVP_MD_CTX *basectx = NULL;
static thread_local EVP_MD *md = NULL;

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength) {
	REQUIRE(out != NULL);
	REQUIRE(mdctx != NULL);
	REQUIRE(basectx != NULL);

	int n = 0;
	size_t len;
	unsigned int outlength = 0;
	const unsigned char *buf;

	if (hashalg != 1) {
		return 0;
	}

	buf = in;
	len = inlength;
	do {
		if (EVP_MD_CTX_copy_ex(mdctx, basectx) != 1) {
			goto fail;
		}

		if (EVP_DigestUpdate(mdctx, buf, len) != 1) {
			goto fail;
		}

		if (EVP_DigestUpdate(mdctx, salt, saltlength) != 1) {
			goto fail;
		}

		if (EVP_DigestFinal_ex(mdctx, out, &outlength) != 1) {
			goto fail;
		}

		buf = out;
		len = outlength;
	} while (n++ < iterations);

	return outlength;

fail:
	ERR_clear_error();
	return 0;
}

void
isc__iterated_hash_initialize(void) {
	if (initialized) {
		return;
	}

	basectx = EVP_MD_CTX_new();
	INSIST(basectx != NULL);
	mdctx = EVP_MD_CTX_new();
	INSIST(mdctx != NULL);
	md = EVP_MD_fetch(NULL, "SHA1", NULL);
	INSIST(md != NULL);

	RUNTIME_CHECK(EVP_DigestInit_ex(basectx, md, NULL) == 1);
	initialized = true;
}

void
isc__iterated_hash_shutdown(void) {
	if (!initialized) {
		return;
	}

	REQUIRE(mdctx != NULL);
	EVP_MD_CTX_free(mdctx);
	mdctx = NULL;
	REQUIRE(basectx != NULL);
	EVP_MD_CTX_free(basectx);
	basectx = NULL;
	EVP_MD_free(md);
	md = NULL;

	initialized = false;
}

#endif /* HAVE_SHA1_INIT */
