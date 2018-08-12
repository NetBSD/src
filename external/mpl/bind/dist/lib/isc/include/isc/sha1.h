/*	$NetBSD: sha1.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#ifndef ISC_SHA1_H
#define ISC_SHA1_H 1


/*	NetBSD: sha1.h,v 1.2 1998/05/29 22:55:44 thorpej Exp 	*/

/*! \file isc/sha1.h
 * \brief SHA-1 in C
 * \author By Steve Reid <steve@edmweb.com>
 * \note 100% Public Domain
 */

#include <isc/lang.h>
#include <isc/platform.h>
#include <isc/types.h>

#define ISC_SHA1_DIGESTLENGTH 20U
#define ISC_SHA1_BLOCK_LENGTH 64U

#ifdef ISC_PLATFORM_OPENSSLHASH
#include <openssl/opensslv.h>
#include <openssl/evp.h>

typedef struct {
	EVP_MD_CTX *ctx;
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	EVP_MD_CTX _ctx;
#endif
} isc_sha1_t;

#elif PKCS11CRYPTO
#include <pk11/pk11.h>

typedef pk11_context_t isc_sha1_t;

#else

typedef struct {
	isc_uint32_t state[5];
	isc_uint32_t count[2];
	unsigned char buffer[ISC_SHA1_BLOCK_LENGTH];
} isc_sha1_t;
#endif

ISC_LANG_BEGINDECLS

void
isc_sha1_init(isc_sha1_t *ctx);

void
isc_sha1_invalidate(isc_sha1_t *ctx);

void
isc_sha1_update(isc_sha1_t *ctx, const unsigned char *data, unsigned int len);

void
isc_sha1_final(isc_sha1_t *ctx, unsigned char *digest);

isc_boolean_t
isc_sha1_check(isc_boolean_t testing);

ISC_LANG_ENDDECLS

#endif /* ISC_SHA1_H */
