/*	$NetBSD: resconf_test.c,v 1.2 2025/05/21 14:48:06 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/util.h>

#include <irs/resconf.h>

#include <tests/isc.h>

static isc_result_t
check_nameserver(irs_resconf_t *resconf, const char *expected) {
	char buf[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddrlist_t *servers = irs_resconf_getnameservers(resconf);
	isc_sockaddr_t *entry = ISC_LIST_HEAD(*servers);
	assert_true(entry != NULL);
	isc_sockaddr_format(entry, buf, sizeof(buf));
	assert_string_equal(buf, expected);
	return ISC_R_SUCCESS;
}

static isc_result_t
check_ns4(irs_resconf_t *resconf) {
	return check_nameserver(resconf, "10.0.0.1#53");
}

static isc_result_t
check_ns6(irs_resconf_t *resconf) {
	return check_nameserver(resconf, "2001:db8::1#53");
}

static isc_result_t
check_scoped(irs_resconf_t *resconf) {
	return check_nameserver(resconf, "fe80::1%1#53");
}

static isc_result_t
check_number(unsigned int n, unsigned int expected) {
	return (n == expected) ? ISC_R_SUCCESS : ISC_R_BADNUMBER;
}

static isc_result_t
check_attempts(irs_resconf_t *resconf) {
	return check_number(irs_resconf_getattempts(resconf), 4);
}

static isc_result_t
check_timeout(irs_resconf_t *resconf) {
	return check_number(irs_resconf_gettimeout(resconf), 1);
}

static isc_result_t
check_ndots(irs_resconf_t *resconf) {
	return check_number(irs_resconf_getndots(resconf), 2);
}

static isc_result_t
search_example(irs_resconf_t *resconf) {
	irs_resconf_search_t *entry;
	irs_resconf_searchlist_t *list;
	list = irs_resconf_getsearchlist(resconf);
	if (list == NULL) {
		return ISC_R_NOTFOUND;
	}
	entry = ISC_LIST_HEAD(*list);
	assert_true(entry != NULL && entry->domain != NULL);
	assert_string_equal(entry->domain, "example.com");

	entry = ISC_LIST_TAIL(*list);
	assert_true(entry != NULL && entry->domain != NULL);
	assert_string_equal(entry->domain, "example.net");
	return ISC_R_SUCCESS;
}

static isc_result_t
check_options(irs_resconf_t *resconf) {
	if (irs_resconf_getattempts(resconf) != 3) {
		return ISC_R_BADNUMBER; /* default value only */
	}

	if (irs_resconf_getndots(resconf) != 2) {
		return ISC_R_BADNUMBER;
	}

	if (irs_resconf_gettimeout(resconf) != 1) {
		return ISC_R_BADNUMBER;
	}

	return ISC_R_SUCCESS;
}

/* test irs_resconf_load() */
ISC_RUN_TEST_IMPL(irs_resconf_load) {
	isc_result_t result;
	irs_resconf_t *resconf = NULL;
	unsigned int i;
	struct {
		const char *file;
		isc_result_t loadres;
		isc_result_t (*check)(irs_resconf_t *resconf);
		isc_result_t checkres;
	} tests[] = { { "testdata/resconf/domain.conf", ISC_R_SUCCESS, NULL,
			ISC_R_SUCCESS },
		      { "testdata/resconf/nameserver-v4.conf", ISC_R_SUCCESS,
			check_ns4, ISC_R_SUCCESS },
		      { "testdata/resconf/nameserver-v6.conf", ISC_R_SUCCESS,
			check_ns6, ISC_R_SUCCESS },
		      { "testdata/resconf/nameserver-v6-scoped.conf",
			ISC_R_SUCCESS, check_scoped, ISC_R_SUCCESS },
		      { "testdata/resconf/options-attempts.conf", ISC_R_SUCCESS,
			check_attempts, ISC_R_SUCCESS },
		      { "testdata/resconf/options-debug.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/options-ndots.conf", ISC_R_SUCCESS,
			check_ndots, ISC_R_SUCCESS },
		      { "testdata/resconf/options-timeout.conf", ISC_R_SUCCESS,
			check_timeout, ISC_R_SUCCESS },
		      { "testdata/resconf/options-unknown.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/options.conf", ISC_R_SUCCESS,
			check_options, ISC_R_SUCCESS },
		      { "testdata/resconf/options-bad-ndots.conf", ISC_R_RANGE,
			NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/options-empty.conf",
			ISC_R_UNEXPECTEDEND, NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/port.conf", ISC_R_SUCCESS, NULL,
			ISC_R_SUCCESS },
		      { "testdata/resconf/resolv.conf", ISC_R_SUCCESS, NULL,
			ISC_R_SUCCESS },
		      { "testdata/resconf/search.conf", ISC_R_SUCCESS,
			search_example, ISC_R_SUCCESS },
		      { "testdata/resconf/sortlist-v4.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/timeout.conf", ISC_R_SUCCESS, NULL,
			ISC_R_SUCCESS },
		      { "testdata/resconf/unknown-with-value.conf",
			ISC_R_SUCCESS, NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/unknown-without-value.conf",
			ISC_R_SUCCESS, NULL, ISC_R_SUCCESS },
		      { "testdata/resconf/unknown+search.conf", ISC_R_SUCCESS,
			search_example, ISC_R_SUCCESS } };

	UNUSED(state);

	assert_return_code(chdir(TESTS_DIR), 0);

	for (i = 0; i < sizeof(tests) / sizeof(tests[1]); i++) {
		if (debug) {
			fprintf(stderr, "# testing '%s'\n", tests[i].file);
		}
		result = irs_resconf_load(mctx, tests[i].file, &resconf);
		if (result != tests[i].loadres) {
			fail_msg("# unexpected result %s loading %s",
				 isc_result_totext(result), tests[i].file);
		}

		if (result == ISC_R_SUCCESS && resconf == NULL) {
			fail_msg("# NULL on success loading %s", tests[i].file);
		} else if (result != ISC_R_SUCCESS && resconf != NULL) {
			fail_msg("# non-NULL on failure loading %s",
				 tests[i].file);
		}

		if (resconf != NULL && tests[i].check != NULL) {
			result = (tests[i].check)(resconf);
			if (result != tests[i].checkres) {
				fail_msg("# unexpected result %s loading %s",
					 isc_result_totext(result),
					 tests[i].file);
			}
		}
		if (resconf != NULL) {
			irs_resconf_destroy(&resconf);
		}
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(irs_resconf_load)
ISC_TEST_LIST_END

ISC_TEST_MAIN
