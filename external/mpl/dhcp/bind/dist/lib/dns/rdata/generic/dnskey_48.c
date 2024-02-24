/*	$NetBSD: dnskey_48.c,v 1.1.2.2 2024/02/24 13:07:10 martin Exp $	*/

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

/* RFC2535 */

#ifndef RDATA_GENERIC_DNSKEY_48_C
#define RDATA_GENERIC_DNSKEY_48_C

#include <dst/dst.h>

#define RRTYPE_DNSKEY_ATTRIBUTES (DNS_RDATATYPEATTR_DNSSEC)

static isc_result_t
fromtext_dnskey(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_dnskey);

	return (generic_fromtext_key(CALL_FROMTEXT));
}

static isc_result_t
totext_dnskey(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dnskey);

	return (generic_totext_key(CALL_TOTEXT));
}

static isc_result_t
fromwire_dnskey(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_dnskey);

	return (generic_fromwire_key(CALL_FROMWIRE));
}

static isc_result_t
towire_dnskey(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dnskey);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_dnskey(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1 != NULL);
	REQUIRE(rdata2 != NULL);
	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_dnskey);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_dnskey(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_dnskey);

	return (generic_fromstruct_key(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_dnskey(ARGS_TOSTRUCT) {
	dns_rdata_dnskey_t *dnskey = target;

	REQUIRE(dnskey != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dnskey);

	dnskey->common.rdclass = rdata->rdclass;
	dnskey->common.rdtype = rdata->type;
	ISC_LINK_INIT(&dnskey->common, link);

	return (generic_tostruct_key(CALL_TOSTRUCT));
}

static void
freestruct_dnskey(ARGS_FREESTRUCT) {
	dns_rdata_dnskey_t *dnskey = (dns_rdata_dnskey_t *)source;

	REQUIRE(dnskey != NULL);
	REQUIRE(dnskey->common.rdtype == dns_rdatatype_dnskey);

	generic_freestruct_key(source);
}

static isc_result_t
additionaldata_dnskey(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_dnskey);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_dnskey(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dnskey);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_dnskey(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_dnskey);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_dnskey(ARGS_CHECKNAMES) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dnskey);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_dnskey(ARGS_COMPARE) {
	/*
	 * Treat ALG 253 (private DNS) subtype name case sensitively.
	 */
	return (compare_dnskey(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_DNSKEY_48_C */
