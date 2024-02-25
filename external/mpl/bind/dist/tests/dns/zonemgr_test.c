/*	$NetBSD: zonemgr_test.c,v 1.2.2.2 2024/02/25 15:47:41 martin Exp $	*/

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
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <tests/dns.h>

/* create zone manager */
ISC_RUN_TEST_IMPL(dns_zonemgr_create) {
	dns_zonemgr_t *myzonemgr = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, netmgr,
				    &myzonemgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);
}

/* manage and release a zone */
ISC_RUN_TEST_IMPL(dns_zonemgr_managezone) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, netmgr,
				    &myzonemgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_makezone("foo", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* This should not succeed until the dns_zonemgr_setsize() is run */
	result = dns_zonemgr_managezone(myzonemgr, zone);
	assert_int_equal(result, ISC_R_FAILURE);

	assert_int_equal(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 0);

	result = dns_zonemgr_setsize(myzonemgr, 1);
	assert_int_equal(result, ISC_R_SUCCESS);

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
}

/* create and release a zone */
ISC_RUN_TEST_IMPL(dns_zonemgr_createzone) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, netmgr,
				    &myzonemgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* This should not succeed until the dns_zonemgr_setsize() is run */
	result = dns_zonemgr_createzone(myzonemgr, &zone);
	assert_int_equal(result, ISC_R_FAILURE);

	result = dns_zonemgr_setsize(myzonemgr, 1);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Now it should succeed */
	result = dns_zonemgr_createzone(myzonemgr, &zone);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(zone);

	result = dns_zonemgr_managezone(myzonemgr, zone);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_zone_detach(&zone);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	assert_null(myzonemgr);
}

/* manage and release a zone */
ISC_RUN_TEST_IMPL(dns_zonemgr_unreachable) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_sockaddr_t addr1, addr2;
	struct in_addr in;
	isc_result_t result;
	isc_time_t now;

	UNUSED(state);

	TIME_NOW(&now);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, netmgr,
				    &myzonemgr);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_makezone("foo", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_zonemgr_setsize(myzonemgr, 1);
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
}

/*
 * XXX:
 * dns_zonemgr API calls that are not yet part of this unit test:
 *
 * 	- dns_zonemgr_attach
 * 	- dns_zonemgr_forcemaint
 * 	- dns_zonemgr_resumexfrs
 * 	- dns_zonemgr_shutdown
 * 	- dns_zonemgr_setsize
 * 	- dns_zonemgr_settransfersin
 * 	- dns_zonemgr_getttransfersin
 * 	- dns_zonemgr_settransfersperns
 * 	- dns_zonemgr_getttransfersperns
 * 	- dns_zonemgr_setiolimit
 * 	- dns_zonemgr_getiolimit
 * 	- dns_zonemgr_dbdestroyed
 * 	- dns_zonemgr_setserialqueryrate
 * 	- dns_zonemgr_getserialqueryrate
 */

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(dns_zonemgr_create, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_zonemgr_managezone, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_zonemgr_createzone, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_zonemgr_unreachable, setup_managers,
		      teardown_managers)

ISC_TEST_LIST_END

ISC_TEST_MAIN
