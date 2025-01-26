/*	$NetBSD: nametree_test.c,v 1.1.1.1 2025/01/26 16:12:37 christos Exp $	*/

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/md.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/nametree.h>

#include <dst/dst.h>

#include <tests/dns.h>

dns_nametree_t *booltree = NULL;
dns_nametree_t *bitstree = NULL;
dns_nametree_t *counttree = NULL;

/*
 * Test utilities.  In general, these assume input parameters are valid
 * (checking with assert_int_equal, thus aborting if not) and unlikely run time
 * errors (such as memory allocation failure) won't happen.  This helps keep
 * the test code concise.
 */

/* Common setup: create trees of each type with a few keys */
static int
setup(void **state ISC_ATTR_UNUSED) {
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	dns_nametree_create(mctx, DNS_NAMETREE_BOOL, "bool test", &booltree);
	dns_nametree_create(mctx, DNS_NAMETREE_BITS, "bits test", &bitstree);
	dns_nametree_create(mctx, DNS_NAMETREE_COUNT, "count test", &counttree);

	/* Add a positive boolean node */
	dns_test_namefromstring("example.com.", &fn);
	assert_int_equal(dns_nametree_add(booltree, name, true), ISC_R_SUCCESS);

	/* Add assorted bits to a bitfield node */
	assert_int_equal(dns_nametree_add(bitstree, name, 1), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(bitstree, name, 9), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(bitstree, name, 53), ISC_R_SUCCESS);

	/* Add negative boolean nodes with and without parents */
	dns_test_namefromstring("negative.example.com.", &fn);
	assert_int_equal(dns_nametree_add(booltree, name, false),
			 ISC_R_SUCCESS);
	dns_test_namefromstring("negative.example.org.", &fn);
	assert_int_equal(dns_nametree_add(booltree, name, false),
			 ISC_R_SUCCESS);

	/* Add a bitfield node under a parent */
	dns_test_namefromstring("sub.example.com.", &fn);
	assert_int_equal(dns_nametree_add(bitstree, name, 2), ISC_R_SUCCESS);

	return 0;
}

static int
teardown(void **state ISC_ATTR_UNUSED) {
	dns_nametree_detach(&booltree);
	dns_nametree_detach(&bitstree);
	dns_nametree_detach(&counttree);
	rcu_barrier();
	return 0;
}

ISC_RUN_TEST_IMPL(add_bool) {
	dns_ntnode_t *node = NULL;
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/*
	 * Getting the node for example.com should succeed.
	 */
	dns_test_namefromstring("example.com.", &fn);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);

	/*
	 * Try to add the same name.  This should fail.
	 */
	assert_int_equal(dns_nametree_add(booltree, name, false), ISC_R_EXISTS);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);

	/*
	 * Try to add a new name.
	 */
	dns_test_namefromstring("newname.com.", &fn);
	assert_int_equal(dns_nametree_add(booltree, name, true), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);
}

ISC_RUN_TEST_IMPL(add_bits) {
	dns_ntnode_t *node = NULL;
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/*
	 * Getting the node for example.com should succeed.
	 */
	dns_test_namefromstring("example.com.", &fn);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);

	/*
	 * Try to add the same name. This should succeed.
	 */
	assert_int_equal(dns_nametree_add(bitstree, name, 1), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(bitstree, name, 2), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(bitstree, name, 3), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);

	/*
	 * Try to add a new name.
	 */
	dns_test_namefromstring("newname.com.", &fn);
	assert_int_equal(dns_nametree_add(booltree, name, true), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);
}

ISC_RUN_TEST_IMPL(add_count) {
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/* add a counter node five times */
	dns_test_namefromstring("example.com.", &fn);
	assert_int_equal(dns_nametree_add(counttree, name, 0), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(counttree, name, 0), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(counttree, name, 0), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(counttree, name, 0), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_add(counttree, name, 0), ISC_R_SUCCESS);

	/* delete it five times, checking coverage each time */
	assert_true(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_SUCCESS);

	assert_true(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_SUCCESS);

	assert_true(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_SUCCESS);

	assert_true(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_SUCCESS);

	assert_true(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_SUCCESS);

	assert_false(dns_nametree_covered(counttree, name, NULL, 0));
	assert_int_equal(dns_nametree_delete(counttree, name), ISC_R_NOTFOUND);
}

ISC_RUN_TEST_IMPL(covered_bool) {
	dns_fixedname_t fn, fn2;
	dns_name_t *name = dns_fixedname_initname(&fn);
	dns_name_t *found = dns_fixedname_initname(&fn2);
	char buf[DNS_NAME_FORMATSIZE];
	const char *yesnames[] = { "example.com.", "sub.example.com.", NULL };
	const char *nonames[] = { "whatever.com.", "negative.example.com.",
				  "example.org.", "negative.example.org.",
				  NULL };

	for (const char **n = yesnames; *n != NULL; n++) {
		dns_test_namefromstring(*n, &fn);
		assert_true(dns_nametree_covered(booltree, name, NULL, 0));
	}
	for (const char **n = nonames; *n != NULL; n++) {
		dns_test_namefromstring(*n, &fn);
		assert_false(dns_nametree_covered(booltree, name, NULL, 0));
	}

	/* Check that the found name is as expected */
	dns_test_namefromstring("other.example.com.", &fn);
	assert_true(dns_nametree_covered(booltree, name, found, 0));
	dns_name_format(found, buf, sizeof(buf));
	assert_string_equal(buf, "example.com");
}

ISC_RUN_TEST_IMPL(covered_bits) {
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/* check existing bit values */
	dns_test_namefromstring("example.com.", &fn);
	assert_false(dns_nametree_covered(bitstree, name, NULL, 0));
	assert_true(dns_nametree_covered(bitstree, name, NULL, 1));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 2));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 3));
	assert_true(dns_nametree_covered(bitstree, name, NULL, 9));
	assert_true(dns_nametree_covered(bitstree, name, NULL, 53));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 288));

	/* add a small bit value, test again */
	assert_int_equal(dns_nametree_add(bitstree, name, 3), ISC_R_SUCCESS);
	assert_true(dns_nametree_covered(bitstree, name, NULL, 3));

	/* add a large bit value, test again */
	assert_false(dns_nametree_covered(bitstree, name, NULL, 615));
	assert_int_equal(dns_nametree_add(bitstree, name, 615), ISC_R_SUCCESS);
	assert_true(dns_nametree_covered(bitstree, name, NULL, 615));
	assert_int_equal(dns_nametree_add(bitstree, name, 999), ISC_R_SUCCESS);
	assert_true(dns_nametree_covered(bitstree, name, NULL, 999));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 998));

	/* check existing bit values for subdomain */
	dns_test_namefromstring("sub.example.com.", &fn);
	assert_false(dns_nametree_covered(bitstree, name, NULL, 0));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 1));
	assert_true(dns_nametree_covered(bitstree, name, NULL, 2));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 3));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 9));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 53));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 288));

	/* check nonexistent subdomain is all false */
	dns_test_namefromstring("other.example.com", &fn);
	assert_false(dns_nametree_covered(bitstree, name, NULL, 0));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 1));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 2));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 3));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 9));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 53));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 288));

	/* check nonexistent domain is all false */
	dns_test_namefromstring("anyname.", &fn);
	assert_false(dns_nametree_covered(bitstree, name, NULL, 0));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 1));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 2));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 3));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 9));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 53));
	assert_false(dns_nametree_covered(bitstree, name, NULL, 288));
}

ISC_RUN_TEST_IMPL(delete) {
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/* name doesn't match */
	dns_test_namefromstring("example.org.", &fn);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_NOTFOUND);

	/* subdomain match is the same as no match */
	dns_test_namefromstring("sub.example.org.", &fn);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_NOTFOUND);

	/*
	 * delete requires exact match: this should return SUCCESS on
	 * the first try, then NOTFOUND on the second even though an
	 * ancestor does exist.
	 */
	dns_test_namefromstring("negative.example.com.", &fn);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_NOTFOUND);

	dns_test_namefromstring("negative.example.org.", &fn);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_SUCCESS);
	assert_int_equal(dns_nametree_delete(booltree, name), ISC_R_NOTFOUND);
}

ISC_RUN_TEST_IMPL(find) {
	dns_ntnode_t *node = NULL;
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_name(&fn);

	/*
	 * dns_nametree_find() requires exact name match.  It matches node
	 * that has a null key, too.
	 */
	dns_test_namefromstring("example.org.", &fn);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_NOTFOUND);
	dns_test_namefromstring("sub.example.com.", &fn);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_NOTFOUND);
	dns_test_namefromstring("example.com.", &fn);
	assert_int_equal(dns_nametree_find(booltree, name, &node),
			 ISC_R_SUCCESS);
	dns_ntnode_detach(&node);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(add_bool, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(add_bits, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(add_count, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(covered_bool, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(covered_bits, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(delete, setup, teardown)
ISC_TEST_ENTRY_CUSTOM(find, setup, teardown)
ISC_TEST_LIST_END

ISC_TEST_MAIN
