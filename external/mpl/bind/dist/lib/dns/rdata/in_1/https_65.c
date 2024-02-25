/*	$NetBSD: https_65.c,v 1.2.2.1 2024/02/25 15:47:07 martin Exp $	*/

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

/* draft-ietf-dnsop-svcb-https-02 */

#pragma once

#define RRTYPE_HTTPS_ATTRIBUTES (DNS_RDATATYPEATTR_FOLLOWADDITIONAL)

/*
 * Most of these functions refer to equivalent functions for SVCB,
 * since wire and presentation formats are identical.
 */

static isc_result_t
fromtext_in_https(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_https);
	REQUIRE(rdclass == dns_rdataclass_in);

	return (generic_fromtext_in_svcb(CALL_FROMTEXT));
}

static isc_result_t
totext_in_https(ARGS_TOTEXT) {
	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	return (generic_totext_in_svcb(CALL_TOTEXT));
}

static isc_result_t
fromwire_in_https(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_https);
	REQUIRE(rdclass == dns_rdataclass_in);

	return (generic_fromwire_in_svcb(CALL_FROMWIRE));
}

static isc_result_t
towire_in_https(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(rdata->length != 0);

	return (generic_towire_in_svcb(CALL_TOWIRE));
}

static int
compare_in_https(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_https);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	return (isc_region_compare(&region1, &region2));
}

static isc_result_t
fromstruct_in_https(ARGS_FROMSTRUCT) {
	dns_rdata_in_https_t *https = source;

	REQUIRE(type == dns_rdatatype_https);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(https != NULL);
	REQUIRE(https->common.rdtype == type);
	REQUIRE(https->common.rdclass == rdclass);

	return (generic_fromstruct_in_svcb(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_in_https(ARGS_TOSTRUCT) {
	dns_rdata_in_https_t *https = target;

	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(https != NULL);
	REQUIRE(rdata->length != 0);

	return (generic_tostruct_in_svcb(CALL_TOSTRUCT));
}

static void
freestruct_in_https(ARGS_FREESTRUCT) {
	dns_rdata_in_https_t *https = source;

	REQUIRE(https != NULL);
	REQUIRE(https->common.rdclass == dns_rdataclass_in);
	REQUIRE(https->common.rdtype == dns_rdatatype_https);

	generic_freestruct_in_svcb(CALL_FREESTRUCT);
}

static isc_result_t
additionaldata_in_https(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	return (generic_additionaldata_in_svcb(CALL_ADDLDATA));
}

static isc_result_t
digest_in_https(ARGS_DIGEST) {
	isc_region_t region1;

	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &region1);
	return ((digest)(arg, &region1));
}

static bool
checkowner_in_https(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_https);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_in_https(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_https);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	return (generic_checknames_in_svcb(CALL_CHECKNAMES));
}

static int
casecompare_in_https(ARGS_COMPARE) {
	return (compare_in_https(rdata1, rdata2));
}

isc_result_t
dns_rdata_in_https_first(dns_rdata_in_https_t *https) {
	REQUIRE(https != NULL);
	REQUIRE(https->common.rdtype == dns_rdatatype_https);
	REQUIRE(https->common.rdclass == dns_rdataclass_in);

	return (generic_rdata_in_svcb_first(https));
}

isc_result_t
dns_rdata_in_https_next(dns_rdata_in_https_t *https) {
	REQUIRE(https != NULL);
	REQUIRE(https->common.rdtype == dns_rdatatype_https);
	REQUIRE(https->common.rdclass == dns_rdataclass_in);

	return (generic_rdata_in_svcb_next(https));
}

void
dns_rdata_in_https_current(dns_rdata_in_https_t *https, isc_region_t *region) {
	REQUIRE(https != NULL);
	REQUIRE(https->common.rdtype == dns_rdatatype_https);
	REQUIRE(https->common.rdclass == dns_rdataclass_in);
	REQUIRE(region != NULL);

	generic_rdata_in_svcb_current(https, region);
}
