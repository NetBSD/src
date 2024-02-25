/*	$NetBSD: null_10.c,v 1.7.2.1 2024/02/25 15:47:04 martin Exp $	*/

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

#ifndef RDATA_GENERIC_NULL_10_C
#define RDATA_GENERIC_NULL_10_C

#define RRTYPE_NULL_ATTRIBUTES (0)

static isc_result_t
fromtext_null(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_null);

	UNUSED(rdclass);
	UNUSED(type);
	UNUSED(lexer);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(target);
	UNUSED(callbacks);

	return (DNS_R_SYNTAX);
}

static isc_result_t
totext_null(ARGS_TOTEXT) {
	REQUIRE(rdata->type == dns_rdatatype_null);

	return (unknown_totext(rdata, tctx, target));
}

static isc_result_t
fromwire_null(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == dns_rdatatype_null);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static isc_result_t
towire_null(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_null);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_null(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_null);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_null(ARGS_FROMSTRUCT) {
	dns_rdata_null_t *null = source;

	REQUIRE(type == dns_rdatatype_null);
	REQUIRE(null != NULL);
	REQUIRE(null->common.rdtype == type);
	REQUIRE(null->common.rdclass == rdclass);
	REQUIRE(null->data != NULL || null->length == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, null->data, null->length));
}

static isc_result_t
tostruct_null(ARGS_TOSTRUCT) {
	dns_rdata_null_t *null = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_null);
	REQUIRE(null != NULL);

	null->common.rdclass = rdata->rdclass;
	null->common.rdtype = rdata->type;
	ISC_LINK_INIT(&null->common, link);

	dns_rdata_toregion(rdata, &r);
	null->length = r.length;
	null->data = mem_maybedup(mctx, r.base, r.length);
	if (null->data == NULL) {
		return (ISC_R_NOMEMORY);
	}

	null->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_null(ARGS_FREESTRUCT) {
	dns_rdata_null_t *null = source;

	REQUIRE(null != NULL);
	REQUIRE(null->common.rdtype == dns_rdatatype_null);

	if (null->mctx == NULL) {
		return;
	}

	if (null->data != NULL) {
		isc_mem_free(null->mctx, null->data);
	}
	null->mctx = NULL;
}

static isc_result_t
additionaldata_null(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_null);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_null(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_null);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_null(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_null);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_null(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_null);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_null(ARGS_COMPARE) {
	return (compare_null(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_NULL_10_C */
