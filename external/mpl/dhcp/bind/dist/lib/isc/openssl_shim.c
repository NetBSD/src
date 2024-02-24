/*	$NetBSD: openssl_shim.c,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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
