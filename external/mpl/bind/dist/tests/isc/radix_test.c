/*	$NetBSD: radix_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/radix.h>
#include <isc/result.h>
#include <isc/util.h>

#include <tests/isc.h>

/* test radix node removal */
ISC_RUN_TEST_IMPL(isc_radix_remove) {
	isc_radix_tree_t *radix = NULL;
	isc_radix_node_t *node;
	isc_prefix_t prefix;
	isc_result_t result;
	struct in_addr in_addr;
	isc_netaddr_t netaddr;

	UNUSED(state);

	result = isc_radix_create(mctx, &radix, 32);
	assert_int_equal(result, ISC_R_SUCCESS);

	in_addr.s_addr = inet_addr("1.1.1.1");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 32);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	node->data[0] = (void *)32;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("1.0.0.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 8);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	node->data[0] = (void *)8;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("1.1.1.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 24);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	assert_int_equal(result, ISC_R_SUCCESS);
	node->data[0] = (void *)24;
	isc_refcount_destroy(&prefix.refcount);

	isc_radix_remove(radix, node);

	isc_radix_destroy(radix, NULL);
}

/* test radix searching */
ISC_RUN_TEST_IMPL(isc_radix_search) {
	isc_radix_tree_t *radix = NULL;
	isc_radix_node_t *node;
	isc_prefix_t prefix;
	isc_result_t result;
	struct in_addr in_addr;
	isc_netaddr_t netaddr;

	UNUSED(state);

	result = isc_radix_create(mctx, &radix, 32);
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

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_radix_remove)
ISC_TEST_ENTRY(isc_radix_search)

ISC_TEST_LIST_END
ISC_TEST_MAIN
