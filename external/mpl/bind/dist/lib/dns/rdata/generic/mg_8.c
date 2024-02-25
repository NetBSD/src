/*	$NetBSD: mg_8.c,v 1.7.2.1 2024/02/25 15:47:03 martin Exp $	*/

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

#ifndef RDATA_GENERIC_MG_8_C
#define RDATA_GENERIC_MG_8_C

#define RRTYPE_MG_ATTRIBUTES (0)

static isc_result_t
fromtext_mg(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == dns_rdatatype_mg);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL) {
		origin = dns_rootname;
	}
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_mg(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;

	REQUIRE(rdata->type == dns_rdatatype_mg);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static isc_result_t
fromwire_mg(ARGS_FROMWIRE) {
	dns_name_t name;

	REQUIRE(type == dns_rdatatype_mg);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, NULL);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static isc_result_t
towire_mg(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_mg);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static int
compare_mg(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_mg);
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

static isc_result_t
fromstruct_mg(ARGS_FROMSTRUCT) {
	dns_rdata_mg_t *mg = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_mg);
	REQUIRE(mg != NULL);
	REQUIRE(mg->common.rdtype == type);
	REQUIRE(mg->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&mg->mg, &region);
	return (isc_buffer_copyregion(target, &region));
}

static isc_result_t
tostruct_mg(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_mg_t *mg = target;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_mg);
	REQUIRE(mg != NULL);
	REQUIRE(rdata->length != 0);

	mg->common.rdclass = rdata->rdclass;
	mg->common.rdtype = rdata->type;
	ISC_LINK_INIT(&mg->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&mg->mg, NULL);
	name_duporclone(&name, mctx, &mg->mg);
	mg->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_mg(ARGS_FREESTRUCT) {
	dns_rdata_mg_t *mg = source;

	REQUIRE(mg != NULL);
	REQUIRE(mg->common.rdtype == dns_rdatatype_mg);

	if (mg->mctx == NULL) {
		return;
	}
	dns_name_free(&mg->mg, mg->mctx);
	mg->mctx = NULL;
}

static isc_result_t
additionaldata_mg(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_mg);

	UNUSED(add);
	UNUSED(arg);
	UNUSED(rdata);
	UNUSED(owner);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_mg(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_mg);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static bool
checkowner_mg(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_mg);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (dns_name_ismailbox(name));
}

static bool
checknames_mg(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_mg);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_mg(ARGS_COMPARE) {
	return (compare_mg(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_MG_8_C */
