/*	$NetBSD: rdatasetstats_test.c,v 1.2.2.3 2019/01/18 08:49:56 pgoyette Exp $	*/

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

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/print.h>
#include <isc/util.h>

#include <dns/stats.h>

#include "dnstest.h"

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

static void
set_typestats(dns_stats_t *stats, dns_rdatatype_t type, bool stale) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = 0;
	if (stale) {
		attributes |= DNS_RDATASTATSTYPE_ATTR_STALE;
	}
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);

	attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET;
	if (stale) {
		attributes |= DNS_RDATASTATSTYPE_ATTR_STALE;
	}
	which = DNS_RDATASTATSTYPE_VALUE(type, attributes);
	dns_rdatasetstats_increment(stats, which);
}

static void
set_nxdomainstats(dns_stats_t *stats, bool stale) {
	dns_rdatastatstype_t which;
	unsigned int attributes;

	attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN;
	if (stale) {
		attributes |= DNS_RDATASTATSTYPE_ATTR_STALE;
	}
	which = DNS_RDATASTATSTYPE_VALUE(0, attributes);
	dns_rdatasetstats_increment(stats, which);
}

#define ATTRIBUTE_SET(y) ((attributes & (y)) != 0)
static void
checkit1(dns_rdatastatstype_t which, uint64_t value, void *arg) {
	unsigned int attributes;
#if debug
	unsigned int type;
#endif

	UNUSED(which);
	UNUSED(arg);

	attributes = DNS_RDATASTATSTYPE_ATTR(which);
#if debug
	type = DNS_RDATASTATSTYPE_BASE(which);

	fprintf(stderr, "%s%s%s%s/%u, %u\n",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) ? "O" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXRRSET) ? "!" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_STALE) ? "#" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) ? "X" : " ",
		type, (unsigned)value);
#endif
	if ((attributes & DNS_RDATASTATSTYPE_ATTR_STALE) == 0) {
		assert_int_equal(value, 1);
	} else {
		assert_int_equal(value, 0);
	}
}

static void
checkit2(dns_rdatastatstype_t which, uint64_t value, void *arg) {
	unsigned int attributes;
#if debug
	unsigned int type;
#endif

	UNUSED(which);
	UNUSED(arg);

	attributes = DNS_RDATASTATSTYPE_ATTR(which);
#if debug
	type = DNS_RDATASTATSTYPE_BASE(which);

	fprintf(stderr, "%s%s%s%s/%u, %u\n",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) ? "O" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXRRSET) ? "!" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_STALE) ? "#" : " ",
		ATTRIBUTE_SET(DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) ? "X" : " ",
		type, (unsigned)value);
#endif
	if ((attributes & DNS_RDATASTATSTYPE_ATTR_STALE) == 0) {
		assert_int_equal(value, 0);
	} else {
		assert_int_equal(value, 1);
	}
}
/*
 * Individual unit tests
 */

/* test that rdatasetstats counters are properly set */
static void
rdatasetstats(void **state) {
	unsigned int i;
	dns_stats_t *stats = NULL;
	isc_result_t result;

	UNUSED(state);

	result = dns_rdatasetstats_create(mctx, &stats);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* First 256 types. */
	for (i = 0; i <= 255; i++)
		set_typestats(stats, (dns_rdatatype_t)i, false);
	/* Specials */
	set_typestats(stats, dns_rdatatype_dlv, false);
	set_typestats(stats, (dns_rdatatype_t)1000, false);
	set_nxdomainstats(stats, false);

	/*
	 * Check that all counters are set to appropriately.
	 */
	dns_rdatasetstats_dump(stats, checkit1, NULL, 1);

	/* First 256 types. */
	for (i = 0; i <= 255; i++)
		set_typestats(stats, (dns_rdatatype_t)i, true);
	/* Specials */
	set_typestats(stats, dns_rdatatype_dlv, true);
	set_typestats(stats, (dns_rdatatype_t)1000, true);
	set_nxdomainstats(stats, true);

	/*
	 * Check that all counters are set to appropriately.
	 */
	dns_rdatasetstats_dump(stats, checkit2, NULL, 1);

	dns_stats_detach(&stats);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(rdatasetstats,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
