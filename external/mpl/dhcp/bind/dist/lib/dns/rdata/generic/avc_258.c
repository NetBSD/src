/*	$NetBSD: avc_258.c,v 1.1.2.2 2024/02/24 13:07:09 martin Exp $	*/

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

#ifndef RDATA_GENERIC_AVC_258_C
#define RDATA_GENERIC_AVC_258_C

#define RRTYPE_AVC_ATTRIBUTES (0)

static isc_result_t
fromtext_avc(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_avc);

	return (generic_fromtext_txt(CALL_FROMTEXT));
}

static isc_result_t
totext_avc(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_avc);

	return (generic_totext_txt(CALL_TOTEXT));
}

static isc_result_t
fromwire_avc(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_avc);

	return (generic_fromwire_txt(CALL_FROMWIRE));
}

static isc_result_t
towire_avc(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_avc(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_avc);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_avc(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_avc);

	return (generic_fromstruct_txt(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_avc(ARGS_TOSTRUCT) {
	dns_rdata_avc_t *avc = target;

	REQUIRE(rdata->type == dns_rdatatype_avc);
	REQUIRE(avc != NULL);

	avc->common.rdclass = rdata->rdclass;
	avc->common.rdtype = rdata->type;
	ISC_LINK_INIT(&avc->common, link);

	return (generic_tostruct_txt(CALL_TOSTRUCT));
}

static void
freestruct_avc(ARGS_FREESTRUCT) {
	dns_rdata_avc_t *avc = source;

	REQUIRE(avc != NULL);
	REQUIRE(avc->common.rdtype == dns_rdatatype_avc);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_avc(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_avc(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_avc);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_avc(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_avc);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_avc(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_avc(ARGS_COMPARE) {
	return (compare_avc(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_AVC_258_C */
