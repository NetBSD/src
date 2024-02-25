/*	$NetBSD: spf_99.c,v 1.8.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

#ifndef RDATA_GENERIC_SPF_99_C
#define RDATA_GENERIC_SPF_99_C

#define RRTYPE_SPF_ATTRIBUTES (0)

static isc_result_t
fromtext_spf(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_spf);

	return (generic_fromtext_txt(CALL_FROMTEXT));
}

static isc_result_t
totext_spf(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_spf);

	return (generic_totext_txt(CALL_TOTEXT));
}

static isc_result_t
fromwire_spf(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_spf);

	return (generic_fromwire_txt(CALL_FROMWIRE));
}

static isc_result_t
towire_spf(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_spf(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_spf);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_spf(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_spf);

	return (generic_fromstruct_txt(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_spf(ARGS_TOSTRUCT) {
	dns_rdata_spf_t *spf = target;

	REQUIRE(spf != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_spf);

	spf->common.rdclass = rdata->rdclass;
	spf->common.rdtype = rdata->type;
	ISC_LINK_INIT(&spf->common, link);

	return (generic_tostruct_txt(CALL_TOSTRUCT));
}

static void
freestruct_spf(ARGS_FREESTRUCT) {
	dns_rdata_spf_t *spf = source;

	REQUIRE(spf != NULL);
	REQUIRE(spf->common.rdtype == dns_rdatatype_spf);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_spf(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_spf(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_spf);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_spf(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_spf);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_spf(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_spf(ARGS_COMPARE) {
	return (compare_spf(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_SPF_99_C */
