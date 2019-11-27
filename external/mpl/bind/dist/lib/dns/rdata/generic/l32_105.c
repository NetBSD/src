/*	$NetBSD: l32_105.c,v 1.4 2019/11/27 05:48:42 christos Exp $	*/

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

#ifndef RDATA_GENERIC_L32_105_C
#define RDATA_GENERIC_L32_105_C

#include <string.h>

#include <isc/net.h>

#define RRTYPE_L32_ATTRIBUTES (0)

static inline isc_result_t
fromtext_l32(ARGS_FROMTEXT) {
	isc_token_t token;
	struct in_addr addr;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_l32);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	if (inet_pton(AF_INET, DNS_AS_STR(token), &addr) != 1)
		RETTOK(DNS_R_BADDOTTEDQUAD);
	isc_buffer_availableregion(target, &region);
	if (region.length < 4)
		return (ISC_R_NOSPACE);
	memmove(region.base, &addr, 4);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_l32(ARGS_TOTEXT) {
	isc_region_t region;
	char buf[sizeof("65000")];
	unsigned short num;

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(rdata->length == 6);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	snprintf(buf, sizeof(buf), "%u", num);
	RETERR(str_totext(buf, target));

	RETERR(str_totext(" ", target));

	return (inet_totext(AF_INET, &region, target));
}

static inline isc_result_t
fromwire_l32(ARGS_FROMWIRE) {
	isc_region_t sregion;

	REQUIRE(type == dns_rdatatype_l32);

	UNUSED(type);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(dctx);

	isc_buffer_activeregion(source, &sregion);
	if (sregion.length != 6)
		return (DNS_R_FORMERR);
	isc_buffer_forward(source, sregion.length);
	return (mem_tobuffer(target, sregion.base, sregion.length));
}

static inline isc_result_t
towire_l32(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(rdata->length == 6);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_l32(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_l32);
	REQUIRE(rdata1->length == 6);
	REQUIRE(rdata2->length == 6);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);
	return (isc_region_compare(&region1, &region2));
}

static inline isc_result_t
fromstruct_l32(ARGS_FROMSTRUCT) {
	dns_rdata_l32_t *l32 = source;
	uint32_t n;

	REQUIRE(type == dns_rdatatype_l32);
	REQUIRE(l32 != NULL);
	REQUIRE(l32->common.rdtype == type);
	REQUIRE(l32->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(l32->pref, target));
	n = ntohl(l32->l32.s_addr);
	return (uint32_tobuffer(n, target));
}

static inline isc_result_t
tostruct_l32(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_l32_t *l32 = target;
	uint32_t n;

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(l32 != NULL);
	REQUIRE(rdata->length == 6);

	UNUSED(mctx);

	l32->common.rdclass = rdata->rdclass;
	l32->common.rdtype = rdata->type;
	ISC_LINK_INIT(&l32->common, link);

	dns_rdata_toregion(rdata, &region);
	l32->pref = uint16_fromregion(&region);
	n = uint32_fromregion(&region);
	l32->l32.s_addr = htonl(n);
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_l32(ARGS_FREESTRUCT) {
	dns_rdata_l32_t *l32 = source;

	REQUIRE(l32 != NULL);
	REQUIRE(l32->common.rdtype == dns_rdatatype_l32);

	return;
}

static inline isc_result_t
additionaldata_l32(ARGS_ADDLDATA) {

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(rdata->length == 6);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_l32(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(rdata->length == 6);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline bool
checkowner_l32(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_l32);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_l32(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_l32);
	REQUIRE(rdata->length == 6);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_l32(ARGS_COMPARE) {
	return (compare_l32(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_L32_105_C */
