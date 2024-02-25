/*	$NetBSD: smimea_53.c,v 1.8.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

#ifndef RDATA_GENERIC_SMIMEA_53_C
#define RDATA_GENERIC_SMIMEA_53_C

#define RRTYPE_SMIMEA_ATTRIBUTES 0

static isc_result_t
fromtext_smimea(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromtext_tlsa(CALL_FROMTEXT));
}

static isc_result_t
totext_smimea(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_smimea);

	return (generic_totext_tlsa(CALL_TOTEXT));
}

static isc_result_t
fromwire_smimea(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromwire_tlsa(CALL_FROMWIRE));
}

static isc_result_t
towire_smimea(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_smimea);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_smimea(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_smimea);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_smimea(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromstruct_tlsa(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_smimea(ARGS_TOSTRUCT) {
	dns_rdata_smimea_t *smimea = target;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_smimea);
	REQUIRE(smimea != NULL);

	smimea->common.rdclass = rdata->rdclass;
	smimea->common.rdtype = rdata->type;
	ISC_LINK_INIT(&smimea->common, link);

	return (generic_tostruct_tlsa(CALL_TOSTRUCT));
}

static void
freestruct_smimea(ARGS_FREESTRUCT) {
	dns_rdata_smimea_t *smimea = source;

	REQUIRE(smimea != NULL);
	REQUIRE(smimea->common.rdtype == dns_rdatatype_smimea);

	generic_freestruct_tlsa(source);
}

static isc_result_t
additionaldata_smimea(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_smimea);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_smimea(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_smimea);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_smimea(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_smimea);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_smimea(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_smimea);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_smimea(ARGS_COMPARE) {
	return (compare_smimea(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_SMIMEA_53_C */
