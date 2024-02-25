/*	$NetBSD: openssl_shim.c,v 1.6.2.1 2024/02/25 15:47:17 martin Exp $	*/

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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>

#include "openssl_shim.h"

#if !HAVE_CRYPTO_ZALLOC
void *
CRYPTO_zalloc(size_t num, const char *file, int line) {
	void *ret = CRYPTO_malloc(num, file, line);
	if (ret != NULL) {
		memset(ret, 0, num);
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

#if !HAVE_EVP_MD_CTX_RESET
int
EVP_MD_CTX_reset(EVP_MD_CTX *ctx) {
	return (EVP_MD_CTX_cleanup(ctx));
}
#endif /* if !HAVE_EVP_MD_CTX_RESET */

#if !HAVE_SSL_READ_EX
int
SSL_read_ex(SSL *ssl, void *buf, size_t num, size_t *readbytes) {
	int rv = SSL_read(ssl, buf, num);
	if (rv > 0) {
		*readbytes = rv;
		rv = 1;
	}

	return (rv);
}
#endif

#if !HAVE_SSL_PEEK_EX
int
SSL_peek_ex(SSL *ssl, void *buf, size_t num, size_t *readbytes) {
	int rv = SSL_peek(ssl, buf, num);
	if (rv > 0) {
		*readbytes = rv;
		rv = 1;
	}

	return (rv);
}
#endif

#if !HAVE_SSL_WRITE_EX
int
SSL_write_ex(SSL *ssl, const void *buf, size_t num, size_t *written) {
	int rv = SSL_write(ssl, buf, num);
	if (rv > 0) {
		*written = rv;
		rv = 1;
	}

	return (rv);
}
#endif

#if !HAVE_BIO_READ_EX
int
BIO_read_ex(BIO *b, void *data, size_t dlen, size_t *readbytes) {
	int rv = BIO_read(b, data, dlen);
	if (rv > 0) {
		*readbytes = rv;
		rv = 1;
	}

	return (rv);
}
#endif

#if !HAVE_BIO_WRITE_EX
int
BIO_write_ex(BIO *b, const void *data, size_t dlen, size_t *written) {
	int rv = BIO_write(b, data, dlen);
	if (rv > 0) {
		*written = rv;
		rv = 1;
	}

	return (rv);
}
#endif

#if !HAVE_OPENSSL_INIT_CRYPTO
int
OPENSSL_init_crypto(uint64_t opts, const void *settings) {
	(void)settings;

	if ((opts & OPENSSL_INIT_NO_LOAD_CRYPTO_STRINGS) == 0) {
		ERR_load_crypto_strings();
	}

	if ((opts & (OPENSSL_INIT_NO_ADD_ALL_CIPHERS |
		     OPENSSL_INIT_NO_ADD_ALL_CIPHERS)) == 0)
	{
		OpenSSL_add_all_algorithms();
	} else if ((opts & OPENSSL_INIT_NO_ADD_ALL_CIPHERS) == 0) {
		OpenSSL_add_all_digests();
	} else if ((opts & OPENSSL_INIT_NO_ADD_ALL_CIPHERS) == 0) {
		OpenSSL_add_all_ciphers();
	}

	return (1);
}
#endif

#if !HAVE_OPENSSL_INIT_SSL
int
OPENSSL_init_ssl(uint64_t opts, const void *settings) {
	OPENSSL_init_crypto(opts, settings);

	SSL_library_init();

	if ((opts & OPENSSL_INIT_NO_LOAD_SSL_STRINGS) == 0) {
		SSL_load_error_strings();
	}

	return (1);
}
#endif

#if !HAVE_OPENSSL_CLEANUP
void
OPENSSL_cleanup(void) {
	return;
}
#endif

#if !HAVE_SSL_CTX_UP_REF
int
SSL_CTX_up_ref(SSL_CTX *ctx) {
	return (CRYPTO_add(&ctx->references, 1, CRYPTO_LOCK_SSL_CTX) > 0);
}
#endif /* !HAVE_SSL_CTX_UP_REF */

#if !HAVE_X509_STORE_UP_REF

int
X509_STORE_up_ref(X509_STORE *store) {
	return (CRYPTO_add(&store->references, 1, CRYPTO_LOCK_X509_STORE) > 0);
}

#endif /* !HAVE_OPENSSL_CLEANUP */

#if !HAVE_SSL_CTX_SET1_CERT_STORE

void
SSL_CTX_set1_cert_store(SSL_CTX *ctx, X509_STORE *store) {
	(void)X509_STORE_up_ref(store);

	SSL_CTX_set_cert_store(ctx, store);
}

#endif /* !HAVE_SSL_CTX_SET1_CERT_STORE */
