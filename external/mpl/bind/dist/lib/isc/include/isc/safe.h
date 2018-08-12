/*	$NetBSD: safe.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#include <isc/types.h>
#include <stdlib.h>

ISC_LANG_BEGINDECLS

isc_boolean_t
isc_safe_memequal(const void *s1, const void *s2, size_t n);
/*%<
 * Returns ISC_TRUE iff. two blocks of memory are equal, otherwise
 * ISC_FALSE.
 *
 */

int
isc_safe_memcompare(const void *b1, const void *b2, size_t len);
/*%<
 * Clone of libc memcmp() which is safe to differential timing attacks.
 */

void
isc_safe_memwipe(void *ptr, size_t len);
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
