/*	$NetBSD: sockaddr_test.c,v 1.3.4.1 2019/09/12 19:18:16 martin Exp $	*/

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
#include <setjmp.h>

#include <sched.h> /* IWYU pragma: keep */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/netaddr.h>
#include <isc/sockaddr.h>
#include <isc/print.h>
#include <isc/util.h>

#include "isctest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

/* test sockaddr hash */
static void
sockaddr_hash(void **state) {
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
static void
sockaddr_isnetzero(void **state) {
	isc_sockaddr_t addr;
	struct in_addr in;
	struct in6_addr in6;
	bool r;
	int ret;
	size_t i;
	struct {
		const char *string;
		bool expect;
	} data4[] = {
		{ "0.0.0.0", true },
		{ "0.0.0.1", true },
		{ "0.0.1.0", true },
		{ "0.1.0.0", true },
		{ "1.0.0.0", false },
		{ "0.0.0.127", true },
		{ "0.0.0.255", true },
		{ "127.0.0.1", false },
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

	for (i = 0; i < sizeof(data4)/sizeof(data4[0]); i++) {
		in.s_addr = inet_addr(data4[i].string);
		isc_sockaddr_fromin(&addr, &in, 1);
		r = isc_sockaddr_isnetzero(&addr);
		assert_int_equal(r, data4[i].expect);
	}

	for (i = 0; i < sizeof(data6)/sizeof(data6[0]); i++) {
		ret = inet_pton(AF_INET6, data6[i].string, &in6);
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
static void
sockaddr_eqaddrprefix(void **state) {
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

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(sockaddr_hash,
						_setup, _teardown),
		cmocka_unit_test(sockaddr_isnetzero),
		cmocka_unit_test(sockaddr_eqaddrprefix),
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
