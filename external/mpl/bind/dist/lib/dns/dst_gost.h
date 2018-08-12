/*	$NetBSD: dst_gost.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DST_GOST_H
#define DST_GOST_H 1

#include <isc/lang.h>
#include <isc/log.h>
#include <dst/result.h>

#define ISC_GOST_DIGESTLENGTH 32U

#ifdef HAVE_OPENSSL_GOST
#include <openssl/evp.h>

typedef struct {
	EVP_MD_CTX *ctx;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_MD_CTX _ctx;
#endif
} isc_gost_t;

#endif
#ifdef HAVE_PKCS11_GOST
#include <pk11/pk11.h>

typedef pk11_context_t isc_gost_t;
#endif

ISC_LANG_BEGINDECLS

#if defined(HAVE_OPENSSL_GOST) || defined(HAVE_PKCS11_GOST)

isc_result_t
isc_gost_init(isc_gost_t *ctx);

void
isc_gost_invalidate(isc_gost_t *ctx);

isc_result_t
isc_gost_update(isc_gost_t *ctx, const unsigned char *data, unsigned int len);

isc_result_t
isc_gost_final(isc_gost_t *ctx, unsigned char *digest);

ISC_LANG_ENDDECLS

#endif /* HAVE_OPENSSL_GOST || HAVE_PKCS11_GOST */

#endif /* DST_GOST_H */
