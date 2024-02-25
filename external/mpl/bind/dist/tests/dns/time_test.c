/*	$NetBSD: time_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/time.h>

#include <tests/dns.h>

#define TEST_ORIGIN "test"

/* value = 0xfffffffff <-> 19691231235959 */
ISC_RUN_TEST_IMPL(epoch_minus_one) {
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
ISC_RUN_TEST_IMPL(epoch) {
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
ISC_RUN_TEST_IMPL(half_maxint) {
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
ISC_RUN_TEST_IMPL(half_plus_one) {
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
ISC_RUN_TEST_IMPL(fifty_before) {
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
ISC_RUN_TEST_IMPL(some_ago) {
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

ISC_TEST_LIST_START
ISC_TEST_ENTRY(epoch_minus_one)
ISC_TEST_ENTRY(epoch)
ISC_TEST_ENTRY(half_maxint)
ISC_TEST_ENTRY(half_plus_one)
ISC_TEST_ENTRY(fifty_before)
ISC_TEST_ENTRY(some_ago)
ISC_TEST_LIST_END

ISC_TEST_MAIN
