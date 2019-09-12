/*	$NetBSD: hash_test.c,v 1.3.4.1 2019/09/12 19:18:16 martin Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#define TEST_INPUT(x) (x), sizeof(x)-1

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
	return (0);
}

#endif
