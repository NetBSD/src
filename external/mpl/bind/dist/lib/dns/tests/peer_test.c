/*	$NetBSD: peer_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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

#include <dns/peer.h>

#include "dnstest.h"

/*
 * Individual unit tests
 */
ATF_TC(dscp);
ATF_TC_HEAD(dscp, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "Test DSCP set/get functions");
}
ATF_TC_BODY(dscp, tc) {
	isc_result_t result;
	isc_netaddr_t netaddr;
	struct in_addr ina;
	dns_peer_t *peer = NULL;
	isc_dscp_t dscp;

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Create peer structure for the loopback address.
	 */
	ina.s_addr = INADDR_LOOPBACK;
	isc_netaddr_fromin(&netaddr, &ina);
	result = dns_peer_new(mctx, &netaddr, &peer);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * All should be not set on creation.
	 * 'dscp' should remain unchanged.
	 */
	dscp = 100;
	result = dns_peer_getquerydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	result = dns_peer_getnotifydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	result = dns_peer_gettransferdscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	/*
	 * Test that setting query dscp does not affect the other
	 * dscp values.  'dscp' should remain unchanged until
	 * dns_peer_getquerydscp is called.
	 */
	dscp = 100;
	result = dns_peer_setquerydscp(peer, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_peer_getnotifydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	result = dns_peer_gettransferdscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	result = dns_peer_getquerydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 1);

	/*
	 * Test that setting notify dscp does not affect the other
	 * dscp values.  'dscp' should remain unchanged until
	 * dns_peer_getquerydscp is called then should change again
	 * on dns_peer_getnotifydscp.
	 */
	dscp = 100;
	result = dns_peer_setnotifydscp(peer, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_peer_gettransferdscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dscp, 100);

	result = dns_peer_getquerydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 1);

	result = dns_peer_getnotifydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 2);

	/*
	 * Test that setting notify dscp does not affect the other
	 * dscp values.  Check that appropriate values are returned.
	 */
	dscp = 100;
	result = dns_peer_settransferdscp(peer, 3);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_peer_getquerydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 1);

	result = dns_peer_getnotifydscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 2);

	result = dns_peer_gettransferdscp(peer, &dscp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dscp, 3);

	dns_peer_detach(&peer);
	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, dscp);
	return (atf_no_error());
}
