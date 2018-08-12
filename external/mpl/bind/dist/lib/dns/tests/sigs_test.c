/*	$NetBSD: sigs_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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

#include <isc/util.h>

#if defined(OPENSSL) || defined(PKCS11CRYPTO)
#include <string.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/types.h>
#include <dns/zone.h>

#include <dst/dst.h>

#include <isc/buffer.h>
#include <isc/list.h>
#include <isc/region.h>
#include <isc/stdtime.h>
#include <isc/result.h>
#include <isc/types.h>

#include "../zone_p.h"

#include "dnstest.h"

/*%
 * Structure characterizing a single diff tuple in the dns_diff_t structure
 * prepared by dns__zone_updatesigs().
 */
typedef struct {
	dns_diffop_t op;
	const char *owner;
	dns_ttl_t ttl;
	const char *type;
} zonediff_t;

#define ZONEDIFF_SENTINEL { 0, NULL, 0, NULL }

/*%
 * Structure defining a dns__zone_updatesigs() test.
 */
typedef struct {
	const char *description;	/* test description */
	const zonechange_t *changes;	/* array of "raw" zone changes */
	const zonediff_t *zonediff;	/* array of "processed" zone changes */
} updatesigs_test_params_t;

/*%
 * Check whether the 'found' tuple matches the 'expected' tuple.  'found' is
 * the 'index'th tuple output by dns__zone_updatesigs() in test 'test'.
 */
static void
compare_tuples(const zonediff_t *expected, dns_difftuple_t *found,
	       const updatesigs_test_params_t *test, size_t index)
{
	char found_covers[DNS_RDATATYPE_FORMATSIZE] = { };
	char found_type[DNS_RDATATYPE_FORMATSIZE] = { };
	char found_name[DNS_NAME_FORMATSIZE];
	isc_consttextregion_t typeregion;
	dns_fixedname_t expected_fname;
	dns_rdatatype_t expected_type;
	dns_name_t *expected_name;
	dns_rdata_rrsig_t rrsig;
	isc_buffer_t typebuf;
	isc_result_t result;

	REQUIRE(expected != NULL);
	REQUIRE(found != NULL);
	REQUIRE(index > 0);

	/*
	 * Check operation.
	 */
	ATF_CHECK_EQ_MSG(expected->op, found->op,
			 "test \"%s\": tuple %zu: "
			 "expected op %d, found %d",
			 test->description, index,
			 expected->op, found->op);

	/*
	 * Check owner name.
	 */
	expected_name = dns_fixedname_initname(&expected_fname);
	result = dns_name_fromstring(expected_name, expected->owner, 0, mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	dns_name_format(&found->name, found_name, sizeof(found_name));
	ATF_CHECK_MSG(dns_name_equal(expected_name, &found->name),
		      "test \"%s\": tuple %zu: "
		      "expected owner \"%s\", found \"%s\"",
		      test->description, index,
		      expected->owner, found_name);

	/*
	 * Check TTL.
	 */
	ATF_CHECK_EQ_MSG(expected->ttl, found->ttl,
		      "test \"%s\": tuple %zu: "
		      "expected TTL %u, found %u",
		      test->description, index,
		      expected->ttl, found->ttl);

	/*
	 * Parse expected RR type.
	 */
	typeregion.base = expected->type;
	typeregion.length = strlen(expected->type);
	result = dns_rdatatype_fromtext(&expected_type,
					(isc_textregion_t*)&typeregion);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Format found RR type for reporting purposes.
	 */
	isc_buffer_init(&typebuf, found_type, sizeof(found_type));
	result = dns_rdatatype_totext(found->rdata.type, &typebuf);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Check RR type.
	 */
	switch (expected->op) {
	case DNS_DIFFOP_ADDRESIGN:
	case DNS_DIFFOP_DELRESIGN:
		/*
		 * Found tuple must be of type RRSIG.
		 */
		ATF_CHECK_EQ_MSG(found->rdata.type, dns_rdatatype_rrsig,
				 "test \"%s\": tuple %zu: "
				 "expected type RRSIG, found %s",
				 test->description, index,
				 found_type);
		if (found->rdata.type != dns_rdatatype_rrsig) {
			break;
		}
		/*
		 * The signature must cover an RRset of type 'expected->type'.
		 */
		result = dns_rdata_tostruct(&found->rdata, &rrsig, NULL);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		isc_buffer_init(&typebuf, found_covers, sizeof(found_covers));
		result = dns_rdatatype_totext(rrsig.covered, &typebuf);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		ATF_CHECK_EQ_MSG(expected_type, rrsig.covered,
				 "test \"%s\": tuple %zu: "
				 "expected RRSIG to cover %s, found covers %s",
				 test->description, index,
				 expected->type, found_covers);
		break;
	default:
		/*
		 * Found tuple must be of type 'expected->type'.
		 */
		ATF_CHECK_EQ_MSG(expected_type, found->rdata.type,
				 "test \"%s\": tuple %zu: "
				 "expected type %s, found %s",
				 test->description, index,
				 expected->type, found_type);
		break;
	}
}

/*%
 * Perform a single dns__zone_updatesigs() test defined in 'test'.  All other
 * arguments are expected to remain constant between subsequent invocations of
 * this function.
 */
static void
updatesigs_test(const updatesigs_test_params_t *test, dns_zone_t *zone,
		dns_db_t *db, dst_key_t *zone_keys[], unsigned int nkeys,
		isc_stdtime_t now)
{
	size_t tuples_expected, tuples_found, index;
	dns_dbversion_t *version = NULL;
	dns_diff_t raw_diff, zone_diff;
	const zonediff_t *expected;
	dns_difftuple_t *found;
	isc_result_t result;

	dns__zonediff_t zonediff = {
		.diff = &zone_diff,
		.offline = ISC_FALSE,
	};

	REQUIRE(test != NULL);
	REQUIRE(test->description != NULL);
	REQUIRE(test->changes != NULL);
	REQUIRE(zone != NULL);
	REQUIRE(db != NULL);
	REQUIRE(zone_keys != NULL);

	/*
	 * Create a new version of the zone's database.
	 */
	result = dns_db_newversion(db, &version);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Create a diff representing the supplied changes.
	 */
	result = dns_test_difffromchanges(&raw_diff, test->changes);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Apply the "raw" diff to the new version of the zone's database as
	 * this is what dns__zone_updatesigs() expects to happen before it is
	 * called.
	 */
	dns_diff_apply(&raw_diff, db, version);

	/*
	 * Initialize the structure dns__zone_updatesigs() will modify.
	 */
	dns_diff_init(mctx, &zone_diff);

	/*
	 * Check whether dns__zone_updatesigs() behaves as expected.
	 */
	result = dns__zone_updatesigs(&raw_diff, db, version, zone_keys, nkeys,
				      zone, now - 3600, now + 3600, now,
				      ISC_TRUE, ISC_FALSE, &zonediff);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "test \"%s\": expected success, got %s",
			 test->description, isc_result_totext(result));
	ATF_CHECK_MSG(ISC_LIST_EMPTY(raw_diff.tuples),
		      "test \"%s\": raw diff was not emptied",
		      test->description);
	ATF_CHECK_MSG(!ISC_LIST_EMPTY(zone_diff.tuples),
		      "test \"%s\": zone diff was not created",
		      test->description);

	/*
	 * Ensure that the number of tuples in the zone diff is as expected.
	 */

	tuples_expected = 0;
	for (expected = test->zonediff;
	     expected->owner != NULL;
	     expected++)
	{
		tuples_expected++;
	}

	tuples_found = 0;
	for (found = ISC_LIST_HEAD(zone_diff.tuples);
	     found != NULL;
	     found = ISC_LIST_NEXT(found, link))
	{
		tuples_found++;
	}

	ATF_REQUIRE_EQ_MSG(tuples_expected, tuples_found,
			   "test \"%s\": "
			   "expected %zu tuples in output, found %zu",
			   test->description,
			   tuples_expected, tuples_found);

	/*
	 * Ensure that every tuple in the zone diff matches expectations.
	 */
	expected = test->zonediff;
	index = 1;
	for (found = ISC_LIST_HEAD(zone_diff.tuples);
	     found != NULL;
	     found = ISC_LIST_NEXT(found, link))
	{
		compare_tuples(expected, found, test, index);
		expected++;
		index++;
	}

	/*
	 * Apply changes to zone database contents and clean up.
	 */
	dns_db_closeversion(db, &version, ISC_TRUE);
	dns_diff_clear(&zone_diff);
	dns_diff_clear(&raw_diff);
}

ATF_TC(updatesigs);
ATF_TC_HEAD(updatesigs, tc) {
	atf_tc_set_md_var(tc, "descr", "dns__zone_updatesigs() tests");
}
ATF_TC_BODY(updatesigs, tc) {
	dst_key_t *zone_keys[DNS_MAXZONEKEYS];
	dns_zone_t *zone = NULL;
	dns_db_t *db = NULL;
	isc_result_t result;
	unsigned int nkeys;
	isc_stdtime_t now;
	size_t i;

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Prepare a zone along with its signing keys.
	 */

	result = dns_test_makezone("example", &zone, NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_loaddb(&db, dns_dbtype_zone, "example",
				 "testdata/master/master18.data");
	ATF_REQUIRE_EQ(result, DNS_R_SEENINCLUDE);

	result = dns_zone_setkeydirectory(zone, "testkeys");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_stdtime_get(&now);
	result = dns__zone_findkeys(zone, db, NULL, now, mctx, DNS_MAXZONEKEYS,
				    zone_keys, &nkeys);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(nkeys, 2);

	/*
	 * Define the tests to be run.  Note that changes to zone database
	 * contents introduced by each test are preserved between tests.
	 */

	const zonechange_t changes_add[] = {
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT", "foo" },
		{ DNS_DIFFOP_ADD, "bar.example", 600, "TXT", "bar" },
		ZONECHANGE_SENTINEL,
	};
	const zonediff_t zonediff_add[] = {
		{ DNS_DIFFOP_ADDRESIGN, "foo.example", 300, "TXT" },
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT" },
		{ DNS_DIFFOP_ADDRESIGN, "bar.example", 600, "TXT" },
		{ DNS_DIFFOP_ADD, "bar.example", 600, "TXT" },
		ZONEDIFF_SENTINEL,
	};
	const updatesigs_test_params_t test_add = {
		.description = "add new RRsets",
		.changes = changes_add,
		.zonediff = zonediff_add,
	};

	const zonechange_t changes_append[] = {
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT", "foo1" },
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT", "foo2" },
		ZONECHANGE_SENTINEL,
	};
	const zonediff_t zonediff_append[] = {
		{ DNS_DIFFOP_DELRESIGN, "foo.example", 300, "TXT" },
		{ DNS_DIFFOP_ADDRESIGN, "foo.example", 300, "TXT" },
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT" },
		{ DNS_DIFFOP_ADD, "foo.example", 300, "TXT" },
		ZONEDIFF_SENTINEL,
	};
	const updatesigs_test_params_t test_append = {
		.description = "append multiple RRs to an existing RRset",
		.changes = changes_append,
		.zonediff = zonediff_append,
	};

	const zonechange_t changes_replace[] = {
		{ DNS_DIFFOP_DEL, "bar.example", 600, "TXT", "bar" },
		{ DNS_DIFFOP_ADD, "bar.example", 600, "TXT", "rab" },
		ZONECHANGE_SENTINEL,
	};
	const zonediff_t zonediff_replace[] = {
		{ DNS_DIFFOP_DELRESIGN, "bar.example", 600, "TXT" },
		{ DNS_DIFFOP_ADDRESIGN, "bar.example", 600, "TXT" },
		{ DNS_DIFFOP_DEL, "bar.example", 600, "TXT" },
		{ DNS_DIFFOP_ADD, "bar.example", 600, "TXT" },
		ZONEDIFF_SENTINEL,
	};
	const updatesigs_test_params_t test_replace = {
		.description = "replace an existing RRset",
		.changes = changes_replace,
		.zonediff = zonediff_replace,
	};

	const zonechange_t changes_delete[] = {
		{ DNS_DIFFOP_DEL, "bar.example", 600, "TXT", "rab" },
		ZONECHANGE_SENTINEL,
	};
	const zonediff_t zonediff_delete[] = {
		{ DNS_DIFFOP_DELRESIGN, "bar.example", 600, "TXT" },
		{ DNS_DIFFOP_DEL, "bar.example", 600, "TXT" },
		ZONEDIFF_SENTINEL,
	};
	const updatesigs_test_params_t test_delete = {
		.description = "delete an existing RRset",
		.changes = changes_delete,
		.zonediff = zonediff_delete,
	};

	const zonechange_t changes_mixed[] = {
		{ DNS_DIFFOP_ADD, "baz.example", 900, "TXT", "baz1" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "A", "127.0.0.1" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "TXT", "baz2" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "AAAA", "::1" },
		ZONECHANGE_SENTINEL,
	};
	const zonediff_t zonediff_mixed[] = {
		{ DNS_DIFFOP_ADDRESIGN, "baz.example", 900, "TXT" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "TXT" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "TXT" },
		{ DNS_DIFFOP_ADDRESIGN, "baz.example", 900, "A" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "A" },
		{ DNS_DIFFOP_ADDRESIGN, "baz.example", 900, "AAAA" },
		{ DNS_DIFFOP_ADD, "baz.example", 900, "AAAA" },
		ZONEDIFF_SENTINEL,
	};
	const updatesigs_test_params_t test_mixed = {
		.description = "add different RRsets with common owner name",
		.changes = changes_mixed,
		.zonediff = zonediff_mixed,
	};

	const updatesigs_test_params_t *tests[] = {
		&test_add,
		&test_append,
		&test_replace,
		&test_delete,
		&test_mixed,
	};

	/*
	 * Run tests.
	 */
	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		updatesigs_test(tests[i], zone, db, zone_keys, nkeys, now);
	}

	/*
	 * Clean up.
	 */
	for (i = 0; i < nkeys; i++) {
		dst_key_free(&zone_keys[i]);
	}
	dns_db_detach(&db);
	dns_zone_detach(&zone);

	dns_test_end();
}
#else
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping dns__zone_updatesigs() test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("DNSSEC support not compiled in");
}
#endif

ATF_TP_ADD_TCS(tp) {
#if defined(OPENSSL) || defined(PKCS11CRYPTO)
	ATF_TP_ADD_TC(tp, updatesigs);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}
