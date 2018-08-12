/*	$NetBSD: hmacmd5.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


/*! \file isc/hmacmd5.h
 * \brief This is the header file for the HMAC-MD5 keyed hash algorithm
 * described in RFC2104.
 */

#ifndef ISC_HMACMD5_H
#define ISC_HMACMD5_H 1

#include <pk11/site.h>

#ifndef PK11_MD5_DISABLE

#include <isc/lang.h>
#include <isc/md5.h>
#include <isc/platform.h>
#include <isc/types.h>

#define ISC_HMACMD5_KEYLENGTH 64

#ifdef ISC_PLATFORM_OPENSSLHASH
#include <openssl/opensslv.h>
#include <openssl/hmac.h>

typedef struct {
	HMAC_CTX *ctx;
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	HMAC_CTX _ctx;
#endif
} isc_hmacmd5_t;

#elif PKCS11CRYPTO
#include <pk11/pk11.h>

typedef pk11_context_t isc_hmacmd5_t;

#else

typedef struct {
	isc_md5_t md5ctx;
	unsigned char key[ISC_HMACMD5_KEYLENGTH];
} isc_hmacmd5_t;
#endif

ISC_LANG_BEGINDECLS

void
isc_hmacmd5_init(isc_hmacmd5_t *ctx, const unsigned char *key,
		 unsigned int len);

void
isc_hmacmd5_invalidate(isc_hmacmd5_t *ctx);

void
isc_hmacmd5_update(isc_hmacmd5_t *ctx, const unsigned char *buf,
		   unsigned int len);

void
isc_hmacmd5_sign(isc_hmacmd5_t *ctx, unsigned char *digest);

isc_boolean_t
isc_hmacmd5_verify(isc_hmacmd5_t *ctx, unsigned char *digest);

isc_boolean_t
isc_hmacmd5_verify2(isc_hmacmd5_t *ctx, unsigned char *digest, size_t len);

isc_boolean_t
isc_hmacmd5_check(int testing);

ISC_LANG_ENDDECLS

#endif /* !PK11_MD5_DISABLE */

#endif /* ISC_HMACMD5_H */
