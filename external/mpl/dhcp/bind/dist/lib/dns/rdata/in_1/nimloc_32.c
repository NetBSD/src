/*	$NetBSD: nimloc_32.c,v 1.1.2.2 2024/02/24 13:07:16 martin Exp $	*/

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

/* http://ana-3.lcs.mit.edu/~jnc/nimrod/dns.txt */

#ifndef RDATA_IN_1_NIMLOC_32_C
#define RDATA_IN_1_NIMLOC_32_C

#define RRTYPE_NIMLOC_ATTRIBUTES (0)

static isc_result_t
fromtext_in_nimloc(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_nimloc);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	return (isc_hex_tobuffer(lexer, target, -2));
}

static isc_result_t
totext_in_nimloc(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext("( ", target));
	}
	if (tctx->width == 0) {
		RETERR(isc_hex_totext(&region, 60, "", target));
	} else {
		RETERR(isc_hex_totext(&region, tctx->width - 2, tctx->linebreak,
				      target));
	}
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" )", target));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
fromwire_in_nimloc(ARGS_FROMWIRE) {
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_nimloc);
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

static isc_result_t
towire_in_nimloc(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_in_nimloc(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_nimloc);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_in_nimloc(ARGS_FROMSTRUCT) {
	dns_rdata_in_nimloc_t *nimloc = source;

	REQUIRE(type == dns_rdatatype_nimloc);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(nimloc != NULL);
	REQUIRE(nimloc->common.rdtype == type);
	REQUIRE(nimloc->common.rdclass == rdclass);
	REQUIRE(nimloc->nimloc != NULL || nimloc->nimloc_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, nimloc->nimloc, nimloc->nimloc_len));
}

static isc_result_t
tostruct_in_nimloc(ARGS_TOSTRUCT) {
	dns_rdata_in_nimloc_t *nimloc = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(nimloc != NULL);
	REQUIRE(rdata->length != 0);

	nimloc->common.rdclass = rdata->rdclass;
	nimloc->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nimloc->common, link);

	dns_rdata_toregion(rdata, &r);
	nimloc->nimloc_len = r.length;
	nimloc->nimloc = mem_maybedup(mctx, r.base, r.length);
	if (nimloc->nimloc == NULL) {
		return (ISC_R_NOMEMORY);
	}

	nimloc->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_in_nimloc(ARGS_FREESTRUCT) {
	dns_rdata_in_nimloc_t *nimloc = source;

	REQUIRE(nimloc != NULL);
	REQUIRE(nimloc->common.rdclass == dns_rdataclass_in);
	REQUIRE(nimloc->common.rdtype == dns_rdatatype_nimloc);

	if (nimloc->mctx == NULL) {
		return;
	}

	if (nimloc->nimloc != NULL) {
		isc_mem_free(nimloc->mctx, nimloc->nimloc);
	}
	nimloc->mctx = NULL;
}

static isc_result_t
additionaldata_in_nimloc(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_in_nimloc(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_in_nimloc(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_nimloc);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_in_nimloc(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_nimloc);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_in_nimloc(ARGS_COMPARE) {
	return (compare_in_nimloc(rdata1, rdata2));
}

#endif /* RDATA_IN_1_NIMLOC_32_C */
