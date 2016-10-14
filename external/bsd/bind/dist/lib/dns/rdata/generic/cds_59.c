/*	$NetBSD: cds_59.c,v 1.1.1.3.2.5 2016/10/14 12:01:29 martin Exp $	*/

/*
 * Copyright (C) 2014, 2015  Internet Systems Consortium, Inc. ("ISC")
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

/* draft-ietf-dnsop-delegation-trust-maintainance-14 */

#ifndef RDATA_GENERIC_CDS_59_C
#define RDATA_GENERIC_CDS_59_C

#define RRTYPE_CDS_ATTRIBUTES 0

#include <isc/sha1.h>
#include <isc/sha2.h>

#include <dns/ds.h>

#include "dst_gost.h"

static inline isc_result_t
fromtext_cds(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromtext_ds(rdclass, type, lexer, origin, options,
				    target, callbacks));
}

static inline isc_result_t
totext_cds(ARGS_TOTEXT) {

	REQUIRE(rdata->type == dns_rdatatype_cds);

	return (generic_totext_ds(rdata, tctx, target));
}

static inline isc_result_t
fromwire_cds(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromwire_ds(rdclass, type, source, dctx, options,
				    target));
}

static inline isc_result_t
towire_cds(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_cds);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_cds(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_cds);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_cds(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromstruct_ds(rdclass, type, source, target));
}

static inline isc_result_t
tostruct_cds(ARGS_TOSTRUCT) {
	dns_rdata_cds_t *cds = target;

	REQUIRE(rdata->type == dns_rdatatype_cds);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	/*
	 * Checked by generic_tostruct_ds().
	 */
	cds->common.rdclass = rdata->rdclass;
	cds->common.rdtype = rdata->type;
	ISC_LINK_INIT(&cds->common, link);

	return (generic_tostruct_ds(rdata, target, mctx));
}

static inline void
freestruct_cds(ARGS_FREESTRUCT) {
	dns_rdata_cds_t *ds = source;

	REQUIRE(ds != NULL);
	REQUIRE(ds->common.rdtype == dns_rdatatype_cds);

	if (ds->mctx == NULL)
		return;

	if (ds->digest != NULL)
		isc_mem_free(ds->mctx, ds->digest);
	ds->mctx = NULL;
}

static inline isc_result_t
additionaldata_cds(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_cds);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_cds(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_cds);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_cds(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_cds);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_cds(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_cds);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_cds(ARGS_COMPARE) {
	return (compare_cds(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_CDS_59_C */
