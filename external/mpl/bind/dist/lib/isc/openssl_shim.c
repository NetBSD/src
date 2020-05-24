/*	$NetBSD: openssl_shim.c,v 1.3 2020/05/24 19:46:26 christos Exp $	*/

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

#include "openssl_shim.h"
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#if !HAVE_CRYPTO_ZALLOC
void *
CRYPTO_zalloc(size_t size) {
	void *ret = OPENSSL_malloc(size);
	if (ret != NULL) {
		memset(ret, 0, size);
	}
	return (ret);
}
#endif /* if !HAVE_CRYPTO_ZALLOC */

#if !HAVE_EVP_CIPHER_CTX_NEW
EVP_CIPHER_CTX *
EVP_CIPHER_CTX_new(void) {
	EVP_CIPHER_CTX *ctx = OPENSSL_zalloc(sizeof(*ctx));
	return (ctx);
}
#endif /* if !HAVE_EVP_CIPHER_CTX_NEW */

#if !HAVE_EVP_CIPHER_CTX_FREE
void
EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx) {
	if (ctx != NULL) {
		EVP_CIPHER_CTX_cleanup(ctx);
		OPENSSL_free(ctx);
	}
}
#endif /* if !HAVE_EVP_CIPHER_CTX_FREE */

#if !HAVE_EVP_MD_CTX_NEW
EVP_MD_CTX *
EVP_MD_CTX_new(void) {
	EVP_MD_CTX *ctx = OPENSSL_malloc(sizeof(*ctx));
	if (ctx != NULL) {
		memset(ctx, 0, sizeof(*ctx));
	}
	return (ctx);
}
#endif /* if !HAVE_EVP_MD_CTX_NEW */

#if !HAVE_EVP_MD_CTX_FREE
void
EVP_MD_CTX_free(EVP_MD_CTX *ctx) {
	if (ctx != NULL) {
		EVP_MD_CTX_cleanup(ctx);
		OPENSSL_free(ctx);
	}
}
#endif /* if !HAVE_EVP_MD_CTX_FREE */

#if !HAVE_EVP_MD_CTX_RESET
int
EVP_MD_CTX_reset(EVP_MD_CTX *ctx) {
	return (EVP_MD_CTX_cleanup(ctx));
}
#endif /* if !HAVE_EVP_MD_CTX_RESET */

#if !HAVE_HMAC_CTX_NEW
HMAC_CTX *
HMAC_CTX_new(void) {
	HMAC_CTX *ctx = OPENSSL_zalloc(sizeof(*ctx));
	if (ctx != NULL) {
		if (!HMAC_CTX_reset(ctx)) {
			HMAC_CTX_free(ctx);
			return (NULL);
		}
	}
	return (ctx);
}
#endif /* if !HAVE_HMAC_CTX_NEW */

#if !HAVE_HMAC_CTX_FREE
void
HMAC_CTX_free(HMAC_CTX *ctx) {
	if (ctx != NULL) {
		HMAC_CTX_cleanup(ctx);
		OPENSSL_free(ctx);
	}
}
#endif /* if !HAVE_HMAC_CTX_FREE */

#if !HAVE_HMAC_CTX_RESET
int
HMAC_CTX_reset(HMAC_CTX *ctx) {
	HMAC_CTX_cleanup(ctx);
	return (1);
}
#endif /* if !HAVE_HMAC_CTX_RESET */

#if !HAVE_HMAC_CTX_GET_MD
const EVP_MD *
HMAC_CTX_get_md(const HMAC_CTX *ctx) {
	return (ctx->md);
}
#endif /* if !HAVE_HMAC_CTX_GET_MD */
