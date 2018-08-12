/*	$NetBSD: rdataset_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <dns/rdataset.h>
#include <dns/rdatastruct.h>

#include "dnstest.h"


/*
 * Individual unit tests
 */

/* Successful load test */
ATF_TC(trimttl);
ATF_TC_HEAD(trimttl, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() loads a "
				       "valid master file and returns success");
}
ATF_TC_BODY(trimttl, tc) {
	isc_result_t result;
	dns_rdataset_t rdataset, sigrdataset;
	dns_rdata_rrsig_t rrsig;
	isc_stdtime_t ttltimenow, ttltimeexpire;

	ttltimenow = 10000000;
	ttltimeexpire = ttltimenow + 800;

	UNUSED(tc);

	dns_rdataset_init(&rdataset);
	dns_rdataset_init(&sigrdataset);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimeexpire;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_TRUE);
	ATF_REQUIRE_EQ(rdataset.ttl, 800);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 800);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_TRUE);
	ATF_REQUIRE_EQ(rdataset.ttl, 120);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 120);

	rdataset.ttl = 900;
	sigrdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_FALSE);
	ATF_REQUIRE_EQ(rdataset.ttl, 0);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 0);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimeexpire;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_TRUE);
	ATF_REQUIRE_EQ(rdataset.ttl, 800);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 800);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_TRUE);
	ATF_REQUIRE_EQ(rdataset.ttl, 120);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 120);

	sigrdataset.ttl = 900;
	rdataset.ttl = 1000;
	rrsig.timeexpire = ttltimenow - 200;
	rrsig.originalttl = 1000;

	dns_rdataset_trimttl(&rdataset, &sigrdataset, &rrsig, ttltimenow,
			     ISC_FALSE);
	ATF_REQUIRE_EQ(rdataset.ttl, 0);
	ATF_REQUIRE_EQ(sigrdataset.ttl, 0);

	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, trimttl);

	return (atf_no_error());
}

