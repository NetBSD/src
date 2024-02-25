/*	$NetBSD: db_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/journal.h>
#include <dns/name.h>
#include <dns/rdatalist.h>

#include <tests/dns.h>

#define BUFLEN	    255
#define BIGBUFLEN   (64 * 1024)
#define TEST_ORIGIN "test"

/*
 * Individual unit tests
 */

/* test multiple calls to dns_db_getoriginnode */
ISC_RUN_TEST_IMPL(getoriginnode) {
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_getoriginnode(db, &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_db_detachnode(db, &node);

	result = dns_db_getoriginnode(db, &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_db_detachnode(db, &node);

	dns_db_detach(&db);
}

/* test getservestalettl and setservestalettl */
ISC_RUN_TEST_IMPL(getsetservestalettl) {
	dns_db_t *db = NULL;
	isc_result_t result;
	dns_ttl_t ttl;

	UNUSED(state);

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_cache,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	ttl = 5000;
	result = dns_db_getservestalettl(db, &ttl);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(ttl, 0);

	ttl = 6 * 3600;
	result = dns_db_setservestalettl(db, ttl);
	assert_int_equal(result, ISC_R_SUCCESS);

	ttl = 5000;
	result = dns_db_getservestalettl(db, &ttl);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(ttl, 6 * 3600);

	dns_db_detach(&db);
}

/* check DNS_DBFIND_STALEOK works */
ISC_RUN_TEST_IMPL(dns_dbfind_staleok) {
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t example_fixed;
	dns_fixedname_t found_fixed;
	dns_name_t *example;
	dns_name_t *found;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	int count;
	int pass;
	isc_result_t result;
	unsigned char data[] = { 0x0a, 0x00, 0x00, 0x01 };

	UNUSED(state);

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_cache,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	example = dns_fixedname_initname(&example_fixed);
	found = dns_fixedname_initname(&found_fixed);

	result = dns_name_fromstring(example, "example", 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Pass 0: default; no stale processing permitted.
	 * Pass 1: stale processing for 1 second.
	 * Pass 2: stale turned off after being on.
	 */
	for (pass = 0; pass < 3; pass++) {
		dns_rdata_t rdata = DNS_RDATA_INIT;

		/* 10.0.0.1 */
		rdata.data = data;
		rdata.length = 4;
		rdata.rdclass = dns_rdataclass_in;
		rdata.type = dns_rdatatype_a;

		dns_rdatalist_init(&rdatalist);
		rdatalist.ttl = 2;
		rdatalist.type = dns_rdatatype_a;
		rdatalist.rdclass = dns_rdataclass_in;
		ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

		switch (pass) {
		case 0:
			/* default: stale processing off */
			break;
		case 1:
			/* turn on stale processing */
			result = dns_db_setservestalettl(db, 1);
			assert_int_equal(result, ISC_R_SUCCESS);
			break;
		case 2:
			/* turn off stale processing */
			result = dns_db_setservestalettl(db, 0);
			assert_int_equal(result, ISC_R_SUCCESS);
			break;
		}

		dns_rdataset_init(&rdataset);
		result = dns_rdatalist_tordataset(&rdatalist, &rdataset);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_db_findnode(db, example, true, &node);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_db_addrdataset(db, node, NULL, 0, &rdataset, 0,
					    NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		dns_db_detachnode(db, &node);
		dns_rdataset_disassociate(&rdataset);

		result = dns_db_find(db, example, NULL, dns_rdatatype_a, 0, 0,
				     &node, found, &rdataset, NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		/*
		 * May loop for up to 2 seconds performing non stale lookups.
		 */
		count = 0;
		do {
			count++;
			assert_in_range(count, 1, 21); /* loop sanity */
			assert_int_equal(rdataset.attributes &
						 DNS_RDATASETATTR_STALE,
					 0);
			assert_true(rdataset.ttl > 0);
			dns_db_detachnode(db, &node);
			dns_rdataset_disassociate(&rdataset);

			usleep(100000); /* 100 ms */

			result = dns_db_find(db, example, NULL, dns_rdatatype_a,
					     0, 0, &node, found, &rdataset,
					     NULL);
		} while (result == ISC_R_SUCCESS);

		assert_int_equal(result, ISC_R_NOTFOUND);

		/*
		 * Check whether we can get stale data.
		 */
		result = dns_db_find(db, example, NULL, dns_rdatatype_a,
				     DNS_DBFIND_STALEOK, 0, &node, found,
				     &rdataset, NULL);
		switch (pass) {
		case 0:
			assert_int_equal(result, ISC_R_NOTFOUND);
			break;
		case 1:
			/*
			 * Should loop for 1 second with stale lookups then
			 * stop.
			 */
			count = 0;
			do {
				count++;
				assert_in_range(count, 0, 49); /* loop sanity */
				assert_int_equal(result, ISC_R_SUCCESS);
				assert_int_equal(rdataset.attributes &
							 DNS_RDATASETATTR_STALE,
						 DNS_RDATASETATTR_STALE);
				dns_db_detachnode(db, &node);
				dns_rdataset_disassociate(&rdataset);

				usleep(100000); /* 100 ms */

				result = dns_db_find(
					db, example, NULL, dns_rdatatype_a,
					DNS_DBFIND_STALEOK, 0, &node, found,
					&rdataset, NULL);
			} while (result == ISC_R_SUCCESS);
			/*
			 * usleep(100000) can be slightly less than 10ms so
			 * allow the count to reach 11.
			 */
			assert_in_range(count, 1, 11);
			assert_int_equal(result, ISC_R_NOTFOUND);
			break;
		case 2:
			assert_int_equal(result, ISC_R_NOTFOUND);
			break;
		}
	}

	dns_db_detach(&db);
}

/* database class */
ISC_RUN_TEST_IMPL(class) {
	isc_result_t result;
	dns_db_t *db = NULL;

	UNUSED(state);

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_load(db, TESTS_DIR "/testdata/db/data.db",
			     dns_masterformat_text, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(dns_db_class(db), dns_rdataclass_in);

	dns_db_detach(&db);
}

/* database type */
ISC_RUN_TEST_IMPL(dbtype) {
	isc_result_t result;
	dns_db_t *db = NULL;

	UNUSED(state);

	/* DB has zone semantics */
	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_db_load(db, TESTS_DIR "/testdata/db/data.db",
			     dns_masterformat_text, 0);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(dns_db_iszone(db));
	assert_false(dns_db_iscache(db));
	dns_db_detach(&db);

	/* DB has cache semantics */
	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_cache,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_db_load(db, TESTS_DIR "/testdata/db/data.db",
			     dns_masterformat_text, 0);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(dns_db_iscache(db));
	assert_false(dns_db_iszone(db));
	dns_db_detach(&db);
}

/* database versions */
ISC_RUN_TEST_IMPL(version) {
	isc_result_t result;
	dns_fixedname_t fname, ffound;
	dns_name_t *name, *foundname;
	dns_db_t *db = NULL;
	dns_dbversion_t *ver = NULL, *new = NULL;
	dns_dbnode_t *node = NULL;
	dns_rdataset_t rdataset;

	UNUSED(state);

	result = dns_test_loaddb(&db, dns_dbtype_zone, "test.test",
				 TESTS_DIR "/testdata/db/data.db");
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Open current version for reading */
	dns_db_currentversion(db, &ver);
	dns_test_namefromstring("b.test.test", &fname);
	name = dns_fixedname_name(&fname);
	foundname = dns_fixedname_initname(&ffound);
	dns_rdataset_init(&rdataset);
	result = dns_db_find(db, name, ver, dns_rdatatype_a, 0, 0, &node,
			     foundname, &rdataset, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_rdataset_disassociate(&rdataset);
	dns_db_detachnode(db, &node);
	dns_db_closeversion(db, &ver, false);

	/* Open new version for writing */
	dns_db_currentversion(db, &ver);
	dns_test_namefromstring("b.test.test", &fname);
	name = dns_fixedname_name(&fname);
	foundname = dns_fixedname_initname(&ffound);
	dns_rdataset_init(&rdataset);
	result = dns_db_find(db, name, ver, dns_rdatatype_a, 0, 0, &node,
			     foundname, &rdataset, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_newversion(db, &new);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Delete the rdataset from the new version */
	result = dns_db_deleterdataset(db, node, new, dns_rdatatype_a, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_disassociate(&rdataset);
	dns_db_detachnode(db, &node);

	/* This should fail now */
	result = dns_db_find(db, name, new, dns_rdatatype_a, 0, 0, &node,
			     foundname, &rdataset, NULL);
	assert_int_equal(result, DNS_R_NXDOMAIN);

	dns_db_closeversion(db, &new, true);

	/* But this should still succeed */
	result = dns_db_find(db, name, ver, dns_rdatatype_a, 0, 0, &node,
			     foundname, &rdataset, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_rdataset_disassociate(&rdataset);
	dns_db_detachnode(db, &node);
	dns_db_closeversion(db, &ver, false);

	dns_db_detach(&db);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(getoriginnode)
ISC_TEST_ENTRY(getsetservestalettl)
ISC_TEST_ENTRY(dns_dbfind_staleok)
ISC_TEST_ENTRY(class)
ISC_TEST_ENTRY(dbtype)
ISC_TEST_ENTRY(version)
ISC_TEST_LIST_END

ISC_TEST_MAIN
