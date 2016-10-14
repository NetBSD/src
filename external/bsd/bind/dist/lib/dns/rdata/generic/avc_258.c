/*	$NetBSD: avc_258.c,v 1.1.1.1.6.2 2016/10/14 12:01:29 martin Exp $	*/

/*
 * Copyright (C) 2016  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef RDATA_GENERIC_AVC_258_C
#define RDATA_GENERIC_AVC_258_C

#define RRTYPE_AVC_ATTRIBUTES (0)

static inline isc_result_t
fromtext_avc(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_avc);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return (generic_fromtext_txt(rdclass, type, lexer, origin, options,
				     target, callbacks));
}

static inline isc_result_t
totext_avc(ARGS_TOTEXT) {

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_avc);

	return (generic_totext_txt(rdata, tctx, target));
}

static inline isc_result_t
fromwire_avc(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_avc);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	return (generic_fromwire_txt(rdclass, type, source, dctx, options,
				     target));
}

static inline isc_result_t
towire_avc(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
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

static inline isc_result_t
fromstruct_avc(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_avc);

	return (generic_fromstruct_txt(rdclass, type, source, target));
}

static inline isc_result_t
tostruct_avc(ARGS_TOSTRUCT) {
	dns_rdata_avc_t *avc = target;

	REQUIRE(rdata->type == dns_rdatatype_avc);
	REQUIRE(target != NULL);

	avc->common.rdclass = rdata->rdclass;
	avc->common.rdtype = rdata->type;
	ISC_LINK_INIT(&avc->common, link);

	return (generic_tostruct_txt(rdata, target, mctx));
}

static inline void
freestruct_avc(ARGS_FREESTRUCT) {
	dns_rdata_avc_t *txt = source;

	REQUIRE(source != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_avc);

	generic_freestruct_txt(source);
}

static inline isc_result_t
additionaldata_avc(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_avc(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_avc);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_avc(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_avc);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_avc(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_avc);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_avc(ARGS_COMPARE) {
	return (compare_avc(rdata1, rdata2));
}
#endif	/* RDATA_GENERIC_AVC_258_C */
