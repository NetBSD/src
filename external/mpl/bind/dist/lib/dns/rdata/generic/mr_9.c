/*	$NetBSD: mr_9.c,v 1.4 2019/11/27 05:48:42 christos Exp $	*/

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

#ifndef RDATA_GENERIC_MR_9_C
#define RDATA_GENERIC_MR_9_C

#define RRTYPE_MR_ATTRIBUTES (0)

static inline isc_result_t
fromtext_mr(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == dns_rdatatype_mr);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL)
		origin = dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_mr(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;

	REQUIRE(rdata->type == dns_rdatatype_mr);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_mr(ARGS_FROMWIRE) {
	dns_name_t name;

	REQUIRE(type == dns_rdatatype_mr);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, NULL);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_mr(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_mr);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_mr(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_mr);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_mr(ARGS_FROMSTRUCT) {
	dns_rdata_mr_t *mr = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_mr);
	REQUIRE(mr != NULL);
	REQUIRE(mr->common.rdtype == type);
	REQUIRE(mr->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&mr->mr, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_mr(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_mr_t *mr = target;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_mr);
	REQUIRE(mr != NULL);
	REQUIRE(rdata->length != 0);

	mr->common.rdclass = rdata->rdclass;
	mr->common.rdtype = rdata->type;
	ISC_LINK_INIT(&mr->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&mr->mr, NULL);
	RETERR(name_duporclone(&name, mctx, &mr->mr));
	mr->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_mr(ARGS_FREESTRUCT) {
	dns_rdata_mr_t *mr = source;

	REQUIRE(mr != NULL);
	REQUIRE(mr->common.rdtype == dns_rdatatype_mr);

	if (mr->mctx == NULL)
		return;
	dns_name_free(&mr->mr, mr->mctx);
	mr->mctx = NULL;
}

static inline isc_result_t
additionaldata_mr(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_mr);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_mr(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_mr);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static inline bool
checkowner_mr(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_mr);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_mr(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_mr);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_mr(ARGS_COMPARE) {
	return (compare_mr(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_MR_9_C */
