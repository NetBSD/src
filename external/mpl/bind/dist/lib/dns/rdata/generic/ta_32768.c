/*	$NetBSD: ta_32768.c,v 1.8.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

/* http://www.watson.org/~weiler/INI1999-19.pdf */

#ifndef RDATA_GENERIC_TA_32768_C
#define RDATA_GENERIC_TA_32768_C

#define RRTYPE_TA_ATTRIBUTES 0

static isc_result_t
fromtext_ta(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_ta);

	return (generic_fromtext_ds(CALL_FROMTEXT));
}

static isc_result_t
totext_ta(ARGS_TOTEXT) {
	REQUIRE(rdata->type == dns_rdatatype_ta);

	return (generic_totext_ds(CALL_TOTEXT));
}

static isc_result_t
fromwire_ta(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_ta);

	return (generic_fromwire_ds(CALL_FROMWIRE));
}

static isc_result_t
towire_ta(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_ta);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_ta(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_ta);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_ta(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_ta);

	return (generic_fromstruct_ds(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_ta(ARGS_TOSTRUCT) {
	dns_rdata_ds_t *ds = target;

	REQUIRE(rdata->type == dns_rdatatype_ta);
	REQUIRE(ds != NULL);

	/*
	 * Checked by generic_tostruct_ds().
	 */
	ds->common.rdclass = rdata->rdclass;
	ds->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ds->common, link);

	return (generic_tostruct_ds(CALL_TOSTRUCT));
}

static void
freestruct_ta(ARGS_FREESTRUCT) {
	dns_rdata_ta_t *ds = source;

	REQUIRE(ds != NULL);
	REQUIRE(ds->common.rdtype == dns_rdatatype_ta);

	if (ds->mctx == NULL) {
		return;
	}

	if (ds->digest != NULL) {
		isc_mem_free(ds->mctx, ds->digest);
	}
	ds->mctx = NULL;
}

static isc_result_t
additionaldata_ta(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_ta);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_ta(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_ta);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_ta(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_ta);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_ta(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_ta);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_ta(ARGS_COMPARE) {
	return (compare_ta(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_TA_32768_C */
