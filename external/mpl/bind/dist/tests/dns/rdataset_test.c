/*	$NetBSD: rdataset_test.c,v 1.1.1.2 2026/09/17 17:45:09 christos Exp $	*/

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

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>

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

/*
 * A rdataset at the NOQNAME proof owner, in wire order.
 */
typedef struct {
	dns_rdatatype_t type;
	dns_rdatatype_t covers;
	dns_ttl_t ttl;
} proofset_t;

#define ANSWER_TTL 3600
#define MAXSETS	   4

static void
addset(dns_name_t *owner, dns_rdatalist_t *rdatalist, dns_rdataset_t *rdataset,
       dns_rdatatype_t type, dns_rdatatype_t covers, dns_ttl_t ttl) {
	dns_rdatalist_init(rdatalist);
	rdatalist->rdclass = dns_rdataclass_in;
	rdatalist->type = type;
	rdatalist->covers = covers;
	rdatalist->ttl = ttl;

	dns_rdataset_init(rdataset);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	if (owner != NULL) {
		ISC_LIST_APPEND(owner->list, rdataset, link);
	}
}

/*
 * Check that dns_rdataset_addnoqname() selects the proof of 'type' at an
 * owner carrying 'sets' in wire order, and that dns_rdataset_getnoqname()
 * then returns that same proof rather than one chosen by wire order.
 */
static void
check_noqname(const proofset_t *sets, size_t nsets, dns_rdatatype_t type,
	      isc_result_t expected) {
	dns_fixedname_t fowner;
	dns_name_t *owner = dns_fixedname_initname(&fowner);
	dns_rdatalist_t rdatalists[MAXSETS], answerlist;
	dns_rdataset_t rdatasets[MAXSETS], answer;
	dns_rdataset_t neg = DNS_RDATASET_INIT, negsig = DNS_RDATASET_INIT;
	dns_name_t found = DNS_NAME_INITEMPTY;
	dns_ttl_t ttl = ANSWER_TTL;
	isc_result_t result;

	assert_true(nsets <= MAXSETS);
	result = dns_name_fromstring(owner, "proof.example.", dns_rootname, 0,
				     NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (size_t i = 0; i < nsets; i++) {
		addset(owner, &rdatalists[i], &rdatasets[i], sets[i].type,
		       sets[i].covers, sets[i].ttl);
		if (sets[i].type == type ||
		    (sets[i].type == dns_rdatatype_rrsig &&
		     sets[i].covers == type))
		{
			ttl = ISC_MIN(ttl, sets[i].ttl);
		}
	}
	addset(NULL, &answerlist, &answer, dns_rdatatype_a, 0, ANSWER_TTL);

	result = dns_rdataset_addnoqname(&answer, owner, type);
	assert_int_equal(result, expected);
	if (result != ISC_R_SUCCESS) {
		assert_false((answer.attributes & DNS_RDATASETATTR_NOQNAME) !=
			     0);
		assert_int_equal(answer.ttl, ANSWER_TTL);
		return;
	}
	assert_true((answer.attributes & DNS_RDATASETATTR_NOQNAME) != 0);
	assert_int_equal(answer.ttl, ttl);

	result = dns_rdataset_getnoqname(&answer, &found, &neg, &negsig);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(dns_name_equal(&found, owner));
	assert_int_equal(neg.type, type);
	assert_int_equal(negsig.type, dns_rdatatype_rrsig);
	assert_int_equal(negsig.covers, type);

	dns_rdataset_disassociate(&neg);
	dns_rdataset_disassociate(&negsig);
	dns_rdataset_disassociate(&answer);
}

#define CHECK_NOQNAME(sets, type, expected) \
	check_noqname(sets, sizeof(sets) / sizeof((sets)[0]), type, expected)

/* test NOQNAME proof selection by dns_rdataset_{add,get}noqname() */
ISC_RUN_TEST_IMPL(noqname) {
	/* Signed NSEC followed by an unsigned NSEC3 (#5985). */
	const proofset_t nsec_unsigned_nsec3[] = {
		{ dns_rdatatype_nsec, 0, 300 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec, 200 },
		{ dns_rdatatype_nsec3, 0, 100 },
	};
	/* Signed NSEC3 followed by an unsigned NSEC (#6369). */
	const proofset_t nsec3_unsigned_nsec[] = {
		{ dns_rdatatype_nsec3, 0, 300 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec3, 200 },
		{ dns_rdatatype_nsec, 0, 100 },
	};
	/* Both denial types signed, in both wire orders. */
	const proofset_t nsec_nsec3[] = {
		{ dns_rdatatype_nsec, 0, 300 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec, 300 },
		{ dns_rdatatype_nsec3, 0, 200 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec3, 200 },
	};
	const proofset_t nsec3_nsec[] = {
		{ dns_rdatatype_nsec3, 0, 200 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec3, 200 },
		{ dns_rdatatype_nsec, 0, 300 },
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec, 300 },
	};
	/* RRSIG preceding the rdataset it covers. */
	const proofset_t rrsig_first[] = {
		{ dns_rdatatype_rrsig, dns_rdatatype_nsec, 300 },
		{ dns_rdatatype_nsec, 0, 300 },
	};
	/* No signed denial at all. */
	const proofset_t unsigned_only[] = {
		{ dns_rdatatype_nsec, 0, 300 },
		{ dns_rdatatype_nsec3, 0, 300 },
	};

	UNUSED(state);

	CHECK_NOQNAME(nsec_unsigned_nsec3, dns_rdatatype_nsec, ISC_R_SUCCESS);
	CHECK_NOQNAME(nsec_unsigned_nsec3, dns_rdatatype_nsec3, ISC_R_NOTFOUND);

	CHECK_NOQNAME(nsec3_unsigned_nsec, dns_rdatatype_nsec3, ISC_R_SUCCESS);
	CHECK_NOQNAME(nsec3_unsigned_nsec, dns_rdatatype_nsec, ISC_R_NOTFOUND);

	/* The caller's choice wins regardless of wire order. */
	CHECK_NOQNAME(nsec_nsec3, dns_rdatatype_nsec, ISC_R_SUCCESS);
	CHECK_NOQNAME(nsec_nsec3, dns_rdatatype_nsec3, ISC_R_SUCCESS);
	CHECK_NOQNAME(nsec3_nsec, dns_rdatatype_nsec, ISC_R_SUCCESS);
	CHECK_NOQNAME(nsec3_nsec, dns_rdatatype_nsec3, ISC_R_SUCCESS);

	CHECK_NOQNAME(rrsig_first, dns_rdatatype_nsec, ISC_R_SUCCESS);

	CHECK_NOQNAME(unsigned_only, dns_rdatatype_nsec, ISC_R_NOTFOUND);
	CHECK_NOQNAME(unsigned_only, dns_rdatatype_nsec3, ISC_R_NOTFOUND);

	check_noqname(NULL, 0, dns_rdatatype_nsec, ISC_R_NOTFOUND);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(trimttl)
ISC_TEST_ENTRY(noqname)
ISC_TEST_LIST_END

ISC_TEST_MAIN
