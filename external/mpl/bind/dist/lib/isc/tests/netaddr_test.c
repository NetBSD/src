/*	$NetBSD: netaddr_test.c,v 1.4 2019/04/28 00:01:15 christos Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/netaddr.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

/* test isc_netaddr_isnetzero() */
static void
netaddr_isnetzero(void **state) {
	unsigned int i;
	struct in_addr ina;
	struct {
		const char *address;
		bool expect;
	} tests[] = {
		{ "0.0.0.0", true },
		{ "0.0.0.1", true },
		{ "0.0.1.2", true },
		{ "0.1.2.3", true },
		{ "10.0.0.0", false },
		{ "10.9.0.0", false },
		{ "10.9.8.0", false },
		{ "10.9.8.7", false },
		{ "127.0.0.0", false },
		{ "127.0.0.1", false }
	};
	bool result;
	isc_netaddr_t netaddr;

	UNUSED(state);

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		ina.s_addr = inet_addr(tests[i].address);
		isc_netaddr_fromin(&netaddr, &ina);
		result = isc_netaddr_isnetzero(&netaddr);
		assert_int_equal(result, tests[i].expect);
	}
}

/* test isc_netaddr_masktoprefixlen() calculates correct prefix lengths */
static void
netaddr_masktoprefixlen(void **state) {
	struct in_addr na_a;
	struct in_addr na_b;
	struct in_addr na_c;
	struct in_addr na_d;
	isc_netaddr_t ina_a;
	isc_netaddr_t ina_b;
	isc_netaddr_t ina_c;
	isc_netaddr_t ina_d;
	unsigned int plen;

	UNUSED(state);

	assert_true(inet_pton(AF_INET, "0.0.0.0", &na_a) >= 0);
	assert_true(inet_pton(AF_INET, "255.255.255.254", &na_b) >= 0);
	assert_true(inet_pton(AF_INET, "255.255.255.255", &na_c) >= 0);
	assert_true(inet_pton(AF_INET, "255.255.255.0", &na_d) >= 0);

	isc_netaddr_fromin(&ina_a, &na_a);
	isc_netaddr_fromin(&ina_b, &na_b);
	isc_netaddr_fromin(&ina_c, &na_c);
	isc_netaddr_fromin(&ina_d, &na_d);

	assert_int_equal(isc_netaddr_masktoprefixlen(&ina_a, &plen),
			 ISC_R_SUCCESS);
	assert_int_equal(plen, 0);

	assert_int_equal(isc_netaddr_masktoprefixlen(&ina_b, &plen),
		     ISC_R_SUCCESS);
	assert_int_equal(plen, 31);

	assert_int_equal(isc_netaddr_masktoprefixlen(&ina_c, &plen),
			 ISC_R_SUCCESS);
	assert_int_equal(plen, 32);

	assert_int_equal(isc_netaddr_masktoprefixlen(&ina_d, &plen),
			 ISC_R_SUCCESS);
	assert_int_equal(plen, 24);
}

/* check multicast addresses are detected properly */
static void
netaddr_multicast(void **state) {
	unsigned int i;
	struct {
		int family;
		const char *addr;
		bool is_multicast;
	} tests[] = {
		{ AF_INET, "1.2.3.4", false },
		{ AF_INET, "4.3.2.1", false },
		{ AF_INET, "224.1.1.1", true },
		{ AF_INET, "1.1.1.244", false },
		{ AF_INET6, "::1", false },
		{ AF_INET6, "ff02::1", true }
	};

	UNUSED(state);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		isc_netaddr_t na;
		struct in_addr in;
		struct in6_addr in6;
		int r;

		if (tests[i].family == AF_INET) {
			r = inet_pton(AF_INET, tests[i].addr,
				      (unsigned char *)&in);
			assert_int_equal(r, 1);
			isc_netaddr_fromin(&na, &in);
		} else {
			r = inet_pton(AF_INET6, tests[i].addr,
				      (unsigned char *)&in6);
			assert_int_equal(r, 1);
			isc_netaddr_fromin6(&na, &in6);
		}

		assert_int_equal(isc_netaddr_ismulticast(&na),
				 tests[i].is_multicast);
	}
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(netaddr_isnetzero),
		cmocka_unit_test(netaddr_masktoprefixlen),
		cmocka_unit_test(netaddr_multicast),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
