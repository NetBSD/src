/*	$NetBSD: update_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/serial.h>
#include <isc/stdtime.h>
#include <isc/util.h>

#include <dns/update.h>
#define KEEP_BEFORE

/*
 * Fix the linking order problem for overridden isc_stdtime_get() by making
 * everything local.  This also allows static functions from update.c to be
 * tested.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#undef CHECK
#include "update.c"
#pragma GCC diagnostic pop

#undef CHECK
#include <tests/dns.h>

static int
setup_test(void **state) {
	UNUSED(state);

	setenv("TZ", "", 1);

	return (0);
}

static uint32_t mystdtime;

static void
set_mystdtime(int year, int month, int day) {
	struct tm tm;

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	mystdtime = timegm(&tm);
}

void
isc_stdtime_get(isc_stdtime_t *now) {
	*now = mystdtime;
}

/* simple increment by 1 */
ISC_RUN_TEST_IMPL(increment) {
	uint32_t old = 50;
	uint32_t serial;

	UNUSED(state);

	serial = dns_update_soaserial(old, dns_updatemethod_increment, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 51);
}

/* increment past zero, 0xfffffffff -> 1 */
ISC_RUN_TEST_IMPL(increment_past_zero) {
	uint32_t old = 0xffffffffu;
	uint32_t serial;

	UNUSED(state);

	serial = dns_update_soaserial(old, dns_updatemethod_increment, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 1u);
}

/* past to unixtime */
ISC_RUN_TEST_IMPL(past_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime - 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, mystdtime);
}

/* now to unixtime */
ISC_RUN_TEST_IMPL(now_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* future to unixtime */
ISC_RUN_TEST_IMPL(future_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime + 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* undefined plus 1 to unixtime */
ISC_RUN_TEST_IMPL(undefined_plus1_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old += 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, mystdtime);
}

/* undefined minus 1 to unixtime */
ISC_RUN_TEST_IMPL(undefined_minus1_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old -= 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* undefined to unixtime */
ISC_RUN_TEST_IMPL(undefined_to_unix) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* handle unixtime being zero */
ISC_RUN_TEST_IMPL(unixtime_zero) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	mystdtime = 0;
	old = 0xfffffff0;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime, NULL);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* past to date */
ISC_RUN_TEST_IMPL(past_to_date) {
	uint32_t old, serial;
	dns_updatemethod_t used = dns_updatemethod_none;

	UNUSED(state);

	set_mystdtime(2014, 3, 31);
	old = dns_update_soaserial(0, dns_updatemethod_date, NULL);
	set_mystdtime(2014, 4, 1);

	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040100);
	assert_int_equal(dns_updatemethod_date, used);
}

/* now to date */
ISC_RUN_TEST_IMPL(now_to_date) {
	uint32_t old;
	uint32_t serial;
	dns_updatemethod_t used = dns_updatemethod_none;

	UNUSED(state);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date, NULL);

	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040101);
	assert_int_equal(dns_updatemethod_date, used);

	old = 2014040198;
	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040199);
	assert_int_equal(dns_updatemethod_date, used);

	/*
	 * Stealing from "tomorrow".
	 */
	old = 2014040199;
	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040200);
	assert_int_equal(dns_updatemethod_increment, used);
}

/* future to date */
ISC_RUN_TEST_IMPL(future_to_date) {
	uint32_t old;
	uint32_t serial;
	dns_updatemethod_t used = dns_updatemethod_none;

	UNUSED(state);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date, NULL);
	set_mystdtime(2014, 3, 31);

	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040101);
	assert_int_equal(dns_updatemethod_increment, used);

	old = serial;
	serial = dns_update_soaserial(old, dns_updatemethod_date, &used);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040102);
	assert_int_equal(dns_updatemethod_increment, used);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(increment, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(increment_past_zero, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(past_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(now_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(future_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(undefined_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(undefined_plus1_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(undefined_minus1_to_unix, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(unixtime_zero, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(past_to_date, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(now_to_date, setup_test, NULL)
ISC_TEST_ENTRY_CUSTOM(future_to_date, setup_test, NULL)
ISC_TEST_LIST_END

ISC_TEST_MAIN
