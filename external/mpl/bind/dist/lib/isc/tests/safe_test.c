/*	$NetBSD: safe_test.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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

/* ! \file */

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/safe.h>
#include <isc/util.h>

/* test isc_safe_memequal() */
static void
isc_safe_memequal_test(void **state) {
	UNUSED(state);

	assert_true(isc_safe_memequal("test", "test", 4));
	assert_true(!isc_safe_memequal("test", "tesc", 4));
	assert_true(
		isc_safe_memequal("\x00\x00\x00\x00", "\x00\x00\x00\x00", 4));
	assert_true(
		!isc_safe_memequal("\x00\x00\x00\x00", "\x00\x00\x00\x01", 4));
	assert_true(
		!isc_safe_memequal("\x00\x00\x00\x02", "\x00\x00\x00\x00", 4));
}

/* test isc_safe_memwipe() */
static void
isc_safe_memwipe_test(void **state) {
	UNUSED(state);

	/* These should pass. */
	isc_safe_memwipe(NULL, 0);
	isc_safe_memwipe((void *)-1, 0);

	/*
	 * isc_safe_memwipe(ptr, size) should function same as
	 * memset(ptr, 0, size);
	 */
	{
		char buf1[4] = { 1, 2, 3, 4 };
		char buf2[4] = { 1, 2, 3, 4 };

		isc_safe_memwipe(buf1, sizeof(buf1));
		memset(buf2, 0, sizeof(buf2));

		assert_int_equal(memcmp(buf1, buf2, sizeof(buf1)), 0);
	}

	/*
	 * Boundary test.
	 */
	{
		char buf1[4] = { 1, 2, 3, 4 };
		char buf2[4] = { 1, 2, 3, 4 };

		/*
		 * We wipe 3 elements on purpose, keeping the 4th in
		 * place.
		 */
		isc_safe_memwipe(buf1, 3);
		memset(buf2, 0, 3);

		assert_int_equal(memcmp(buf1, buf2, sizeof(buf1)), 0);
	}
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_safe_memequal_test),
		cmocka_unit_test(isc_safe_memwipe_test),
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
