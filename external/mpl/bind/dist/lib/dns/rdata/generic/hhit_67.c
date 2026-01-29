/*	$NetBSD: hhit_67.c,v 1.1.1.1 2026/01/29 18:19:55 christos Exp $	*/

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

#ifndef RDATA_GENERIC_HHIT_67_C
#define RDATA_GENERIC_HHIT_67_C

#include <dst/dst.h>

#define RRTYPE_HHIT_ATTRIBUTES (0)

static isc_result_t
fromtext_hhit(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_hhit);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return isc_base64_tobuffer(lexer, target, -1);
}

static isc_result_t
totext_hhit(ARGS_TOTEXT) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_hhit);
	REQUIRE(rdata->length > 0);

	dns_rdata_toregion(rdata, &sr);

	/* data */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" (", target));
	}

	RETERR(str_totext(tctx->linebreak, target));

	if (tctx->width == 0) { /* No splitting */
		RETERR(isc_base64_totext(&sr, 60, "", target));
	} else {
		RETERR(isc_base64_totext(&sr, tctx->width - 2, tctx->linebreak,
					 target));
	}

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" )", target));
	}

	return ISC_R_SUCCESS;
}

static isc_result_t
fromwire_hhit(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == dns_rdatatype_hhit);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);

	isc_buffer_activeregion(source, &sr);
	if (sr.length == 0) {
		return ISC_R_UNEXPECTEDEND;
	}

	RETERR(mem_tobuffer(target, sr.base, sr.length));
	isc_buffer_forward(source, sr.length);
	return ISC_R_SUCCESS;
}

static isc_result_t
towire_hhit(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_hhit);
	REQUIRE(rdata->length > 0);

	UNUSED(cctx);

	return mem_tobuffer(target, rdata->data, rdata->length);
}

static int
compare_hhit(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_hhit);
	REQUIRE(rdata1->length > 0);
	REQUIRE(rdata2->length > 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return isc_region_compare(&r1, &r2);
}

static isc_result_t
fromstruct_hhit(ARGS_FROMSTRUCT) {
	dns_rdata_hhit_t *hhit = source;

	REQUIRE(type == dns_rdatatype_hhit);
	REQUIRE(hhit != NULL);
	REQUIRE(hhit->common.rdtype == type);
	REQUIRE(hhit->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	/* Data */
	return mem_tobuffer(target, hhit->data, hhit->datalen);
}

static isc_result_t
tostruct_hhit(ARGS_TOSTRUCT) {
	dns_rdata_hhit_t *hhit = target;
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_hhit);
	REQUIRE(hhit != NULL);
	REQUIRE(rdata->length > 0);

	DNS_RDATACOMMON_INIT(hhit, rdata->type, rdata->rdclass);

	dns_rdata_toregion(rdata, &sr);

	/* Data */
	hhit->datalen = sr.length;
	hhit->data = mem_maybedup(mctx, sr.base, hhit->datalen);
	hhit->mctx = mctx;
	return ISC_R_SUCCESS;
}

static void
freestruct_hhit(ARGS_FREESTRUCT) {
	dns_rdata_hhit_t *hhit = (dns_rdata_hhit_t *)source;

	REQUIRE(hhit != NULL);
	REQUIRE(hhit->common.rdtype == dns_rdatatype_hhit);

	if (hhit->mctx == NULL) {
		return;
	}

	if (hhit->data != NULL) {
		isc_mem_free(hhit->mctx, hhit->data);
	}
	hhit->mctx = NULL;
}

static isc_result_t
additionaldata_hhit(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_hhit);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return ISC_R_SUCCESS;
}

static isc_result_t
digest_hhit(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_hhit);

	dns_rdata_toregion(rdata, &r);

	return (digest)(arg, &r);
}

static bool
checkowner_hhit(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_hhit);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return true;
}

static bool
checknames_hhit(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_hhit);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return true;
}

static int
casecompare_hhit(ARGS_COMPARE) {
	return compare_hhit(rdata1, rdata2);
}
#endif /* RDATA_GENERIC_HHIT_67_C */
