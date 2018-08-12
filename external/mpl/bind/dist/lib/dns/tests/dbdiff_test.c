/*	$NetBSD: dbdiff_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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
#include <stdlib.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/name.h>
#include <dns/journal.h>

#include "dnstest.h"

/*
 * Helper functions
 */

#define	BUFLEN		255
#define	BIGBUFLEN	(64 * 1024)
#define TEST_ORIGIN	"test"

static void
test_create(const atf_tc_t *tc, dns_db_t **old, dns_db_t **newdb) {
	isc_result_t result;

	result = dns_test_loaddb(old, dns_dbtype_zone, TEST_ORIGIN,
				 atf_tc_get_md_var(tc, "X-old"));
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_loaddb(newdb, dns_dbtype_zone, TEST_ORIGIN,
				 atf_tc_get_md_var(tc, "X-new"));
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
}

/*
 * Individual unit tests
 */

ATF_TC(diffx_same);
ATF_TC_HEAD(diffx_same, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_db_diffx of identical content");
	atf_tc_set_md_var(tc, "X-old", "testdata/diff/zone1.data");
	atf_tc_set_md_var(tc, "X-new", "testdata/diff/zone1.data"); }
ATF_TC_BODY(diffx_same, tc) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	isc_result_t result;
	dns_diff_t diff;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	test_create(tc, &olddb, &newdb);

	dns_diff_init(mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(ISC_LIST_EMPTY(diff.tuples), ISC_TRUE);

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
	dns_test_end();
}

ATF_TC(diffx_add);
ATF_TC_HEAD(diffx_add, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "dns_db_diffx of zone with record added");
	atf_tc_set_md_var(tc, "X-old", "testdata/diff/zone1.data");
	atf_tc_set_md_var(tc, "X-new", "testdata/diff/zone2.data");
}
ATF_TC_BODY(diffx_add, tc) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	test_create(tc, &olddb, &newdb);

	dns_diff_init(mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(ISC_LIST_EMPTY(diff.tuples), ISC_FALSE);
	for (tuple = ISC_LIST_HEAD(diff.tuples); tuple != NULL;
	     tuple = ISC_LIST_NEXT(tuple, link)) {
		ATF_REQUIRE_EQ(tuple->op, DNS_DIFFOP_ADD);
		count++;
	}
	ATF_REQUIRE_EQ(count, 1);

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
	dns_test_end();
}

ATF_TC(diffx_remove);
ATF_TC_HEAD(diffx_remove, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "dns_db_diffx of zone with record removed");
	atf_tc_set_md_var(tc, "X-old", "testdata/diff/zone1.data");
	atf_tc_set_md_var(tc, "X-new", "testdata/diff/zone3.data");
}
ATF_TC_BODY(diffx_remove, tc) {
	dns_db_t *newdb = NULL, *olddb = NULL;
	dns_difftuple_t *tuple;
	isc_result_t result;
	dns_diff_t diff;
	int count = 0;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	test_create(tc, &olddb, &newdb);

	dns_diff_init(mctx, &diff);

	result = dns_db_diffx(&diff, newdb, NULL, olddb, NULL, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(ISC_LIST_EMPTY(diff.tuples), ISC_FALSE);
	for (tuple = ISC_LIST_HEAD(diff.tuples); tuple != NULL;
	     tuple = ISC_LIST_NEXT(tuple, link)) {
		ATF_REQUIRE_EQ(tuple->op, DNS_DIFFOP_DEL);
		count++;
	}
	ATF_REQUIRE_EQ(count, 1);

	dns_diff_clear(&diff);
	dns_db_detach(&newdb);
	dns_db_detach(&olddb);
	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, diffx_same);
	ATF_TP_ADD_TC(tp, diffx_add);
	ATF_TP_ADD_TC(tp, diffx_remove);
	return (atf_no_error());
}
