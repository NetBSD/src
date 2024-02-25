/*	$NetBSD: safe_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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

#include <inttypes.h>
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

#include <tests/isc.h>

/* test isc_safe_memequal() */
ISC_RUN_TEST_IMPL(isc_safe_memequal) {
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
ISC_RUN_TEST_IMPL(isc_safe_memwipe) {
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

ISC_TEST_LIST_START
ISC_TEST_ENTRY(isc_safe_memequal)
ISC_TEST_ENTRY(isc_safe_memwipe)

ISC_TEST_LIST_END

ISC_TEST_MAIN
