/*	$NetBSD: hash.h,v 1.8 2025/01/26 16:25:41 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/assertions.h>
#include <isc/lang.h>
#include <isc/siphash.h>
#include <isc/types.h>
#include <isc/util.h>

#define ISC_HASHSIZE(bits)  (UINT64_C(1) << (bits))
#define ISC_HASH_OVERCOMMIT 3
#define ISC_HASH_MIN_BITS   2U
#define ISC_HASH_MAX_BITS   32U

typedef struct isc_halfsiphash24 isc_hash32_t;
typedef struct isc_siphash24	 isc_hash64_t;

/***
 *** Functions
 ***/
ISC_LANG_BEGINDECLS

void
isc__hash_initialize(void);

const void *
isc_hash_get_initializer(void);

void
isc_hash_set_initializer(const void *initializer);

void
isc_hash32_init(isc_hash32_t *restrict state);
void
isc_hash32_hash(isc_hash32_t *restrict state, const void *data,
		const size_t length, const bool case_sensitive);
uint32_t
isc_hash32_finalize(isc_hash32_t *restrict state);
static inline uint32_t
isc_hash32(const void *data, const size_t length, const bool case_sensitive) {
	isc_hash32_t state;
	isc_hash32_init(&state);
	isc_hash32_hash(&state, data, length, case_sensitive);
	return isc_hash32_finalize(&state);
}

void
isc_hash64_init(isc_hash64_t *restrict state);
void
isc_hash64_hash(isc_hash64_t *restrict state, const void *data,
		const size_t length, const bool case_sensitive);
uint64_t
isc_hash64_finalize(isc_hash64_t *restrict state);
static inline uint64_t
isc_hash64(const void *data, const size_t length, const bool case_sensitive) {
	isc_hash64_t state;
	isc_hash64_init(&state);
	isc_hash64_hash(&state, data, length, case_sensitive);
	return isc_hash64_finalize(&state);
}
/*!<
 * \brief Calculate a hash over data.
 *
 * This hash function is useful for hashtables. The hash function is
 * opaque and not important to the caller. The returned hash values are
 * non-deterministic and will have different mapping every time a
 * process using this library is run, but will have uniform
 * distribution.
 *
 * isc_hash_32/64() calculates the hash from start to end over the
 * input data.
 *
 * 'data' is the data to be hashed.
 *
 * 'length' is the size of the data to be hashed.
 *
 * 'case_sensitive' specifies whether the hash key should be treated as
 * case_sensitive values.  It should typically be false if the hash key
 * is a DNS name.
 *
 * Returns:
 * \li 32 or 64-bit hash value
 */

/*!
 * \brief Return a hash value of a specified number of bits
 *
 * This function uses Fibonacci Hashing to convert a 32 bit hash value
 * 'val' into a smaller hash value of up to 'bits' bits. This results
 * in better hash table distribution than the use of modulo.
 *
 * Requires:
 * \li 'bits' is less than 32.
 *
 * Returns:
 * \li a hash value of length 'bits'.
 */
#define ISC_HASH_GOLDENRATIO_32 0x61C88647

static inline uint32_t
isc_hash_bits32(uint32_t val, unsigned int bits) {
	ISC_REQUIRE(bits <= ISC_HASH_MAX_BITS);
	/* High bits are more random. */
	return val * ISC_HASH_GOLDENRATIO_32 >> (32 - bits);
}

ISC_LANG_ENDDECLS
