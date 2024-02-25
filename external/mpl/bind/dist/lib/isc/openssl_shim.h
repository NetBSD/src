/*	$NetBSD: openssl_shim.h,v 1.6.2.1 2024/02/25 15:47:17 martin Exp $	*/

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

#pragma once

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>

#if !HAVE_CRYPTO_ZALLOC
void *
CRYPTO_zalloc(size_t num, const char *file, int line);
#endif /* if !HAVE_CRYPTO_ZALLOC */

#if !defined(OPENSSL_zalloc)
#define OPENSSL_zalloc(num) CRYPTO_zalloc(num, __FILE__, __LINE__)
#endif

#if !HAVE_EVP_PKEY_NEW_RAW_PRIVATE_KEY
#define EVP_PKEY_new_raw_private_key(type, e, key, keylen) \
	EVP_PKEY_new_mac_key(type, e, key, (int)(keylen))
#endif /* if !HAVE_EVP_PKEY_NEW_RAW_PRIVATE_KEY */

#if !HAVE_EVP_CIPHER_CTX_NEW
EVP_CIPHER_CTX *
EVP_CIPHER_CTX_new(void);
#endif /* if !HAVE_EVP_CIPHER_CTX_NEW */

#if !HAVE_EVP_CIPHER_CTX_FREE
void
EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx);
#endif /* if !HAVE_EVP_CIPHER_CTX_FREE */

#if !HAVE_EVP_MD_CTX_NEW
#define EVP_MD_CTX_new EVP_MD_CTX_create
#endif /* if !HAVE_EVP_MD_CTX_NEW */

#if !HAVE_EVP_MD_CTX_FREE
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif /* if !HAVE_EVP_MD_CTX_FREE */

#if !HAVE_EVP_MD_CTX_RESET
int
EVP_MD_CTX_reset(EVP_MD_CTX *ctx);
#endif /* if !HAVE_EVP_MD_CTX_RESET */

#if !HAVE_EVP_MD_CTX_GET0_MD
#define EVP_MD_CTX_get0_md EVP_MD_CTX_md
#endif /* if !HAVE_EVP_MD_CTX_GET0_MD */

#if !HAVE_SSL_READ_EX
int
SSL_read_ex(SSL *ssl, void *buf, size_t num, size_t *readbytes);
#endif

#if !HAVE_SSL_PEEK_EX
int
SSL_peek_ex(SSL *ssl, void *buf, size_t num, size_t *readbytes);
#endif

#if !HAVE_SSL_WRITE_EX
int
SSL_write_ex(SSL *ssl, const void *buf, size_t num, size_t *written);
#endif

#if !HAVE_BIO_READ_EX
int
BIO_read_ex(BIO *b, void *data, size_t dlen, size_t *readbytes);
#endif

#if !HAVE_BIO_WRITE_EX
int
BIO_write_ex(BIO *b, const void *data, size_t dlen, size_t *written);
#endif

#if !HAVE_OPENSSL_INIT_CRYPTO

#define OPENSSL_INIT_NO_LOAD_CRYPTO_STRINGS 0x00000001L
#define OPENSSL_INIT_LOAD_CRYPTO_STRINGS    0x00000002L
#define OPENSSL_INIT_ADD_ALL_CIPHERS	    0x00000004L
#define OPENSSL_INIT_ADD_ALL_DIGESTS	    0x00000008L
#define OPENSSL_INIT_NO_ADD_ALL_CIPHERS	    0x00000010L
#define OPENSSL_INIT_NO_ADD_ALL_DIGESTS	    0x00000020L

int
OPENSSL_init_crypto(uint64_t opts, const void *settings);
#endif

#if !HAVE_OPENSSL_INIT_SSL
#define OPENSSL_INIT_NO_LOAD_SSL_STRINGS 0x00100000L
#define OPENSSL_INIT_LOAD_SSL_STRINGS	 0x00200000L

int
OPENSSL_init_ssl(uint64_t opts, const void *settings);

#endif

#if !HAVE_OPENSSL_CLEANUP
void
OPENSSL_cleanup(void);
#endif

#if !HAVE_TLS_SERVER_METHOD
#define TLS_server_method SSLv23_server_method
#endif

#if !HAVE_TLS_CLIENT_METHOD
#define TLS_client_method SSLv23_client_method
#endif

#if !HAVE_SSL_CTX_UP_REF
int
SSL_CTX_up_ref(SSL_CTX *store);
#endif /* !HAVE_SSL_CTX_UP_REF */

#if !HAVE_X509_STORE_UP_REF
int
X509_STORE_up_ref(X509_STORE *v);
#endif /* !HAVE_OPENSSL_CLEANUP */

#if !HAVE_SSL_CTX_SET1_CERT_STORE
void
SSL_CTX_set1_cert_store(SSL_CTX *ctx, X509_STORE *store);
#endif /* !HAVE_SSL_CTX_SET1_CERT_STORE */
