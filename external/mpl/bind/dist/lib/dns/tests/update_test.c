/*	$NetBSD: update_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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

#include <atf-c.h>

#include <unistd.h>
#include <time.h>

#include <isc/serial.h>
#include <isc/stdtime.h>

#include <dns/update.h>

#include "dnstest.h"

static isc_uint32_t mystdtime;

static void set_mystdtime(int year, int month, int day) {
	struct tm tm;

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	mystdtime = timegm(&tm) ;
}

void isc_stdtime_get(isc_stdtime_t *now) {
	*now = mystdtime;
}

/*
 * Individual unit tests
 */

ATF_TC(increment);
ATF_TC_HEAD(increment, tc) {
  atf_tc_set_md_var(tc, "descr", "simple increment by 1");
}
ATF_TC_BODY(increment, tc) {
	isc_uint32_t old = 50;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_increment);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK_MSG(serial != 0, "serial (%d) should not equal 0", serial);
	ATF_REQUIRE_EQ(serial, 51);
	dns_test_end();
}

/* 0xfffffffff -> 1 */
ATF_TC(increment_past_zero);
ATF_TC_HEAD(increment_past_zero, tc) {
  atf_tc_set_md_var(tc, "descr", "increment past zero, ffffffff -> 1");
}
ATF_TC_BODY(increment_past_zero, tc) {
	isc_uint32_t old = 0xffffffffu;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_increment);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, 1u);
	dns_test_end();
}

ATF_TC(past_to_unix);
ATF_TC_HEAD(past_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "past to unixtime");
}
ATF_TC_BODY(past_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime - 1;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, mystdtime);
	dns_test_end();
}

ATF_TC(now_to_unix);
ATF_TC_HEAD(now_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "now to unixtime");
}
ATF_TC_BODY(now_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, old + 1);
	dns_test_end();
}

ATF_TC(future_to_unix);
ATF_TC_HEAD(future_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "future to unixtime");
}
ATF_TC_BODY(future_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime + 1;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, old + 1);
	dns_test_end();
}

ATF_TC(undefined_plus1_to_unix);
ATF_TC_HEAD(undefined_plus1_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "undefined plus 1 to unixtime");
}
ATF_TC_BODY(undefined_plus1_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old += 1;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, mystdtime);
	dns_test_end();
}

ATF_TC(undefined_minus1_to_unix);
ATF_TC_HEAD(undefined_minus1_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "undefined minus 1 to unixtime");
}
ATF_TC_BODY(undefined_minus1_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old -= 1;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, old + 1);
	dns_test_end();
}

ATF_TC(undefined_to_unix);
ATF_TC_HEAD(undefined_to_unix, tc) {
  atf_tc_set_md_var(tc, "descr", "undefined to unixtime");
}
ATF_TC_BODY(undefined_to_unix, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, old + 1);
	dns_test_end();
}

ATF_TC(unixtime_zero);
ATF_TC_HEAD(unixtime_zero, tc) {
  atf_tc_set_md_var(tc, "descr", "handle unixtime being zero");
}
ATF_TC_BODY(unixtime_zero, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	mystdtime = 0;
	old = 0xfffffff0;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, old + 1);
	dns_test_end();
}

ATF_TC(past_to_date);
ATF_TC_HEAD(past_to_date, tc) {
  atf_tc_set_md_var(tc, "descr", "past to date");
}
ATF_TC_BODY(past_to_date, tc) {
	isc_uint32_t old, serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2014, 3, 31);
	old = dns_update_soaserial(0, dns_updatemethod_date);
	set_mystdtime(2014, 4, 1);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_date);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, 2014040100);
	dns_test_end();
}

ATF_TC(now_to_date);
ATF_TC_HEAD(now_to_date, tc) {
  atf_tc_set_md_var(tc, "descr", "now to date");
}
ATF_TC_BODY(now_to_date, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_date);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, 2014040101);
	dns_test_end();
}

ATF_TC(future_to_date);
ATF_TC_HEAD(future_to_date, tc) {
  atf_tc_set_md_var(tc, "descr", "future to date");
}
ATF_TC_BODY(future_to_date, tc) {
	isc_uint32_t old;
	isc_uint32_t serial;
	isc_result_t result;

	UNUSED(tc);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date);
	set_mystdtime(2014, 3, 31);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	serial = dns_update_soaserial(old, dns_updatemethod_date);
	ATF_REQUIRE_EQ(isc_serial_lt(old, serial), ISC_TRUE);
	ATF_CHECK(serial != 0);
	ATF_REQUIRE_EQ(serial, 2014040101);
	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, increment);
	ATF_TP_ADD_TC(tp, increment_past_zero);
	ATF_TP_ADD_TC(tp, past_to_unix);
	ATF_TP_ADD_TC(tp, now_to_unix);
	ATF_TP_ADD_TC(tp, future_to_unix);
	ATF_TP_ADD_TC(tp, undefined_to_unix);
	ATF_TP_ADD_TC(tp, undefined_plus1_to_unix);
	ATF_TP_ADD_TC(tp, undefined_minus1_to_unix);
	ATF_TP_ADD_TC(tp, unixtime_zero);
	ATF_TP_ADD_TC(tp, past_to_date);
	ATF_TP_ADD_TC(tp, now_to_date);
	ATF_TP_ADD_TC(tp, future_to_date);

	return (atf_no_error());
}

