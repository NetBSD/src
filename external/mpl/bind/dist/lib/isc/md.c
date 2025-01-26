/*	$NetBSD: md.c,v 1.7 2025/01/26 16:25:37 christos Exp $	*/

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
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#include <isc/md.h>
#include <isc/util.h>

#include "openssl_shim.h"

isc_md_t *
isc_md_new(void) {
	isc_md_t *md = EVP_MD_CTX_new();
	RUNTIME_CHECK(md != NULL);
	return md;
}

void
isc_md_free(isc_md_t *md) {
	if (md == NULL) {
		return;
	}

	EVP_MD_CTX_free(md);
}

isc_result_t
isc_md_init(isc_md_t *md, const isc_md_type_t *md_type) {
	REQUIRE(md != NULL);

	if (md_type == NULL) {
		return ISC_R_NOTIMPLEMENTED;
	}

	if (EVP_DigestInit_ex(md, md_type, NULL) != 1) {
		ERR_clear_error();
		return ISC_R_CRYPTOFAILURE;
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_md_reset(isc_md_t *md) {
	REQUIRE(md != NULL);

	if (EVP_MD_CTX_reset(md) != 1) {
		ERR_clear_error();
		return ISC_R_CRYPTOFAILURE;
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_md_update(isc_md_t *md, const unsigned char *buf, const size_t len) {
	REQUIRE(md != NULL);

	if (buf == NULL || len == 0) {
		return ISC_R_SUCCESS;
	}

	if (EVP_DigestUpdate(md, buf, len) != 1) {
		ERR_clear_error();
		return ISC_R_CRYPTOFAILURE;
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_md_final(isc_md_t *md, unsigned char *digest, unsigned int *digestlen) {
	REQUIRE(md != NULL);
	REQUIRE(digest != NULL);

	if (EVP_DigestFinal_ex(md, digest, digestlen) != 1) {
		ERR_clear_error();
		return ISC_R_CRYPTOFAILURE;
	}

	return ISC_R_SUCCESS;
}

const isc_md_type_t *
isc_md_get_md_type(isc_md_t *md) {
	REQUIRE(md != NULL);

	return EVP_MD_CTX_get0_md(md);
}

size_t
isc_md_get_size(isc_md_t *md) {
	REQUIRE(md != NULL);

	return EVP_MD_CTX_size(md);
}

size_t
isc_md_get_block_size(isc_md_t *md) {
	REQUIRE(md != NULL);

	return EVP_MD_CTX_block_size(md);
}

size_t
isc_md_type_get_size(const isc_md_type_t *md_type) {
	STATIC_ASSERT(ISC_MAX_MD_SIZE >= EVP_MAX_MD_SIZE,
		      "Change ISC_MAX_MD_SIZE to be greater than or equal to "
		      "EVP_MAX_MD_SIZE");
	if (md_type != NULL) {
		return (size_t)EVP_MD_size(md_type);
	}

	return ISC_MAX_MD_SIZE;
}

size_t
isc_md_type_get_block_size(const isc_md_type_t *md_type) {
	STATIC_ASSERT(ISC_MAX_MD_SIZE >= EVP_MAX_MD_SIZE,
		      "Change ISC_MAX_MD_SIZE to be greater than or equal to "
		      "EVP_MAX_MD_SIZE");
	if (md_type != NULL) {
		return (size_t)EVP_MD_block_size(md_type);
	}

	return ISC_MAX_MD_SIZE;
}

isc_result_t
isc_md(const isc_md_type_t *md_type, const unsigned char *buf, const size_t len,
       unsigned char *digest, unsigned int *digestlen) {
	isc_md_t *md;
	isc_result_t res;

	md = isc_md_new();

	res = isc_md_init(md, md_type);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}

	res = isc_md_update(md, buf, len);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}

	res = isc_md_final(md, digest, digestlen);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}
end:
	isc_md_free(md);

	return res;
}

#ifndef UNIT_TESTING
const isc_md_type_t *isc__md_md5 = NULL;
const isc_md_type_t *isc__md_sha1 = NULL;
const isc_md_type_t *isc__md_sha224 = NULL;
const isc_md_type_t *isc__md_sha256 = NULL;
const isc_md_type_t *isc__md_sha384 = NULL;
const isc_md_type_t *isc__md_sha512 = NULL;

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define md_register_algorithm(alg, algname)                        \
	{                                                          \
		REQUIRE(isc__md_##alg == NULL);                    \
		isc__md_##alg = EVP_MD_fetch(NULL, algname, NULL); \
		if (isc__md_##alg == NULL) {                       \
			ERR_clear_error();                         \
		}                                                  \
	}

#define md_unregister_algorithm(alg)                                    \
	{                                                               \
		if (isc__md_##alg != NULL) {                            \
			EVP_MD_free(*(isc_md_type_t **)&isc__md_##alg); \
			isc__md_##alg = NULL;                           \
		}                                                       \
	}

#else
#define md_register_algorithm(alg, algname)  \
	{                                    \
		isc__md_##alg = EVP_##alg(); \
		if (isc__md_##alg == NULL) { \
			ERR_clear_error();   \
		}                            \
	}
#define md_unregister_algorithm(alg)
#endif

void
isc__md_initialize(void) {
	md_register_algorithm(md5, "MD5");
	md_register_algorithm(sha1, "SHA1");
	md_register_algorithm(sha224, "SHA224");
	md_register_algorithm(sha256, "SHA256");
	md_register_algorithm(sha384, "SHA384");
	md_register_algorithm(sha512, "SHA512");
}

void
isc__md_shutdown(void) {
	md_unregister_algorithm(sha512);
	md_unregister_algorithm(sha384);
	md_unregister_algorithm(sha256);
	md_unregister_algorithm(sha224);
	md_unregister_algorithm(sha1);
	md_unregister_algorithm(md5);
}

#endif /* UNIT_TESTING */
