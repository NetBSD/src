/*	$NetBSD: netaddr_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <string.h>

#include <isc/netaddr.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

ATF_TC(netaddr_isnetzero);
ATF_TC_HEAD(netaddr_isnetzero, tc) {
	atf_tc_set_md_var(tc, "descr", "test netaddr_isnetzero");
}
ATF_TC_BODY(netaddr_isnetzero, tc) {
	unsigned int i;
	struct in_addr ina;
	struct {
		const char *address;
		isc_boolean_t expect;
	} tests[] = {
		{ "0.0.0.0", ISC_TRUE },
		{ "0.0.0.1", ISC_TRUE },
		{ "0.0.1.2", ISC_TRUE },
		{ "0.1.2.3", ISC_TRUE },
		{ "10.0.0.0", ISC_FALSE },
		{ "10.9.0.0", ISC_FALSE },
		{ "10.9.8.0", ISC_FALSE },
		{ "10.9.8.7", ISC_FALSE },
		{ "127.0.0.0", ISC_FALSE },
		{ "127.0.0.1", ISC_FALSE }
	};
	isc_boolean_t result;
	isc_netaddr_t netaddr;

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		ina.s_addr = inet_addr(tests[i].address);
		isc_netaddr_fromin(&netaddr, &ina);
		result = isc_netaddr_isnetzero(&netaddr);
		ATF_CHECK_EQ_MSG(result, tests[i].expect,
				 "%s", tests[i].address);
	}
}

ATF_TC(netaddr_masktoprefixlen);
ATF_TC_HEAD(netaddr_masktoprefixlen, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "isc_netaddr_masktoprefixlen() "
			  "calculates correct prefix lengths ");
}
ATF_TC_BODY(netaddr_masktoprefixlen, tc) {
	struct in_addr na_a;
	struct in_addr na_b;
	struct in_addr na_c;
	struct in_addr na_d;
	isc_netaddr_t ina_a;
	isc_netaddr_t ina_b;
	isc_netaddr_t ina_c;
	isc_netaddr_t ina_d;
	unsigned int plen;

	UNUSED(tc);

	ATF_CHECK(inet_pton(AF_INET, "0.0.0.0", &na_a) >= 0);
	ATF_CHECK(inet_pton(AF_INET, "255.255.255.254", &na_b) >= 0);
	ATF_CHECK(inet_pton(AF_INET, "255.255.255.255", &na_c) >= 0);
	ATF_CHECK(inet_pton(AF_INET, "255.255.255.0", &na_d) >= 0);

	isc_netaddr_fromin(&ina_a, &na_a);
	isc_netaddr_fromin(&ina_b, &na_b);
	isc_netaddr_fromin(&ina_c, &na_c);
	isc_netaddr_fromin(&ina_d, &na_d);

	ATF_CHECK_EQ(isc_netaddr_masktoprefixlen(&ina_a, &plen),
		     ISC_R_SUCCESS);
	ATF_CHECK_EQ(plen, 0);

	ATF_CHECK_EQ(isc_netaddr_masktoprefixlen(&ina_b, &plen),
		     ISC_R_SUCCESS);
	ATF_CHECK_EQ(plen, 31);

	ATF_CHECK_EQ(isc_netaddr_masktoprefixlen(&ina_c, &plen),
		     ISC_R_SUCCESS);
	ATF_CHECK_EQ(plen, 32);

	ATF_CHECK_EQ(isc_netaddr_masktoprefixlen(&ina_d, &plen),
		     ISC_R_SUCCESS);
	ATF_CHECK_EQ(plen, 24);
}

ATF_TC(netaddr_multicast);
ATF_TC_HEAD(netaddr_multicast, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "check multicast addresses are detected properly");
}
ATF_TC_BODY(netaddr_multicast, tc) {
	unsigned int i;
	struct {
		int family;
		const char *addr;
		isc_boolean_t is_multicast;
	} tests[] = {
		{ AF_INET, "1.2.3.4", ISC_FALSE },
		{ AF_INET, "4.3.2.1", ISC_FALSE },
		{ AF_INET, "224.1.1.1", ISC_TRUE },
		{ AF_INET, "1.1.1.244", ISC_FALSE },
		{ AF_INET6, "::1", ISC_FALSE },
		{ AF_INET6, "ff02::1", ISC_TRUE }
	};

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		isc_netaddr_t na;
		struct in_addr in;
		struct in6_addr in6;
		int r;

		if (tests[i].family == AF_INET) {
			r = inet_pton(AF_INET, tests[i].addr,
				      (unsigned char *)&in);
			ATF_REQUIRE_EQ(r, 1);
			isc_netaddr_fromin(&na, &in);
		} else {
			r = inet_pton(AF_INET6, tests[i].addr,
				      (unsigned char *)&in6);
			ATF_REQUIRE_EQ(r, 1);
			isc_netaddr_fromin6(&na, &in6);
		}

		ATF_CHECK_EQ(isc_netaddr_ismulticast(&na),
			     tests[i].is_multicast);
	}
}
/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, netaddr_isnetzero);
	ATF_TP_ADD_TC(tp, netaddr_masktoprefixlen);
	ATF_TP_ADD_TC(tp, netaddr_multicast);

	return (atf_no_error());
}
