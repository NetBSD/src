/*	$NetBSD: rbt_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/rbt.h>

#include <dst/dst.h>

#include <tests/dns.h>

typedef struct {
	dns_rbt_t *rbt;
	dns_rbt_t *rbt_distances;
} test_context_t;

/* The initial structure of domain tree will be as follows:
 *
 *	       .
 *	       |
 *	       b
 *	     /	 \
 *	    a	 d.e.f
 *		/  |   \
 *	       c   |	g.h
 *		   |	 |
 *		  w.y	 i
 *		/  |  \	  \
 *	       x   |   z   k
 *		   |   |
 *		   p   j
 *		 /   \
 *		o     q
 */

/* The full absolute names of the nodes in the tree (the tree also
 * contains "." which is not included in this list).
 */
static const char *const domain_names[] = {
	"c",	     "b",	    "a",	   "x.d.e.f",
	"z.d.e.f",   "g.h",	    "i.g.h",	   "o.w.y.d.e.f",
	"j.z.d.e.f", "p.w.y.d.e.f", "q.w.y.d.e.f", "k.g.h"
};

static const size_t domain_names_count =
	(sizeof(domain_names) / sizeof(domain_names[0]));

/* These are set as the node data for the tree used in distances check
 * (for the names in domain_names[] above).
 */
static const int node_distances[] = { 3, 1, 2, 2, 2, 3, 1, 2, 1, 1, 2, 2 };

/*
 * The domain order should be:
 * ., a, b, c, d.e.f, x.d.e.f, w.y.d.e.f, o.w.y.d.e.f, p.w.y.d.e.f,
 * q.w.y.d.e.f, z.d.e.f, j.z.d.e.f, g.h, i.g.h, k.g.h
 *	       . (no data, can't be found)
 *	       |
 *	       b
 *	     /	 \
 *	    a	 d.e.f
 *		/  |   \
 *	       c   |	g.h
 *		   |	 |
 *		  w.y	 i
 *		/  |  \	  \
 *	       x   |   z   k
 *		   |   |
 *		   p   j
 *		 /   \
 *		o     q
 */

static const char *const ordered_names[] = {
	"a",	     "b",	    "c",	   "d.e.f",	  "x.d.e.f",
	"w.y.d.e.f", "o.w.y.d.e.f", "p.w.y.d.e.f", "q.w.y.d.e.f", "z.d.e.f",
	"j.z.d.e.f", "g.h",	    "i.g.h",	   "k.g.h"
};

static const size_t ordered_names_count =
	(sizeof(ordered_names) / sizeof(*ordered_names));

static void
delete_data(void *data, void *arg) {
	UNUSED(arg);

	isc_mem_put(mctx, data, sizeof(size_t));
}

static test_context_t *
test_context_setup(void) {
	test_context_t *ctx;
	isc_result_t result;
	size_t i;

	ctx = isc_mem_get(mctx, sizeof(*ctx));
	assert_non_null(ctx);

	ctx->rbt = NULL;
	result = dns_rbt_create(mctx, delete_data, NULL, &ctx->rbt);
	assert_int_equal(result, ISC_R_SUCCESS);

	ctx->rbt_distances = NULL;
	result = dns_rbt_create(mctx, delete_data, NULL, &ctx->rbt_distances);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (i = 0; i < domain_names_count; i++) {
		size_t *n;
		dns_fixedname_t fname;
		dns_name_t *name;

		dns_test_namefromstring(domain_names[i], &fname);

		name = dns_fixedname_name(&fname);

		n = isc_mem_get(mctx, sizeof(size_t));
		assert_non_null(n);
		*n = i + 1;
		result = dns_rbt_addname(ctx->rbt, name, n);
		assert_int_equal(result, ISC_R_SUCCESS);

		n = isc_mem_get(mctx, sizeof(size_t));
		assert_non_null(n);
		*n = node_distances[i];
		result = dns_rbt_addname(ctx->rbt_distances, name, n);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	return (ctx);
}

static void
test_context_teardown(test_context_t *ctx) {
	dns_rbt_destroy(&ctx->rbt);
	dns_rbt_destroy(&ctx->rbt_distances);

	isc_mem_put(mctx, ctx, sizeof(*ctx));
}

/*
 * Walk the tree and ensure that all the test nodes are present.
 */
static void
check_test_data(dns_rbt_t *rbt) {
	dns_fixedname_t fixed;
	isc_result_t result;
	dns_name_t *foundname;
	size_t i;

	foundname = dns_fixedname_initname(&fixed);

	for (i = 0; i < domain_names_count; i++) {
		dns_fixedname_t fname;
		dns_name_t *name;
		size_t *n;

		dns_test_namefromstring(domain_names[i], &fname);

		name = dns_fixedname_name(&fname);
		n = NULL;
		result = dns_rbt_findname(rbt, name, 0, foundname, (void *)&n);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_int_equal(*n, i + 1);
	}
}

/* Test the creation of an rbt */
ISC_RUN_TEST_IMPL(rbt_create) {
	test_context_t *ctx;
	bool tree_ok;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	check_test_data(ctx->rbt);

	tree_ok = dns__rbt_checkproperties(ctx->rbt);
	assert_true(tree_ok);

	test_context_teardown(ctx);
}

/* Test dns_rbt_nodecount() on a tree */
ISC_RUN_TEST_IMPL(rbt_nodecount) {
	test_context_t *ctx;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	assert_int_equal(15, dns_rbt_nodecount(ctx->rbt));

	test_context_teardown(ctx);
}

/* Test dns_rbtnode_get_distance() on a tree */
ISC_RUN_TEST_IMPL(rbtnode_get_distance) {
	isc_result_t result;
	test_context_t *ctx;
	const char *name_str = "a";
	dns_fixedname_t fname;
	dns_name_t *name;
	dns_rbtnode_t *node = NULL;
	dns_rbtnodechain_t chain;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	dns_test_namefromstring(name_str, &fname);
	name = dns_fixedname_name(&fname);

	dns_rbtnodechain_init(&chain);

	result = dns_rbt_findnode(ctx->rbt_distances, name, NULL, &node, &chain,
				  0, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (node != NULL) {
		const size_t *distance = (const size_t *)node->data;
		if (distance != NULL) {
			assert_int_equal(*distance,
					 dns__rbtnode_getdistance(node));
		}
		result = dns_rbtnodechain_next(&chain, NULL, NULL);
		if (result == ISC_R_NOMORE) {
			break;
		}
		dns_rbtnodechain_current(&chain, NULL, NULL, &node);
	}

	assert_int_equal(result, ISC_R_NOMORE);

	dns_rbtnodechain_invalidate(&chain);

	test_context_teardown(ctx);
}

/*
 * Test tree balance, inserting names in random order.
 *
 * This test checks an important performance-related property of
 * the red-black tree, which is important for us: the longest
 * path from a sub-tree's root to a node is no more than
 * 2log(n). This check verifies that the tree is balanced.
 */
ISC_RUN_TEST_IMPL(rbt_check_distance_random) {
	dns_rbt_t *mytree = NULL;
	const unsigned int log_num_nodes = 16;
	isc_result_t result;
	bool tree_ok;
	int i;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	result = dns_rbt_create(mctx, delete_data, NULL, &mytree);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Names are inserted in random order. */

	/* Make a large 65536 node top-level domain tree, i.e., the
	 * following code inserts names such as:
	 *
	 * savoucnsrkrqzpkqypbygwoiliawpbmz.
	 * wkadamcbbpjtundbxcmuayuycposvngx.
	 * wzbpznemtooxdpjecdxynsfztvnuyfao.
	 * yueojmhyffslpvfmgyfwioxegfhepnqq.
	 */
	for (i = 0; i < (1 << log_num_nodes); i++) {
		size_t *n;
		char namebuf[34];

		n = isc_mem_get(mctx, sizeof(size_t));
		assert_non_null(n);
		*n = i + 1;

		while (1) {
			int j;
			dns_fixedname_t fname;
			dns_name_t *name;

			for (j = 0; j < 32; j++) {
				uint32_t v = isc_random_uniform(26);
				namebuf[j] = 'a' + v;
			}
			namebuf[32] = '.';
			namebuf[33] = 0;

			dns_test_namefromstring(namebuf, &fname);
			name = dns_fixedname_name(&fname);

			result = dns_rbt_addname(mytree, name, n);
			if (result == ISC_R_SUCCESS) {
				break;
			}
		}
	}

	/* 1 (root . node) + (1 << log_num_nodes) */
	assert_int_equal(1U + (1U << log_num_nodes), dns_rbt_nodecount(mytree));

	/* The distance from each node to its sub-tree root must be less
	 * than 2 * log(n).
	 */
	assert_true((2U * log_num_nodes) >= dns__rbt_getheight(mytree));

	/* Also check RB tree properties */
	tree_ok = dns__rbt_checkproperties(mytree);
	assert_true(tree_ok);

	dns_rbt_destroy(&mytree);
}

/*
 * Test tree balance, inserting names in sorted order.
 *
 * This test checks an important performance-related property of
 * the red-black tree, which is important for us: the longest
 * path from a sub-tree's root to a node is no more than
 * 2log(n). This check verifies that the tree is balanced.
 */
ISC_RUN_TEST_IMPL(rbt_check_distance_ordered) {
	dns_rbt_t *mytree = NULL;
	const unsigned int log_num_nodes = 16;
	isc_result_t result;
	bool tree_ok;
	int i;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	result = dns_rbt_create(mctx, delete_data, NULL, &mytree);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Names are inserted in sorted order. */

	/* Make a large 65536 node top-level domain tree, i.e., the
	 * following code inserts names such as:
	 *
	 *   name00000000.
	 *   name00000001.
	 *   name00000002.
	 *   name00000003.
	 */
	for (i = 0; i < (1 << log_num_nodes); i++) {
		size_t *n;
		char namebuf[14];
		dns_fixedname_t fname;
		dns_name_t *name;

		n = isc_mem_get(mctx, sizeof(size_t));
		assert_non_null(n);
		*n = i + 1;

		snprintf(namebuf, sizeof(namebuf), "name%08x.", i);
		dns_test_namefromstring(namebuf, &fname);
		name = dns_fixedname_name(&fname);

		result = dns_rbt_addname(mytree, name, n);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	/* 1 (root . node) + (1 << log_num_nodes) */
	assert_int_equal(1U + (1U << log_num_nodes), dns_rbt_nodecount(mytree));

	/* The distance from each node to its sub-tree root must be less
	 * than 2 * log(n).
	 */
	assert_true((2U * log_num_nodes) >= dns__rbt_getheight(mytree));

	/* Also check RB tree properties */
	tree_ok = dns__rbt_checkproperties(mytree);
	assert_true(tree_ok);

	dns_rbt_destroy(&mytree);
}

static isc_result_t
insert_helper(dns_rbt_t *rbt, const char *namestr, dns_rbtnode_t **node) {
	dns_fixedname_t fname;
	dns_name_t *name;

	dns_test_namefromstring(namestr, &fname);
	name = dns_fixedname_name(&fname);

	return (dns_rbt_addnode(rbt, name, node));
}

static bool
compare_labelsequences(dns_rbtnode_t *node, const char *labelstr) {
	dns_name_t name;
	isc_result_t result;
	char *nodestr = NULL;
	bool is_equal;

	dns_name_init(&name, NULL);
	dns_rbt_namefromnode(node, &name);

	result = dns_name_tostring(&name, &nodestr, mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	is_equal = strcmp(labelstr, nodestr) == 0 ? true : false;

	isc_mem_free(mctx, nodestr);

	return (is_equal);
}

/* Test insertion into a tree */
ISC_RUN_TEST_IMPL(rbt_insert) {
	isc_result_t result;
	test_context_t *ctx;
	dns_rbtnode_t *node;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	/* Check node count before beginning. */
	assert_int_equal(15, dns_rbt_nodecount(ctx->rbt));

	/* Try to insert a node that already exists. */
	node = NULL;
	result = insert_helper(ctx->rbt, "d.e.f", &node);
	assert_int_equal(result, ISC_R_EXISTS);

	/* Node count must not have changed. */
	assert_int_equal(15, dns_rbt_nodecount(ctx->rbt));

	/* Try to insert a node that doesn't exist. */
	node = NULL;
	result = insert_helper(ctx->rbt, "0", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "0"));

	/* Node count must have increased. */
	assert_int_equal(16, dns_rbt_nodecount(ctx->rbt));

	/* Another. */
	node = NULL;
	result = insert_helper(ctx->rbt, "example.com", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(node);
	assert_null(node->data);

	/* Node count must have increased. */
	assert_int_equal(17, dns_rbt_nodecount(ctx->rbt));

	/* Re-adding it should return EXISTS */
	node = NULL;
	result = insert_helper(ctx->rbt, "example.com", &node);
	assert_int_equal(result, ISC_R_EXISTS);

	/* Node count must not have changed. */
	assert_int_equal(17, dns_rbt_nodecount(ctx->rbt));

	/* Fission the node d.e.f */
	node = NULL;
	result = insert_helper(ctx->rbt, "k.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "k"));

	/* Node count must have incremented twice ("d.e.f" fissioned to
	 * "d" and "e.f", and the newly added "k").
	 */
	assert_int_equal(19, dns_rbt_nodecount(ctx->rbt));

	/* Fission the node "g.h" */
	node = NULL;
	result = insert_helper(ctx->rbt, "h", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "h"));

	/* Node count must have incremented ("g.h" fissioned to "g" and
	 * "h").
	 */
	assert_int_equal(20, dns_rbt_nodecount(ctx->rbt));

	/* Add child domains */

	node = NULL;
	result = insert_helper(ctx->rbt, "m.p.w.y.d.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "m"));
	assert_int_equal(21, dns_rbt_nodecount(ctx->rbt));

	node = NULL;
	result = insert_helper(ctx->rbt, "n.p.w.y.d.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "n"));
	assert_int_equal(22, dns_rbt_nodecount(ctx->rbt));

	node = NULL;
	result = insert_helper(ctx->rbt, "l.a", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(compare_labelsequences(node, "l"));
	assert_int_equal(23, dns_rbt_nodecount(ctx->rbt));

	node = NULL;
	result = insert_helper(ctx->rbt, "r.d.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	node = NULL;
	result = insert_helper(ctx->rbt, "s.d.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(25, dns_rbt_nodecount(ctx->rbt));

	node = NULL;
	result = insert_helper(ctx->rbt, "h.w.y.d.e.f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Add more nodes one by one to cover left and right rotation
	 * functions.
	 */
	node = NULL;
	result = insert_helper(ctx->rbt, "f", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "m", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "nm", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "om", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "k", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "l", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "fe", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "ge", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "i", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "ae", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	node = NULL;
	result = insert_helper(ctx->rbt, "n", &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	test_context_teardown(ctx);
}

/*
 * Test removal from a tree
 *
 * This testcase checks that after node removal, the binary-search tree is
 * valid and all nodes that are supposed to exist are present in the
 * correct order. It mainly tests DomainTree as a BST, and not particularly
 * as a red-black tree. This test checks node deletion when upper nodes
 * have data.
 */
ISC_RUN_TEST_IMPL(rbt_remove) {
	isc_result_t result;
	size_t j;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	/*
	 * Delete single nodes and check if the rest of the nodes exist.
	 */
	for (j = 0; j < ordered_names_count; j++) {
		dns_rbt_t *mytree = NULL;
		dns_rbtnode_t *node;
		size_t i;
		size_t *n;
		bool tree_ok;
		dns_rbtnodechain_t chain;
		size_t start_node;

		/* Create a tree. */
		result = dns_rbt_create(mctx, delete_data, NULL, &mytree);
		assert_int_equal(result, ISC_R_SUCCESS);

		/* Insert test data into the tree. */
		for (i = 0; i < domain_names_count; i++) {
			node = NULL;
			result = insert_helper(mytree, domain_names[i], &node);
			assert_int_equal(result, ISC_R_SUCCESS);
		}

		/* Check that all names exist in order. */
		for (i = 0; i < ordered_names_count; i++) {
			dns_fixedname_t fname;
			dns_name_t *name;

			dns_test_namefromstring(ordered_names[i], &fname);

			name = dns_fixedname_name(&fname);
			node = NULL;
			result = dns_rbt_findnode(mytree, name, NULL, &node,
						  NULL, DNS_RBTFIND_EMPTYDATA,
						  NULL, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);

			/* Add node data */
			assert_non_null(node);
			assert_null(node->data);

			n = isc_mem_get(mctx, sizeof(size_t));
			assert_non_null(n);
			*n = i;

			node->data = n;
		}

		/* Now, delete the j'th node from the tree. */
		{
			dns_fixedname_t fname;
			dns_name_t *name;

			dns_test_namefromstring(ordered_names[j], &fname);

			name = dns_fixedname_name(&fname);

			result = dns_rbt_deletename(mytree, name, false);
			assert_int_equal(result, ISC_R_SUCCESS);
		}

		/* Check RB tree properties. */
		tree_ok = dns__rbt_checkproperties(mytree);
		assert_true(tree_ok);

		dns_rbtnodechain_init(&chain);

		/* Now, walk through nodes in order. */
		if (j == 0) {
			/*
			 * Node for ordered_names[0] was already deleted
			 * above. We start from node 1.
			 */
			dns_fixedname_t fname;
			dns_name_t *name;

			dns_test_namefromstring(ordered_names[0], &fname);
			name = dns_fixedname_name(&fname);
			node = NULL;
			result = dns_rbt_findnode(mytree, name, NULL, &node,
						  NULL, 0, NULL, NULL);
			assert_int_equal(result, ISC_R_NOTFOUND);

			dns_test_namefromstring(ordered_names[1], &fname);
			name = dns_fixedname_name(&fname);
			node = NULL;
			result = dns_rbt_findnode(mytree, name, NULL, &node,
						  &chain, 0, NULL, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);
			start_node = 1;
		} else {
			/* Start from node 0. */
			dns_fixedname_t fname;
			dns_name_t *name;

			dns_test_namefromstring(ordered_names[0], &fname);
			name = dns_fixedname_name(&fname);
			node = NULL;
			result = dns_rbt_findnode(mytree, name, NULL, &node,
						  &chain, 0, NULL, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);
			start_node = 0;
		}

		/*
		 * node and chain have been set by the code above at
		 * this point.
		 */
		for (i = start_node; i < ordered_names_count; i++) {
			dns_fixedname_t fname_j, fname_i;
			dns_name_t *name_j, *name_i;

			dns_test_namefromstring(ordered_names[j], &fname_j);
			name_j = dns_fixedname_name(&fname_j);
			dns_test_namefromstring(ordered_names[i], &fname_i);
			name_i = dns_fixedname_name(&fname_i);

			if (dns_name_equal(name_i, name_j)) {
				/*
				 * This may be true for the last node if
				 * we seek ahead in the loop using
				 * dns_rbtnodechain_next() below.
				 */
				if (node == NULL) {
					break;
				}

				/* All ordered nodes have data
				 * initially. If any node is empty, it
				 * means it was removed, but an empty
				 * node exists because it is a
				 * super-domain. Just skip it.
				 */
				if (node->data == NULL) {
					result = dns_rbtnodechain_next(
						&chain, NULL, NULL);
					if (result == ISC_R_NOMORE) {
						node = NULL;
					} else {
						dns_rbtnodechain_current(
							&chain, NULL, NULL,
							&node);
					}
				}
				continue;
			}

			assert_non_null(node);

			n = (size_t *)node->data;
			if (n != NULL) {
				/* printf("n=%zu, i=%zu\n", *n, i); */
				assert_int_equal(*n, i);
			}

			result = dns_rbtnodechain_next(&chain, NULL, NULL);
			if (result == ISC_R_NOMORE) {
				node = NULL;
			} else {
				dns_rbtnodechain_current(&chain, NULL, NULL,
							 &node);
			}
		}

		/* We should have reached the end of the tree. */
		assert_null(node);

		dns_rbt_destroy(&mytree);
	}
}

static void
insert_nodes(dns_rbt_t *mytree, char **names, size_t *names_count,
	     uint32_t num_names) {
	uint32_t i;
	dns_rbtnode_t *node;

	for (i = 0; i < num_names; i++) {
		size_t *n;
		char namebuf[34];

		n = isc_mem_get(mctx, sizeof(size_t));
		assert_non_null(n);

		*n = i; /* Unused value */

		while (1) {
			int j;
			dns_fixedname_t fname;
			dns_name_t *name;
			isc_result_t result;

			for (j = 0; j < 32; j++) {
				uint32_t v = isc_random_uniform(26);
				namebuf[j] = 'a' + v;
			}
			namebuf[32] = '.';
			namebuf[33] = 0;

			dns_test_namefromstring(namebuf, &fname);
			name = dns_fixedname_name(&fname);

			node = NULL;
			result = dns_rbt_addnode(mytree, name, &node);
			if (result == ISC_R_SUCCESS) {
				node->data = n;
				names[*names_count] = isc_mem_strdup(mctx,
								     namebuf);
				assert_non_null(names[*names_count]);
				*names_count += 1;
				break;
			}
		}
	}
}

static void
remove_nodes(dns_rbt_t *mytree, char **names, size_t *names_count,
	     uint32_t num_names) {
	uint32_t i;

	UNUSED(mytree);

	for (i = 0; i < num_names; i++) {
		uint32_t node;
		dns_fixedname_t fname;
		dns_name_t *name;
		isc_result_t result;

		node = isc_random_uniform(*names_count);

		dns_test_namefromstring(names[node], &fname);
		name = dns_fixedname_name(&fname);

		result = dns_rbt_deletename(mytree, name, false);
		assert_int_equal(result, ISC_R_SUCCESS);

		isc_mem_free(mctx, names[node]);
		if (*names_count > 0) {
			names[node] = names[*names_count - 1];
			names[*names_count - 1] = NULL;
			*names_count -= 1;
		}
	}
}

static void
check_tree(dns_rbt_t *mytree, char **names, size_t names_count) {
	bool tree_ok;

	UNUSED(names);

	assert_int_equal(names_count + 1, dns_rbt_nodecount(mytree));

	/*
	 * The distance from each node to its sub-tree root must be less
	 * than 2 * log_2(1024).
	 */
	assert_true((2 * 10) >= dns__rbt_getheight(mytree));

	/* Also check RB tree properties */
	tree_ok = dns__rbt_checkproperties(mytree);
	assert_true(tree_ok);
}

/*
 * Test insert and remove in a loop.
 *
 * What is the best way to test our red-black tree code? It is
 * not a good method to test every case handled in the actual
 * code itself. This is because our approach itself may be
 * incorrect.
 *
 * We test our code at the interface level here by exercising the
 * tree randomly multiple times, checking that red-black tree
 * properties are valid, and all the nodes that are supposed to be
 * in the tree exist and are in order.
 *
 * NOTE: These tests are run within a single tree level in the
 * forest. The number of nodes in the tree level doesn't grow
 * over 1024.
 */
ISC_RUN_TEST_IMPL(rbt_insert_and_remove) {
	isc_result_t result;
	dns_rbt_t *mytree = NULL;
	size_t *n;
	char *names[1024];
	size_t names_count;
	int i;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	result = dns_rbt_create(mctx, delete_data, NULL, &mytree);
	assert_int_equal(result, ISC_R_SUCCESS);

	n = isc_mem_get(mctx, sizeof(size_t));
	assert_non_null(n);
	result = dns_rbt_addname(mytree, dns_rootname, n);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(names, 0, sizeof(names));
	names_count = 0;

	/* Repeat the insert/remove test some 4096 times */
	for (i = 0; i < 4096; i++) {
		uint32_t num_names;

		if (names_count < 1024) {
			num_names = isc_random_uniform(1024 - names_count);
			num_names++;
		} else {
			num_names = 0;
		}

		insert_nodes(mytree, names, &names_count, num_names);
		check_tree(mytree, names, names_count);

		if (names_count > 0) {
			num_names = isc_random_uniform(names_count);
			num_names++;
		} else {
			num_names = 0;
		}

		remove_nodes(mytree, names, &names_count, num_names);
		check_tree(mytree, names, names_count);
	}

	/* Remove the rest of the nodes */
	remove_nodes(mytree, names, &names_count, names_count);
	check_tree(mytree, names, names_count);

	for (i = 0; i < 1024; i++) {
		if (names[i] != NULL) {
			isc_mem_free(mctx, names[i]);
		}
	}

	result = dns_rbt_deletename(mytree, dns_rootname, false);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(dns_rbt_nodecount(mytree), 0);

	dns_rbt_destroy(&mytree);
}

/* Test findname return values */
ISC_RUN_TEST_IMPL(rbt_findname) {
	isc_result_t result;
	test_context_t *ctx = NULL;
	dns_fixedname_t fname, found;
	dns_name_t *name = NULL, *foundname = NULL;
	size_t *n = NULL;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	/* Try to find a name that exists. */
	dns_test_namefromstring("d.e.f", &fname);
	name = dns_fixedname_name(&fname);

	foundname = dns_fixedname_initname(&found);

	result = dns_rbt_findname(ctx->rbt, name, DNS_RBTFIND_EMPTYDATA,
				  foundname, (void *)&n);
	assert_true(dns_name_equal(foundname, name));
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Now without EMPTYDATA */
	result = dns_rbt_findname(ctx->rbt, name, 0, foundname, (void *)&n);
	assert_int_equal(result, ISC_R_NOTFOUND);

	/* Now one that partially matches */
	dns_test_namefromstring("d.e.f.g.h.i.j", &fname);
	name = dns_fixedname_name(&fname);
	result = dns_rbt_findname(ctx->rbt, name, DNS_RBTFIND_EMPTYDATA,
				  foundname, (void *)&n);
	assert_int_equal(result, DNS_R_PARTIALMATCH);

	/* Now one that doesn't match */
	dns_test_namefromstring("1.2", &fname);
	name = dns_fixedname_name(&fname);
	result = dns_rbt_findname(ctx->rbt, name, DNS_RBTFIND_EMPTYDATA,
				  foundname, (void *)&n);
	assert_int_equal(result, DNS_R_PARTIALMATCH);
	assert_true(dns_name_equal(foundname, dns_rootname));

	test_context_teardown(ctx);
}

/* Test addname return values */
ISC_RUN_TEST_IMPL(rbt_addname) {
	isc_result_t result;
	test_context_t *ctx = NULL;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	size_t *n;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	n = isc_mem_get(mctx, sizeof(size_t));
	assert_non_null(n);
	*n = 1;

	dns_test_namefromstring("d.e.f.g.h.i.j.k", &fname);
	name = dns_fixedname_name(&fname);

	/* Add a name that doesn't exist */
	result = dns_rbt_addname(ctx->rbt, name, n);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Now add again, should get ISC_R_EXISTS */
	n = isc_mem_get(mctx, sizeof(size_t));
	assert_non_null(n);
	*n = 2;
	result = dns_rbt_addname(ctx->rbt, name, n);
	assert_int_equal(result, ISC_R_EXISTS);
	isc_mem_put(mctx, n, sizeof(size_t));

	test_context_teardown(ctx);
}

/* Test deletename return values */
ISC_RUN_TEST_IMPL(rbt_deletename) {
	isc_result_t result;
	test_context_t *ctx = NULL;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	/* Delete a name that doesn't exist */
	dns_test_namefromstring("z.x.y.w", &fname);
	name = dns_fixedname_name(&fname);
	result = dns_rbt_deletename(ctx->rbt, name, false);
	assert_int_equal(result, ISC_R_NOTFOUND);

	/* Now one that does */
	dns_test_namefromstring("d.e.f", &fname);
	name = dns_fixedname_name(&fname);
	result = dns_rbt_deletename(ctx->rbt, name, false);
	assert_int_equal(result, ISC_R_NOTFOUND);

	test_context_teardown(ctx);
}

/* Test nodechain */
ISC_RUN_TEST_IMPL(rbt_nodechain) {
	isc_result_t result;
	test_context_t *ctx;
	dns_fixedname_t fname, found, expect;
	dns_name_t *name, *foundname, *expected;
	dns_rbtnode_t *node = NULL;
	dns_rbtnodechain_t chain;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	dns_rbtnodechain_init(&chain);

	dns_test_namefromstring("a", &fname);
	name = dns_fixedname_name(&fname);

	result = dns_rbt_findnode(ctx->rbt, name, NULL, &node, &chain, 0, NULL,
				  NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	foundname = dns_fixedname_initname(&found);

	dns_test_namefromstring("a", &expect);
	expected = dns_fixedname_name(&expect);
	UNUSED(expected);

	result = dns_rbtnodechain_first(&chain, ctx->rbt, foundname, NULL);
	assert_int_equal(result, DNS_R_NEWORIGIN);
	assert_int_equal(dns_name_countlabels(foundname), 0);

	result = dns_rbtnodechain_prev(&chain, NULL, NULL);
	assert_int_equal(result, ISC_R_NOMORE);

	result = dns_rbtnodechain_next(&chain, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_rbtnodechain_next(&chain, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_rbtnodechain_last(&chain, ctx->rbt, NULL, NULL);
	assert_int_equal(result, DNS_R_NEWORIGIN);

	result = dns_rbtnodechain_next(&chain, NULL, NULL);
	assert_int_equal(result, ISC_R_NOMORE);

	result = dns_rbtnodechain_last(&chain, ctx->rbt, NULL, NULL);
	assert_int_equal(result, DNS_R_NEWORIGIN);

	result = dns_rbtnodechain_prev(&chain, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rbtnodechain_invalidate(&chain);

	test_context_teardown(ctx);
}

/* Test addname return values */
ISC_RUN_TEST_IMPL(rbtnode_namelen) {
	isc_result_t result;
	test_context_t *ctx = NULL;
	dns_rbtnode_t *node;
	unsigned int len;

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	ctx = test_context_setup();

	node = NULL;
	result = insert_helper(ctx->rbt, ".", &node);
	len = dns__rbtnode_namelen(node);
	assert_int_equal(result, ISC_R_EXISTS);
	assert_int_equal(len, 1);
	node = NULL;

	result = insert_helper(ctx->rbt, "a.b.c.d.e.f.g.h.i.j.k.l.m", &node);
	len = dns__rbtnode_namelen(node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(len, 27);

	node = NULL;
	result = insert_helper(ctx->rbt, "isc.org", &node);
	len = dns__rbtnode_namelen(node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(len, 9);

	node = NULL;
	result = insert_helper(ctx->rbt, "example.com", &node);
	len = dns__rbtnode_namelen(node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(len, 13);

	test_context_teardown(ctx);
}

#if defined(DNS_BENCHMARK_TESTS) && !defined(__SANITIZE_THREAD__)

/*
 * XXXMUKS: Don't delete this code. It is useful in benchmarking the
 * RBT, but we don't require it as part of the unit test runs.
 */

static dns_fixedname_t *fnames;
static dns_name_t **names;
static int *values;

static void *
find_thread(void *arg) {
	dns_rbt_t *mytree;
	isc_result_t result;
	dns_rbtnode_t *node;
	unsigned int j, i;
	unsigned int start = 0;

	mytree = (dns_rbt_t *)arg;
	while (start == 0) {
		start = random() % 4000000;
	}

	/* Query 32 million random names from it in each thread */
	for (j = 0; j < 8; j++) {
		for (i = start; i != start - 1; i = (i + 1) % 4000000) {
			node = NULL;
			result = dns_rbt_findnode(mytree, names[i], NULL, &node,
						  NULL, DNS_RBTFIND_EMPTYDATA,
						  NULL, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_non_null(node);
			assert_int_equal(values[i], (intptr_t)node->data);
		}
	}

	return (NULL);
}

/* Benchmark RBT implementation */
ISC_RUN_TEST_IMPL(benchmark) {
	isc_result_t result;
	char namestr[sizeof("name18446744073709551616.example.org.")];
	unsigned int r;
	dns_rbt_t *mytree;
	dns_rbtnode_t *node;
	unsigned int i;
	unsigned int maxvalue = 1000000;
	isc_time_t ts1, ts2;
	double t;
	unsigned int nthreads;
	isc_thread_t threads[32];

	srandom(time(NULL));

	debug_mem_record = false;

	fnames = (dns_fixedname_t *)malloc(4000000 * sizeof(dns_fixedname_t));
	names = (dns_name_t **)malloc(4000000 * sizeof(dns_name_t *));
	values = (int *)malloc(4000000 * sizeof(int));

	for (i = 0; i < 4000000; i++) {
		r = ((unsigned long)random()) % maxvalue;
		snprintf(namestr, sizeof(namestr), "name%u.example.org.", r);
		dns_test_namefromstring(namestr, &fnames[i]);
		names[i] = dns_fixedname_name(&fnames[i]);
		values[i] = r;
	}

	/* Create a tree. */
	mytree = NULL;
	result = dns_rbt_create(mctx, NULL, NULL, &mytree);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Insert test data into the tree. */
	for (i = 0; i < maxvalue; i++) {
		snprintf(namestr, sizeof(namestr), "name%u.example.org.", i);
		node = NULL;
		result = insert_helper(mytree, namestr, &node);
		assert_int_equal(result, ISC_R_SUCCESS);
		node->data = (void *)(intptr_t)i;
	}

	result = isc_time_now(&ts1);
	assert_int_equal(result, ISC_R_SUCCESS);

	nthreads = ISC_MIN(isc_os_ncpus(), 32);
	nthreads = ISC_MAX(nthreads, 1);
	for (i = 0; i < nthreads; i++) {
		isc_thread_create(find_thread, mytree, &threads[i]);
	}

	for (i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	result = isc_time_now(&ts2);
	assert_int_equal(result, ISC_R_SUCCESS);

	t = isc_time_microdiff(&ts2, &ts1);

	printf("%u findnode calls, %f seconds, %f calls/second\n",
	       nthreads * 8 * 4000000, t / 1000000.0,
	       (nthreads * 8 * 4000000) / (t / 1000000.0));

	free(values);
	free(names);
	free(fnames);

	dns_rbt_destroy(&mytree);
}
#endif /* defined(DNS_BENCHMARK_TESTS) && !defined(__SANITIZE_THREAD__)  */

ISC_TEST_LIST_START
ISC_TEST_ENTRY(rbt_create)
ISC_TEST_ENTRY(rbt_nodecount)
ISC_TEST_ENTRY(rbtnode_get_distance)
ISC_TEST_ENTRY(rbt_check_distance_random)
ISC_TEST_ENTRY(rbt_check_distance_ordered)
ISC_TEST_ENTRY(rbt_insert)
ISC_TEST_ENTRY(rbt_remove)
ISC_TEST_ENTRY(rbt_insert_and_remove)
ISC_TEST_ENTRY(rbt_findname)
ISC_TEST_ENTRY(rbt_addname)
ISC_TEST_ENTRY(rbt_deletename)
ISC_TEST_ENTRY(rbt_nodechain)
ISC_TEST_ENTRY(rbtnode_namelen)
#if defined(DNS_BENCHMARK_TESTS) && !defined(__SANITIZE_THREAD__)
ISC_TEST_ENTRY(benchmark)
#endif /* defined(DNS_BENCHMARK_TESTS) && !defined(__SANITIZE_THREAD__) */

ISC_TEST_LIST_END

ISC_TEST_MAIN
