/*	$NetBSD: dlv_32769.c,v 1.1.2.2 2024/02/24 13:07:10 martin Exp $	*/

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

/* RFC3658 */

#ifndef RDATA_GENERIC_DLV_32769_C
#define RDATA_GENERIC_DLV_32769_C

#define RRTYPE_DLV_ATTRIBUTES 0

#include <dns/ds.h>

static isc_result_t
fromtext_dlv(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_dlv);

	return (generic_fromtext_ds(CALL_FROMTEXT));
}

static isc_result_t
totext_dlv(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_dlv);

	return (generic_totext_ds(CALL_TOTEXT));
}

static isc_result_t
fromwire_dlv(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_dlv);

	return (generic_fromwire_ds(CALL_FROMWIRE));
}

static isc_result_t
towire_dlv(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_dlv);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_dlv(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_dlv);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_dlv(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_dlv);

	return (generic_fromstruct_ds(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_dlv(ARGS_TOSTRUCT) {
	dns_rdata_dlv_t *dlv = target;

	REQUIRE(rdata->type == dns_rdatatype_dlv);
	REQUIRE(dlv != NULL);

	dlv->common.rdclass = rdata->rdclass;
	dlv->common.rdtype = rdata->type;
	ISC_LINK_INIT(&dlv->common, link);

	return (generic_tostruct_ds(CALL_TOSTRUCT));
}

static void
freestruct_dlv(ARGS_FREESTRUCT) {
	dns_rdata_dlv_t *dlv = source;

	REQUIRE(dlv != NULL);
	REQUIRE(dlv->common.rdtype == dns_rdatatype_dlv);

	if (dlv->mctx == NULL) {
		return;
	}

	if (dlv->digest != NULL) {
		isc_mem_free(dlv->mctx, dlv->digest);
	}
	dlv->mctx = NULL;
}

static isc_result_t
additionaldata_dlv(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_dlv);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_dlv(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_dlv);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_dlv(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_dlv);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_dlv(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_dlv);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_dlv(ARGS_COMPARE) {
	return (compare_dlv(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_DLV_32769_C */
