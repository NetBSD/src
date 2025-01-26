/*	$NetBSD: qpzone_test.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/qp.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdataslab.h>
#include <dns/rdatastruct.h>
#define KEEP_BEFORE

/* Include the main file */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#undef CHECK
#include "qpzone.c"
#pragma GCC diagnostic pop

#undef CHECK
#include <tests/dns.h>

#define CASESET(header)                                \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_CASESET) != 0)

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
	db_nodelock_t node_locks[1];
	qpzonedb_t qpdb = {
		.common.methods = &qpdb_zonemethods,
		.common.mctx = mctx,
		.node_locks = node_locks,
	};
	qpznode_t node = { .locknum = 0 };
	dns_slabheader_t header = {
		.node = &node,
		.db = (dns_db_t *)&qpdb,
	};
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.slab = { .db = (dns_db_t *)&qpdb, .node = &node, .raw = raw },
		.methods = &dns_rdataslab_rdatasetmethods,
	};
	isc_buffer_t b;
	dns_fixedname_t fname1, fname2;
	dns_name_t *name1 = dns_fixedname_initname(&fname1);
	dns_name_t *name2 = dns_fixedname_initname(&fname2);

	memset(node_locks, 0, sizeof(node_locks));
	/* Minimal initialization of the mock objects */
	NODE_INITLOCK(&qpdb.node_locks[0].lock);

	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name1, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_constinit(&b, str2, strlen(str2));
	isc_buffer_add(&b, strlen(str2));
	result = dns_name_fromtext(name2, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Store the case from name1 */
	dns_rdataset_setownercase(&rdataset, name1);

	assert_true(CASESET(&header));

	/* Retrieve the case to name2 */
	dns_rdataset_getownercase(&rdataset, name2);

	NODE_DESTROYLOCK(&qpdb.node_locks[0].lock);

	return dns_name_caseequal(name1, name2);
}

ISC_RUN_TEST_IMPL(ownercase) {
	UNUSED(state);

	for (size_t n = 0; n < ARRAY_SIZE(ownercase_vectors); n++) {
		assert_true(ownercase_test_one(ownercase_vectors[n][0],
					       ownercase_vectors[n][1]));
	}

	assert_false(ownercase_test_one("W.example.org", "\\000.example.org"));

	/* Ö and ö in ISO Latin 1 */
	assert_false(ownercase_test_one("\\216", "\\246"));
}

ISC_RUN_TEST_IMPL(setownercase) {
	isc_result_t result;
	db_nodelock_t node_locks[1];
	qpzonedb_t qpdb = {
		.common.methods = &qpdb_zonemethods,
		.common.mctx = mctx,
		.node_locks = node_locks,
	};
	qpznode_t node = { .locknum = 0 };
	dns_slabheader_t header = {
		.node = &node,
		.db = (dns_db_t *)&qpdb,
	};
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.slab = { .db = (dns_db_t *)&qpdb, .node = &node, .raw = raw },
		.methods = &dns_rdataslab_rdatasetmethods,
	};
	const char *str1 =
		"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
	isc_buffer_t b;
	dns_fixedname_t fname1, fname2;
	dns_name_t *name1 = dns_fixedname_initname(&fname1);
	dns_name_t *name2 = dns_fixedname_initname(&fname2);

	UNUSED(state);

	/* Minimal initialization of the mock objects */
	memset(node_locks, 0, sizeof(node_locks));
	NODE_INITLOCK(&qpdb.node_locks[0].lock);

	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name1, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_constinit(&b, str1, strlen(str1));
	isc_buffer_add(&b, strlen(str1));
	result = dns_name_fromtext(name2, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(CASESET(&header));

	/* Retrieve the case to name2 */
	dns_rdataset_getownercase(&rdataset, name2);

	NODE_DESTROYLOCK(&qpdb.node_locks[0].lock);

	assert_true(dns_name_caseequal(name1, name2));
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(ownercase)
ISC_TEST_ENTRY(setownercase)
ISC_TEST_LIST_END

ISC_TEST_MAIN
