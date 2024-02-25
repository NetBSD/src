/*	$NetBSD: sockaddr_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <tests/isc.h>

/* test sockaddr hash */
ISC_RUN_TEST_IMPL(sockaddr_hash) {
	isc_sockaddr_t addr;
	struct in_addr in;
	struct in6_addr in6;
	unsigned int h1, h2, h3, h4;
	int ret;

	UNUSED(state);

	in.s_addr = inet_addr("127.0.0.1");
	isc_sockaddr_fromin(&addr, &in, 1);
	h1 = isc_sockaddr_hash(&addr, true);
	h2 = isc_sockaddr_hash(&addr, false);
	assert_int_not_equal(h1, h2);

	ret = inet_pton(AF_INET6, "::ffff:127.0.0.1", &in6);
	assert_int_equal(ret, 1);
	isc_sockaddr_fromin6(&addr, &in6, 1);
	h3 = isc_sockaddr_hash(&addr, true);
	h4 = isc_sockaddr_hash(&addr, false);
	assert_int_equal(h1, h3);
	assert_int_equal(h2, h4);
}

/* test isc_sockaddr_isnetzero() */
ISC_RUN_TEST_IMPL(sockaddr_isnetzero) {
	isc_sockaddr_t addr;
	struct in_addr in;
	struct in6_addr in6;
	bool r;

	size_t i;
	struct {
		const char *string;
		bool expect;
	} data4[] = {
		{ "0.0.0.0", true },	      { "0.0.0.1", true },
		{ "0.0.1.0", true },	      { "0.1.0.0", true },
		{ "1.0.0.0", false },	      { "0.0.0.127", true },
		{ "0.0.0.255", true },	      { "127.0.0.1", false },
		{ "255.255.255.255", false },
	};
	/*
	 * Mapped addresses are currently not netzero.
	 */
	struct {
		const char *string;
		bool expect;
	} data6[] = {
		{ "::ffff:0.0.0.0", false },
		{ "::ffff:0.0.0.1", false },
		{ "::ffff:0.0.0.127", false },
		{ "::ffff:0.0.0.255", false },
		{ "::ffff:127.0.0.1", false },
		{ "::ffff:255.255.255.255", false },
	};

	UNUSED(state);

	for (i = 0; i < sizeof(data4) / sizeof(data4[0]); i++) {
		in.s_addr = inet_addr(data4[i].string);
		isc_sockaddr_fromin(&addr, &in, 1);
		r = isc_sockaddr_isnetzero(&addr);
		assert_int_equal(r, data4[i].expect);
	}

	for (i = 0; i < sizeof(data6) / sizeof(data6[0]); i++) {
		int ret = inet_pton(AF_INET6, data6[i].string, &in6);
		assert_int_equal(ret, 1);
		isc_sockaddr_fromin6(&addr, &in6, 1);
		r = isc_sockaddr_isnetzero(&addr);
		assert_int_equal(r, data6[i].expect);
	}
}

/*
 * test that isc_sockaddr_eqaddrprefix() returns true when prefixes of a
 * and b are equal, and false when they are not equal
 */
ISC_RUN_TEST_IMPL(sockaddr_eqaddrprefix) {
	struct in_addr ina_a;
	struct in_addr ina_b;
	struct in_addr ina_c;
	isc_sockaddr_t isa_a;
	isc_sockaddr_t isa_b;
	isc_sockaddr_t isa_c;

	UNUSED(state);

	assert_true(inet_pton(AF_INET, "194.100.32.87", &ina_a) >= 0);
	assert_true(inet_pton(AF_INET, "194.100.32.80", &ina_b) >= 0);
	assert_true(inet_pton(AF_INET, "194.101.32.87", &ina_c) >= 0);

	isc_sockaddr_fromin(&isa_a, &ina_a, 0);
	isc_sockaddr_fromin(&isa_b, &ina_b, 42);
	isc_sockaddr_fromin(&isa_c, &ina_c, 0);

	assert_true(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 0));
	assert_true(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 29));
	assert_true(isc_sockaddr_eqaddrprefix(&isa_a, &isa_c, 8));

	assert_false(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 30));
	assert_false(isc_sockaddr_eqaddrprefix(&isa_a, &isa_b, 32));
	assert_false(isc_sockaddr_eqaddrprefix(&isa_a, &isa_c, 16));
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(sockaddr_hash)
ISC_TEST_ENTRY(sockaddr_isnetzero)
ISC_TEST_ENTRY(sockaddr_eqaddrprefix)

ISC_TEST_LIST_END

ISC_TEST_MAIN
