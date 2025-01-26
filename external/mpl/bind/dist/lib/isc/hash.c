/*	$NetBSD: hash.c,v 1.9 2025/01/26 16:25:37 christos Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>

#include <isc/ascii.h>
#include <isc/entropy.h>
#include <isc/hash.h> /* IWYU pragma: keep */
#include <isc/random.h>
#include <isc/result.h>
#include <isc/siphash.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

static uint8_t isc_hash_key[16];

void
isc__hash_initialize(void) {
	/*
	 * Set a constant key to help in problem reproduction should
	 * fuzzing find a crash or a hang.
	 */
	uint8_t key[16] = { 1 };
#if !FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
	isc_entropy_get(key, sizeof(key));
#endif /* if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
	STATIC_ASSERT(sizeof(key) >= sizeof(isc_hash_key),
		      "sizeof(key) < sizeof(isc_hash_key)");
	memmove(isc_hash_key, key, sizeof(isc_hash_key));
}

const void *
isc_hash_get_initializer(void) {
	return isc_hash_key;
}

void
isc_hash_set_initializer(const void *initializer) {
	REQUIRE(initializer != NULL);

	memmove(isc_hash_key, initializer, sizeof(isc_hash_key));
}

void
isc_hash32_init(isc_hash32_t *restrict state) {
	isc_halfsiphash24_init(state, isc_hash_key);
}

void
isc_hash32_hash(isc_hash32_t *restrict state, const void *data,
		const size_t length, const bool case_sensitive) {
	REQUIRE(length == 0 || data != NULL);

	isc_halfsiphash24_hash(state, data, length, case_sensitive);
}

uint32_t
isc_hash32_finalize(isc_hash32_t *restrict state) {
	uint32_t hval;

	isc_halfsiphash24_finalize(state, (uint8_t *)&hval);

	return hval;
}

void
isc_hash64_init(isc_hash64_t *restrict state) {
	isc_siphash24_init(state, isc_hash_key);
}

void
isc_hash64_hash(isc_hash64_t *restrict state, const void *data,
		const size_t length, const bool case_sensitive) {
	REQUIRE(length == 0 || data != NULL);

	isc_siphash24_hash(state, data, length, case_sensitive);
}

uint64_t
isc_hash64_finalize(isc_hash64_t *restrict state) {
	uint64_t hval;

	isc_siphash24_finalize(state, (uint8_t *)&hval);

	return hval;
}
