/*	$NetBSD: time_test.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <atf-c.h>

#include <isc/time.h>
#include <isc/result.h>

ATF_TC(isc_time_parsehttptimestamp);
ATF_TC_HEAD(isc_time_parsehttptimestamp, tc) {
	atf_tc_set_md_var(tc, "descr", "parse http time stamp");
}
ATF_TC_BODY(isc_time_parsehttptimestamp, tc) {
	isc_result_t result;
	isc_time_t t, x;
	char buf[ISC_FORMATHTTPTIMESTAMP_SIZE];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_time_formathttptimestamp(&t, buf, sizeof(buf));
	result = isc_time_parsehttptimestamp(buf, &x);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_time_seconds(&t), isc_time_seconds(&x));
}

ATF_TC(isc_time_formatISO8601);
ATF_TC_HEAD(isc_time_formatISO8601, tc) {
	atf_tc_set_md_var(tc, "descr", "print UTC in ISO8601");
}
ATF_TC_BODY(isc_time_formatISO8601, tc) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ssZ */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	ATF_CHECK_EQ(strlen(buf), 20);
	ATF_CHECK_EQ(buf[4], '-');
	ATF_CHECK_EQ(buf[7], '-');
	ATF_CHECK_EQ(buf[10], 'T');
	ATF_CHECK_EQ(buf[13], ':');
	ATF_CHECK_EQ(buf[16], ':');
	ATF_CHECK_EQ(buf[19], 'Z');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "1970-01-01T00:00:00Z");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "2015-12-13T09:46:40Z");
}

ATF_TC(isc_time_formatISO8601ms);
ATF_TC_HEAD(isc_time_formatISO8601ms, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "print UTC in ISO8601 with milliseconds");
}
ATF_TC_BODY(isc_time_formatISO8601ms, tc) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.sssZ */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	ATF_CHECK_EQ(strlen(buf), 24);
	ATF_CHECK_EQ(buf[4], '-');
	ATF_CHECK_EQ(buf[7], '-');
	ATF_CHECK_EQ(buf[10], 'T');
	ATF_CHECK_EQ(buf[13], ':');
	ATF_CHECK_EQ(buf[16], ':');
	ATF_CHECK_EQ(buf[19], '.');
	ATF_CHECK_EQ(buf[23], 'Z');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "1970-01-01T00:00:00.000Z");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601ms(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "2015-12-13T09:46:40.123Z");
}

ATF_TC(isc_time_formatISO8601L);
ATF_TC_HEAD(isc_time_formatISO8601L, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "print local time in ISO8601");
}
ATF_TC_BODY(isc_time_formatISO8601L, tc) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	ATF_CHECK_EQ(strlen(buf), 19);
	ATF_CHECK_EQ(buf[4], '-');
	ATF_CHECK_EQ(buf[7], '-');
	ATF_CHECK_EQ(buf[10], 'T');
	ATF_CHECK_EQ(buf[13], ':');
	ATF_CHECK_EQ(buf[16], ':');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "1969-12-31T16:00:00");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601L(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "2015-12-13T01:46:40");
}

ATF_TC(isc_time_formatISO8601Lms);
ATF_TC_HEAD(isc_time_formatISO8601Lms, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "print local time in ISO8601 with milliseconds");
}
ATF_TC_BODY(isc_time_formatISO8601Lms, tc) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* check formatting: yyyy-mm-ddThh:mm:ss.sss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	ATF_CHECK_EQ(strlen(buf), 23);
	ATF_CHECK_EQ(buf[4], '-');
	ATF_CHECK_EQ(buf[7], '-');
	ATF_CHECK_EQ(buf[10], 'T');
	ATF_CHECK_EQ(buf[13], ':');
	ATF_CHECK_EQ(buf[16], ':');
	ATF_CHECK_EQ(buf[19], '.');

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "1969-12-31T16:00:00.000");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatISO8601Lms(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "2015-12-13T01:46:40.123");
}

ATF_TC(isc_time_formatshorttimestamp);
ATF_TC_HEAD(isc_time_formatshorttimestamp, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "print UTC time as yyyymmddhhmmsssss");
}
ATF_TC_BODY(isc_time_formatshorttimestamp, tc) {
	isc_result_t result;
	isc_time_t t;
	char buf[64];

	setenv("TZ", "PST8PDT", 1);
	result = isc_time_now(&t);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* check formatting: yyyymmddhhmmsssss */
	memset(buf, 'X', sizeof(buf));
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	ATF_CHECK_EQ(strlen(buf), 17);

	/* check time conversion correctness */
	memset(buf, 'X', sizeof(buf));
	isc_time_settoepoch(&t);
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "19700101000000000");

	memset(buf, 'X', sizeof(buf));
	isc_time_set(&t, 1450000000, 123000000);
	isc_time_formatshorttimestamp(&t, buf, sizeof(buf));
	ATF_CHECK_STREQ(buf, "20151213094640123");
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_time_parsehttptimestamp);
	ATF_TP_ADD_TC(tp, isc_time_formatISO8601);
	ATF_TP_ADD_TC(tp, isc_time_formatISO8601ms);
	ATF_TP_ADD_TC(tp, isc_time_formatISO8601L);
	ATF_TP_ADD_TC(tp, isc_time_formatISO8601Lms);
	ATF_TP_ADD_TC(tp, isc_time_formatshorttimestamp);
	return (atf_no_error());
}
