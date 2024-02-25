/*	$NetBSD: resolver_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/net.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/resolver.h>
#include <dns/view.h>

#include <tests/dns.h>

static dns_dispatchmgr_t *dispatchmgr = NULL;
static dns_dispatch_t *dispatch = NULL;
static dns_view_t *view = NULL;

static int
_setup(void **state) {
	isc_result_t result;
	isc_sockaddr_t local;

	setup_managers(state);

	result = dns_dispatchmgr_create(mctx, netmgr, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_makeview("view", true, &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_sockaddr_any(&local);
	result = dns_dispatch_createudp(dispatchmgr, &local, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	dns_dispatch_detach(&dispatch);
	dns_view_detach(&view);
	dns_dispatchmgr_detach(&dispatchmgr);

	teardown_managers(state);

	return (0);
}

static void
mkres(dns_resolver_t **resolverp) {
	isc_result_t result;

	result = dns_resolver_create(view, taskmgr, 1, 1, netmgr, timermgr, 0,
				     dispatchmgr, dispatch, NULL, resolverp);
	assert_int_equal(result, ISC_R_SUCCESS);
}

static void
destroy_resolver(dns_resolver_t **resolverp) {
	dns_resolver_shutdown(*resolverp);
	dns_resolver_detach(resolverp);
}

/* dns_resolver_create */
ISC_RUN_TEST_IMPL(dns_resolver_create) {
	dns_resolver_t *resolver = NULL;

	UNUSED(state);

	mkres(&resolver);
	destroy_resolver(&resolver);
}

/* dns_resolver_gettimeout */
ISC_RUN_TEST_IMPL(dns_resolver_gettimeout) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(state);

	mkres(&resolver);

	timeout = dns_resolver_gettimeout(resolver);
	assert_true(timeout > 0);

	destroy_resolver(&resolver);
}

/* dns_resolver_settimeout */
ISC_RUN_TEST_IMPL(dns_resolver_settimeout) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(state);

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 1);
	timeout = dns_resolver_gettimeout(resolver);
	assert_true(timeout == default_timeout + 1);

	destroy_resolver(&resolver);
}

/* dns_resolver_settimeout */
ISC_RUN_TEST_IMPL(dns_resolver_settimeout_default) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(state);

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 100);

	timeout = dns_resolver_gettimeout(resolver);
	assert_int_equal(timeout, default_timeout + 100);

	dns_resolver_settimeout(resolver, 0);
	timeout = dns_resolver_gettimeout(resolver);
	assert_int_equal(timeout, default_timeout);

	destroy_resolver(&resolver);
}

/* dns_resolver_settimeout below minimum */
ISC_RUN_TEST_IMPL(dns_resolver_settimeout_belowmin) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(state);

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, 9000);

	timeout = dns_resolver_gettimeout(resolver);
	assert_int_equal(timeout, default_timeout);

	destroy_resolver(&resolver);
}

/* dns_resolver_settimeout over maximum */
ISC_RUN_TEST_IMPL(dns_resolver_settimeout_overmax) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(state);

	mkres(&resolver);

	dns_resolver_settimeout(resolver, 4000000);
	timeout = dns_resolver_gettimeout(resolver);
	assert_in_range(timeout, 0, 3999999);
	destroy_resolver(&resolver);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(dns_resolver_create, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dns_resolver_gettimeout, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dns_resolver_settimeout, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dns_resolver_settimeout_default, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dns_resolver_settimeout_belowmin, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(dns_resolver_settimeout_overmax, _setup, _teardown)

ISC_TEST_LIST_END

ISC_TEST_MAIN
