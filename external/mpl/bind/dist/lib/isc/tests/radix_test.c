/*	$NetBSD: radix_test.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/radix.h>
#include <isc/result.h>
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

/* test radix searching */
static void
isc_radix_search_test(void **state) {
	isc_radix_tree_t *radix = NULL;
	isc_radix_node_t *node;
	isc_prefix_t prefix;
	isc_result_t result;
	struct in_addr in_addr;
	isc_netaddr_t netaddr;

	UNUSED(state);

	result = isc_radix_create(test_mctx, &radix, 32);
	assert_int_equal(result, ISC_R_SUCCESS);

	in_addr.s_addr = inet_addr("3.3.3.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 24);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	node->data[0] = (void *)1;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("3.3.0.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 16);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	node->data[0] = (void *)2;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("3.3.3.3");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 22);

	node = NULL;
	result = isc_radix_search(radix, &node, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_ptr_equal(node->data[0], (void *)2);

	isc_refcount_destroy(&prefix.refcount);

	isc_radix_destroy(radix, NULL);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(isc_radix_search_test, _setup,
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
