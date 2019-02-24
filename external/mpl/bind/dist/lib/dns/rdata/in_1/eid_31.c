/*	$NetBSD: eid_31.c,v 1.3 2019/02/24 20:01:31 christos Exp $	*/

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

/* http://ana-3.lcs.mit.edu/~jnc/nimrod/dns.txt */

#ifndef RDATA_IN_1_EID_31_C
#define RDATA_IN_1_EID_31_C

#define RRTYPE_EID_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_eid(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_eid);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	return (isc_hex_tobuffer(lexer, target, -2));
}

static inline isc_result_t
totext_in_eid(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);

	if (tctx->width == 0) {
		return (isc_hex_totext(&region, 60, "", target));
	} else {
		return (isc_hex_totext(&region, tctx->width - 2,
				       tctx->linebreak, target));
	}
}

static inline isc_result_t
fromwire_in_eid(ARGS_FROMWIRE) {
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_eid);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &region);
	if (region.length < 1) {
		return (ISC_R_UNEXPECTEDEND);
	}

	RETERR(mem_tobuffer(target, region.base, region.length));
	isc_buffer_forward(source, region.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_in_eid(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_in_eid(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_eid);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_in_eid(ARGS_FROMSTRUCT) {
	dns_rdata_in_eid_t *eid = source;

	REQUIRE(type == dns_rdatatype_eid);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(source != NULL);
	REQUIRE(eid->common.rdtype == type);
	REQUIRE(eid->common.rdclass == rdclass);
	REQUIRE(eid->eid != NULL || eid->eid_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, eid->eid, eid->eid_len));
}

static inline isc_result_t
tostruct_in_eid(ARGS_TOSTRUCT) {
	dns_rdata_in_eid_t *eid = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	eid->common.rdclass = rdata->rdclass;
	eid->common.rdtype = rdata->type;
	ISC_LINK_INIT(&eid->common, link);

	dns_rdata_toregion(rdata, &r);
	eid->eid_len = r.length;
	eid->eid = mem_maybedup(mctx, r.base, r.length);
	if (eid->eid == NULL) {
		return (ISC_R_NOMEMORY);
	}

	eid->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_eid(ARGS_FREESTRUCT) {
	dns_rdata_in_eid_t *eid = source;

	REQUIRE(source != NULL);
	REQUIRE(eid->common.rdclass == dns_rdataclass_in);
	REQUIRE(eid->common.rdtype == dns_rdatatype_eid);

	if (eid->mctx == NULL) {
		return;
	}

	if (eid->eid != NULL) {
		isc_mem_free(eid->mctx, eid->eid);
	}
	eid->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_eid(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_eid(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline bool
checkowner_in_eid(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_eid);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_in_eid(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_eid);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_in_eid(ARGS_COMPARE) {
	return (compare_in_eid(rdata1, rdata2));
}

#endif	/* RDATA_IN_1_EID_31_C */
