/*	$NetBSD: query_test.c,v 1.2.2.2 2024/02/25 15:47:54 martin Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/quota.h>
#include <isc/util.h>

#include <dns/badcache.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <ns/client.h>
#include <ns/events.h>
#include <ns/hooks.h>
#include <ns/query.h>
#include <ns/server.h>
#include <ns/stats.h>

#include <tests/ns.h>

static int
setup_test(void **state) {
	isc__nm_force_tid(0);
	setup_server(state);
	return (0);
}

static int
teardown_test(void **state) {
	isc__nm_force_tid(-1);
	teardown_server(state);
	return (0);
}

/* can be used for client->sendcb to avoid disruption on sending a response */
static void
send_noop(isc_buffer_t *buffer) {
	UNUSED(buffer);
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
ISC_RUN_TEST_IMPL(ns_query_sfcache) {
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
		UNREACHABLE();
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
ISC_RUN_TEST_IMPL(ns_query_start) {
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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
			.auth_zone_path = TESTS_DIR "/testdata/query/foo.db",
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

/*****
***** tests for ns_query_hookasync().
*****/

/*%
 * Structure containing parameters for ns__query_hookasync_test().
 */
typedef struct {
	const ns_test_id_t id;	   /* libns test identifier */
	ns_hookpoint_t hookpoint;  /* hook point specified for resume */
	ns_hookpoint_t hookpoint2; /* expected hook point used after resume */
	ns_hook_action_t action;   /* action for the hook point */
	isc_result_t start_result; /* result of 'runasync' */
	bool quota_ok;		   /* true if recursion quota should be okay */
	bool do_cancel;		   /* true if query should be canceled
				    * in test */
} ns__query_hookasync_test_params_t;

/* Data structure passed from tests to hooks */
typedef struct hookasync_data {
	bool async;		      /* true if in a hook-triggered
				       * asynchronous process */
	bool canceled;		      /* true if the query has been canceled  */
	isc_result_t start_result;    /* result of 'runasync' */
	ns_hook_resevent_t *rev;      /* resume event sent on completion */
	query_ctx_t qctx;	      /* shallow copy of qctx passed to hook */
	ns_hookpoint_t hookpoint;     /* specifies where to resume */
	ns_hookpoint_t lasthookpoint; /* remember the last hook point called */
} hookasync_data_t;

/*
 * 'destroy' callback of hook recursion ctx.
 * The dynamically allocated context will be freed here, thereby proving
 * this is actually called; otherwise tests would fail due to memory leak.
 */
static void
destroy_hookactx(ns_hookasync_t **ctxp) {
	ns_hookasync_t *ctx = *ctxp;

	*ctxp = NULL;
	isc_mem_putanddetach(&ctx->mctx, ctx, sizeof(*ctx));
}

/* 'cancel' callback of hook recursion ctx. */
static void
cancel_hookactx(ns_hookasync_t *ctx) {
	/* Mark the hook data so the test can confirm this is called. */
	((hookasync_data_t *)ctx->private)->canceled = true;
}

/* 'runasync' callback passed to ns_query_hookasync */
static isc_result_t
test_hookasync(query_ctx_t *qctx, isc_mem_t *memctx, void *arg,
	       isc_task_t *task, isc_taskaction_t action, void *evarg,
	       ns_hookasync_t **ctxp) {
	hookasync_data_t *asdata = arg;
	ns_hookasync_t *ctx = NULL;
	ns_hook_resevent_t *rev = NULL;

	if (asdata->start_result != ISC_R_SUCCESS) {
		return (asdata->start_result);
	}

	ctx = isc_mem_get(memctx, sizeof(*ctx));
	rev = (ns_hook_resevent_t *)isc_event_allocate(
		memctx, task, NS_EVENT_HOOKASYNCDONE, action, evarg,
		sizeof(*rev));

	rev->hookpoint = asdata->hookpoint;
	rev->origresult = DNS_R_NXDOMAIN;
	rev->saved_qctx = qctx;
	rev->ctx = ctx;
	asdata->rev = rev;

	*ctx = (ns_hookasync_t){ .private = asdata };
	isc_mem_attach(memctx, &ctx->mctx);
	ctx->destroy = destroy_hookactx;
	ctx->cancel = cancel_hookactx;

	*ctxp = ctx;
	return (ISC_R_SUCCESS);
}

/*
 * Main logic for hook actions.
 * 'hookpoint' should identify the point that calls the hook.  It will be
 * remembered in the hook data, so that the test can confirm which hook point
 * was last used.
 */
static ns_hookresult_t
hook_async_common(void *arg, void *data, isc_result_t *resultp,
		  ns_hookpoint_t hookpoint) {
	query_ctx_t *qctx = arg;
	hookasync_data_t *asdata = data;
	isc_result_t result;

	asdata->qctx = *qctx; /* remember passed ctx for inspection */
	asdata->lasthookpoint = hookpoint; /* ditto */

	if (!asdata->async) {
		/* Initial call to the hook; start recursion */
		result = ns_query_hookasync(qctx, test_hookasync, asdata);
		if (result == ISC_R_SUCCESS) {
			asdata->async = true;
		}
	} else {
		/*
		 * Resume from the completion of async event.
		 * fetchhandle should have been detached so that we can start
		 * another async event or DNS recursive resolution.
		 */
		INSIST(qctx->client->fetchhandle == NULL);
		asdata->async = false;
		switch (hookpoint) {
		case NS_QUERY_GOT_ANSWER_BEGIN:
		case NS_QUERY_NODATA_BEGIN:
		case NS_QUERY_NXDOMAIN_BEGIN:
		case NS_QUERY_NCACHE_BEGIN:
			INSIST(*resultp == DNS_R_NXDOMAIN);
			break;
		default:;
		}
	}

	*resultp = ISC_R_UNSET;
	return (NS_HOOK_RETURN);
}

static ns_hookresult_t
hook_async_query_setup(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_SETUP));
}

static ns_hookresult_t
hook_async_query_start_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_START_BEGIN));
}

static ns_hookresult_t
hook_async_query_lookup_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_LOOKUP_BEGIN));
}

static ns_hookresult_t
hook_async_query_resume_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_RESUME_BEGIN));
}

static ns_hookresult_t
hook_async_query_got_answer_begin(void *arg, void *data,
				  isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_GOT_ANSWER_BEGIN));
}

static ns_hookresult_t
hook_async_query_respond_any_begin(void *arg, void *data,
				   isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_RESPOND_ANY_BEGIN));
}

static ns_hookresult_t
hook_async_query_addanswer_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_ADDANSWER_BEGIN));
}

static ns_hookresult_t
hook_async_query_notfound_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_NOTFOUND_BEGIN));
}

static ns_hookresult_t
hook_async_query_prep_delegation_begin(void *arg, void *data,
				       isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_PREP_DELEGATION_BEGIN));
}

static ns_hookresult_t
hook_async_query_zone_delegation_begin(void *arg, void *data,
				       isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_ZONE_DELEGATION_BEGIN));
}

static ns_hookresult_t
hook_async_query_delegation_begin(void *arg, void *data,
				  isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_DELEGATION_BEGIN));
}

static ns_hookresult_t
hook_async_query_delegation_recurse_begin(void *arg, void *data,
					  isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_DELEGATION_RECURSE_BEGIN));
}

static ns_hookresult_t
hook_async_query_nodata_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_NODATA_BEGIN));
}

static ns_hookresult_t
hook_async_query_nxdomain_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_NXDOMAIN_BEGIN));
}

static ns_hookresult_t
hook_async_query_ncache_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_NCACHE_BEGIN));
}

static ns_hookresult_t
hook_async_query_cname_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_CNAME_BEGIN));
}

static ns_hookresult_t
hook_async_query_dname_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_DNAME_BEGIN));
}

static ns_hookresult_t
hook_async_query_respond_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_RESPOND_BEGIN));
}

static ns_hookresult_t
hook_async_query_response_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp,
				  NS_QUERY_PREP_RESPONSE_BEGIN));
}

static ns_hookresult_t
hook_async_query_done_begin(void *arg, void *data, isc_result_t *resultp) {
	return (hook_async_common(arg, data, resultp, NS_QUERY_DONE_BEGIN));
}

/*
 * hook on destroying actx.  Can't be used for async event, but we use this
 * to remember the qctx at that point.
 */
static ns_hookresult_t
ns_test_qctx_destroy_hook(void *arg, void *data, isc_result_t *resultp) {
	query_ctx_t *qctx = arg;
	hookasync_data_t *asdata = data;

	asdata->qctx = *qctx; /* remember passed ctx for inspection */
	*resultp = ISC_R_UNSET;
	return (NS_HOOK_CONTINUE);
}

static void
run_hookasync_test(const ns__query_hookasync_test_params_t *test) {
	query_ctx_t *qctx = NULL;
	isc_result_t result;
	hookasync_data_t asdata = {
		.async = false,
		.canceled = false,
		.start_result = test->start_result,
		.hookpoint = test->hookpoint,
	};
	const ns_hook_t testhook = {
		.action = test->action,
		.action_data = &asdata,
	};
	const ns_hook_t destroyhook = {
		.action = ns_test_qctx_destroy_hook,
		.action_data = &asdata,
	};
	isc_quota_t *quota = NULL;
	isc_statscounter_t srvfail_cnt;
	bool expect_servfail = false;

	/*
	 * Prepare hooks.  We always begin with ns__query_start for simplicity.
	 * Its action will specify various different resume points (unusual
	 * in practice, but that's fine for the testing purpose).
	 */
	ns__hook_table = NULL;
	ns_hooktable_create(mctx, &ns__hook_table);
	ns_hook_add(ns__hook_table, mctx, NS_QUERY_START_BEGIN, &testhook);
	if (test->hookpoint2 != NS_QUERY_START_BEGIN) {
		/*
		 * unless testing START_BEGIN itself, specify the hook for the
		 * expected resume point, too.
		 */
		ns_hook_add(ns__hook_table, mctx, test->hookpoint2, &testhook);
	}
	ns_hook_add(ns__hook_table, mctx, NS_QUERY_QCTX_DESTROYED,
		    &destroyhook);

	{
		const ns_test_qctx_create_params_t qctx_params = {
			.qname = "test.example.com",
			.qtype = dns_rdatatype_aaaa,
		};
		result = ns_test_qctx_create(&qctx_params, &qctx);
		INSIST(result == ISC_R_SUCCESS);
		qctx->client->sendcb = send_noop;
	}

	/*
	 * Set recursion quota to the lowest possible value, then make it full
	 * if we want to exercise a quota failure case.
	 */
	isc_quota_max(&sctx->recursionquota, 1);
	if (!test->quota_ok) {
		result = isc_quota_attach(&sctx->recursionquota, &quota);
		INSIST(result == ISC_R_SUCCESS);
	}

	/* Remember SERVFAIL counter */
	srvfail_cnt = ns_stats_get_counter(qctx->client->sctx->nsstats,
					   ns_statscounter_servfail);

	/*
	 * If the query has been canceled, or async event didn't succeed,
	 * SERVFAIL will have to be sent.  In this case we need to have
	 * 'reqhandle' attach to the client's handle as it's detached in
	 * query_error.
	 */
	if (test->start_result != ISC_R_SUCCESS || !test->quota_ok ||
	    test->do_cancel)
	{
		expect_servfail = true;
		isc_nmhandle_attach(qctx->client->handle,
				    &qctx->client->reqhandle);
	}

	/*
	 * Emulate query handling from query_start.
	 * Specified hook should be called.
	 */
	qctx->client->state = NS_CLIENTSTATE_WORKING;
	result = ns__query_start(qctx);
	INSIST(result == ISC_R_UNSET);

	/*
	 * hook-triggered async event should be happening unless it hits
	 * recursion quota limit or 'runasync' callback fails.
	 */
	INSIST(asdata.async ==
	       (test->quota_ok && test->start_result == ISC_R_SUCCESS));

	/*
	 * Emulate cancel if so specified.
	 * The cancel callback should be called.
	 */
	if (test->do_cancel) {
		ns_query_cancel(qctx->client);
	}
	INSIST(asdata.canceled == test->do_cancel);

	/* If async event has started, manually invoke the 'done' event. */
	if (asdata.async) {
		qctx->client->now = 0; /* set to sentinel before resume */
		asdata.rev->ev_action(asdata.rev->ev_sender,
				      (isc_event_t *)asdata.rev);

		/* Confirm necessary cleanup has been performed. */
		INSIST(qctx->client->query.hookactx == NULL);
		INSIST(qctx->client->state == NS_CLIENTSTATE_WORKING);
		INSIST(qctx->client->recursionquota == NULL);
		INSIST(ns_stats_get_counter(qctx->client->sctx->nsstats,
					    ns_statscounter_recursclients) ==
		       0);
		INSIST(!ISC_LINK_LINKED(qctx->client, rlink));
		if (!test->do_cancel) {
			/*
			 * In the normal case the client's timestamp is updated
			 * and the query handling has been resumed from the
			 * expected point.
			 */
			INSIST(qctx->client->now != 0);
			INSIST(asdata.lasthookpoint == test->hookpoint2);
		}
	} else {
		INSIST(qctx->client->query.hookactx == NULL);
	}

	/*
	 * Confirm SERVFAIL has been sent if it was expected.
	 * Also, the last-generated qctx should have detach_client being true.
	 */
	if (expect_servfail) {
		INSIST(ns_stats_get_counter(qctx->client->sctx->nsstats,
					    ns_statscounter_servfail) ==
		       srvfail_cnt + 1);
		if (test->do_cancel) {
			/* qctx was created on resume and copied in hook */
			INSIST(asdata.qctx.detach_client);
		} else {
			INSIST(qctx->detach_client);
		}
	}

	/*
	 * Cleanup.  Note that we've kept 'qctx' until now; otherwise
	 * qctx->client may have been invalidated while we still need it.
	 */
	ns_test_qctx_destroy(&qctx);
	ns_hooktable_free(mctx, (void **)&ns__hook_table);
	if (quota != NULL) {
		isc_quota_detach(&quota);
	}
}

ISC_RUN_TEST_IMPL(ns_query_hookasync) {
	size_t i;

	UNUSED(state);

	const ns__query_hookasync_test_params_t tests[] = {
		{
			NS_TEST_ID("normal case"),
			NS_QUERY_START_BEGIN,
			NS_QUERY_START_BEGIN,
			hook_async_query_start_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("quota fail"),
			NS_QUERY_START_BEGIN,
			NS_QUERY_START_BEGIN,
			hook_async_query_start_begin,
			ISC_R_SUCCESS,
			false,
			false,
		},
		{
			NS_TEST_ID("start fail"),
			NS_QUERY_START_BEGIN,
			NS_QUERY_START_BEGIN,
			hook_async_query_start_begin,
			ISC_R_FAILURE,
			true,
			false,
		},
		{
			NS_TEST_ID("query cancel"),
			NS_QUERY_START_BEGIN,
			NS_QUERY_START_BEGIN,
			hook_async_query_start_begin,
			ISC_R_SUCCESS,
			true,
			true,
		},
		/*
		 * The rest of the test case just confirms supported hookpoints
		 * with the same test logic.
		 */
		{
			NS_TEST_ID("async from setup"),
			NS_QUERY_SETUP,
			NS_QUERY_SETUP,
			hook_async_query_setup,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from lookup"),
			NS_QUERY_LOOKUP_BEGIN,
			NS_QUERY_LOOKUP_BEGIN,
			hook_async_query_lookup_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from resume"),
			NS_QUERY_RESUME_BEGIN,
			NS_QUERY_RESUME_BEGIN,
			hook_async_query_resume_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from resume restored"),
			NS_QUERY_RESUME_RESTORED,
			NS_QUERY_RESUME_BEGIN,
			hook_async_query_resume_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from gotanswer"),
			NS_QUERY_GOT_ANSWER_BEGIN,
			NS_QUERY_GOT_ANSWER_BEGIN,
			hook_async_query_got_answer_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from respond any"),
			NS_QUERY_RESPOND_ANY_BEGIN,
			NS_QUERY_RESPOND_ANY_BEGIN,
			hook_async_query_respond_any_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from add answer"),
			NS_QUERY_ADDANSWER_BEGIN,
			NS_QUERY_ADDANSWER_BEGIN,
			hook_async_query_addanswer_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from notfound"),
			NS_QUERY_NOTFOUND_BEGIN,
			NS_QUERY_NOTFOUND_BEGIN,
			hook_async_query_notfound_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from prep delegation"),
			NS_QUERY_PREP_DELEGATION_BEGIN,
			NS_QUERY_PREP_DELEGATION_BEGIN,
			hook_async_query_prep_delegation_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from zone delegation"),
			NS_QUERY_ZONE_DELEGATION_BEGIN,
			NS_QUERY_ZONE_DELEGATION_BEGIN,
			hook_async_query_zone_delegation_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from delegation"),
			NS_QUERY_DELEGATION_BEGIN,
			NS_QUERY_DELEGATION_BEGIN,
			hook_async_query_delegation_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from async delegation"),
			NS_QUERY_DELEGATION_RECURSE_BEGIN,
			NS_QUERY_DELEGATION_RECURSE_BEGIN,
			hook_async_query_delegation_recurse_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from nodata"),
			NS_QUERY_NODATA_BEGIN,
			NS_QUERY_NODATA_BEGIN,
			hook_async_query_nodata_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from nxdomain"),
			NS_QUERY_NXDOMAIN_BEGIN,
			NS_QUERY_NXDOMAIN_BEGIN,
			hook_async_query_nxdomain_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from ncache"),
			NS_QUERY_NCACHE_BEGIN,
			NS_QUERY_NCACHE_BEGIN,
			hook_async_query_ncache_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from CNAME"),
			NS_QUERY_CNAME_BEGIN,
			NS_QUERY_CNAME_BEGIN,
			hook_async_query_cname_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from DNAME"),
			NS_QUERY_DNAME_BEGIN,
			NS_QUERY_DNAME_BEGIN,
			hook_async_query_dname_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from prep response"),
			NS_QUERY_PREP_RESPONSE_BEGIN,
			NS_QUERY_PREP_RESPONSE_BEGIN,
			hook_async_query_response_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from respond"),
			NS_QUERY_RESPOND_BEGIN,
			NS_QUERY_RESPOND_BEGIN,
			hook_async_query_respond_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from done begin"),
			NS_QUERY_DONE_BEGIN,
			NS_QUERY_DONE_BEGIN,
			hook_async_query_done_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
		{
			NS_TEST_ID("async from done send"),
			NS_QUERY_DONE_SEND,
			NS_QUERY_DONE_BEGIN,
			hook_async_query_done_begin,
			ISC_R_SUCCESS,
			true,
			false,
		},
	};

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		run_hookasync_test(&tests[i]);
	}
}

/*****
***** tests for higher level ("e2e") behavior of ns_query_hookasync().
***** It exercises overall behavior for some selected cases, while
***** ns__query_hookasync_test exercises implementation details for a
***** simple scenario and for all supported hook points.
*****/

/*%
 * Structure containing parameters for ns__query_hookasync_e2e_test().
 */
typedef struct {
	const ns_test_id_t id;	   /* libns test identifier */
	const char *qname;	   /* QNAME */
	ns_hookpoint_t hookpoint;  /* hook point specified for resume */
	isc_result_t start_result; /* result of 'runasync' */
	bool do_cancel;		   /* true if query should be canceled
				    * in test */
	dns_rcode_t expected_rcode;
} ns__query_hookasync_e2e_test_params_t;

/* data structure passed from tests to hooks */
typedef struct hookasync_e2e_data {
	bool async;		   /* true if in a hook-triggered
				    * asynchronous process */
	ns_hook_resevent_t *rev;   /* resume event sent on completion */
	ns_hookpoint_t hookpoint;  /* specifies where to resume */
	isc_result_t start_result; /* result of 'runasync' */
	dns_rcode_t expected_rcode;
	bool done; /* if SEND_DONE hook is called */
} hookasync_e2e_data_t;

/* Cancel callback.  Just need to be defined, it doesn't have to do anything. */
static void
cancel_e2ehookactx(ns_hookasync_t *ctx) {
	UNUSED(ctx);
}

/* 'runasync' callback passed to ns_query_hookasync */
static isc_result_t
test_hookasync_e2e(query_ctx_t *qctx, isc_mem_t *memctx, void *arg,
		   isc_task_t *task, isc_taskaction_t action, void *evarg,
		   ns_hookasync_t **ctxp) {
	ns_hookasync_t *ctx = NULL;
	ns_hook_resevent_t *rev = NULL;
	hookasync_e2e_data_t *asdata = arg;

	if (asdata->start_result != ISC_R_SUCCESS) {
		return (asdata->start_result);
	}

	ctx = isc_mem_get(memctx, sizeof(*ctx));
	rev = (ns_hook_resevent_t *)isc_event_allocate(
		memctx, task, NS_EVENT_HOOKASYNCDONE, action, evarg,
		sizeof(*rev));

	rev->hookpoint = asdata->hookpoint;
	rev->saved_qctx = qctx;
	rev->ctx = ctx;
	asdata->rev = rev;

	*ctx = (ns_hookasync_t){ .private = asdata };
	isc_mem_attach(memctx, &ctx->mctx);
	ctx->destroy = destroy_hookactx;
	ctx->cancel = cancel_e2ehookactx;

	*ctxp = ctx;
	return (ISC_R_SUCCESS);
}

static ns_hookresult_t
hook_async_e2e(void *arg, void *data, isc_result_t *resultp) {
	query_ctx_t *qctx = arg;
	hookasync_e2e_data_t *asdata = data;
	isc_result_t result;

	if (!asdata->async) {
		/* Initial call to the hook; start async event */
		result = ns_query_hookasync(qctx, test_hookasync_e2e, asdata);
		if (result != ISC_R_SUCCESS) {
			*resultp = result;
			return (NS_HOOK_RETURN);
		}

		asdata->async = true;
		asdata->rev->origresult = *resultp; /* save it for resume */
		*resultp = ISC_R_UNSET;
		return (NS_HOOK_RETURN);
	} else {
		/* Resume from the completion of async event */
		asdata->async = false;
		/* Don't touch 'resultp' */
		return (NS_HOOK_CONTINUE);
	}
}

/*
 * Check whether the final response has expected the RCODE according to
 * the test scenario.
 */
static ns_hookresult_t
hook_donesend(void *arg, void *data, isc_result_t *resultp) {
	query_ctx_t *qctx = arg;
	hookasync_e2e_data_t *asdata = data;

	INSIST(qctx->client->message->rcode == asdata->expected_rcode);
	asdata->done = true; /* Let the test know this hook is called */
	*resultp = ISC_R_UNSET;
	return (NS_HOOK_CONTINUE);
}

static void
run_hookasync_e2e_test(const ns__query_hookasync_e2e_test_params_t *test) {
	query_ctx_t *qctx = NULL;
	isc_result_t result;
	hookasync_e2e_data_t asdata = {
		.async = false,
		.hookpoint = test->hookpoint,
		.start_result = test->start_result,
		.expected_rcode = test->expected_rcode,
		.done = false,
	};
	const ns_hook_t donesend_hook = {
		.action = hook_donesend,
		.action_data = &asdata,
	};
	const ns_hook_t hook = {
		.action = hook_async_e2e,
		.action_data = &asdata,
	};
	const ns_test_qctx_create_params_t qctx_params = {
		.qname = test->qname,
		.qtype = dns_rdatatype_a,
		.with_cache = true,
	};

	ns__hook_table = NULL;
	ns_hooktable_create(mctx, &ns__hook_table);
	ns_hook_add(ns__hook_table, mctx, test->hookpoint, &hook);
	ns_hook_add(ns__hook_table, mctx, NS_QUERY_DONE_SEND, &donesend_hook);

	result = ns_test_qctx_create(&qctx_params, &qctx);
	INSIST(result == ISC_R_SUCCESS);

	isc_sockaddr_any(&qctx->client->peeraddr); /* for sortlist */
	qctx->client->sendcb = send_noop;

	/* Load a zone.  it should have ns.foo/A */
	result = ns_test_serve_zone("foo", TESTS_DIR "/testdata/query/foo.db",
				    qctx->client->view);
	INSIST(result == ISC_R_SUCCESS);

	/*
	 * We expect to have a response sent all cases, so we need to
	 * setup reqhandle (which will be detached on the send).
	 */
	isc_nmhandle_attach(qctx->client->handle, &qctx->client->reqhandle);

	/* Handle the query.  hook-based async event will be triggered. */
	qctx->client->state = NS_CLIENTSTATE_WORKING;
	ns__query_start(qctx);

	/* If specified cancel the query at this point. */
	if (test->do_cancel) {
		ns_query_cancel(qctx->client);
	}

	if (test->start_result == ISC_R_SUCCESS) {
		/*
		 * If async event has started, manually invoke the done event.
		 */
		INSIST(asdata.async);
		asdata.rev->ev_action(asdata.rev->ev_sender,
				      (isc_event_t *)asdata.rev);

		/*
		 * Usually 'async' is reset to false on the 2nd call to
		 * the hook.  But the hook isn't called if the query is
		 * canceled.
		 */
		INSIST(asdata.done == !test->do_cancel);
		INSIST(asdata.async == test->do_cancel);
	} else {
		INSIST(!asdata.async);
	}

	/* Cleanup */
	ns_test_qctx_destroy(&qctx);
	ns_test_cleanup_zone();
	ns_hooktable_free(mctx, (void **)&ns__hook_table);
}

ISC_RUN_TEST_IMPL(ns_query_hookasync_e2e) {
	UNUSED(state);

	const ns__query_hookasync_e2e_test_params_t tests[] = {
		{
			NS_TEST_ID("positive answer"),
			"ns.foo",
			NS_QUERY_GOT_ANSWER_BEGIN,
			ISC_R_SUCCESS,
			false,
			dns_rcode_noerror,
		},
		{
			NS_TEST_ID("NXDOMAIN"),
			"notexist.foo",
			NS_QUERY_NXDOMAIN_BEGIN,
			ISC_R_SUCCESS,
			false,
			dns_rcode_nxdomain,
		},
		{
			NS_TEST_ID("async fail"),
			"ns.foo",
			NS_QUERY_DONE_BEGIN,
			ISC_R_FAILURE,
			false,
			-1,
		},
		{
			NS_TEST_ID("cancel query"),
			"ns.foo",
			NS_QUERY_DONE_BEGIN,
			ISC_R_SUCCESS,
			true,
			-1,
		},
	};

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		run_hookasync_e2e_test(&tests[i]);
	}
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(ns_query_sfcache, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ns_query_start, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ns_query_hookasync, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ns_query_hookasync_e2e, setup_test, teardown_test)

ISC_TEST_LIST_END
ISC_TEST_MAIN
