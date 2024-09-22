/*	$NetBSD: resinfo_261.c,v 1.2 2024/09/22 00:14:08 christos Exp $	*/

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

#ifndef RDATA_GENERIC_RESINFO_261_C
#define RDATA_GENERIC_RESINFO_261_C

#define RRTYPE_RESINFO_ATTRIBUTES (DNS_RDATATYPEATTR_SINGLETON)

static isc_result_t
fromtext_resinfo(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_resinfo);

	return (generic_fromtext_txt(CALL_FROMTEXT));
}

static isc_result_t
totext_resinfo(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	return (generic_totext_txt(CALL_TOTEXT));
}

static isc_result_t
fromwire_resinfo(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_resinfo);

	return (generic_fromwire_txt(CALL_FROMWIRE));
}

static isc_result_t
towire_resinfo(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_resinfo(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_resinfo);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_resinfo(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_resinfo);

	return (generic_fromstruct_txt(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_resinfo(ARGS_TOSTRUCT) {
	dns_rdata_resinfo_t *resinfo = target;

	REQUIRE(resinfo != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	resinfo->common.rdclass = rdata->rdclass;
	resinfo->common.rdtype = rdata->type;
	ISC_LINK_INIT(&resinfo->common, link);

	return (generic_tostruct_txt(CALL_TOSTRUCT));
}

static void
freestruct_resinfo(ARGS_FREESTRUCT) {
	dns_rdata_resinfo_t *resinfo = source;

	REQUIRE(resinfo != NULL);
	REQUIRE(resinfo->common.rdtype == dns_rdatatype_resinfo);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_resinfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_resinfo(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_resinfo(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_resinfo);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_resinfo(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_resinfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_resinfo(ARGS_COMPARE) {
	return (compare_resinfo(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_RESINFO_261_C */
