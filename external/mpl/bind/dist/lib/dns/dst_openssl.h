/*	$NetBSD: dst_openssl.h,v 1.7 2025/01/26 16:25:22 christos Exp $	*/

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
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <isc/lang.h>
#include <isc/log.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

isc_result_t
dst__openssl_toresult(isc_result_t fallback);

#define dst__openssl_toresult2(A, B) \
	dst___openssl_toresult2(A, B, __FILE__, __LINE__)
isc_result_t
dst___openssl_toresult2(const char *funcname, isc_result_t fallback,
			const char *file, int line);

#define dst__openssl_toresult3(A, B, C) \
	dst___openssl_toresult3(A, B, C, __FILE__, __LINE__)
isc_result_t
dst___openssl_toresult3(isc_logcategory_t *category, const char *funcname,
			isc_result_t fallback, const char *file, int line);

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
ENGINE *
dst__openssl_getengine(const char *engine);
#endif /* if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000 */

isc_result_t
dst__openssl_fromlabel(int key_base_id, const char *engine, const char *label,
		       const char *pin, EVP_PKEY **ppub, EVP_PKEY **ppriv);

bool
dst__openssl_keypair_compare(const dst_key_t *key1, const dst_key_t *key2);

bool
dst__openssl_keypair_isprivate(const dst_key_t *key);

void
dst__openssl_keypair_destroy(dst_key_t *key);

ISC_LANG_ENDDECLS
