/*	$NetBSD: rdataset_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <dns/rdataset.h>
#include <dns/rdatastruct.h>

#include <tests/dns.h>

/* test trimming of rdataset TTLs */
ISC_RUN_TEST_IMPL(trimttl) {
	dns_rdataset_t rdataset, sigrdataset;
	dns_rdata_rrsig_t rrsig;
	isc_stdtime_t ttltimenow, ttltimeexpire;

	ttltimenow = 10000000;
	ttltimeexpire = ttltimenow + 800;

	UNUSED(state);

	dns_rdataset_init(&rdataset);
	dns_rdataset_init(&sigrdataset);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimeexpire;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow, true);
	assert_int_equal(rdataset.ttl, 800);
	assert_int_equal(sigrdataset.ttl, 800);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow, true);
	assert_int_equal(rdataset.ttl, 120);
	assert_int_equal(sigrdataset.ttl, 120);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     false);
	assert_int_equal(rdataset.ttl, 0);
	assert_int_equal(sigrdataset.ttl, 0);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimeexpire;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow, true);
	assert_int_equal(rdataset.ttl, 800);
	assert_int_equal(sigrdataset.ttl, 800);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow, true);
	assert_int_equal(rdataset.ttl, 120);
	assert_int_equal(sigrdataset.ttl, 120);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     false);
	assert_int_equal(rdataset.ttl, 0);
	assert_int_equal(sigrdataset.ttl, 0);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(trimttl)
ISC_TEST_LIST_END

ISC_TEST_MAIN
