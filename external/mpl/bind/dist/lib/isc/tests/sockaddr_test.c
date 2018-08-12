/*	$NetBSD: sockaddr_test.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

#include <isc/netaddr.h>
#include <isc/sockaddr.h>
#include <isc/print.h>

#include "isctest.h"

/*
 * Individual unit tests
 */

ATF_TC(sockaddr_hash);
ATF_TC_HEAD(sockaddr_hash, tc) {
	atf_tc_set_md_var(tc, "descr", "sockaddr hash");
}
ATF_TC_BODY(sockaddr_hash, tc) {
	isc_result_t result;
	isc_sockaddr_t addr;
	struct in_addr in;
	struct in6_addr in6;
	unsigned int h1, h2, h3, h4;
	int ret;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	in.s_addr = inet_addr("127.0.0.1");
	isc_sockaddr_fromin(&addr, &in, 1);
	h1 = isc_sockaddr_hash(&addr, ISC_TRUE);
	h2 = isc_sockaddr_hash(&addr, ISC_FALSE);
	ATF_CHECK(h1 != h2);

	ret = inet_pton(AF_INET6, "::ffff:127.0.0.1", &in6);
	ATF_CHECK(ret == 1);
	isc_sockaddr_fromin6(&addr, &in6, 1);
	h3 = isc_sockaddr_hash(&addr, ISC_TRUE);
	h4 = isc_sockaddr_hash(&addr, ISC_FALSE);
	ATF_CHECK(h1 == h3);
	ATF_CHECK(h2 == h4);

	isc_test_end();
}

ATF_TC(sockaddr_isnetzero);
ATF_TC_HEAD(sockaddr_isnetzero, tc) {
	atf_tc_set_md_var(tc, "descr", "sockaddr is net zero");
}
ATF_TC_BODY(sockaddr_isnetzero, tc) {
	isc_sockaddr_t addr;
	struct in_addr in;
	struct in6_addr in6;
	isc_boolean_t r;
	int ret;
	size_t i;
	struct {
		const char *string;
		isc_boolean_t expect;
	} data4[] = {
		{ "0.0.0.0", ISC_TRUE },
		{ "0.0.0.1", ISC_TRUE },
		{ "0.0.1.0", ISC_TRUE },
		{ "0.1.0.0", ISC_TRUE },
		{ "1.0.0.0", ISC_FALSE },
		{ "0.0.0.127", ISC_TRUE },
		{ "0.0.0.255", ISC_TRUE },
		{ "127.0.0.1", ISC_FALSE },
		{ "255.255.255.255", ISC_FALSE },
	};
	/*
	 * Mapped addresses are currently not netzero.
	 */
	struct {
		const char *string;
		isc_boolean_t expect;
	} data6[] = {
		{ "::ffff:0.0.0.0", ISC_FALSE },
		{ "::ffff:0.0.0.1", ISC_FALSE },
		{ "::ffff:0.0.0.127", ISC_FALSE },
		{ "::ffff:0.0.0.255", ISC_FALSE },
		{ "::ffff:127.0.0.1", ISC_FALSE },
		{ "::ffff:255.255.255.255", ISC_FALSE },
	};

	UNUSED(tc);

	for (i = 0; i < sizeof(data4)/sizeof(data4[0]); i++) {
		in.s_addr = inet_addr(data4[i].string);
		isc_sockaddr_fromin(&addr, &in, 1);
		r = isc_sockaddr_isnetzero(&addr);
		ATF_CHECK_EQ_MSG(r, data4[i].expect, "%s", data4[i].string);
	}

	for (i = 0; i < sizeof(data6)/sizeof(data6[0]); i++) {
		ret = inet_pton(AF_INET6, data6[i].string, &in6);
		ATF_CHECK_EQ(ret, 1);
		isc_sockaddr_fromin6(&addr, &in6, 1);
		r = isc_sockaddr_isnetzero(&addr);
		ATF_CHECK_EQ_MSG(r, data6[i].expect, "%s", data6[i].string);
	}
}

ATF_TC(sockaddr_eqaddrprefix);
ATF_TC_HEAD(sockaddr_eqaddrprefix, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "isc_sockaddr_eqaddrprefix() returns ISC_TRUE when "
			  "prefixes of a and b are equal, and ISC_FALSE when "
			  "they are not equal");
}
ATF_TC_BODY(sockaddr_eqaddrprefix, tc) {
	struct in_addr ina_a;
	struct in_addr ina_b;
	struct in_addr ina_c;
	isc_sockaddr_t isa_a;
	isc_sockaddr_t isa_b;
	isc_sockaddr_t isa_c;

	UNUSED(tc);

	ATF_CHECK(inet_pton(AF_INET, "194.100.32.87", &ina_a) >= 0);
	ATF_CHECK(inet_pton(AF_INET, "194.100.32.80", &ina_b) >= 0);
	ATF_CHECK(inet_pton(AF_INET, "194.101.32.87", &ina_c) >= 0);

	isc_sockaddr_fromin(&isa_a, &ina_a, 0);
	isc_sockaddr_fromin(&isa_b, &ina_b, 42);
	isc_sockaddr_fromin(&isa_c, &ina_c, 0);

	ATF_CHECK(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 0));
	ATF_CHECK(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 29));
	ATF_CHECK(isc_sockaddr_eqaddrprefix(&isa_a, &isa_c, 8));

	ATF_CHECK(! isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 30));
	ATF_CHECK(! isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 32));
	ATF_CHECK(! isc_sockaddr_eqaddrprefix(&isa_a, &isa_c, 16));
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, sockaddr_hash);
	ATF_TP_ADD_TC(tp, sockaddr_isnetzero);
	ATF_TP_ADD_TC(tp, sockaddr_eqaddrprefix);

	return (atf_no_error());
}

