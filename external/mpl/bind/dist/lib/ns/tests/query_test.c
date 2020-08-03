/*	$NetBSD: query_test.c,v 1.7 2020/08/03 17:23:43 christos Exp $	*/

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

#include <isc/util.h>

#if HAVE_CMOCKA && !__SANITIZE_ADDRESS__

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <dns/badcache.h>
#include <dns/view.h>

#include <ns/client.h>
#include <ns/hooks.h>
#include <ns/query.h>

#include "nstest.h"

#if defined(USE_LIBTOOL) || LD_WRAP
static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = ns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	ns_test_end();

	return (0);
}

/*****
***** ns__query_sfcache() tests
*****/

/*%
 * Structure containing parameters for ns__query_sfcache_test().
 */
typedef struct {
	const ns_test_id_t id;	    /* libns test identifier */
	unsigned int qflags;	    /* query flags */
	bool cache_entry_present;   /* whether a SERVFAIL
				     * cache entry
				     * matching the query
				     * should be
				     * present */
	uint32_t cache_entry_flags; /* NS_FAILCACHE_* flags to
				     * set for
				     * the SERVFAIL cache entry
				     * */
	bool servfail_expected;	    /* whether a cached
				     * SERVFAIL is
				     * expected to be returned
				     * */
} ns__query_sfcache_test_params_t;

/*%
 * Perform a single ns__query_sfcache() check using given parameters.
 */
static void
run_sfcache_test(const ns__query_sfcache_test_params_t *test) {
	ns_hooktable_t *query_hooks = NULL;
	query_ctx_t *qctx = NULL;
	isc_result_t result;
	const ns_hook_t hook = {
		.action = ns_test_hook_catch_call,
	};

	REQUIRE(test != NULL);
	REQUIRE(test->id.description != NULL);
	REQUIRE(test->cache_entry_present || test->cache_entry_flags == 0);

	/*
	 * Interrupt execution if ns_query_done() is called.
	 */

	ns_hooktable_create(mctx, &query_hooks);
	ns_hook_add(query_hooks, mctx, NS_QUERY_DONE_BEGIN, &hook);
	ns__hook_table = query_hooks;

	/*
	 * Construct a query context for a ./NS query with given flags.
	 */
	{
		const ns_test_qctx_create_params_t qctx_params = {
			.qname = ".",
			.qtype = dns_rdatatype_ns,
			.qflags = test->qflags,
			.with_cache = true,
		};

		result = ns_test_qctx_create(&qctx_params, &qctx);
		assert_int_equal(result, ISC_R_SUCCESS);
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
		assert_int_equal(result, ISC_R_SUCCESS);

		dns_badcache_add(qctx->client->view->failcache, dns_rootname,
				 dns_rdatatype_ns, true,
				 test->cache_entry_flags, &expire);
	}

	/*
	 * Check whether ns__query_sfcache() behaves as expected.
	 */
	ns__query_sfcache(qctx);

	if (test->servfail_expected) {
		if (qctx->result != DNS_R_SERVFAIL) {
			fail_msg("# test \"%s\" on line %d: "
				 "expected SERVFAIL, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		}
	} else {
		if (qctx->result != ISC_R_SUCCESS) {
			fail_msg("# test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		}
	}

	/*
	 * Clean up.
	 */
	ns_test_qctx_destroy(&qctx);
	ns_hooktable_free(mctx, (void **)&query_hooks);
}

/* test ns__query_sfcache() */
static void
ns__query_sfcache_test(void **state) {
	size_t i;

	const ns__query_sfcache_test_params_t tests[] = {
		/*
		 * Sanity check for an empty SERVFAIL cache.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: empty"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = false,
			.servfail_expected = false,
		},
		/*
		 * Query: RD=1, CD=0.  Cache entry: CD=0.  Should SERVFAIL.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: CD=0"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = true,
			.cache_entry_flags = 0,
			.servfail_expected = true,
		},
		/*
		 * Query: RD=1, CD=1.  Cache entry: CD=0.  Should not SERVFAIL:
		 * failed validation should not influence CD=1 queries.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=1; cache: CD=0"),
			.qflags = DNS_MESSAGEFLAG_RD | DNS_MESSAGEFLAG_CD,
			.cache_entry_present = true,
			.cache_entry_flags = 0,
			.servfail_expected = false,
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
			.cache_entry_present = true,
			.cache_entry_flags = NS_FAILCACHE_CD,
			.servfail_expected = true,
		},
		/*
		 * Query: RD=1, CD=0.  Cache entry: CD=1.  Should SERVFAIL: if
		 * a CD=1 query elicited a SERVFAIL, a CD=0 query for the same
		 * QNAME and QTYPE will SERVFAIL as well.
		 */
		{
			NS_TEST_ID("query: RD=1, CD=0; cache: CD=1"),
			.qflags = DNS_MESSAGEFLAG_RD,
			.cache_entry_present = true,
			.cache_entry_flags = NS_FAILCACHE_CD,
			.servfail_expected = true,
		},
		/*
		 * Query: RD=0, CD=0.  Cache entry: CD=0.  Should not SERVFAIL
		 * despite a matching entry being present as the SERVFAIL cache
		 * should not be consulted for non-recursive queries.
		 */
		{
			NS_TEST_ID("query: RD=0, CD=0; cache: CD=0"),
			.qflags = 0,
			.cache_entry_present = true,
			.cache_entry_flags = 0,
			.servfail_expected = false,
		},
	};

	UNUSED(state);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		run_sfcache_test(&tests[i]);
	}
}

/*****
***** ns__query_start() tests
*****/

/*%
 * Structure containing parameters for ns__query_start_test().
 */
typedef struct {
	const ns_test_id_t id;	      /* libns test identifier */
	const char *qname;	      /* QNAME */
	dns_rdatatype_t qtype;	      /* QTYPE */
	unsigned int qflags;	      /* query flags */
	bool disable_name_checks;     /* if set to true, owner
				       * name
				       *          checks will
				       * be disabled for the
				       *          view created
				       * */
	bool recursive_service;	      /* if set to true, the view
				       *          created will
				       * have a cache
				       *          database
				       * attached */
	const char *auth_zone_origin; /* origin name of the zone
				       * the
				       * created view will be
				       * authoritative for */
	const char *auth_zone_path;   /* path to load the
				       * authoritative
				       * zone from */
	enum {			      /* expected result: */
	       NS__QUERY_START_R_INVALID,
	       NS__QUERY_START_R_REFUSE, /* query should be REFUSED */
	       NS__QUERY_START_R_CACHE,	 /* query should be answered from
					  * cache */
	       NS__QUERY_START_R_AUTH,	 /* query should be answered using
					  * authoritative data */
	} expected_result;
} ns__query_start_test_params_t;

/*%
 * Perform a single ns__query_start() check using given parameters.
 */
static void
run_start_test(const ns__query_start_test_params_t *test) {
	ns_hooktable_t *query_hooks = NULL;
	query_ctx_t *qctx = NULL;
	isc_result_t result;
	const ns_hook_t hook = {
		.action = ns_test_hook_catch_call,
	};

	REQUIRE(test != NULL);
	REQUIRE(test->id.description != NULL);
	REQUIRE((test->auth_zone_origin == NULL &&
		 test->auth_zone_path == NULL) ||
		(test->auth_zone_origin != NULL &&
		 test->auth_zone_path != NULL));

	/*
	 * Interrupt execution if query_lookup() or ns_query_done() is called.
	 */
	ns_hooktable_create(mctx, &query_hooks);
	ns_hook_add(query_hooks, mctx, NS_QUERY_LOOKUP_BEGIN, &hook);
	ns_hook_add(query_hooks, mctx, NS_QUERY_DONE_BEGIN, &hook);
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
		assert_int_equal(result, ISC_R_SUCCESS);
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
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	/*
	 * Check whether ns__query_start() behaves as expected.
	 */
	ns__query_start(qctx);

	switch (test->expected_result) {
	case NS__QUERY_START_R_REFUSE:
		if (qctx->result != DNS_R_REFUSED) {
			fail_msg("# test \"%s\" on line %d: "
				 "expected REFUSED, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		}
		if (qctx->zone != NULL) {
			fail_msg("# test \"%s\" on line %d: "
				 "no zone was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		}
		if (qctx->db != NULL) {
			fail_msg("# test \"%s\" on line %d: "
				 "no database was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		}
		break;
	case NS__QUERY_START_R_CACHE:
		if (qctx->result != ISC_R_SUCCESS) {
			fail_msg("# test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		}
		if (qctx->zone != NULL) {
			fail_msg("# test \"%s\" on line %d: "
				 "no zone was expected to be attached to "
				 "query context, but some was",
				 test->id.description, test->id.lineno);
		}
		if (qctx->db == NULL || qctx->db != qctx->client->view->cachedb)
		{
			fail_msg("# test \"%s\" on line %d: "
				 "cache database was expected to be "
				 "attached to query context, but it was not",
				 test->id.description, test->id.lineno);
		}
		break;
	case NS__QUERY_START_R_AUTH:
		if (qctx->result != ISC_R_SUCCESS) {
			fail_msg("# test \"%s\" on line %d: "
				 "expected success, got %s",
				 test->id.description, test->id.lineno,
				 isc_result_totext(qctx->result));
		}
		if (qctx->zone == NULL) {
			fail_msg("# test \"%s\" on line %d: "
				 "a zone was expected to be attached to query "
				 "context, but it was not",
				 test->id.description, test->id.lineno);
		}
		if (qctx->db == qctx->client->view->cachedb) {
			fail_msg("# test \"%s\" on line %d: "
				 "cache database was not expected to be "
				 "attached to query context, but it is",
				 test->id.description, test->id.lineno);
		}
		break;
	case NS__QUERY_START_R_INVALID:
		fail_msg("# test \"%s\" on line %d has no expected result set",
			 test->id.description, test->id.lineno);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	/*
	 * Clean up.
	 */
	if (test->auth_zone_path != NULL) {
		ns_test_cleanup_zone();
	}
	ns_test_qctx_destroy(&qctx);
	ns_hooktable_free(mctx, (void **)&query_hooks);
}

/* test ns__query_start() */
static void
ns__query_start_test(void **state) {
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
			.recursive_service = false,
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
			.recursive_service = true,
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
			.recursive_service = true,
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
			.recursive_service = false,
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
			.recursive_service = true,
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
			.recursive_service = true,
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
			.recursive_service = true,
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
			.recursive_service = true,
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
			.recursive_service = true,
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
			.disable_name_checks = true,
			.recursive_service = true,
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
			.disable_name_checks = false,
			.recursive_service = true,
			.expected_result = NS__QUERY_START_R_REFUSE,
		},
	};

	UNUSED(state);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		run_start_test(&tests[i]);
	}
}
#endif /* if defined(USE_LIBTOOL) || LD_WRAP */

int
main(void) {
#if defined(USE_LIBTOOL) || LD_WRAP
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(ns__query_sfcache_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(ns__query_start_test, _setup,
						_teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else  /* if defined(USE_LIBTOOL) || LD_WRAP */
	print_message("1..0 # Skip query_test requires libtool or LD_WRAP\n");
#endif /* if defined(USE_LIBTOOL) || LD_WRAP */
}

#else /* HAVE_CMOCKA && !__SANITIZE_ADDRESS__ */

#include <stdio.h>

int
main(void) {
#if __SANITIZE_ADDRESS__
	/*
	 * We disable this test when the address sanitizer is in
	 * the use, as libuv will trigger errors.
	 */
	printf("1..0 # Skip ASAN is in use\n");
#else  /* ADDRESS_SANIZITER */
	printf("1..0 # Skip cmocka not available\n");
#endif /* __SANITIZE_ADDRESS__ */
	return (0);
}

#endif /* HAVE_CMOCKA && !__SANITIZE_ADDRESS__ */
