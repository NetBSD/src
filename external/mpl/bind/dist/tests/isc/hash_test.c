/*	$NetBSD: hash_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

#include <tests/isc.h>

/* Hash function test */
ISC_RUN_TEST_IMPL(isc_hash_function) {
	unsigned int h1;
	unsigned int h2;

	UNUSED(state);

	/* Immutability of hash function */
	h1 = isc_hash_function(NULL, 0, true);
	h2 = isc_hash_function(NULL, 0, true);

	assert_int_equal(h1, h2);

	/* Hash function characteristics */
	h1 = isc_hash_function("Hello world", 12, true);
	h2 = isc_hash_function("Hello world", 12, true);

	assert_int_equal(h1, h2);

	/* Case */
	h1 = isc_hash_function("Hello world", 12, false);
	h2 = isc_hash_function("heLLo WorLd", 12, false);

	assert_int_equal(h1, h2);

	/* Unequal */
	h1 = isc_hash_function("Hello world", 12, true);
	h2 = isc_hash_function("heLLo WorLd", 12, true);

	assert_int_not_equal(h1, h2);
}

/* Hash function initializer test */
ISC_RUN_TEST_IMPL(isc_hash_initializer) {
	unsigned int h1;
	unsigned int h2;

	UNUSED(state);

	h1 = isc_hash_function("Hello world", 12, true);
	h2 = isc_hash_function("Hello world", 12, true);

	assert_int_equal(h1, h2);

	isc_hash_set_initializer(isc_hash_get_initializer());

	/* Hash value must not change */
	h2 = isc_hash_function("Hello world", 12, true);

	assert_int_equal(h1, h2);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_hash_function)
ISC_TEST_ENTRY(isc_hash_initializer)

ISC_TEST_LIST_END

ISC_TEST_MAIN
