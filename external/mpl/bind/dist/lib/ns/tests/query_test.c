/*	$NetBSD: query_test.c,v 1.2 2018/08/12 13:02:41 christos Exp $	*/

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

#include <dns/badcache.h>
#include <dns/view.h>
#include <isc/util.h>
#include <ns/client.h>
#include <ns/query.h>

#include "../hooks.h"

#include "nstest.h"

/*****
 ***** ns__query_sfcache() tests
 *****/

/*%
 * Structure containing parameters for ns__query_sfcache_test().
 */
typedef struct {
	const ns_test_id_t id;		   /* libns test identifier */
	unsigned int qflags;		   /* query flags */
	isc_boolean_t cache_entry_present; /* whether a SERVFAIL cache entry
					      matching the query should be
					      present */
	isc_uint32_t cache_entry_flags;	   /* NS_FAILCACHE_* flags to set for
					      the SERVFAIL cache entry */
	isc_boolean_t servfail_expected;   /* whether a cached SERVFAIL is
					      expected to be returned */
} ns__query_sfcache_test_params_t;

/*%
 * Perform a single ns__query_sfcache() check using given parameters.
 */
static void
ns__query_sfcache_test(const ns__query_sfcache_test_params_t *test) {
	query_ctx_t *qctx = NULL;
	isc_result_t result;

	REQUIRE(test != NULL);
	REQUIRE(test->id.description != NULL);
	REQUIRE(test->cache_entry_present == ISC_TRUE ||
		test->cache_entry_flags == 0);

	/*
	 * Interrupt execution if query_done() is called.
	 */
	ns_hook_t query_hooks[NS_QUERY_HOOKS_COUNT + 1] = {
		[NS_QUERY_DONE_BEGIN] = {
			.callback = ns_test_hook_catch_call,
			.callback_data = NULL,
		},
	};
	ns__hook_table = query_hooks;

	/*
	 * Construct a query context for a ./NS query with given flags.
	 */
	{
		const ns_test_qctx_create_params_t qctx_params = {
			.qname = ".",
			.qtype = dns_rdatatype_ns,
			.qflags = test->qflags,
			.with_cache = ISC_TRUE,
		};

		result = ns_test_qctx_create(&qctx_params, &qctx);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	/*
	 * If this test wants a SERVFAIL cache entry matching the query to
	 * exist, create it.
	 */
	if (test->cache_entry_present) {
		isc_interval_t hour;
		isc_time_t expire;

		isc_interval_set(&hour, 3600, 0);
		result = isc_time_nowplusinterval(&expire, &hour);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		dns_badcache_add(qctx->client->view->failcache, dns_rootname,
				 dns_rdatatype_ns, ISC_TRUE,
				 test->cache_entry_flags, &expire);
	}

	/*
	 * Check whether ns__query_sfcache() behaves as expected.
	 */
	ns__query_sfcache(qctx);

	if (test->servfail_expected) {
		ATF_CHECK_EQ_MSG(qctx->result, DNS_R_SERVFAIL,
				 "test \"%s\" on line %d: "
				 "expected SERVFAIL, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
	} else {
		ATF_CHECK_EQ_MSG(qctx->result, ISC_R_SUCCESS,
				 "test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
	}

	/*
	 * Clean up.
	 */
	ns_test_qctx_destroy(&qctx);

}

ATF_TC(ns__query_sfcache);
ATF_TC_HEAD(ns__query_sfcache, tc) {
	atf_tc_set_md_var(tc, "descr", "ns__query_sfcache()");
}
ATF_TC_BODY(ns__query_sfcache, tc) {
	isc_result_t result;
	size_t i;

	const ns__query_sfcache_test_params_t tests[] = {
		/*
		 * Sanity check for an empty SERVFAIL cache.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: empty"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = ISC_FALSE,
			.servfail_expected = ISC_FALSE,
		},
		/*
		 * Query: RD=1, CD=0.  Cache entry: CD=0.  Should SERVFAIL.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: CD=0"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = ISC_TRUE,
			.cache_entry_flags = 0,
			.servfail_expected = ISC_TRUE,
		},
		/*
		 * Query: RD=1, CD=1.  Cache entry: CD=0.  Should not SERVFAIL:
		 * failed validation should not influence CD=1 queries.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=1; cache: CD=0"),
			.qflags = DNS_MESSAGEFLAG_RD | DNS_MESSAGEFLAG_CD,
			.cache_entry_present = ISC_TRUE,
			.cache_entry_flags = 0,
			.servfail_expected = ISC_FALSE,
		},
		/*
		 * Query: RD=1, CD=1.  Cache entry: CD=1.  Should SERVFAIL:
		 * SERVFAIL responses elicited by CD=1 queries can be
		 * "replayed" for other CD=1 queries during the lifetime of the
		 * SERVFAIL cache entry.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=1; cache: CD=1"),
			.qflags = DNS_MESSAGEFLAG_RD | DNS_MESSAGEFLAG_CD,
			.cache_entry_present = ISC_TRUE,
			.cache_entry_flags = NS_FAILCACHE_CD,
			.servfail_expected = ISC_TRUE,
		},
		/*
		 * Query: RD=1, CD=0.  Cache entry: CD=1.  Should SERVFAIL: if
		 * a CD=1 query elicited a SERVFAIL, a CD=0 query for the same
		 * QNAME and QTYPE will SERVFAIL as well.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: CD=1"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = ISC_TRUE,
			.cache_entry_flags = NS_FAILCACHE_CD,
			.servfail_expected = ISC_TRUE,
		},
		/*
		 * Query: RD=0, CD=0.  Cache entry: CD=0.  Should not SERVFAIL
		 * despite a matching entry being present as the SERVFAIL cache
		 * should not be consulted for non-recursive queries.
		 */
		{
			NS_TEST_ID("query: RD=0, CD=0; cache: CD=0"),
			.qflags = 0,
			.cache_entry_present = ISC_TRUE,
			.cache_entry_flags = 0,
			.servfail_expected = ISC_FALSE,
		},
	};

	UNUSED(tc);

	result = ns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		ns__query_sfcache_test(&tests[i]);
	}

	ns_test_end();
}

/*****
 ***** ns__query_start() tests
 *****/

/*%
 * Structure containing parameters for ns__query_start_test().
 */
typedef struct {
	const ns_test_id_t id;		   /* libns test identifier */
	const char *qname;		   /* QNAME */
	dns_rdatatype_t qtype;		   /* QTYPE */
	unsigned int qflags;		   /* query flags */
	isc_boolean_t disable_name_checks; /* if set to ISC_TRUE, owner name
					      checks will be disabled for the
					      view created */
	isc_boolean_t recursive_service;   /* if set to ISC_TRUE, the view
					      created will have a cache
					      database attached */
	const char *auth_zone_origin;	   /* origin name of the zone the
					      created view will be
					      authoritative for */
	const char *auth_zone_path;	   /* path to load the authoritative
					      zone from */
	enum {				   /* expected result: */
		NS__QUERY_START_R_INVALID,
		NS__QUERY_START_R_REFUSE,  /* query should be REFUSED */
		NS__QUERY_START_R_CACHE,   /* query should be answered from
					      cache */
		NS__QUERY_START_R_AUTH,	   /* query should be answered using
					      authoritative data */
	} expected_result;
} ns__query_start_test_params_t;

/*%
 * Perform a single ns__query_start() check using given parameters.
 */
static void
ns__query_start_test(const ns__query_start_test_params_t *test) {
	query_ctx_t *qctx = NULL;
	isc_result_t result;

	REQUIRE(test != NULL);
	REQUIRE(test->id.description != NULL);
	REQUIRE((test->auth_zone_origin == NULL &&
		 test->auth_zone_path == NULL) ||
		(test->auth_zone_origin != NULL &&
		 test->auth_zone_path != NULL));

	/*
	 * Interrupt execution if query_lookup() or query_done() is called.
	 */
	ns_hook_t query_hooks[NS_QUERY_HOOKS_COUNT + 1] = {
		[NS_QUERY_LOOKUP_BEGIN] = {
			.callback = ns_test_hook_catch_call,
			.callback_data = NULL,
		},
		[NS_QUERY_DONE_BEGIN] = {
			.callback = ns_test_hook_catch_call,
			.callback_data = NULL,
		},
	};
	ns__hook_table = query_hooks;

	/*
	 * Construct a query context using the supplied parameters.
	 */
	{
		const ns_test_qctx_create_params_t qctx_params = {
			.qname = test->qname,
			.qtype = test->qtype,
			.qflags = test->qflags,
			.with_cache = test->recursive_service,
		};
		result = ns_test_qctx_create(&qctx_params, &qctx);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	/*
	 * Enable view->checknames by default, disable if requested.
	 */
	qctx->client->view->checknames = !test->disable_name_checks;

	/*
	 * Load zone from file and attach it to the client's view, if
	 * requested.
	 */
	if (test->auth_zone_path != NULL) {
		result = ns_test_serve_zone(test->auth_zone_origin,
					    test->auth_zone_path,
					    qctx->client->view);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	/*
	 * Check whether ns__query_start() behaves as expected.
	 */
	ns__query_start(qctx);

	switch (test->expected_result) {
	case NS__QUERY_START_R_REFUSE:
		ATF_CHECK_EQ_MSG(qctx->result, DNS_R_REFUSED,
				 "test \"%s\" on line %d: "
				 "expected REFUSED, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		ATF_CHECK_EQ_MSG(qctx->zone, NULL,
				 "test \"%s\" on line %d: "
				 "no zone was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		ATF_CHECK_EQ_MSG(qctx->db, NULL,
				 "test \"%s\" on line %d: "
				 "no database was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		break;
	case NS__QUERY_START_R_CACHE:
		ATF_CHECK_EQ_MSG(qctx->result, ISC_R_SUCCESS,
				 "test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		ATF_CHECK_EQ_MSG(qctx->zone, NULL,
				 "test \"%s\" on line %d: "
				 "no zone was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		ATF_CHECK_MSG((qctx->db != NULL &&
			       qctx->db == qctx->client->view->cachedb),
			      "test \"%s\" on line %d: "
			      "cache database was expected to be attached to "
			      "query context, but it was not",
			      test->id.description, test->id.lineno);
		break;
	case NS__QUERY_START_R_AUTH:
		ATF_CHECK_EQ_MSG(qctx->result, ISC_R_SUCCESS,
				 "test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		ATF_CHECK_MSG(qctx->zone != NULL,
			      "test \"%s\" on line %d: "
			      "a zone was expected to be attached to query "
			      "context, but it was not",
			      test->id.description, test->id.lineno);
		ATF_CHECK_MSG((qctx->db != NULL &&
			       qctx->db != qctx->client->view->cachedb),
			      "test \"%s\" on line %d: "
			      "cache database was not expected to be attached "
			      "to query context, but it is",
			      test->id.description, test->id.lineno);
		break;
	case NS__QUERY_START_R_INVALID:
		ATF_REQUIRE_MSG(ISC_FALSE,
				"test \"%s\" on line %d has no expected "
				"result set",
				test->id.description, test->id.lineno);
		break;
	default:
		INSIST(0);
		break;
	}

	/*
	 * Clean up.
	 */
	if (test->auth_zone_path != NULL) {
		ns_test_cleanup_zone();
	}
	ns_test_qctx_destroy(&qctx);
}

ATF_TC(ns__query_start);
ATF_TC_HEAD(ns__query_start, tc) {
	atf_tc_set_md_var(tc, "descr", "ns__query_start()");
}
ATF_TC_BODY(ns__query_start, tc) {
	isc_result_t result;
	size_t i;

	const ns__query_start_test_params_t tests[] = {
		/*
		 * Recursive foo/A query to a server without recursive service
		 * and no zones configured.  Query should be REFUSED.
		 */
		{
			NS_TEST_ID("foo/A, no cache, no auth"),
			.qname = "foo",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_FALSE,
			.expected_result = NS__QUERY_START_R_REFUSE,
		},
		/*
		 * Recursive foo/A query to a server with recursive service and
		 * no zones configured.  Query should be answered from cache.
		 */
		{
			NS_TEST_ID("foo/A, cache, no auth"),
			.qname = "foo",
			.qtype = dns_rdatatype_a,
			.recursive_service = ISC_TRUE,
			.expected_result = NS__QUERY_START_R_CACHE,
		},
		/*
		 * Recursive foo/A query to a server with recursive service and
		 * zone "foo" configured.  Query should be answered from
		 * authoritative data.
		 */
		{
			NS_TEST_ID("foo/A, RD=1, cache, auth for foo"),
			.qname = "foo",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_AUTH,
		},
		/*
		 * Recursive bar/A query to a server without recursive service
		 * and zone "foo" configured.  Query should be REFUSED.
		 */
		{
			NS_TEST_ID("bar/A, RD=1, no cache, auth for foo"),
			.qname = "bar",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_FALSE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_REFUSE,
		},
		/*
		 * Recursive bar/A query to a server with recursive service and
		 * zone "foo" configured.  Query should be answered from
		 * cache.
		 */
		{
			NS_TEST_ID("bar/A, RD=1, cache, auth for foo"),
			.qname = "bar",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_CACHE,
		},
		/*
		 * Recursive bar.foo/DS query to a server with recursive
		 * service and zone "foo" configured.  Query should be answered
		 * from authoritative data.
		 */
		{
			NS_TEST_ID("bar.foo/DS, RD=1, cache, auth for foo"),
			.qname = "bar.foo",
			.qtype = dns_rdatatype_ds,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_AUTH,
		},
		/*
		 * Non-recursive bar.foo/DS query to a server with recursive
		 * service and zone "foo" configured.  Query should be answered
		 * from authoritative data.
		 */
		{
			NS_TEST_ID("bar.foo/DS, RD=0, cache, auth for foo"),
			.qname = "bar.foo",
			.qtype = dns_rdatatype_ds,
			.qflags = 0,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_AUTH,
		},
		/*
		 * Recursive foo/DS query to a server with recursive service
		 * and zone "foo" configured.  Query should be answered from
		 * cache.
		 */
		{
			NS_TEST_ID("foo/DS, RD=1, cache, auth for foo"),
			.qname = "foo",
			.qtype = dns_rdatatype_ds,
			.qflags = DNS_MESSAGEFLAG_RD,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_CACHE,
		},
		/*
		 * Non-recursive foo/DS query to a server with recursive
		 * service and zone "foo" configured.  Query should be answered
		 * from authoritative data.
		 */
		{
			NS_TEST_ID("foo/DS, RD=0, cache, auth for foo"),
			.qname = "foo",
			.qtype = dns_rdatatype_ds,
			.qflags = 0,
			.recursive_service = ISC_TRUE,
			.auth_zone_origin = "foo",
			.auth_zone_path = "testdata/query/foo.db",
			.expected_result = NS__QUERY_START_R_AUTH,
		},
		/*
		 * Recursive _foo/A query to a server with recursive service,
		 * no zones configured and owner name checks disabled.  Query
		 * should be answered from cache.
		 */
		{
			NS_TEST_ID("_foo/A, cache, no auth, name checks off"),
			.qname = "_foo",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.disable_name_checks = ISC_TRUE,
			.recursive_service = ISC_TRUE,
			.expected_result = NS__QUERY_START_R_CACHE,
		},
		/*
		 * Recursive _foo/A query to a server with recursive service,
		 * no zones configured and owner name checks enabled.  Query
		 * should be REFUSED.
		 */
		{
			NS_TEST_ID("_foo/A, cache, no auth, name checks on"),
			.qname = "_foo",
			.qtype = dns_rdatatype_a,
			.qflags = DNS_MESSAGEFLAG_RD,
			.disable_name_checks = ISC_FALSE,
			.recursive_service = ISC_TRUE,
			.expected_result = NS__QUERY_START_R_REFUSE,
		},
	};

	UNUSED(tc);

	result = ns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		ns__query_start_test(&tests[i]);
	}

	ns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, ns__query_sfcache);
	ATF_TP_ADD_TC(tp, ns__query_start);

	return (atf_no_error());
}
