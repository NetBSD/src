/*	$NetBSD: safe.h,v 1.2.2.3 2019/01/18 08:49:58 pgoyette Exp $	*/

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


#ifndef ISC_SAFE_H
#define ISC_SAFE_H 1

/*! \file isc/safe.h */

#include <isc/lang.h>

#include <openssl/crypto.h>

ISC_LANG_BEGINDECLS

#define isc_safe_memequal(s1, s2, n) !CRYPTO_memcmp(s1, s2, n)

/*%<
 * Returns true iff. two blocks of memory are equal, otherwise
 * false.
 *
 */

#define isc_safe_memwipe(ptr, len) OPENSSL_cleanse(ptr, len)
/*%<
 * Clear the memory of length `len` pointed to by `ptr`.
 *
 * Some crypto code calls memset() on stack allocated buffers just
 * before return so that they are wiped. Such memset() calls can be
 * optimized away by the compiler. We provide this external non-inline C
 * function to perform the memset operation so that the compiler cannot
 * infer about what the function does and optimize the call away.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_SAFE_H */
