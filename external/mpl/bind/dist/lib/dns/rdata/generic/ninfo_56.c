/*	$NetBSD: ninfo_56.c,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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

#ifndef RDATA_GENERIC_NINFO_56_C
#define RDATA_GENERIC_NINFO_56_C

#define RRTYPE_NINFO_ATTRIBUTES (0)

static inline isc_result_t
fromtext_ninfo(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_ninfo);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return (generic_fromtext_txt(rdclass, type, lexer, origin, options,
				     target, callbacks));
}

static inline isc_result_t
totext_ninfo(ARGS_TOTEXT) {

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	return (generic_totext_txt(rdata, tctx, target));
}

static inline isc_result_t
fromwire_ninfo(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_ninfo);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	return (generic_fromwire_txt(rdclass, type, source, dctx, options,
				     target));
}

static inline isc_result_t
towire_ninfo(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
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

static inline isc_result_t
fromstruct_ninfo(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_ninfo);

	return (generic_fromstruct_txt(rdclass, type, source, target));
}

static inline isc_result_t
tostruct_ninfo(ARGS_TOSTRUCT) {
	dns_rdata_ninfo_t *txt = target;

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	txt->common.rdclass = rdata->rdclass;
	txt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&txt->common, link);

	return (generic_tostruct_txt(rdata, target, mctx));
}

static inline void
freestruct_ninfo(ARGS_FREESTRUCT) {
	dns_rdata_ninfo_t *ninfo = source;

	REQUIRE(source != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	generic_freestruct_txt(source);
}

static inline isc_result_t
additionaldata_ninfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_ninfo(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_ninfo(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_ninfo);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_ninfo(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_ninfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
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
			dns_rdata_ninfo_string_t *string)
{

	REQUIRE(ninfo != NULL);
	REQUIRE(ninfo->common.rdtype == dns_rdatatype_ninfo);

	return (generic_txt_current(ninfo, string));
}
#endif	/* RDATA_GENERIC_NINFO_56_C */
