/*	$NetBSD: rkey_57.c,v 1.7.2.1 2024/02/25 15:47:04 martin Exp $	*/

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

#ifndef RDATA_GENERIC_RKEY_57_C
#define RDATA_GENERIC_RKEY_57_C

#define RRTYPE_RKEY_ATTRIBUTES 0

static isc_result_t
fromtext_rkey(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_rkey);

	return (generic_fromtext_key(CALL_FROMTEXT));
}

static isc_result_t
totext_rkey(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_rkey);

	return (generic_totext_key(CALL_TOTEXT));
}

static isc_result_t
fromwire_rkey(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_rkey);

	return (generic_fromwire_key(CALL_FROMWIRE));
}

static isc_result_t
towire_rkey(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_rkey);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_rkey(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1 != NULL);
	REQUIRE(rdata2 != NULL);
	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_rkey);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_rkey(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_rkey);

	return (generic_fromstruct_key(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_rkey(ARGS_TOSTRUCT) {
	dns_rdata_rkey_t *rkey = target;

	REQUIRE(rkey != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_rkey);

	rkey->common.rdclass = rdata->rdclass;
	rkey->common.rdtype = rdata->type;
	ISC_LINK_INIT(&rkey->common, link);

	return (generic_tostruct_key(CALL_TOSTRUCT));
}

static void
freestruct_rkey(ARGS_FREESTRUCT) {
	dns_rdata_rkey_t *rkey = (dns_rdata_rkey_t *)source;

	REQUIRE(rkey != NULL);
	REQUIRE(rkey->common.rdtype == dns_rdatatype_rkey);

	generic_freestruct_key(source);
}

static isc_result_t
additionaldata_rkey(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_rkey);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_rkey(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_rkey);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_rkey(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_rkey);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_rkey(ARGS_CHECKNAMES) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_rkey);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_rkey(ARGS_COMPARE) {
	/*
	 * Treat ALG 253 (private DNS) subtype name case sensitively.
	 */
	return (compare_rkey(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_RKEY_57_C */
