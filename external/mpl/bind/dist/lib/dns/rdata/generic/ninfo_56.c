/*	$NetBSD: ninfo_56.c,v 1.8.2.1 2024/02/25 15:47:03 martin Exp $	*/

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

#ifndef RDATA_GENERIC_NINFO_56_C
#define RDATA_GENERIC_NINFO_56_C

#define RRTYPE_NINFO_ATTRIBUTES (0)

static isc_result_t
fromtext_ninfo(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_ninfo);

	return (generic_fromtext_txt(CALL_FROMTEXT));
}

static isc_result_t
totext_ninfo(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	return (generic_totext_txt(CALL_TOTEXT));
}

static isc_result_t
fromwire_ninfo(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_ninfo);

	return (generic_fromwire_txt(CALL_FROMWIRE));
}

static isc_result_t
towire_ninfo(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_ninfo(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_ninfo);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_ninfo(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_ninfo);

	return (generic_fromstruct_txt(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_ninfo(ARGS_TOSTRUCT) {
	dns_rdata_ninfo_t *ninfo = target;

	REQUIRE(rdata->type == dns_rdatatype_ninfo);
	REQUIRE(ninfo != NULL);

	ninfo->common.rdclass = rdata->rdclass;
	ninfo->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ninfo->common, link);

	return (generic_tostruct_txt(CALL_TOSTRUCT));
}

static void
freestruct_ninfo(ARGS_FREESTRUCT) {
	dns_rdata_ninfo_t *ninfo = source;

	REQUIRE(ninfo != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_ninfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_ninfo(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_ninfo(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_ninfo);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_ninfo(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_ninfo(ARGS_COMPARE) {
	return (compare_ninfo(rdata1, rdata2));
}

isc_result_t
dns_rdata_ninfo_first(dns_rdata_ninfo_t *ninfo) {
	REQUIRE(ninfo != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	return (generic_txt_first(ninfo));
}

isc_result_t
dns_rdata_ninfo_next(dns_rdata_ninfo_t *ninfo) {
	REQUIRE(ninfo != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	return (generic_txt_next(ninfo));
}

isc_result_t
dns_rdata_ninfo_current(dns_rdata_ninfo_t *ninfo,
			dns_rdata_ninfo_string_t *string) {
	REQUIRE(ninfo != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	return (generic_txt_current(ninfo, string));
}
#endif /* RDATA_GENERIC_NINFO_56_C */
