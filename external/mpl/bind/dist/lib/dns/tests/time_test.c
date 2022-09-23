/*	$NetBSD: time_test.c,v 1.8 2022/09/23 12:15:32 christos Exp $	*/

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
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/time.h>

#include "dnstest.h"

#define TEST_ORIGIN "test"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

/* value = 0xfffffffff <-> 19691231235959 */
static void
epoch_minus_one_test(void **state) {
	const char *test_text = "19691231235959";
	const uint32_t test_time = 0xffffffff;
	isc_result_t result;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

/* value = 0x000000000 <-> 19700101000000*/
static void
epoch_test(void **state) {
	const char *test_text = "19700101000000";
	const uint32_t test_time = 0x00000000;
	isc_result_t result;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

/* value = 0x7fffffff <-> 20380119031407 */
static void
half_maxint_test(void **state) {
	const char *test_text = "20380119031407";
	const uint32_t test_time = 0x7fffffff;
	isc_result_t result;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

/* value = 0x80000000 <-> 20380119031408 */
static void
half_plus_one_test(void **state) {
	const char *test_text = "20380119031408";
	const uint32_t test_time = 0x80000000;
	isc_result_t result;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

/* value = 0xef68f5d0 <-> 19610307130000 */
static void
fifty_before_test(void **state) {
	isc_result_t result;
	const char *test_text = "19610307130000";
	const uint32_t test_time = 0xef68f5d0;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

/* value = 0x4d74d6d0 <-> 20110307130000 */
static void
some_ago_test(void **state) {
	const char *test_text = "20110307130000";
	const uint32_t test_time = 0x4d74d6d0;
	isc_result_t result;
	isc_buffer_t target;
	uint32_t when;
	char buf[128];

	UNUSED(state);

	memset(buf, 0, sizeof(buf));
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_time32_totext(test_time, &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, test_text);
	result = dns_time32_fromtext(test_text, &when);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(when, test_time);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(epoch_minus_one_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(epoch_test, _setup, _teardown),
		cmocka_unit_test_setup_teardown(half_maxint_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(half_plus_one_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(fifty_before_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(some_ago_test, _setup,
						_teardown),
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
