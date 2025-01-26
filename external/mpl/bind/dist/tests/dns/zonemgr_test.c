/*	$NetBSD: zonemgr_test.c,v 1.3 2025/01/26 16:25:48 christos Exp $	*/

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

#include <isc/buffer.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <tests/dns.h>

static int
setup_test(void **state) {
	setup_loopmgr(state);
	setup_netmgr(state);

	return 0;
}

static int
teardown_test(void **state) {
	teardown_netmgr(state);
	teardown_loopmgr(state);

	return 0;
}

/* create zone manager */
ISC_LOOP_TEST_IMPL(zonemgr_create) {
	dns_zonemgr_t *myzonemgr = NULL;

	UNUSED(arg);

	dns_zonemgr_create(mctx, netmgr, &myzonemgr);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);

	isc_loopmgr_shutdown(loopmgr);
}

/* manage and release a zone */
ISC_LOOP_TEST_IMPL(zonemgr_managezone) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(arg);

	dns_zonemgr_create(mctx, netmgr, &myzonemgr);

	result = dns_test_makezone("foo", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 0);

	/* Now it should succeed */
	result = dns_zonemgr_managezone(myzonemgr, zone);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 1);

	dns_zonemgr_releasezone(myzonemgr, zone);
	dns_zone_detach(&zone);

	assert_int_equal(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 0);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);

	isc_loopmgr_shutdown(loopmgr);
}

/* create and release a zone */
ISC_LOOP_TEST_IMPL(zonemgr_createzone) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(arg);

	dns_zonemgr_create(mctx, netmgr, &myzonemgr);

	result = dns_zonemgr_createzone(myzonemgr, &zone);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(zone);

	result = dns_zonemgr_managezone(myzonemgr, zone);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_zone_detach(&zone);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);

	isc_loopmgr_shutdown(loopmgr);
}

/* manage and release a zone */
ISC_LOOP_TEST_IMPL(zonemgr_unreachable) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_sockaddr_t addr1, addr2;
	struct in_addr in;
	isc_result_t result;
	isc_time_t now;

	UNUSED(arg);

	now = isc_time_now();

	dns_zonemgr_create(mctx, netmgr, &myzonemgr);

	result = dns_test_makezone("foo", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_zonemgr_managezone(myzonemgr, zone);
	assert_int_equal(result, ISC_R_SUCCESS);

	in.s_addr = inet_addr("10.53.0.1");
	isc_sockaddr_fromin(&addr1, &in, 2112);
	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	assert_false(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	/*
	 * We require multiple unreachableadd calls to mark a server as
	 * unreachable.
	 */
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	assert_false(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	assert_true(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	in.s_addr = inet_addr("10.53.0.3");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	assert_false(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	/*
	 * We require multiple unreachableadd calls to mark a server as
	 * unreachable.
	 */
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	assert_true(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	dns_zonemgr_unreachabledel(myzonemgr, &addr1, &addr2);
	assert_false(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	assert_true(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	dns_zonemgr_unreachabledel(myzonemgr, &addr1, &addr2);
	assert_false(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	dns_zonemgr_releasezone(myzonemgr, zone);
	dns_zone_detach(&zone);
	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(zonemgr_create, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(zonemgr_managezone, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(zonemgr_createzone, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(zonemgr_unreachable, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
