/*	$NetBSD: parse_test.c,v 1.3.4.1 2019/09/12 19:18:16 martin Exp $	*/

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

/*! \file */

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/parseint.h>
#include <isc/util.h>

#include "isctest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

/* Test for 32 bit overflow on 64 bit machines in isc_parse_uint32 */
static void
parse_overflow(void **state) {
	isc_result_t result;
	uint32_t output;

	UNUSED(state);

	result = isc_parse_uint32(&output, "1234567890", 10);
	assert_int_equal(ISC_R_SUCCESS, result);
	assert_int_equal(1234567890, output);

	result = isc_parse_uint32(&output, "123456789012345", 10);
	assert_int_equal(ISC_R_RANGE, result);

	result = isc_parse_uint32(&output, "12345678901234567890", 10);
	assert_int_equal(ISC_R_RANGE, result);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(parse_overflow,
						_setup, _teardown),
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
