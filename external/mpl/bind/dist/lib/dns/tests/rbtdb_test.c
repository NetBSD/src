/*	$NetBSD: rbtdb_test.c,v 1.3 2022/09/23 12:15:32 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/rbt.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>

#include "dnstest.h"

/* Include the main file */

#include "../rbtdb.c"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

const char *ownercase_vectors[12][2] = {
	{
		"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
		"AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ",
	},
	{
		"AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVxXyYzZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvxxyyzz",
	},
	{
		"WwW.ExAmPlE.OrG",
		"wWw.eXaMpLe.oRg",
	},
	{
		"_SIP.tcp.example.org",
		"_sip.TCP.example.org",
	},
	{
		"bind-USERS.lists.example.org",
		"bind-users.lists.example.org",
	},
	{
		"a0123456789.example.org",
		"A0123456789.example.org",
	},
	{
		"\\000.example.org",
		"\\000.example.org",
	},
	{
		"wWw.\\000.isc.org",
		"www.\\000.isc.org",
	},
	{
		"\255.example.org",
		"\255.example.ORG",
	}
};

static bool
ownercase_test_one(const char *str1, const char *str2) {
	isc_result_t result;
	rbtdb_nodelock_t node_locks[1];
	dns_rbtdb_t rbtdb = { .node_locks = node_locks };
	dns_rbtnode_t rbtnode = { .locknum = 0 };
	rdatasetheader_t header = { 0 };
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.private1 = &rbtdb,
		.private2 = &rbtnode,
		.private3 = raw,
		.methods = &rdataset_methods,
	};

	isc_buffer_t b;
	dns_fixedname_t fname1, fname2;
	dns_name_t *name1, *name2;

	memset(node_locks, 0, sizeof(node_locks));
	/* Minimal initialization of the mock objects */
	NODE_INITLOCK(&rbtdb.node_locks[0].lock);

	name1 = dns_fixedname_initname(&fname1);
	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name1, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	name2 = dns_fixedname_initname(&fname2);
	isc_buffer_constinit(&b, str2, strlen(str2));
	isc_buffer_add(&b, strlen(str2));
	result = dns_name_fromtext(name2, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Store the case from name1 */
	dns_rdataset_setownercase(&rdataset, name1);

	assert_true(CASESET(&header));

	/* Retrieve the case to name2 */
	dns_rdataset_getownercase(&rdataset, name2);

	NODE_DESTROYLOCK(&rbtdb.node_locks[0].lock);

	return (dns_name_caseequal(name1, name2));
}

static void
ownercase_test(void **state) {
	UNUSED(state);

	for (size_t n = 0; n < ARRAY_SIZE(ownercase_vectors); n++) {
		assert_true(ownercase_test_one(ownercase_vectors[n][0],
					       ownercase_vectors[n][1]));
	}

	assert_false(ownercase_test_one("W.example.org", "\\000.example.org"));

	/* Ö and ö in ISO Latin 1 */
	assert_false(ownercase_test_one("\\216", "\\246"));
}

static void
setownercase_test(void **state) {
	isc_result_t result;
	rbtdb_nodelock_t node_locks[1];
	dns_rbtdb_t rbtdb = { .node_locks = node_locks };
	dns_rbtnode_t rbtnode = { .locknum = 0 };
	rdatasetheader_t header = { 0 };
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.private1 = &rbtdb,
		.private2 = &rbtnode,
		.private3 = raw,
		.methods = &rdataset_methods,
	};
	const char *str1 =
		"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

	isc_buffer_t b;
	dns_fixedname_t fname1, fname2;
	dns_name_t *name1, *name2;

	UNUSED(state);

	/* Minimal initialization of the mock objects */
	memset(node_locks, 0, sizeof(node_locks));
	NODE_INITLOCK(&rbtdb.node_locks[0].lock);

	name1 = dns_fixedname_initname(&fname1);
	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name1, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	name2 = dns_fixedname_initname(&fname2);
	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name2, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(CASESET(&header));

	/* Retrieve the case to name2 */
	dns_rdataset_getownercase(&rdataset, name2);

	NODE_DESTROYLOCK(&rbtdb.node_locks[0].lock);

	assert_true(dns_name_caseequal(name1, name2));
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(ownercase_test),
		cmocka_unit_test(setownercase_test),
	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
