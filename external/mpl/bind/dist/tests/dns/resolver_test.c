/*	$NetBSD: resolver_test.c,v 1.3 2025/01/26 16:25:48 christos Exp $	*/

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

#include <isc/buffer.h>
#include <isc/net.h>
#include <isc/timer.h>
#include <isc/tls.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/resolver.h>
#include <dns/view.h>

#include <tests/dns.h>

static dns_dispatch_t *dispatch = NULL;
static dns_view_t *view = NULL;
static isc_tlsctx_cache_t *tlsctx_cache = NULL;

static int
setup_test(void **state) {
	isc_result_t result;
	isc_sockaddr_t local;
	dns_dispatchmgr_t *dispatchmgr = NULL;

	setup_managers(state);

	result = dns_test_makeview("view", true, false, &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	dispatchmgr = dns_view_getdispatchmgr(view);
	assert_non_null(dispatchmgr);

	isc_sockaddr_any(&local);
	result = dns_dispatch_createudp(dispatchmgr, &local, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dispatchmgr_detach(&dispatchmgr);

	return 0;
}

static int
teardown_test(void **state) {
	dns_dispatch_detach(&dispatch);
	dns_view_detach(&view);
	teardown_managers(state);

	return 0;
}

static void
mkres(dns_resolver_t **resolverp) {
	isc_result_t result;

	isc_tlsctx_cache_create(mctx, &tlsctx_cache);
	result = dns_resolver_create(view, loopmgr, netmgr, 0, tlsctx_cache,
				     dispatch, NULL, resolverp);
	assert_int_equal(result, ISC_R_SUCCESS);
}

static void
destroy_resolver(dns_resolver_t **resolverp) {
	dns_resolver_shutdown(*resolverp);
	dns_resolver_detach(resolverp);
	if (tlsctx_cache != NULL) {
		isc_tlsctx_cache_detach(&tlsctx_cache);
	}
}

/* dns_resolver_create */
ISC_LOOP_TEST_IMPL(create) {
	dns_resolver_t *resolver = NULL;

	mkres(&resolver);
	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

/* dns_resolver_gettimeout */
ISC_LOOP_TEST_IMPL(gettimeout) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	mkres(&resolver);

	timeout = dns_resolver_gettimeout(resolver);
	assert_true(timeout > 0);

	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

/* dns_resolver_settimeout */
ISC_LOOP_TEST_IMPL(settimeout) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 1);
	timeout = dns_resolver_gettimeout(resolver);
	assert_true(timeout == default_timeout + 1);

	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

/* dns_resolver_settimeout */
ISC_LOOP_TEST_IMPL(settimeout_default) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 100);

	timeout = dns_resolver_gettimeout(resolver);
	assert_int_equal(timeout, default_timeout + 100);

	dns_resolver_settimeout(resolver, 0);
	timeout = dns_resolver_gettimeout(resolver);
	assert_int_equal(timeout, default_timeout);

	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

/* dns_resolver_settimeout below minimum */
ISC_LOOP_TEST_IMPL(settimeout_belowmin) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, 300);

	timeout = dns_resolver_gettimeout(resolver);
	assert_in_range(timeout, default_timeout, 3999999);

	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

/* dns_resolver_settimeout over maximum */
ISC_LOOP_TEST_IMPL(settimeout_overmax) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, 4000000);

	timeout = dns_resolver_gettimeout(resolver);
	assert_in_range(timeout, default_timeout, 3999999);

	destroy_resolver(&resolver);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(create, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(gettimeout, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(settimeout, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(settimeout_default, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(settimeout_belowmin, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(settimeout_overmax, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
