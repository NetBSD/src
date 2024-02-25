/*	$NetBSD: dbdiff_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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

#include <tests/dns.h>

#define BUFLEN	    255
#define BIGBUFLEN   (64 * 1024)
#define TEST_ORIGIN "test"

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
ISC_RUN_TEST_IMPL(diffx_same) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	isc_result_t result;
	dns_diff_t diff;

	UNUSED(state);

	test_create(TESTS_DIR "/testdata/diff/zone1.data", &olddb,
		    TESTS_DIR "/testdata/diff/zone1.data", &newdb);

	dns_diff_init(mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_true(ISC_LIST_EMPTY(diff.tuples));

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
}

/* dns_db_diffx of zone with record added */
ISC_RUN_TEST_IMPL(diffx_add) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	UNUSED(state);

	test_create(TESTS_DIR "/testdata/diff/zone1.data", &olddb,
		    TESTS_DIR "/testdata/diff/zone2.data", &newdb);

	dns_diff_init(mctx, &diff);

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
ISC_RUN_TEST_IMPL(diffx_remove) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	UNUSED(state);

	test_create(TESTS_DIR "/testdata/diff/zone1.data", &olddb,
		    TESTS_DIR "/testdata/diff/zone3.data", &newdb);

	dns_diff_init(mctx, &diff);

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

ISC_TEST_LIST_START
ISC_TEST_ENTRY(diffx_same)
ISC_TEST_ENTRY(diffx_add)
ISC_TEST_ENTRY(diffx_remove)
ISC_TEST_LIST_END

ISC_TEST_MAIN
