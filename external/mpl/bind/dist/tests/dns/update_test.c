/*	$NetBSD: update_test.c,v 1.6 2026/08/29 14:55:20 christos Exp $	*/

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
#include <limits.h>
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

#include <isc/dir.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/serial.h>
#include <isc/stdtime.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/update.h>
#include <dns/zone.h>

#include <dst/dst.h>
#define KEEP_BEFORE

/*
 * Fix the linking order problem for overridden isc_stdtime_now() by making
 * everything local.  This also allows static functions from update.c to be
 * tested.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "update.c"
#pragma GCC diagnostic pop

#include <tests/dns.h>

static int
setup_test(void **state) {
	UNUSED(state);

	setenv("TZ", "", 1);
	tzset();

	return 0;
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

isc_stdtime_t
isc_stdtime_now(void) {
	return mystdtime;
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

/* Remove every file in 'keydir' and then the directory itself. */
static void
cleanup_keydir(const char *keydir) {
	isc_dir_t dir;
	isc_result_t result;

	isc_dir_init(&dir);
	result = isc_dir_open(&dir, keydir);
	if (result != ISC_R_SUCCESS) {
		return;
	}
	while (isc_dir_read(&dir) == ISC_R_SUCCESS) {
		char path[PATH_MAX];

		if (strcmp(dir.entry.name, ".") == 0 ||
		    strcmp(dir.entry.name, "..") == 0)
		{
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", keydir, dir.entry.name);
		(void)remove(path);
	}
	isc_dir_close(&dir);
	(void)rmdir(keydir);
}

/*
 * Regression test for find_zone_keys() (GL #6051): when more than
 * DNS_MAXZONEKEYS matching private keys are present, the keys beyond the
 * limit must be released rather than leaked.  Before the fix the function
 * destroyed only the first over-limit key and then broke out of the loop,
 * abandoning every key after it on a local list.
 */
ISC_LOOP_TEST_IMPL(find_zone_keys_overflow) {
	isc_result_t result;
	dns_zonemgr_t *mgr = NULL;
	dns_zone_t *zone = NULL;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	dst_key_t *keys[DNS_MAXZONEKEYS] = { NULL };
	unsigned int nkeys = 0;
	/* A few keys past the limit, so a tail survives the overflow entry. */
	const unsigned int total = DNS_MAXZONEKEYS + 3;
	uint16_t ids[DNS_MAXZONEKEYS + 3];
	unsigned int generated = 0;
	char keydir[] = BUILDDIR "/find_zone_keys.XXXXXX";

	dst_lib_init(mctx, NULL);

	assert_non_null(mkdtemp(keydir));

	name = dns_fixedname_initname(&fname);
	result = dns_name_fromstring(name, "example.", dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Generate 'total' distinct zone keys into the temporary key
	 * directory.  Skip the rare key-tag collision so each key gets its own
	 * K*.private file instead of overwriting an earlier one.
	 */
	while (generated < total) {
		dst_key_t *key = NULL;
		uint16_t id;
		bool dup = false;

		result = dst_key_generate(
			name, DST_ALG_ECDSA256, 256, 0, DNS_KEYOWNER_ZONE,
			DNS_KEYPROTO_DNSSEC, dns_rdataclass_in, NULL, mctx,
			&key, NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		id = dst_key_id(key);
		for (unsigned int i = 0; i < generated; i++) {
			if (ids[i] == id) {
				dup = true;
				break;
			}
		}
		if (dup) {
			dst_key_free(&key);
			continue;
		}

		result = dst_key_tofile(key, DST_TYPE_PUBLIC | DST_TYPE_PRIVATE,
					keydir);
		assert_int_equal(result, ISC_R_SUCCESS);
		ids[generated++] = id;
		dst_key_free(&key);
	}

	/* find_zone_keys() reads the keystore list off the zone manager. */
	dns_zonemgr_create(mctx, netmgr, &mgr);
	result = dns_test_makezone("example", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_zonemgr_managezone(mgr, zone);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_zone_setkeydirectory(zone, keydir);

	result = find_zone_keys(zone, mctx, DNS_MAXZONEKEYS, keys, &nkeys);
	assert_int_equal(result, ISC_R_NOSPACE);
	assert_int_equal(nkeys, DNS_MAXZONEKEYS);

	/*
	 * Free the keys handed back to the caller.  Any over-limit key that
	 * find_zone_keys() failed to drain stays allocated in the default
	 * memory context and is reported by its end-of-run leak check.
	 */
	for (unsigned int i = 0; i < nkeys; i++) {
		dst_key_free(&keys[i]);
	}

	dns_zonemgr_releasezone(mgr, zone);
	dns_zone_detach(&zone);
	dns_zonemgr_shutdown(mgr);
	dns_zonemgr_detach(&mgr);

	cleanup_keydir(keydir);

	isc_mem_checkdestroyed(stderr);

	isc_loopmgr_shutdown(loopmgr);

	dst_lib_destroy();
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
ISC_TEST_ENTRY_CUSTOM(find_zone_keys_overflow, setup_managers,
		      teardown_managers)
ISC_TEST_LIST_END

ISC_TEST_MAIN
