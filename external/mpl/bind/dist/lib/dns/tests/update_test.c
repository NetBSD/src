/*	$NetBSD: update_test.c,v 1.7 2020/08/03 17:23:42 christos Exp $	*/

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

#include "dnstest.h"

/*
 * Fix the linking order problem for overridden isc_stdtime_get() by making
 * everything local.  This also allows static functions from update.c to be
 * tested.
 */
#include "../update.c"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	setenv("TZ", "", 1);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

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

/*
 * Override isc_stdtime_get() from lib/isc/[unix/win32]/stdtime.c
 * with our own for testing purposes.
 */
void
isc_stdtime_get(isc_stdtime_t *now) {
	*now = mystdtime;
}

/*
 * Because update_test.o requires dns_update_*() symbols, the linker is able
 * to resolve them using libdns.a(update.o).  That object has other symbol
 * dependencies (dst_key_*()), so it pulls libdns.a(dst_api.o).
 * That object file requires the isc_stdtime_tostring() symbol.
 *
 * Define a local version here so that we don't have to depend on
 * libisc.a(stdtime.o).  If isc_stdtime_tostring() would be left undefined,
 * the linker has to get the required object file, and that will result in a
 * multiple definition error because the isc_stdtime_get() symbol exported
 * there is already in the exported list.
 */
void
isc_stdtime_tostring(isc_stdtime_t t, char *out, size_t outlen) {
	UNUSED(t);
	UNUSED(out);
	UNUSED(outlen);
}

/* simple increment by 1 */
static void
increment_test(void **state) {
	uint32_t old = 50;
	uint32_t serial;

	UNUSED(state);

	serial = dns_update_soaserial(old, dns_updatemethod_increment);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 51);
}

/* increment past zero, 0xfffffffff -> 1 */
static void
increment_past_zero_test(void **state) {
	uint32_t old = 0xffffffffu;
	uint32_t serial;

	UNUSED(state);

	serial = dns_update_soaserial(old, dns_updatemethod_increment);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 1u);
}

/* past to unixtime */
static void
past_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime - 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, mystdtime);
}

/* now to unixtime */
static void
now_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* future to unixtime */
static void
future_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime + 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* undefined plus 1 to unixtime */
static void
undefined_plus1_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old += 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, mystdtime);
}

/* undefined minus 1 to unixtime */
static void
undefined_minus1_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;
	old -= 1;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* undefined to unixtime */
static void
undefined_to_unix_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2011, 6, 22);
	old = mystdtime ^ 0x80000000u;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* handle unixtime being zero */
static void
unixtime_zero_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	mystdtime = 0;
	old = 0xfffffff0;

	serial = dns_update_soaserial(old, dns_updatemethod_unixtime);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, old + 1);
}

/* past to date */
static void
past_to_date_test(void **state) {
	uint32_t old, serial;

	UNUSED(state);

	set_mystdtime(2014, 3, 31);
	old = dns_update_soaserial(0, dns_updatemethod_date);
	set_mystdtime(2014, 4, 1);

	serial = dns_update_soaserial(old, dns_updatemethod_date);

	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040100);
}

/* now to date */
static void
now_to_date_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date);

	serial = dns_update_soaserial(old, dns_updatemethod_date);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040101);
}

/* future to date */
static void
future_to_date_test(void **state) {
	uint32_t old;
	uint32_t serial;

	UNUSED(state);

	set_mystdtime(2014, 4, 1);
	old = dns_update_soaserial(0, dns_updatemethod_date);
	set_mystdtime(2014, 3, 31);

	serial = dns_update_soaserial(old, dns_updatemethod_date);
	assert_true(isc_serial_lt(old, serial));
	assert_int_not_equal(serial, 0);
	assert_int_equal(serial, 2014040101);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(increment_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(increment_past_zero_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(past_to_unix_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(now_to_unix_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(future_to_unix_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(undefined_to_unix_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(undefined_plus1_to_unix_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(undefined_minus1_to_unix_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(unixtime_zero_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(past_to_date_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(now_to_date_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(future_to_date_test, _setup,
						_teardown),
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
