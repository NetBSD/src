/*	$NetBSD: hash_test.c,v 1.3.2.2 2019/06/10 22:04:45 christos Exp $	*/

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
#include <isc/region.h>

#include <isc/util.h>
#include <isc/print.h>
#include <isc/string.h>

#include <pk11/site.h>

#define TEST_INPUT(x) (x), sizeof(x)-1

typedef struct hash_testcase {
	const char *input;
	size_t input_len;
	const char *result;
	int repeats;
} hash_testcase_t;

/*Hash function test */
static void
isc_hash_function_test(void **state) {
	unsigned int h1;
	unsigned int h2;

	UNUSED(state);

	/* Incremental hashing */

	h1 = isc_hash_function(NULL, 0, true, NULL);
	h1 = isc_hash_function("This ", 5, true, &h1);
	h1 = isc_hash_function("is ", 3, true, &h1);
	h1 = isc_hash_function("a long test", 12, true, &h1);

	h2 = isc_hash_function("This is a long test", 20,
			       true, NULL);

	assert_int_equal(h1, h2);

	/* Immutability of hash function */
	h1 = isc_hash_function(NULL, 0, true, NULL);
	h2 = isc_hash_function(NULL, 0, true, NULL);

	assert_int_equal(h1, h2);

	/* Hash function characteristics */
	h1 = isc_hash_function("Hello world", 12, true, NULL);
	h2 = isc_hash_function("Hello world", 12, true, NULL);

	assert_int_equal(h1, h2);

	/* Case */
	h1 = isc_hash_function("Hello world", 12, false, NULL);
	h2 = isc_hash_function("heLLo WorLd", 12, false, NULL);

	assert_int_equal(h1, h2);

	/* Unequal */
	h1 = isc_hash_function("Hello world", 12, true, NULL);
	h2 = isc_hash_function("heLLo WorLd", 12, true, NULL);

	assert_int_not_equal(h1, h2);
}

/* Reverse hash function test */
static void
isc_hash_function_reverse_test(void **state) {
	unsigned int h1;
	unsigned int h2;

	UNUSED(state);

	/* Incremental hashing */

	h1 = isc_hash_function_reverse(NULL, 0, true, NULL);
	h1 = isc_hash_function_reverse("\000", 1, true, &h1);
	h1 = isc_hash_function_reverse("\003org", 4, true, &h1);
	h1 = isc_hash_function_reverse("\007example", 8, true, &h1);

	h2 = isc_hash_function_reverse("\007example\003org\000", 13,
				       true, NULL);

	assert_int_equal(h1, h2);

	/* Immutability of hash function */
	h1 = isc_hash_function_reverse(NULL, 0, true, NULL);
	h2 = isc_hash_function_reverse(NULL, 0, true, NULL);

	assert_int_equal(h1, h2);

	/* Hash function characteristics */
	h1 = isc_hash_function_reverse("Hello world", 12, true, NULL);
	h2 = isc_hash_function_reverse("Hello world", 12, true, NULL);

	assert_int_equal(h1, h2);

	/* Case */
	h1 = isc_hash_function_reverse("Hello world", 12, false, NULL);
	h2 = isc_hash_function_reverse("heLLo WorLd", 12, false, NULL);

	assert_int_equal(h1, h2);

	/* Unequal */
	h1 = isc_hash_function_reverse("Hello world", 12, true, NULL);
	h2 = isc_hash_function_reverse("heLLo WorLd", 12, true, NULL);

	assert_true(h1 != h2);
}

/* Hash function initializer test */
static void
isc_hash_initializer_test(void **state) {
	unsigned int h1;
	unsigned int h2;

	UNUSED(state);

	h1 = isc_hash_function("Hello world", 12, true, NULL);
	h2 = isc_hash_function("Hello world", 12, true, NULL);

	assert_int_equal(h1, h2);

	isc_hash_set_initializer(isc_hash_get_initializer());

	/* Hash value must not change */
	h2 = isc_hash_function("Hello world", 12, true, NULL);

	assert_int_equal(h1, h2);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_hash_function_test),
		cmocka_unit_test(isc_hash_function_reverse_test),
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
