/*	$NetBSD: parse_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/parseint.h>
#include <isc/util.h>

#include <tests/isc.h>

/* Test for 32 bit overflow on 64 bit machines in isc_parse_uint32 */
ISC_RUN_TEST_IMPL(parse_overflow) {
	isc_result_t result;
	uint32_t output;

	result = isc_parse_uint32(&output, "1234567890", 10);
	assert_int_equal(ISC_R_SUCCESS, result);
	assert_int_equal(1234567890, output);

	result = isc_parse_uint32(&output, "123456789012345", 10);
	assert_int_equal(ISC_R_RANGE, result);

	result = isc_parse_uint32(&output, "12345678901234567890", 10);
	assert_int_equal(ISC_R_RANGE, result);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(parse_overflow)

ISC_TEST_LIST_END

ISC_TEST_MAIN
