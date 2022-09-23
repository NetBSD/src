/*	$NetBSD: resolver_test.c,v 1.8 2022/09/23 12:15:32 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/resolver.h>
#include <dns/view.h>

#include "dnstest.h"

static dns_dispatchmgr_t *dispatchmgr = NULL;
static dns_dispatch_t *dispatch = NULL;
static dns_view_t *view = NULL;

static int
_setup(void **state) {
	isc_result_t result;
	isc_sockaddr_t local;

	UNUSED(state);

	result = dns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dispatchmgr_create(dt_mctx, &dispatchmgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_makeview("view", &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_sockaddr_any(&local);
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, &local,
				     4096, 100, 100, 100, 500, 0, 0, &dispatch);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_dispatch_detach(&dispatch);
	dns_view_detach(&view);
	dns_dispatchmgr_destroy(&dispatchmgr);
	dns_test_end();

	return (0);
}

static void
mkres(dns_resolver_t **resolverp) {
	isc_result_t result;

	result = dns_resolver_create(view, taskmgr, 1, 1, socketmgr, timermgr,
				     0, dispatchmgr, dispatch, NULL, resolverp);
	assert_int_equal(result, ISC_R_SUCCESS);
}

static void
destroy_resolver(dns_resolver_t **resolverp) {
	dns_resolver_shutdown(*resolverp);
	dns_resolver_detach(resolverp);
}

/* dns_resolver_create */
static void
create_test(void **state) {
	dns_resolver_t *resolver = NULL;

	UNUSED(state);

	mkres(&resolver);
	destroy_resolver(&resolver);
}

/* dns_resolver_gettimeout */
static void
gettimeout_test(void **state) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(state);

	mkres(&resolver);

	timeout = dns_resolver_gettimeout(resolver);
	assert_true(timeout > 0);

	destroy_resolver(&resolver);
}

/* dns_resolver_settimeout */
static void
settimeout_test(void **state) {
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
static void
settimeout_default_test(void **state) {
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
static void
settimeout_belowmin_test(void **state) {
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
static void
settimeout_overmax_test(void **state) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(state);

	mkres(&resolver);

	dns_resolver_settimeout(resolver, 4000000);
	timeout = dns_resolver_gettimeout(resolver);
	assert_in_range(timeout, 0, 3999999);
	destroy_resolver(&resolver);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(create_test, _setup, _teardown),
		cmocka_unit_test_setup_teardown(gettimeout_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(settimeout_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(settimeout_default_test, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(settimeout_belowmin_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(settimeout_overmax_test, _setup,
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
