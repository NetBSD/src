/*	$NetBSD: hinfo_13.c,v 1.1.2.2 2024/02/24 13:07:11 martin Exp $	*/

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

#ifndef RDATA_GENERIC_HINFO_13_C
#define RDATA_GENERIC_HINFO_13_C

#define RRTYPE_HINFO_ATTRIBUTES (0)

static isc_result_t
fromtext_hinfo(ARGS_FROMTEXT) {
	isc_token_t token;
	int i;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	REQUIRE(type == dns_rdatatype_hinfo);

	for (i = 0; i < 2; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_qstring, false));
		RETTOK(txt_fromtext(&token.value.as_textregion, target));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_hinfo(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_hinfo);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);
	RETERR(txt_totext(&region, true, target));
	RETERR(str_totext(" ", target));
	return (txt_totext(&region, true, target));
}

static isc_result_t
fromwire_hinfo(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_hinfo);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	RETERR(txt_fromwire(source, target));
	return (txt_fromwire(source, target));
}

static isc_result_t
towire_hinfo(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == dns_rdatatype_hinfo);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_hinfo(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_hinfo);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_hinfo(ARGS_FROMSTRUCT) {
	dns_rdata_hinfo_t *hinfo = source;

	REQUIRE(type == dns_rdatatype_hinfo);
	REQUIRE(hinfo != NULL);
	REQUIRE(hinfo->common.rdtype == type);
	REQUIRE(hinfo->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(hinfo->cpu_len, target));
	RETERR(mem_tobuffer(target, hinfo->cpu, hinfo->cpu_len));
	RETERR(uint8_tobuffer(hinfo->os_len, target));
	return (mem_tobuffer(target, hinfo->os, hinfo->os_len));
}

static isc_result_t
tostruct_hinfo(ARGS_TOSTRUCT) {
	dns_rdata_hinfo_t *hinfo = target;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_hinfo);
	REQUIRE(hinfo != NULL);
	REQUIRE(rdata->length != 0);

	hinfo->common.rdclass = rdata->rdclass;
	hinfo->common.rdtype = rdata->type;
	ISC_LINK_INIT(&hinfo->common, link);

	dns_rdata_toregion(rdata, &region);
	hinfo->cpu_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	hinfo->cpu = mem_maybedup(mctx, region.base, hinfo->cpu_len);
	if (hinfo->cpu == NULL) {
		return (ISC_R_NOMEMORY);
	}
	isc_region_consume(&region, hinfo->cpu_len);

	hinfo->os_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	hinfo->os = mem_maybedup(mctx, region.base, hinfo->os_len);
	if (hinfo->os == NULL) {
		goto cleanup;
	}

	hinfo->mctx = mctx;
	return (ISC_R_SUCCESS);

cleanup:
	if (mctx != NULL && hinfo->cpu != NULL) {
		isc_mem_free(mctx, hinfo->cpu);
	}
	return (ISC_R_NOMEMORY);
}

static void
freestruct_hinfo(ARGS_FREESTRUCT) {
	dns_rdata_hinfo_t *hinfo = source;

	REQUIRE(hinfo != NULL);

	if (hinfo->mctx == NULL) {
		return;
	}

	if (hinfo->cpu != NULL) {
		isc_mem_free(hinfo->mctx, hinfo->cpu);
	}
	if (hinfo->os != NULL) {
		isc_mem_free(hinfo->mctx, hinfo->os);
	}
	hinfo->mctx = NULL;
}

static isc_result_t
additionaldata_hinfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_hinfo);

	UNUSED(add);
	UNUSED(arg);
	UNUSED(rdata);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_hinfo(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_hinfo);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_hinfo(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_hinfo);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_hinfo(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_hinfo);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_hinfo(ARGS_COMPARE) {
	return (compare_hinfo(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_HINFO_13_C */
