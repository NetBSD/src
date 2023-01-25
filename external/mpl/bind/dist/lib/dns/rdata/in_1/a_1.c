/*	$NetBSD: a_1.c,v 1.9 2023/01/25 21:43:30 christos Exp $	*/

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

#ifndef RDATA_IN_1_A_1_C
#define RDATA_IN_1_A_1_C

#include <string.h>

#include <isc/net.h>

#define RRTYPE_A_ATTRIBUTES (0)

static isc_result_t
fromtext_in_a(ARGS_FROMTEXT) {
	isc_token_t token;
	struct in_addr addr;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_in);

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
totext_in_a(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length == 4);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	return (inet_totext(AF_INET, tctx->flags, &region, target));
}

static isc_result_t
fromwire_in_a(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_in);

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
towire_in_a(ARGS_TOWIRE) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
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
compare_in_a(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_a);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length == 4);
	REQUIRE(rdata2->length == 4);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_in_a(ARGS_FROMSTRUCT) {
	dns_rdata_in_a_t *a = source;
	uint32_t n;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(a != NULL);
	REQUIRE(a->common.rdtype == type);
	REQUIRE(a->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	n = ntohl(a->in_addr.s_addr);

	return (uint32_tobuffer(n, target));
}

static isc_result_t
tostruct_in_a(ARGS_TOSTRUCT) {
	dns_rdata_in_a_t *a = target;
	uint32_t n;
	isc_region_t region;

	REQUIRE(a != NULL);
	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length == 4);

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
freestruct_in_a(ARGS_FREESTRUCT) {
	dns_rdata_in_a_t *a = source;

	REQUIRE(a != NULL);
	REQUIRE(a->common.rdtype == dns_rdatatype_a);
	REQUIRE(a->common.rdclass == dns_rdataclass_in);

	UNUSED(a);
}

static isc_result_t
additionaldata_in_a(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_in_a(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_in_a(ARGS_CHECKOWNER) {
	dns_name_t prefix, suffix;
	unsigned int labels, i;

	REQUIRE(type == dns_rdatatype_a);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(rdclass);

	labels = dns_name_countlabels(name);
	if (labels > 2U) {
		/*
		 * Handle Active Directory gc._msdcs.<forest> name.
		 */
		dns_name_init(&prefix, NULL);
		dns_name_init(&suffix, NULL);
		dns_name_split(name, labels - 2, &prefix, &suffix);
		if (dns_name_equal(&gc_msdcs, &prefix) &&
		    dns_name_ishostname(&suffix, false))
		{
			return (true);
		}

		/*
		 * Handle SPF exists targets when the seperating label is:
		 * - "_spf" RFC7208, section 5.7
		 * - "_spf_verify" RFC7208, Appendix D1
		 * - "_spf_rate" RFC7208, Appendix D1
		 */
		for (i = 0; i < labels - 2; i++) {
			dns_label_t label;
			dns_name_getlabel(name, i, &label);
			if ((label.length == 5 &&
			     strncasecmp((char *)label.base, "\x04_spf", 5) ==
				     0) ||
			    (label.length == 12 &&
			     strncasecmp((char *)label.base, "\x0b_spf_verify",
					 12) == 0) ||
			    (label.length == 10 &&
			     strncasecmp((char *)label.base, "\x09_spf_rate",
					 10) == 0))
			{
				return (true);
			}
		}
	}

	return (dns_name_ishostname(name, wildcard));
}

static bool
checknames_in_a(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_a);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_in_a(ARGS_COMPARE) {
	return (compare_in_a(rdata1, rdata2));
}

#endif /* RDATA_IN_1_A_1_C */
