/*	$NetBSD: time_test.c,v 1.5 2020/05/24 19:46:27 christos Exp $	*/

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

#if HAVE_CMOCKA

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

/* parse http time stamp */
static void
isc_time_parsehttptimestamp_test(void **state) {
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
static void
isc_time_formatISO8601_test(void **state) {
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
static void
isc_time_formatISO8601ms_test(void **state) {
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

/* print local time in ISO8601 */
static void
isc_time_formatISO8601L_test(void **state) {
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
static void
isc_time_formatISO8601Lms_test(void **state) {
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

/* print UTC time as yyyymmddhhmmsssss */
static void
isc_time_formatshorttimestamp_test(void **state) {
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

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_time_parsehttptimestamp_test),
		cmocka_unit_test(isc_time_formatISO8601_test),
		cmocka_unit_test(isc_time_formatISO8601ms_test),
		cmocka_unit_test(isc_time_formatISO8601L_test),
		cmocka_unit_test(isc_time_formatISO8601Lms_test),
		cmocka_unit_test(isc_time_formatshorttimestamp_test),
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

#endif /* if HAVE_CMOCKA */
