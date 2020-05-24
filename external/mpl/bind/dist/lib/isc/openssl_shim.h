/*	$NetBSD: openssl_shim.h,v 1.3 2020/05/24 19:46:26 christos Exp $	*/

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

#pragma once

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#if !HAVE_CRYPTO_ZALLOC
void *
CRYPTO_zalloc(size_t size);
#define OPENSSL_zalloc(num) CRYPTO_zalloc(num)
#endif /* if !HAVE_CRYPTO_ZALLOC */

#if !HAVE_EVP_CIPHER_CTX_NEW
EVP_CIPHER_CTX *
EVP_CIPHER_CTX_new(void);
#endif /* if !HAVE_EVP_CIPHER_CTX_NEW */

#if !HAVE_EVP_CIPHER_CTX_FREE
void
EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx);
#endif /* if !HAVE_EVP_CIPHER_CTX_FREE */

#if !HAVE_EVP_MD_CTX_NEW
EVP_MD_CTX *
EVP_MD_CTX_new(void);
#endif /* if !HAVE_EVP_MD_CTX_NEW */

#if !HAVE_EVP_MD_CTX_FREE
void
EVP_MD_CTX_free(EVP_MD_CTX *ctx);
#endif /* if !HAVE_EVP_MD_CTX_FREE */

#if !HAVE_EVP_MD_CTX_RESET
int
EVP_MD_CTX_reset(EVP_MD_CTX *ctx);
#endif /* if !HAVE_EVP_MD_CTX_RESET */

#if !HAVE_HMAC_CTX_NEW
HMAC_CTX *
HMAC_CTX_new(void);
#endif /* if !HAVE_HMAC_CTX_NEW */

#if !HAVE_HMAC_CTX_FREE
void
HMAC_CTX_free(HMAC_CTX *ctx);
#endif /* if !HAVE_HMAC_CTX_FREE */

#if !HAVE_HMAC_CTX_RESET
int
HMAC_CTX_reset(HMAC_CTX *ctx);
#endif /* if !HAVE_HMAC_CTX_RESET */

#if !HAVE_HMAC_CTX_GET_MD
const EVP_MD *
HMAC_CTX_get_md(const HMAC_CTX *ctx);
#endif /* if !HAVE_HMAC_CTX_GET_MD */
