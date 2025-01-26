/*	$NetBSD: wallet_262.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

#ifndef RDATA_GENERIC_WALLET_262_C
#define RDATA_GENERIC_WALLET_262_C

#define RRTYPE_WALLET_ATTRIBUTES (0)

static isc_result_t
fromtext_wallet(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_wallet);

	return generic_fromtext_txt(CALL_FROMTEXT);
}

static isc_result_t
totext_wallet(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_wallet);

	return generic_totext_txt(CALL_TOTEXT);
}

static isc_result_t
fromwire_wallet(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_wallet);

	return generic_fromwire_txt(CALL_FROMWIRE);
}

static isc_result_t
towire_wallet(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_wallet);

	UNUSED(cctx);

	return mem_tobuffer(target, rdata->data, rdata->length);
}

static int
compare_wallet(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_wallet);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return isc_region_compare(&r1, &r2);
}

static isc_result_t
fromstruct_wallet(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_wallet);

	return generic_fromstruct_txt(CALL_FROMSTRUCT);
}

static isc_result_t
tostruct_wallet(ARGS_TOSTRUCT) {
	dns_rdata_wallet_t *wallet = target;

	REQUIRE(rdata->type == dns_rdatatype_wallet);
	REQUIRE(wallet != NULL);

	wallet->common.rdclass = rdata->rdclass;
	wallet->common.rdtype = rdata->type;
	ISC_LINK_INIT(&wallet->common, link);

	return generic_tostruct_txt(CALL_TOSTRUCT);
}

static void
freestruct_wallet(ARGS_FREESTRUCT) {
	dns_rdata_wallet_t *wallet = source;

	REQUIRE(wallet != NULL);
	REQUIRE(wallet->common.rdtype == dns_rdatatype_wallet);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_wallet(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_wallet);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return ISC_R_SUCCESS;
}

static isc_result_t
digest_wallet(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_wallet);

	dns_rdata_toregion(rdata, &r);

	return (digest)(arg, &r);
}

static bool
checkowner_wallet(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_wallet);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return true;
}

static bool
checknames_wallet(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_wallet);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return true;
}

static int
casecompare_wallet(ARGS_COMPARE) {
	return compare_wallet(rdata1, rdata2);
}

isc_result_t
dns_rdata_wallet_first(dns_rdata_wallet_t *wallet) {
	REQUIRE(wallet != NULL);
	REQUIRE(wallet->common.rdtype == dns_rdatatype_wallet);

	return generic_txt_first(wallet);
}

isc_result_t
dns_rdata_wallet_next(dns_rdata_wallet_t *wallet) {
	REQUIRE(wallet != NULL);
	REQUIRE(wallet->common.rdtype == dns_rdatatype_wallet);

	return generic_txt_next(wallet);
}

isc_result_t
dns_rdata_wallet_current(dns_rdata_wallet_t *wallet,
			 dns_rdata_wallet_string_t *string) {
	REQUIRE(wallet != NULL);
	REQUIRE(wallet->common.rdtype == dns_rdatatype_wallet);

	return generic_txt_current(wallet, string);
}
#endif /* RDATA_GENERIC_WALLET_262_C */
