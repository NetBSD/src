/*	$NetBSD: hmac.c,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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

#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#include <isc/assertions.h>
#include <isc/hmac.h>
#include <isc/md.h>
#include <isc/platform.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include "openssl_shim.h"

isc_hmac_t *
isc_hmac_new(void) {
	HMAC_CTX *hmac = HMAC_CTX_new();
	RUNTIME_CHECK(hmac != NULL);
	return ((struct hmac *)hmac);
}

void
isc_hmac_free(isc_hmac_t *hmac) {
	if (ISC_UNLIKELY(hmac == NULL)) {
		return;
	}

	HMAC_CTX_free(hmac);
}

isc_result_t
isc_hmac_init(isc_hmac_t *hmac, const void *key, size_t keylen,
	      const isc_md_type_t *md_type) {
	REQUIRE(hmac != NULL);
	REQUIRE(key != NULL);

	if (md_type == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	if (HMAC_Init_ex(hmac, key, keylen, md_type, NULL) != 1) {
		return (ISC_R_CRYPTOFAILURE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_hmac_reset(isc_hmac_t *hmac) {
	REQUIRE(hmac != NULL);

	if (HMAC_CTX_reset(hmac) != 1) {
		return (ISC_R_CRYPTOFAILURE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_hmac_update(isc_hmac_t *hmac, const unsigned char *buf, const size_t len) {
	REQUIRE(hmac != NULL);

	if (ISC_UNLIKELY(buf == NULL || len == 0)) {
		return (ISC_R_SUCCESS);
	}

	if (HMAC_Update(hmac, buf, len) != 1) {
		return (ISC_R_CRYPTOFAILURE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_hmac_final(isc_hmac_t *hmac, unsigned char *digest,
	       unsigned int *digestlen) {
	REQUIRE(hmac != NULL);
	REQUIRE(digest != NULL);

	if (HMAC_Final(hmac, digest, digestlen) != 1) {
		return (ISC_R_CRYPTOFAILURE);
	}

	return (ISC_R_SUCCESS);
}

const isc_md_type_t *
isc_hmac_get_md_type(isc_hmac_t *hmac) {
	REQUIRE(hmac != NULL);

	return (HMAC_CTX_get_md(hmac));
}

size_t
isc_hmac_get_size(isc_hmac_t *hmac) {
	REQUIRE(hmac != NULL);

	return ((size_t)EVP_MD_size(HMAC_CTX_get_md(hmac)));
}

int
isc_hmac_get_block_size(isc_hmac_t *hmac) {
	REQUIRE(hmac != NULL);

	return (EVP_MD_block_size(HMAC_CTX_get_md(hmac)));
}

isc_result_t
isc_hmac(const isc_md_type_t *type, const void *key, const int keylen,
	 const unsigned char *buf, const size_t len, unsigned char *digest,
	 unsigned int *digestlen) {
	isc_result_t res;
	isc_hmac_t *hmac = isc_hmac_new();

	res = isc_hmac_init(hmac, key, keylen, type);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}

	res = isc_hmac_update(hmac, buf, len);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}

	res = isc_hmac_final(hmac, digest, digestlen);
	if (res != ISC_R_SUCCESS) {
		goto end;
	}
end:
	isc_hmac_free(hmac);

	return (res);
}
