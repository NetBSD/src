/*	$NetBSD: a_1.c,v 1.1.2.2 2024/02/24 13:07:15 martin Exp $	*/

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

#ifndef RDATA_HS_4_A_1_C
#define RDATA_HS_4_A_1_C

#include <isc/net.h>

#define RRTYPE_A_ATTRIBUTES (0)

static isc_result_t
fromtext_hs_a(ARGS_FROMTEXT) {
	isc_token_t token;
	struct in_addr addr;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_hs);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	if (inet_pton(AF_INET, DNS_AS_STR(token), &addr) != 1) {
		RETTOK(DNS_R_BADDOTTEDQUAD);
	}
	isc_buffer_availableregion(target, &region);
	if (region.length < 4) {
		return (ISC_R_NOSPACE);
	}
	memmove(region.base, &addr, 4);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_hs_a(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);
	REQUIRE(rdata->length == 4);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	return (inet_totext(AF_INET, tctx->flags, &region, target));
}

static isc_result_t
fromwire_hs_a(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_hs);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);
	if (sregion.length < 4) {
		return (ISC_R_UNEXPECTEDEND);
	}
	if (tregion.length < 4) {
		return (ISC_R_NOSPACE);
	}

	memmove(tregion.base, sregion.base, 4);
	isc_buffer_forward(source, 4);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static isc_result_t
towire_hs_a(ARGS_TOWIRE) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);
	REQUIRE(rdata->length == 4);

	UNUSED(cctx);

	isc_buffer_availableregion(target, &region);
	if (region.length < rdata->length) {
		return (ISC_R_NOSPACE);
	}
	memmove(region.base, rdata->data, rdata->length);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static int
compare_hs_a(ARGS_COMPARE) {
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_a);
	REQUIRE(rdata1->rdclass == dns_rdataclass_hs);
	REQUIRE(rdata1->length == 4);
	REQUIRE(rdata2->length == 4);

	order = memcmp(rdata1->data, rdata2->data, 4);
	if (order != 0) {
		order = (order < 0) ? -1 : 1;
	}

	return (order);
}

static isc_result_t
fromstruct_hs_a(ARGS_FROMSTRUCT) {
	dns_rdata_hs_a_t *a = source;
	uint32_t n;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_hs);
	REQUIRE(a != NULL);
	REQUIRE(a->common.rdtype == type);
	REQUIRE(a->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	n = ntohl(a->in_addr.s_addr);

	return (uint32_tobuffer(n, target));
}

static isc_result_t
tostruct_hs_a(ARGS_TOSTRUCT) {
	dns_rdata_hs_a_t *a = target;
	uint32_t n;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);
	REQUIRE(rdata->length == 4);
	REQUIRE(a != NULL);

	UNUSED(mctx);

	a->common.rdclass = rdata->rdclass;
	a->common.rdtype = rdata->type;
	ISC_LINK_INIT(&a->common, link);

	dns_rdata_toregion(rdata, &region);
	n = uint32_fromregion(&region);
	a->in_addr.s_addr = htonl(n);

	return (ISC_R_SUCCESS);
}

static void
freestruct_hs_a(ARGS_FREESTRUCT) {
	UNUSED(source);

	REQUIRE(source != NULL);
}

static isc_result_t
additionaldata_hs_a(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_hs_a(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_hs_a(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_hs);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_hs_a(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_hs);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_hs_a(ARGS_COMPARE) {
	return (compare_hs_a(rdata1, rdata2));
}

#endif /* RDATA_HS_4_A_1_C */
