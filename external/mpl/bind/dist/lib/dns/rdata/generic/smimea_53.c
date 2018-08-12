/*	$NetBSD: smimea_53.c,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#ifndef RDATA_GENERIC_SMIMEA_53_C
#define RDATA_GENERIC_SMIMEA_53_C

#define RRTYPE_SMIMEA_ATTRIBUTES 0

static inline isc_result_t
fromtext_smimea(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromtext_tlsa(rdclass, type, lexer, origin, options,
				      target, callbacks));
}

static inline isc_result_t
totext_smimea(ARGS_TOTEXT) {

	REQUIRE(rdata->type == dns_rdatatype_smimea);

	return (generic_totext_tlsa(rdata, tctx, target));
}

static inline isc_result_t
fromwire_smimea(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromwire_tlsa(rdclass, type, source, dctx, options,
				      target));
}

static inline isc_result_t
towire_smimea(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_smimea);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
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

static inline isc_result_t
fromstruct_smimea(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_smimea);

	return (generic_fromstruct_tlsa(rdclass, type, source, target));
}

static inline isc_result_t
tostruct_smimea(ARGS_TOSTRUCT) {
	dns_rdata_txt_t *txt = target;

	REQUIRE(rdata->type == dns_rdatatype_smimea);
	REQUIRE(target != NULL);

	txt->common.rdclass = rdata->rdclass;
	txt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&txt->common, link);

	return (generic_tostruct_tlsa(rdata, target, mctx));
}

static inline void
freestruct_smimea(ARGS_FREESTRUCT) {
	dns_rdata_txt_t *txt = source;

	REQUIRE(source != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_smimea);

	generic_freestruct_tlsa(source);
}

static inline isc_result_t
additionaldata_smimea(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_smimea);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_smimea(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_smimea);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_smimea(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_smimea);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_smimea(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_smimea);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_smimea(ARGS_COMPARE) {
	return (compare_smimea(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_SMIMEA_53_C */
