/*	$NetBSD: time_test.c,v 1.2.2.2 2024/02/25 15:47:53 martin Exp $	*/

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

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/result.h>
#include <isc/time.h>
#include <isc/util.h>

#include "time.c"

#include <tests/isc.h>

#define MAX_NS (NS_PER_SEC - 1)

struct time_vectors {
	isc_time_t a;
	isc_interval_t b;
	isc_time_t r;
	isc_result_t result;
};

const struct time_vectors vectors_add[8] = {
	{ { 0, 0 }, { 0, 0 }, { 0, 0 }, ISC_R_SUCCESS },
	{ { 0, MAX_NS }, { 0, MAX_NS }, { 1, MAX_NS - 1 }, ISC_R_SUCCESS },
	{ { 0, NS_PER_SEC / 2 },
	  { 0, NS_PER_SEC / 2 },
	  { 1, 0 },
	  ISC_R_SUCCESS },
	{ { UINT_MAX, MAX_NS }, { 0, 0 }, { UINT_MAX, MAX_NS }, ISC_R_SUCCESS },
	{ { UINT_MAX, 0 }, { 0, MAX_NS }, { UINT_MAX, MAX_NS }, ISC_R_SUCCESS },
	{ { UINT_MAX, 0 }, { 1, 0 }, { 0, 0 }, ISC_R_RANGE },
	{ { UINT_MAX, MAX_NS }, { 0, 1 }, { 0, 0 }, ISC_R_RANGE },
	{ { UINT_MAX / 2 + 1, NS_PER_SEC / 2 },
	  { UINT_MAX / 2, NS_PER_SEC / 2 },
	  { 0, 0 },
	  ISC_R_RANGE },
};

const struct time_vectors vectors_sub[7] = {
	{ { 0, 0 }, { 0, 0 }, { 0, 0 }, ISC_R_SUCCESS },
	{ { 1, 0 }, { 0, MAX_NS }, { 0, 1 }, ISC_R_SUCCESS },
	{ { 1, NS_PER_SEC / 2 },
	  { 0, MAX_NS },
	  { 0, NS_PER_SEC / 2 + 1 },
	  ISC_R_SUCCESS },
	{ { UINT_MAX, MAX_NS }, { UINT_MAX, 0 }, { 0, MAX_NS }, ISC_R_SUCCESS },
	{ { 0, 0 }, { 1, 0 }, { 0, 0 }, ISC_R_RANGE },
	{ { 0, 0 }, { 0, MAX_NS }, { 0, 0 }, ISC_R_RANGE },
};

ISC_RUN_TEST_IMPL(isc_time_add_test) {
	UNUSED(state);

	for (size_t i = 0; i < ARRAY_SIZE(vectors_add); i++) {
		isc_time_t r = { UINT_MAX, UINT_MAX };
		isc_result_t result = isc_time_add(&(vectors_add[i].a),
						   &(vectors_add[i].b), &r);
		assert_int_equal(result, vectors_add[i].result);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		assert_int_equal(r.seconds, vectors_add[i].r.seconds);
		assert_int_equal(r.nanoseconds, vectors_add[i].r.nanoseconds);
	}

	expect_assert_failure((void)isc_time_add(&(isc_time_t){ 0, MAX_NS + 1 },
						 &(isc_interval_t){ 0, 0 },
						 &(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_add(
		&(isc_time_t){ 0, 0 }, &(isc_interval_t){ 0, MAX_NS + 1 },
		&(isc_time_t){ 0, 0 }));

	expect_assert_failure((void)isc_time_add((isc_time_t *)NULL,
						 &(isc_interval_t){ 0, 0 },
						 &(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_add(&(isc_time_t){ 0, 0 },
						 (isc_interval_t *)NULL,
						 &(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_add(
		&(isc_time_t){ 0, 0 }, &(isc_interval_t){ 0, 0 }, NULL));
}

ISC_RUN_TEST_IMPL(isc_time_sub_test) {
	UNUSED(state);

	for (size_t i = 0; i < ARRAY_SIZE(vectors_sub); i++) {
		isc_time_t r = { UINT_MAX, UINT_MAX };
		isc_result_t result = isc_time_subtract(
			&(vectors_sub[i].a), &(vectors_sub[i].b), &r);
		assert_int_equal(result, vectors_sub[i].result);
		if (result != ISC_R_SUCCESS) {
			continue;
		}
		assert_int_equal(r.seconds, vectors_sub[i].r.seconds);
		assert_int_equal(r.nanoseconds, vectors_sub[i].r.nanoseconds);
	}

	expect_assert_failure((void)isc_time_subtract(
		&(isc_time_t){ 0, MAX_NS + 1 }, &(isc_interval_t){ 0, 0 },
		&(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_subtract(
		&(isc_time_t){ 0, 0 }, &(isc_interval_t){ 0, MAX_NS + 1 },
		&(isc_time_t){ 0, 0 }));

	expect_assert_failure((void)isc_time_subtract((isc_time_t *)NULL,
						      &(isc_interval_t){ 0, 0 },
						      &(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_subtract(&(isc_time_t){ 0, 0 },
						      (isc_interval_t *)NULL,
						      &(isc_time_t){ 0, 0 }));
	expect_assert_failure((void)isc_time_subtract(
		&(isc_time_t){ 0, 0 }, &(isc_interval_t){ 0, 0 }, NULL));
}

/* parse http time stamp */

ISC_RUN_TEST_IMPL(isc_time_parsehttptimestamp_test) {
	isc_result_t result;
	isc_time_t t, x;
	char buf[ISC_FORMATHTTPTIMESTAMP_SIZE];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_time_formathttptimestamp(&t, buf, sizeof(buf));
	result = isc_time_parsehttptimestamp(buf, &x);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_time_seconds(&t), isc_time_seconds(&x));
}

/* print UTC in ISO8601 */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ssZ */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 20);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');
	assert_int_equal(buf[19], 'Z');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1970-01-01T00:00:00Z");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T09:46:40Z");
}

/* print UTC in ISO8601 with milliseconds */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601ms_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.sssZ */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 24);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');
	assert_int_equal(buf[19], '.');
	assert_int_equal(buf[23], 'Z');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1970-01-01T00:00:00.000Z");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T09:46:40.123Z");
}

/* print UTC in ISO8601 with microseconds */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601us_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now_hires(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.ssssssZ */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601us(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 27);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');
	assert_int_equal(buf[19], '.');
	assert_int_equal(buf[26], 'Z');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601us(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1970-01-01T00:00:00.000000Z");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123456000);
	isc_time_formatISO8601us(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T09:46:40.123456Z");
}

/* print local time in ISO8601 */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601L_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 19);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1969-12-31T16:00:00");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T01:46:40");
}

/* print local time in ISO8601 with milliseconds */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601Lms_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.sss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 23);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');
	assert_int_equal(buf[19], '.');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1969-12-31T16:00:00.000");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T01:46:40.123");
}

/* print local time in ISO8601 with microseconds */

ISC_RUN_TEST_IMPL(isc_time_formatISO8601Lus_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now_hires(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.ssssss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601Lus(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 26);
	assert_int_equal(buf[4], '-');
	assert_int_equal(buf[7], '-');
	assert_int_equal(buf[10], 'T');
	assert_int_equal(buf[13], ':');
	assert_int_equal(buf[16], ':');
	assert_int_equal(buf[19], '.');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601Lus(&t, buf, sizeof(buf));
	assert_string_equal(buf, "1969-12-31T16:00:00.000000");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123456000);
	isc_time_formatISO8601Lus(&t, buf, sizeof(buf));
	assert_string_equal(buf, "2015-12-13T01:46:40.123456");
}

/* print UTC time as yyyymmddhhmmsssss */

ISC_RUN_TEST_IMPL(isc_time_formatshorttimestamp_test) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	UNUSED(state);

	setenv("TZ", "America/Los_Angeles", 1);
	result = isc_time_now(&t);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* check formatting: yyyymmddhhmmsssss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	assert_int_equal(strlen(buf), 17);

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	assert_string_equal(buf, "19700101000000000");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	assert_string_equal(buf, "20151213094640123");
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_time_add_test)
ISC_TEST_ENTRY(isc_time_sub_test)
ISC_TEST_ENTRY(isc_time_parsehttptimestamp_test)
ISC_TEST_ENTRY(isc_time_formatISO8601_test)
ISC_TEST_ENTRY(isc_time_formatISO8601ms_test)
ISC_TEST_ENTRY(isc_time_formatISO8601us_test)
ISC_TEST_ENTRY(isc_time_formatISO8601L_test)
ISC_TEST_ENTRY(isc_time_formatISO8601Lms_test)
ISC_TEST_ENTRY(isc_time_formatISO8601Lus_test)
ISC_TEST_ENTRY(isc_time_formatshorttimestamp_test)

ISC_TEST_LIST_END

ISC_TEST_MAIN
