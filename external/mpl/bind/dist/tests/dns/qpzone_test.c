/*	$NetBSD: qpzone_test.c,v 1.5 2026/04/08 00:16:17 christos Exp $	*/

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

#include <dns/callbacks.h>
#include <dns/diff.h>
#include <dns/qp.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdataslab.h>
#include <dns/rdatastruct.h>
#define KEEP_BEFORE

/* Include the main file */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "qpzone.c"
#pragma GCC diagnostic pop

#include <tests/dns.h>

#define CASESET(header)                                \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_CASESET) != 0)

/*
 * Macro that uses a for loop to execute a cleanup at the end of scope.
 */
#define WITH_NEWVERSION(db, version_var, should_commit)                      \
	for (dns_dbversion_t *version_var = NULL, *_tmp = ({                 \
		     isc_result_t _result = dns_db_newversion(db,            \
							      &version_var); \
		     assert_int_equal(_result, ISC_R_SUCCESS);               \
		     (dns_dbversion_t *)1;                                   \
	     });                                                             \
	     _tmp != NULL;                                                   \
	     _tmp = ({                                                       \
		     dns_db_closeversion(db, &version_var, should_commit);   \
		     (dns_dbversion_t *)NULL;                                \
	     }))

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

static unsigned char example_org_data[] = { 7,	 'e', 'x', 'a', 'm', 'p', 'l',
					    'e', 3,   'o', 'r', 'g', 0 };
static unsigned char example_org_offsets[] = { 0, 8, 12 };
static dns_name_t example_org_name = DNS_NAME_INITABSOLUTE(example_org_data,
							   example_org_offsets);

/* IPv6 test addresses */
static unsigned char aaaa_test_data[][16] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, /* ::1 */
	{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  2 } /* 2001:db8::2
	       */
};

/* RRSIG test signatures */
static unsigned char rrsig_signature1[64] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c,
	0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
};

static unsigned char rrsig_signature2[64] = {
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
	0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
	0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
	0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c,
	0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80
};

/* RRSIG test structures */
static dns_rdata_rrsig_t rrsig_test_data1 = {
	.common = { .rdtype = dns_rdatatype_rrsig,
		    .rdclass = dns_rdataclass_in },
	.covered = dns_rdatatype_a,
	.algorithm = DST_ALG_RSASHA256,
	.labels = 2,
	.originalttl = 300,
	.timeexpire = 1695820800,
	.timesigned = 1695744000,
	.keyid = 0x1234,
	.signer = DNS_NAME_INITABSOLUTE(example_org_data, example_org_offsets),
	.siglen = 64,
	.signature = rrsig_signature1,
};

static dns_rdata_rrsig_t rrsig_test_data2 = {
	.common = { .rdtype = dns_rdatatype_rrsig,
		    .rdclass = dns_rdataclass_in },
	.covered = dns_rdatatype_a,
	.algorithm = DST_ALG_RSASHA256,
	.labels = 2,
	.originalttl = 300,
	.timeexpire = 1695820800,
	.timesigned = 1695744000,
	.keyid = 0x5678,
	.signer = DNS_NAME_INITABSOLUTE(example_org_data, example_org_offsets),
	.siglen = 64,
	.signature = rrsig_signature2,
};

static bool
ownercase_test_one(const char *str1, const char *str2) {
	isc_result_t result;
	uint8_t qpdb_s[sizeof(qpzonedb_t) + sizeof(qpzone_bucket_t)];
	qpzonedb_t *qpdb = (qpzonedb_t *)&qpdb_s;
	*qpdb = (qpzonedb_t){
		.common.methods = &qpdb_zonemethods,
		.common.mctx = mctx,
		.buckets_count = 1,
	};
	qpznode_t node = { .locknum = 0 };
	dns_slabheader_t header = {
		.node = (dns_dbnode_t *)&node,
		.db = (dns_db_t *)qpdb,
	};
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.slab = { .db = (dns_db_t *)qpdb,
			  .node = (dns_dbnode_t *)&node,
			  .raw = raw,
		},
		.methods = &dns_rdataslab_rdatasetmethods,
	};
	isc_buffer_t b;
	dns_fixedname_t fname1, fname2;
	dns_name_t *name1 = dns_fixedname_initname(&fname1);
	dns_name_t *name2 = dns_fixedname_initname(&fname2);

	/* Minimal initialization of the mock objects */
	NODE_INITLOCK(&qpdb->buckets[0].lock);

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

	NODE_DESTROYLOCK(&qpdb->buckets[0].lock);

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

static ssize_t
find_ip_index(const unsigned char *target_ip, unsigned char (*ips)[16],
	      ssize_t count) {
	for (ssize_t i = 0; i < count; i++) {
		if (memcmp(target_ip, ips[i], 16) == 0) {
			return i;
		}
	}
	return -1;
}

static isc_result_t
apply_dns_update(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
		 dns_rdatatype_t rdtype, dns_rdataclass_t rdclass, uint32_t ttl,
		 const unsigned char *data, size_t data_len, dns_diffop_t op) {
	isc_result_t result;
	dns_rdatacallbacks_t callbacks;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	dns_rdatacallbacks_init(&callbacks);

	result = dns_db_beginupdate(db, version, &callbacks);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Set rdata fields directly without reinitializing */
	rdata.data = (unsigned char *)data;
	rdata.length = data_len;
	rdata.rdclass = rdclass;
	rdata.type = rdtype;

	dns_rdatalist_init(&rdatalist);
	rdatalist.ttl = ttl;
	rdatalist.type = rdtype;
	rdatalist.rdclass = rdclass;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

	dns_rdataset_init(&rdataset);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);

	isc_result_t callback_result = callbacks.update(callbacks.add_private,
							name, &rdataset, op);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_disassociate(&rdataset);

	result = dns_db_commitupdate(db, &callbacks);
	assert_int_equal(result, ISC_R_SUCCESS);

	return callback_result;
}

static void
verify_aaaa_records(dns_db_t *db, dns_dbversion_t *version,
		    const dns_name_t *name, unsigned char (*ips)[16],
		    ssize_t expected_count, uint32_t expected_ttl) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_rdataset_t rdataset;
	bool *found_ips = NULL;
	dns_fixedname_t found_fname;
	dns_name_t *found_name = dns_fixedname_initname(&found_fname);

	/* Allocate zero-initialized found flags array */
	found_ips = isc_mem_cget(mctx, (size_t)expected_count, sizeof(bool));

	dns_rdataset_init(&rdataset);

	result = dns_db_find(db, name, version, dns_rdatatype_aaaa, 0, 0, &node,
			     found_name, &rdataset, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Check rdataset metadata */
	assert_int_equal(rdataset.type, dns_rdatatype_aaaa);
	assert_int_equal(rdataset.rdclass, dns_rdataclass_in);
	assert_int_equal(rdataset.ttl, expected_ttl);

	/* Iterate through all AAAA records */
	for (result = dns_rdataset_first(&rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&rdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(&rdataset, &rdata);

		/* Verify this is a valid IPv6 address */
		assert_int_equal(rdata.length, 16);

		/*
		 * Find whether the IP is in our expected list, and detect
		 * duplicates. Index will be -1 if the IP is not found.
		 */
		ssize_t index = find_ip_index(rdata.data, ips, expected_count);
		assert_true(index >= 0);
		assert_false(found_ips[index]);
		found_ips[index] = true;
	}

	/* Count found IPs by summing overt the boolean array */
	ssize_t found_count = 0;
	for (ssize_t i = 0; i < expected_count; i++) {
		found_count += found_ips[i];
	}

	/* Verify we found exactly the expected number of records */
	assert_int_equal(found_count, expected_count);

	dns_db_detachnode(db, &node);
	dns_rdataset_disassociate(&rdataset);
	isc_mem_cput(mctx, found_ips, (size_t)expected_count, sizeof(bool));
}

ISC_RUN_TEST_IMPL(setownercase) {
	isc_result_t result;
	uint8_t qpdb_s[sizeof(qpzonedb_t) + sizeof(qpzone_bucket_t)];
	qpzonedb_t *qpdb = (qpzonedb_t *)&qpdb_s;
	*qpdb = (qpzonedb_t){
		.common.methods = &qpdb_zonemethods,
		.common.mctx = mctx,
		.buckets_count = 1,
	};
	qpznode_t node = { .locknum = 0 };
	dns_slabheader_t header = {
		.node = (dns_dbnode_t *)&node,
		.db = (dns_db_t *)qpdb,
	};
	unsigned char *raw = (unsigned char *)(&header) + sizeof(header);
	dns_rdataset_t rdataset = {
		.magic = DNS_RDATASET_MAGIC,
		.slab = { .db = (dns_db_t *)qpdb,
			  .node = (dns_dbnode_t *)&node,
			  .raw = raw,
		},
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
	NODE_INITLOCK(&qpdb->buckets[0].lock);

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

	NODE_DESTROYLOCK(&qpdb->buckets[0].lock);

	assert_true(dns_name_caseequal(name1, name2));
}

ISC_RUN_TEST_IMPL(diffop_add_sub) {
	isc_result_t result;
	dns_db_t *db = NULL;

	result = dns__qpzone_create(mctx, &example_org_name, dns_dbtype_zone,
				    dns_rdataclass_in, 0, NULL, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(db);

	WITH_NEWVERSION(db, version, true) {
		apply_dns_update(db, version, &example_org_name,
				 dns_rdatatype_aaaa, dns_rdataclass_in, 300,
				 aaaa_test_data[0], 16, DNS_DIFFOP_ADD);
	}

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_aaaa, dns_rdataclass_in,
					  300, aaaa_test_data[1], 16,
					  DNS_DIFFOP_ADD);
		assert_int_equal(result, ISC_R_SUCCESS);

		verify_aaaa_records(db, version, &example_org_name,
				    aaaa_test_data, 2, 300);
	}

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_aaaa, dns_rdataclass_in,
					  300, aaaa_test_data[0], 16,
					  DNS_DIFFOP_DEL);
		assert_int_equal(result, ISC_R_SUCCESS);

		verify_aaaa_records(db, version, &example_org_name,
				    &aaaa_test_data[1], 1, 300);
	}

	WITH_NEWVERSION(db, version, false) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_aaaa, dns_rdataclass_in,
					  600, aaaa_test_data[0], 16,
					  DNS_DIFFOP_ADD);
		assert_int_equal(result, DNS_R_NOTEXACT);
	}

	dns_db_detach(&db);
	assert_null(db);
}

ISC_RUN_TEST_IMPL(diffop_addresign) {
	isc_result_t result;
	dns_db_t *db = NULL;

	/* Create RRSIG structures and convert to wire format */
	dns_rdata_t rdata1 = DNS_RDATA_INIT, rdata2 = DNS_RDATA_INIT;
	isc_buffer_t buffer1, buffer2;
	unsigned char rrsig_data1[512], rrsig_data2[512];

	isc_buffer_init(&buffer1, rrsig_data1, sizeof(rrsig_data1));
	result = dns_rdata_fromstruct(&rdata1, dns_rdataclass_in,
				      dns_rdatatype_rrsig, &rrsig_test_data1,
				      &buffer1);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&buffer2, rrsig_data2, sizeof(rrsig_data2));
	result = dns_rdata_fromstruct(&rdata2, dns_rdataclass_in,
				      dns_rdatatype_rrsig, &rrsig_test_data2,
				      &buffer2);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns__qpzone_create(mctx, &example_org_name, dns_dbtype_zone,
				    dns_rdataclass_in, 0, NULL, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(db);

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_rrsig,
					  dns_rdataclass_in, 300, rdata1.data,
					  rdata1.length, DNS_DIFFOP_ADDRESIGN);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_rrsig,
					  dns_rdataclass_in, 300, rdata2.data,
					  rdata2.length, DNS_DIFFOP_ADDRESIGN);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_rrsig,
					  dns_rdataclass_in, 300, rdata1.data,
					  rdata1.length, DNS_DIFFOP_DELRESIGN);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	WITH_NEWVERSION(db, version, true) {
		result = apply_dns_update(db, version, &example_org_name,
					  dns_rdatatype_rrsig,
					  dns_rdataclass_in, 300, rdata2.data,
					  rdata2.length, DNS_DIFFOP_DELRESIGN);
		assert_int_equal(result, DNS_R_NXRRSET);
	}

	dns_db_detach(&db);
	assert_null(db);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(ownercase)
ISC_TEST_ENTRY(setownercase)
ISC_TEST_ENTRY(diffop_add_sub)
ISC_TEST_ENTRY(diffop_addresign)
ISC_TEST_LIST_END

ISC_TEST_MAIN
