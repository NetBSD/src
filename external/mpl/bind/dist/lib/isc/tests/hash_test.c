/*	$NetBSD: hash_test.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <inttypes.h>
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

#include <pk11/site.h>

#define TEST_INPUT(x) (x), sizeof(x) - 1

/*Hash function test */
static void
isc_hash_function_test(void **state) {
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
static void
isc_hash_initializer_test(void **state) {
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

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_hash_function_test),
		cmocka_unit_test(isc_hash_initializer_test),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
