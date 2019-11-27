/*	$NetBSD: spf_99.c,v 1.4 2019/11/27 05:48:42 christos Exp $	*/

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

#ifndef RDATA_GENERIC_SPF_99_C
#define RDATA_GENERIC_SPF_99_C

#define RRTYPE_SPF_ATTRIBUTES (0)

static inline isc_result_t
fromtext_spf(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_spf);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return (generic_fromtext_txt(rdclass, type, lexer, origin, options,
				     target, callbacks));
}

static inline isc_result_t
totext_spf(ARGS_TOTEXT) {

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_spf);

	return (generic_totext_txt(rdata, tctx, target));
}

static inline isc_result_t
fromwire_spf(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_spf);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	return (generic_fromwire_txt(rdclass, type, source, dctx, options,
				     target));
}

static inline isc_result_t
towire_spf(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
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

static inline isc_result_t
fromstruct_spf(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_spf);

	return (generic_fromstruct_txt(rdclass, type, source, target));
}

static inline isc_result_t
tostruct_spf(ARGS_TOSTRUCT) {
	dns_rdata_spf_t *spf = target;

	REQUIRE(spf != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_spf);

	spf->common.rdclass = rdata->rdclass;
	spf->common.rdtype = rdata->type;
	ISC_LINK_INIT(&spf->common, link);

	return (generic_tostruct_txt(rdata, target, mctx));
}

static inline void
freestruct_spf(ARGS_FREESTRUCT) {
	dns_rdata_spf_t *spf = source;

	REQUIRE(spf != NULL);
	REQUIRE(spf->common.rdtype == dns_rdatatype_spf);

	generic_freestruct_txt(source);
}

static inline isc_result_t
additionaldata_spf(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_spf(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_spf);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline bool
checkowner_spf(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_spf);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_spf(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_spf);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_spf(ARGS_COMPARE) {
	return (compare_spf(rdata1, rdata2));
}
#endif	/* RDATA_GENERIC_SPF_99_C */
