/*	$NetBSD: cds_59.c,v 1.1.1.7 2022/09/23 12:09:20 christos Exp $	*/

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

/* draft-ietf-dnsop-delegation-trust-maintainance-14 */

#ifndef RDATA_GENERIC_CDS_59_C
#define RDATA_GENERIC_CDS_59_C

#define RRTYPE_CDS_ATTRIBUTES 0

#include <dns/ds.h>

static isc_result_t
fromtext_cds(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromtext_ds(CALL_FROMTEXT));
}

static isc_result_t
totext_cds(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_cds);

	return (generic_totext_ds(CALL_TOTEXT));
}

static isc_result_t
fromwire_cds(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromwire_ds(CALL_FROMWIRE));
}

static isc_result_t
towire_cds(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_cds);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_cds(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_cds);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_cds(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_cds);

	return (generic_fromstruct_ds(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_cds(ARGS_TOSTRUCT) {
	dns_rdata_cds_t *cds = target;

	REQUIRE(rdata->type == dns_rdatatype_cds);
	REQUIRE(cds != NULL);
	REQUIRE(rdata->length != 0);

	/*
	 * Checked by generic_tostruct_ds().
	 */
	cds->common.rdclass = rdata->rdclass;
	cds->common.rdtype = rdata->type;
	ISC_LINK_INIT(&cds->common, link);

	return (generic_tostruct_ds(CALL_TOSTRUCT));
}

static void
freestruct_cds(ARGS_FREESTRUCT) {
	dns_rdata_cds_t *cds = source;

	REQUIRE(cds != NULL);
	REQUIRE(cds->common.rdtype == dns_rdatatype_cds);

	if (cds->mctx == NULL) {
		return;
	}

	if (cds->digest != NULL) {
		isc_mem_free(cds->mctx, cds->digest);
	}
	cds->mctx = NULL;
}

static isc_result_t
additionaldata_cds(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_cds);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_cds(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_cds);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_cds(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_cds);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_cds(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_cds);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_cds(ARGS_COMPARE) {
	return (compare_cds(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_CDS_59_C */
