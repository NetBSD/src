/*	$NetBSD: openssl_shim.h,v 1.2.2.2 2024/02/25 15:46:51 martin Exp $	*/

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

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/rsa.h>

/*
 * Limit the size of public exponents.
 */
#ifndef RSA_MAX_PUBEXP_BITS
#define RSA_MAX_PUBEXP_BITS 35
#endif /* ifndef RSA_MAX_PUBEXP_BITS */

#if !HAVE_RSA_SET0_KEY && OPENSSL_VERSION_NUMBER < 0x30000000L
int
RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d);

int
RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q);

int
RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp);

void
RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e,
	     const BIGNUM **d);

void
RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q);

void
RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1,
		    const BIGNUM **iqmp);

int
RSA_test_flags(const RSA *r, int flags);
#endif /* !HAVE_RSA_SET0_KEY && OPENSSL_VERSION_NUMBER < 0x30000000L */

#if !HAVE_ECDSA_SIG_GET0
void
ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps);

int
ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s);
#endif /* !HAVE_ECDSA_SIG_GET0 */

#if !HAVE_DH_GET0_KEY
void
DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key);

int
DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key);

void
DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g);

int
DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g);

#define DH_clear_flags(d, f) ((d)->flags &= ~(f))
#endif /* !HAVE_DH_GET0_KEY */

#if !HAVE_ERR_GET_ERROR_ALL
unsigned long
ERR_get_error_all(const char **file, int *line, const char **func,
		  const char **data, int *flags);
#endif /* if !HAVE_ERR_GET_ERROR_ALL */

#if !HAVE_EVP_PKEY_EQ
#define EVP_PKEY_eq EVP_PKEY_cmp
#endif
