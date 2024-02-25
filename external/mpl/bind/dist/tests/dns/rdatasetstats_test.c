/*	$NetBSD: rdatasetstats_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/print.h>
#include <isc/util.h>

#include <dns/stats.h>

#include <tests/dns.h>

static void
set_typestats(dns_stats_t *stats, dns_rdatatype_t type) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = 0;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);

	attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);
}

static void
set_nxdomainstats(dns_stats_t *stats) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN;
	which = DNS_RDATASTATSTYPE_VALUE(0, attributes);
	dns_rdatasetstats_increment(stats, which);
}

static void
mark_stale(dns_stats_t *stats, dns_rdatatype_t type, int from, int to) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = from;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_decrement(stats, which);

	attributes |= to;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);

	attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET | from;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_decrement(stats, which);

	attributes |= to;
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);
}

static void
mark_nxdomain_stale(dns_stats_t *stats, int from, int to) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN | from;
	which = DNS_RDATASTATSTYPE_VALUE(0, attributes);
	dns_rdatasetstats_decrement(stats, which);

	attributes |= to;
	which = DNS_RDATASTATSTYPE_VALUE(0, attributes);
	dns_rdatasetstats_increment(stats, which);
}

#define ATTRIBUTE_SET(y) ((attributes & (y)) != 0)
static void
verify_active_counters(dns_rdatastatstype_t which, uint64_t value, void *arg) {
	unsigned int attributes;
#if debug
	unsigned int type;
#endif /* if debug */

	UNUSED(which);
	UNUSED(arg);

	attributes = DNS_RDATASTATSTYPE_ATTR(which);
#if debug
	type = DNS_RDATASTATSTYPE_BASE(which);

	fprintf(stderr, "%s%s%s%s%s/%u, %u\n",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) ? "O" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXRRSET) ? "!" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_ANCIENT) ? "~" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_STALE) ? "#" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) ? "X" : " ",
		type, (unsigned)value);
#endif /* if debug */
	if ((attributes & DNS_RDATASTATSTYPE_ATTR_ANCIENT) == 0 &&
	    (attributes & DNS_RDATASTATSTYPE_ATTR_STALE) == 0)
	{
		assert_int_equal(value, 1);
	} else {
		assert_int_equal(value, 0);
	}
}

static void
verify_stale_counters(dns_rdatastatstype_t which, uint64_t value, void *arg) {
	unsigned int attributes;
#if debug
	unsigned int type;
#endif /* if debug */

	UNUSED(which);
	UNUSED(arg);

	attributes = DNS_RDATASTATSTYPE_ATTR(which);
#if debug
	type = DNS_RDATASTATSTYPE_BASE(which);

	fprintf(stderr, "%s%s%s%s%s/%u, %u\n",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) ? "O" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXRRSET) ? "!" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_ANCIENT) ? "~" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_STALE) ? "#" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) ? "X" : " ",
		type, (unsigned)value);
#endif /* if debug */
	if ((attributes & DNS_RDATASTATSTYPE_ATTR_STALE) != 0) {
		assert_int_equal(value, 1);
	} else {
		assert_int_equal(value, 0);
	}
}

static void
verify_ancient_counters(dns_rdatastatstype_t which, uint64_t value, void *arg) {
	unsigned int attributes;
#if debug
	unsigned int type;
#endif /* if debug */

	UNUSED(which);
	UNUSED(arg);

	attributes = DNS_RDATASTATSTYPE_ATTR(which);
#if debug
	type = DNS_RDATASTATSTYPE_BASE(which);

	fprintf(stderr, "%s%s%s%s%s/%u, %u\n",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) ? "O" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXRRSET) ? "!" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_ANCIENT) ? "~" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_STALE) ? "#" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) ? "X" : " ",
		type, (unsigned)value);
#endif /* if debug */
	if ((attributes & DNS_RDATASTATSTYPE_ATTR_ANCIENT) != 0) {
		assert_int_equal(value, 1);
	} else {
		assert_int_equal(value, 0);
	}
}
/*
 * Individual unit tests
 */

/*
 * Test that rdatasetstats counters are properly set when moving from
 * active -> stale -> ancient.
 */
static void
rdatasetstats(void **state, bool servestale) {
	unsigned int i;
	unsigned int from = 0;
	dns_stats_t *stats = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_rdatasetstats_create(mctx, &stats);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* First 255 types. */
	for (i = 1; i <= 255; i++) {
		set_typestats(stats, (dns_rdatatype_t)i);
	}
	/* Specials */
	set_typestats(stats, (dns_rdatatype_t)1000);
	set_nxdomainstats(stats);

	/* Check that all active counters are set to appropriately. */
	dns_rdatasetstats_dump(stats, verify_active_counters, NULL, 1);

	if (servestale) {
		/* Mark stale */
		for (i = 1; i <= 255; i++) {
			mark_stale(stats, (dns_rdatatype_t)i, 0,
				   DNS_RDATASTATSTYPE_ATTR_STALE);
		}
		mark_stale(stats, (dns_rdatatype_t)1000, 0,
			   DNS_RDATASTATSTYPE_ATTR_STALE);
		mark_nxdomain_stale(stats, 0, DNS_RDATASTATSTYPE_ATTR_STALE);

		/* Check that all counters are set to appropriately. */
		dns_rdatasetstats_dump(stats, verify_stale_counters, NULL, 1);

		/* Set correct staleness state */
		from = DNS_RDATASTATSTYPE_ATTR_STALE;
	}

	/* Mark ancient */
	for (i = 1; i <= 255; i++) {
		mark_stale(stats, (dns_rdatatype_t)i, from,
			   DNS_RDATASTATSTYPE_ATTR_ANCIENT);
	}
	mark_stale(stats, (dns_rdatatype_t)1000, from,
		   DNS_RDATASTATSTYPE_ATTR_ANCIENT);
	mark_nxdomain_stale(stats, from, DNS_RDATASTATSTYPE_ATTR_ANCIENT);

	/*
	 * Check that all counters are set to appropriately.
	 */
	dns_rdatasetstats_dump(stats, verify_ancient_counters, NULL, 1);

	dns_stats_detach(&stats);
}

/*
 * Test that rdatasetstats counters are properly set when moving from
 * active -> stale -> ancient.
 */
ISC_RUN_TEST_IMPL(active_stale_ancient) { rdatasetstats(state, true); }

/*
 * Test that rdatasetstats counters are properly set when moving from
 * active -> ancient.
 */
ISC_RUN_TEST_IMPL(active_ancient) { rdatasetstats(state, false); }

ISC_TEST_LIST_START
ISC_TEST_ENTRY(active_stale_ancient)
ISC_TEST_ENTRY(active_ancient)
ISC_TEST_LIST_END

ISC_TEST_MAIN
