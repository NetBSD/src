/*	$NetBSD: zonemgr_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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

#include <unistd.h>

#include <isc/buffer.h>
#include <isc/task.h>
#include <isc/timer.h>

#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "dnstest.h"

/*
 * Individual unit tests
 */
ATF_TC(zonemgr_create);
ATF_TC_HEAD(zonemgr_create, tc) {
	atf_tc_set_md_var(tc, "descr", "create zone manager");
}
ATF_TC_BODY(zonemgr_create, tc) {
	dns_zonemgr_t *myzonemgr = NULL;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &myzonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	ATF_REQUIRE_EQ(myzonemgr, NULL);

	dns_test_end();
}


ATF_TC(zonemgr_managezone);
ATF_TC_HEAD(zonemgr_managezone, tc) {
	atf_tc_set_md_var(tc, "descr", "manage and release a zone");
}
ATF_TC_BODY(zonemgr_managezone, tc) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &myzonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_makezone("foo", &zone, NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* This should not succeed until the dns_zonemgr_setsize() is run */
	result = dns_zonemgr_managezone(myzonemgr, zone);
	ATF_REQUIRE_EQ(result, ISC_R_FAILURE);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 0);

	result = dns_zonemgr_setsize(myzonemgr, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* Now it should succeed */
	result = dns_zonemgr_managezone(myzonemgr, zone);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 1);

	dns_zonemgr_releasezone(myzonemgr, zone);
	dns_zone_detach(&zone);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(myzonemgr, DNS_ZONESTATE_ANY), 0);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	ATF_REQUIRE_EQ(myzonemgr, NULL);

	dns_test_end();
}

ATF_TC(zonemgr_createzone);
ATF_TC_HEAD(zonemgr_createzone, tc) {
	atf_tc_set_md_var(tc, "descr", "create and release a zone");
}
ATF_TC_BODY(zonemgr_createzone, tc) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &myzonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* This should not succeed until the dns_zonemgr_setsize() is run */
	result = dns_zonemgr_createzone(myzonemgr, &zone);
	ATF_REQUIRE_EQ(result, ISC_R_FAILURE);

	result = dns_zonemgr_setsize(myzonemgr, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* Now it should succeed */
	result = dns_zonemgr_createzone(myzonemgr, &zone);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(zone != NULL);

	if (zone != NULL)
		dns_zone_detach(&zone);

	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	ATF_REQUIRE_EQ(myzonemgr, NULL);

	dns_test_end();
}

ATF_TC(zonemgr_unreachable);
ATF_TC_HEAD(zonemgr_unreachable, tc) {
	atf_tc_set_md_var(tc, "descr", "manage and release a zone");
}
ATF_TC_BODY(zonemgr_unreachable, tc) {
	dns_zonemgr_t *myzonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_sockaddr_t addr1, addr2;
	struct in_addr in;
	isc_result_t result;
	isc_time_t now;

	UNUSED(tc);

	TIME_NOW(&now);

	result = dns_test_begin(NULL, ISC_TRUE);

	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &myzonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_makezone("foo", &zone, NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_setsize(myzonemgr, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_managezone(myzonemgr, zone);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	in.s_addr = inet_addr("10.53.0.1");
	isc_sockaddr_fromin(&addr1, &in, 2112);
	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	ATF_CHECK(! dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	/*
	 * We require multiple unreachableadd calls to mark a server as
	 * unreachable.
	 */
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	ATF_CHECK(! dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	ATF_CHECK(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	in.s_addr = inet_addr("10.53.0.3");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	ATF_CHECK(! dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	/*
	 * We require multiple unreachableadd calls to mark a server as
	 * unreachable.
	 */
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	dns_zonemgr_unreachableadd(myzonemgr, &addr1, &addr2, &now);
	ATF_CHECK(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	dns_zonemgr_unreachabledel(myzonemgr, &addr1, &addr2);
	ATF_CHECK(! dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&addr2, &in, 5150);
	ATF_CHECK(dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));
	dns_zonemgr_unreachabledel(myzonemgr, &addr1, &addr2);
	ATF_CHECK(! dns_zonemgr_unreachable(myzonemgr, &addr1, &addr2, &now));

	dns_zonemgr_releasezone(myzonemgr, zone);
	dns_zone_detach(&zone);
	dns_zonemgr_shutdown(myzonemgr);
	dns_zonemgr_detach(&myzonemgr);
	ATF_REQUIRE_EQ(myzonemgr, NULL);

	dns_test_end();
}


/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, zonemgr_create);
	ATF_TP_ADD_TC(tp, zonemgr_managezone);
	ATF_TP_ADD_TC(tp, zonemgr_createzone);
	ATF_TP_ADD_TC(tp, zonemgr_unreachable);
	return (atf_no_error());
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
