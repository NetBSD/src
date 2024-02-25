/*	$NetBSD: dns64_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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

#include <isc/netaddr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dns64.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>

#include <tests/dns.h>

static void
multiple_prefixes(void) {
	size_t i, count;
	/*
	 * Two prefix, non consectutive.
	 */
	unsigned char aaaa[4][16] = {
		{ 0, 0, 0, 0, 192, 0, 0, 170, 0, 0, 0, 0, 192, 0, 0, 171 },
		{ 0, 0, 0, 0, 192, 55, 0, 170, 0, 0, 0, 0, 192, 0, 0, 170 },
		{ 0, 0, 0, 0, 192, 0, 0, 170, 0, 0, 0, 0, 192, 0, 0, 170 },
		{ 0, 0, 0, 0, 192, 55, 0, 170, 0, 0, 0, 0, 192, 0, 0, 171 },
	};
	dns_rdataset_t rdataset;
	dns_rdatalist_t rdatalist;
	dns_rdata_t rdata[4] = { DNS_RDATA_INIT, DNS_RDATA_INIT, DNS_RDATA_INIT,
				 DNS_RDATA_INIT };
	isc_netprefix_t prefix[2];
	unsigned char p1[] = { 0, 0, 0, 0, 192, 0, 0, 170, 0, 0, 0, 0 };
	unsigned char p2[] = { 0, 0, 0, 0, 192, 55, 0, 170, 0, 0, 0, 0 };
	isc_result_t result;
	bool have_p1, have_p2;

	/*
	 * Construct AAAA rdataset containing 2 prefixes.
	 */
	dns_rdatalist_init(&rdatalist);
	for (i = 0; i < 4; i++) {
		isc_region_t region;
		region.base = aaaa[i];
		region.length = 16;
		dns_rdata_fromregion(&rdata[i], dns_rdataclass_in,
				     dns_rdatatype_aaaa, &region);
		ISC_LIST_APPEND(rdatalist.rdata, &rdata[i], link);
	}
	rdatalist.type = rdata[0].type;
	rdatalist.rdclass = rdata[0].rdclass;
	rdatalist.ttl = 0;
	dns_rdataset_init(&rdataset);
	result = dns_rdatalist_tordataset(&rdatalist, &rdataset);
	assert_int_equal(result, ISC_R_SUCCESS);

	count = ARRAY_SIZE(prefix);
	memset(&prefix, 0, sizeof(prefix));
	result = dns_dns64_findprefix(&rdataset, prefix, &count);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(count, 2);
	have_p1 = have_p2 = false;
	for (i = 0; i < count; i++) {
		assert_int_equal(prefix[i].prefixlen, 96);
		assert_int_equal(prefix[i].addr.family, AF_INET6);
		if (memcmp(prefix[i].addr.type.in6.s6_addr, p1, 12) == 0) {
			have_p1 = true;
		}
		if (memcmp(prefix[i].addr.type.in6.s6_addr, p2, 12) == 0) {
			have_p2 = true;
		}
	}
	assert_true(have_p1);
	assert_true(have_p2);

	/*
	 * Check that insufficient prefix space returns ISC_R_NOSPACE
	 * and that the prefix is populated.
	 */
	count = 1;
	memset(&prefix, 0, sizeof(prefix));
	result = dns_dns64_findprefix(&rdataset, prefix, &count);
	assert_int_equal(result, ISC_R_NOSPACE);
	assert_int_equal(count, 2);
	have_p1 = have_p2 = false;
	assert_int_equal(prefix[0].prefixlen, 96);
	assert_int_equal(prefix[0].addr.family, AF_INET6);
	if (memcmp(prefix[0].addr.type.in6.s6_addr, p1, 12) == 0) {
		have_p1 = true;
	}
	if (memcmp(prefix[0].addr.type.in6.s6_addr, p2, 12) == 0) {
		have_p2 = true;
	}
	if (!have_p2) {
		assert_true(have_p1);
	}
	if (!have_p1) {
		assert_true(have_p2);
	}
	assert_true(have_p1 != have_p2);
}

ISC_RUN_TEST_IMPL(dns64_findprefix) {
	unsigned int i, j, o;
	isc_result_t result;
	struct {
		unsigned char prefix[12];
		unsigned int prefixlen;
		isc_result_t result;
	} tests[] = {
		/* The WKP with various lengths. */
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  32,
		  ISC_R_SUCCESS },
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  40,
		  ISC_R_SUCCESS },
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  48,
		  ISC_R_SUCCESS },
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  56,
		  ISC_R_SUCCESS },
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  64,
		  ISC_R_SUCCESS },
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0 },
		  96,
		  ISC_R_SUCCESS },
		/*
		 * Prefix with the mapped addresses also appearing in the
		 * prefix.
		 */
		{ { 0, 0, 0, 0, 192, 0, 0, 170, 0, 0, 0, 0 },
		  96,
		  ISC_R_SUCCESS },
		{ { 0, 0, 0, 0, 192, 0, 0, 171, 0, 0, 0, 0 },
		  96,
		  ISC_R_SUCCESS },
		/* Bad prefix, MBZ != 0. */
		{ { 0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 1, 0, 0, 0 },
		  96,
		  ISC_R_NOTFOUND },
	};

	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		size_t count = 2;
		dns_rdataset_t rdataset;
		dns_rdatalist_t rdatalist;
		dns_rdata_t rdata[2] = { DNS_RDATA_INIT, DNS_RDATA_INIT };
		struct in6_addr ina6[2];
		isc_netprefix_t prefix[2];
		unsigned char aa[] = { 192, 0, 0, 170 };
		unsigned char ab[] = { 192, 0, 0, 171 };
		isc_region_t region;

		/*
		 * Construct rdata.
		 */
		memset(ina6[0].s6_addr, 0, sizeof(ina6[0].s6_addr));
		memset(ina6[1].s6_addr, 0, sizeof(ina6[1].s6_addr));
		memmove(ina6[0].s6_addr, tests[i].prefix, 12);
		memmove(ina6[1].s6_addr, tests[i].prefix, 12);
		o = tests[i].prefixlen / 8;
		for (j = 0; j < 4; j++) {
			if ((o + j) == 8U) {
				o++; /* skip mbz */
			}
			ina6[0].s6_addr[j + o] = aa[j];
			ina6[1].s6_addr[j + o] = ab[j];
		}
		region.base = ina6[0].s6_addr;
		region.length = sizeof(ina6[0].s6_addr);
		dns_rdata_fromregion(&rdata[0], dns_rdataclass_in,
				     dns_rdatatype_aaaa, &region);
		region.base = ina6[1].s6_addr;
		region.length = sizeof(ina6[1].s6_addr);
		dns_rdata_fromregion(&rdata[1], dns_rdataclass_in,
				     dns_rdatatype_aaaa, &region);

		dns_rdatalist_init(&rdatalist);
		rdatalist.type = rdata[0].type;
		rdatalist.rdclass = rdata[0].rdclass;
		rdatalist.ttl = 0;
		ISC_LIST_APPEND(rdatalist.rdata, &rdata[0], link);
		ISC_LIST_APPEND(rdatalist.rdata, &rdata[1], link);
		dns_rdataset_init(&rdataset);
		result = dns_rdatalist_tordataset(&rdatalist, &rdataset);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_dns64_findprefix(&rdataset, prefix, &count);
		assert_int_equal(result, tests[i].result);
		if (tests[i].result == ISC_R_SUCCESS) {
			assert_int_equal(count, 1);
			assert_int_equal(prefix[0].prefixlen,
					 tests[i].prefixlen);
			assert_int_equal(prefix[0].addr.family, AF_INET6);
			assert_memory_equal(prefix[0].addr.type.in6.s6_addr,
					    tests[i].prefix,
					    tests[i].prefixlen / 8);
		}
	}

	/*
	 * Test multiple prefixes.
	 */
	multiple_prefixes();
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(dns64_findprefix)
ISC_TEST_LIST_END

ISC_TEST_MAIN
