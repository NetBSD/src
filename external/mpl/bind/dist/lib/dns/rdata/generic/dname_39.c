/*	$NetBSD: dname_39.c,v 1.7.2.1 2024/02/25 15:47:01 martin Exp $	*/

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

/* RFC2672 */

#ifndef RDATA_GENERIC_DNAME_39_C
#define RDATA_GENERIC_DNAME_39_C

#define RRTYPE_DNAME_ATTRIBUTES (DNS_RDATATYPEATTR_SINGLETON)

static isc_result_t
fromtext_dname(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == dns_rdatatype_dname);

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
totext_dname(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;

	REQUIRE(rdata->type == dns_rdatatype_dname);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static isc_result_t
fromwire_dname(ARGS_FROMWIRE) {
	dns_name_t name;

	REQUIRE(type == dns_rdatatype_dname);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static isc_result_t
towire_dname(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_dname);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static int
compare_dname(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_dname);
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
fromstruct_dname(ARGS_FROMSTRUCT) {
	dns_rdata_dname_t *dname = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_dname);
	REQUIRE(dname != NULL);
	REQUIRE(dname->common.rdtype == type);
	REQUIRE(dname->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&dname->dname, &region);
	return (isc_buffer_copyregion(target, &region));
}

static isc_result_t
tostruct_dname(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_dname_t *dname = target;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_dname);
	REQUIRE(dname != NULL);
	REQUIRE(rdata->length != 0);

	dname->common.rdclass = rdata->rdclass;
	dname->common.rdtype = rdata->type;
	ISC_LINK_INIT(&dname->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&dname->dname, NULL);
	name_duporclone(&name, mctx, &dname->dname);
	dname->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_dname(ARGS_FREESTRUCT) {
	dns_rdata_dname_t *dname = source;

	REQUIRE(dname != NULL);
	REQUIRE(dname->common.rdtype == dns_rdatatype_dname);

	if (dname->mctx == NULL) {
		return;
	}

	dns_name_free(&dname->dname, dname->mctx);
	dname->mctx = NULL;
}

static isc_result_t
additionaldata_dname(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_dname);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_dname(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_dname);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static bool
checkowner_dname(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_dname);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_dname(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_dname);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_dname(ARGS_COMPARE) {
	return (compare_dname(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_DNAME_39_C */
