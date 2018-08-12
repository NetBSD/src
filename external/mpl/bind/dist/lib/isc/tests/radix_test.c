/*	$NetBSD: radix_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/radix.h>
#include <isc/result.h>
#include <isc/util.h>

#include <atf-c.h>

#include <stdlib.h>
#include <stdint.h>

#include "isctest.h"

ATF_TC(isc_radix_search);
ATF_TC_HEAD(isc_radix_search, tc) {
	atf_tc_set_md_var(tc, "descr", "test radix seaching");
}
ATF_TC_BODY(isc_radix_search, tc) {
	isc_radix_tree_t *radix = NULL;
	isc_radix_node_t *node;
	isc_prefix_t prefix;
	isc_result_t result;
	struct in_addr in_addr;
	isc_netaddr_t netaddr;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_radix_create(mctx, &radix, 32);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	in_addr.s_addr = inet_addr("3.3.3.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 24, ISC_FALSE);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	node->data[0] = (void *)1;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("3.3.0.0");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 16, ISC_FALSE);

	node = NULL;
	result = isc_radix_insert(radix, &node, NULL, &prefix);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	node->data[0] = (void *)2;
	isc_refcount_destroy(&prefix.refcount);

	in_addr.s_addr = inet_addr("3.3.3.3");
	isc_netaddr_fromin(&netaddr, &in_addr);
	NETADDR_TO_PREFIX_T(&netaddr, prefix, 22, ISC_FALSE);

	node = NULL;
	result = isc_radix_search(radix, &node, &prefix);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(node->data[0], (void *)2);

	isc_refcount_destroy(&prefix.refcount);

	isc_radix_destroy(radix, NULL);

	isc_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_radix_search);

	return (atf_no_error());
}
