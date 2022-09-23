/*	$NetBSD: dbdiff_test.c,v 1.8 2022/09/23 12:15:32 christos Exp $	*/

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

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/journal.h>
#include <dns/name.h>

#include "dnstest.h"

#define BUFLEN	    255
#define BIGBUFLEN   (64 * 1024)
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

static void
test_create(const char *oldfile, dns_db_t **old, const char *newfile,
	    dns_db_t **newdb) {
	isc_result_t result;

	result = dns_test_loaddb(old, dns_dbtype_zone, TEST_ORIGIN, oldfile);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_loaddb(newdb, dns_dbtype_zone, TEST_ORIGIN, newfile);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/* dns_db_diffx of identical content */
static void
diffx_same(void **state) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	isc_result_t result;
	dns_diff_t diff;

	UNUSED(state);

	test_create("testdata/diff/zone1.data", &olddb,
		    "testdata/diff/zone1.data", &newdb);

	dns_diff_init(dt_mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_true(ISC_LIST_EMPTY(diff.tuples));

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
}

/* dns_db_diffx of zone with record added */
static void
diffx_add(void **state) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	UNUSED(state);

	test_create("testdata/diff/zone1.data", &olddb,
		    "testdata/diff/zone2.data", &newdb);

	dns_diff_init(dt_mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(ISC_LIST_EMPTY(diff.tuples));
	for (tuple = ISC_LIST_HEAD(diff.tuples); tuple != NULL;
	     tuple = ISC_LIST_NEXT(tuple, link))
	{
		assert_int_equal(tuple->op, DNS_DIFFOP_ADD);
		count++;
	}
	assert_int_equal(count, 1);

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
}

/* dns_db_diffx of zone with record removed */
static void
diffx_remove(void **state) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	UNUSED(state);

	test_create("testdata/diff/zone1.data", &olddb,
		    "testdata/diff/zone3.data", &newdb);

	dns_diff_init(dt_mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(ISC_LIST_EMPTY(diff.tuples));
	for (tuple = ISC_LIST_HEAD(diff.tuples); tuple != NULL;
	     tuple = ISC_LIST_NEXT(tuple, link))
	{
		assert_int_equal(tuple->op, DNS_DIFFOP_DEL);
		count++;
	}
	assert_int_equal(count, 1);

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(diffx_same, _setup, _teardown),
		cmocka_unit_test_setup_teardown(diffx_add, _setup, _teardown),
		cmocka_unit_test_setup_teardown(diffx_remove, _setup,
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
