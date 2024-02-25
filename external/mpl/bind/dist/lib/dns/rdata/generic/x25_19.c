/*	$NetBSD: x25_19.c,v 1.8.2.1 2024/02/25 15:47:06 martin Exp $	*/

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

/* RFC1183 */

#ifndef RDATA_GENERIC_X25_19_C
#define RDATA_GENERIC_X25_19_C

#define RRTYPE_X25_ATTRIBUTES (0)

static isc_result_t
fromtext_x25(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned int i;

	REQUIRE(type == dns_rdatatype_x25);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      false));
	if (token.value.as_textregion.length < 4) {
		RETTOK(DNS_R_SYNTAX);
	}
	for (i = 0; i < token.value.as_textregion.length; i++) {
		if (!isdigit((unsigned char)token.value.as_textregion.base[i]))
		{
			RETTOK(ISC_R_RANGE);
		}
	}
	RETTOK(txt_fromtext(&token.value.as_textregion, target));
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_x25(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_x25);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);
	return (txt_totext(&region, true, target));
}

static isc_result_t
fromwire_x25(ARGS_FROMWIRE) {
	isc_region_t sr;
	unsigned int i;

	REQUIRE(type == dns_rdatatype_x25);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 5 || sr.base[0] != (sr.length - 1)) {
		return (DNS_R_FORMERR);
	}
	for (i = 1; i < sr.length; i++) {
		if (sr.base[i] < 0x30 || sr.base[i] > 0x39) {
			return (DNS_R_FORMERR);
		}
	}
	return (txt_fromwire(source, target));
}

static isc_result_t
towire_x25(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == dns_rdatatype_x25);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_x25(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_x25);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_x25(ARGS_FROMSTRUCT) {
	dns_rdata_x25_t *x25 = source;
	uint8_t i;

	REQUIRE(type == dns_rdatatype_x25);
	REQUIRE(x25 != NULL);
	REQUIRE(x25->common.rdtype == type);
	REQUIRE(x25->common.rdclass == rdclass);
	REQUIRE(x25->x25 != NULL && x25->x25_len != 0);

	UNUSED(type);
	UNUSED(rdclass);

	if (x25->x25_len < 4) {
		return (ISC_R_RANGE);
	}

	for (i = 0; i < x25->x25_len; i++) {
		if (!isdigit((unsigned char)x25->x25[i])) {
			return (ISC_R_RANGE);
		}
	}

	RETERR(uint8_tobuffer(x25->x25_len, target));
	return (mem_tobuffer(target, x25->x25, x25->x25_len));
}

static isc_result_t
tostruct_x25(ARGS_TOSTRUCT) {
	dns_rdata_x25_t *x25 = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_x25);
	REQUIRE(x25 != NULL);
	REQUIRE(rdata->length != 0);

	x25->common.rdclass = rdata->rdclass;
	x25->common.rdtype = rdata->type;
	ISC_LINK_INIT(&x25->common, link);

	dns_rdata_toregion(rdata, &r);
	x25->x25_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	x25->x25 = mem_maybedup(mctx, r.base, x25->x25_len);
	if (x25->x25 == NULL) {
		return (ISC_R_NOMEMORY);
	}

	x25->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_x25(ARGS_FREESTRUCT) {
	dns_rdata_x25_t *x25 = source;

	REQUIRE(x25 != NULL);
	REQUIRE(x25->common.rdtype == dns_rdatatype_x25);

	if (x25->mctx == NULL) {
		return;
	}

	if (x25->x25 != NULL) {
		isc_mem_free(x25->mctx, x25->x25);
	}
	x25->mctx = NULL;
}

static isc_result_t
additionaldata_x25(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_x25);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_x25(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_x25);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_x25(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_x25);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_x25(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_x25);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_x25(ARGS_COMPARE) {
	return (compare_x25(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_X25_19_C */
