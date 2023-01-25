/*	$NetBSD: txt_16.c,v 1.9 2023/01/25 21:43:30 christos Exp $	*/

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

#ifndef RDATA_GENERIC_TXT_16_C
#define RDATA_GENERIC_TXT_16_C

#define RRTYPE_TXT_ATTRIBUTES (0)

static isc_result_t
generic_fromtext_txt(ARGS_FROMTEXT) {
	isc_token_t token;
	int strings;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	strings = 0;
	if ((options & DNS_RDATA_UNKNOWNESCAPE) != 0) {
		isc_textregion_t r;
		DE_CONST("#", r.base);
		r.length = 1;
		RETERR(txt_fromtext(&r, target));
		strings++;
	}
	for (;;) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_qstring, true));
		if (token.type != isc_tokentype_qstring &&
		    token.type != isc_tokentype_string)
		{
			break;
		}
		RETTOK(txt_fromtext(&token.value.as_textregion, target));
		strings++;
	}
	/* Let upper layer handle eol/eof. */
	isc_lex_ungettoken(lexer, &token);
	return (strings == 0 ? ISC_R_UNEXPECTEDEND : ISC_R_SUCCESS);
}

static isc_result_t
generic_totext_txt(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);

	while (region.length > 0) {
		RETERR(txt_totext(&region, true, target));
		if (region.length > 0) {
			RETERR(str_totext(" ", target));
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
generic_fromwire_txt(ARGS_FROMWIRE) {
	isc_result_t result;

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	do {
		result = txt_fromwire(source, target);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	} while (!buffer_empty(source));
	return (ISC_R_SUCCESS);
}

static isc_result_t
fromtext_txt(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_txt);

	return (generic_fromtext_txt(CALL_FROMTEXT));
}

static isc_result_t
totext_txt(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_txt);

	return (generic_totext_txt(CALL_TOTEXT));
}

static isc_result_t
fromwire_txt(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_txt);

	return (generic_fromwire_txt(CALL_FROMWIRE));
}

static isc_result_t
towire_txt(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_txt);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_txt(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_txt);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
generic_fromstruct_txt(ARGS_FROMSTRUCT) {
	dns_rdata_txt_t *txt = source;
	isc_region_t region;
	uint8_t length;

	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdtype == type);
	REQUIRE(txt->common.rdclass == rdclass);
	REQUIRE(txt->txt != NULL && txt->txt_len != 0);

	UNUSED(type);
	UNUSED(rdclass);

	region.base = txt->txt;
	region.length = txt->txt_len;
	while (region.length > 0) {
		length = uint8_fromregion(&region);
		isc_region_consume(&region, 1);
		if (region.length < length) {
			return (ISC_R_UNEXPECTEDEND);
		}
		isc_region_consume(&region, length);
	}

	return (mem_tobuffer(target, txt->txt, txt->txt_len));
}

static isc_result_t
generic_tostruct_txt(ARGS_TOSTRUCT) {
	dns_rdata_txt_t *txt = target;
	isc_region_t r;

	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdclass == rdata->rdclass);
	REQUIRE(txt->common.rdtype == rdata->type);
	REQUIRE(!ISC_LINK_LINKED(&txt->common, link));

	dns_rdata_toregion(rdata, &r);
	txt->txt_len = r.length;
	txt->txt = mem_maybedup(mctx, r.base, r.length);
	if (txt->txt == NULL) {
		return (ISC_R_NOMEMORY);
	}

	txt->offset = 0;
	txt->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
generic_freestruct_txt(ARGS_FREESTRUCT) {
	dns_rdata_txt_t *txt = source;

	REQUIRE(txt != NULL);

	if (txt->mctx == NULL) {
		return;
	}

	if (txt->txt != NULL) {
		isc_mem_free(txt->mctx, txt->txt);
	}
	txt->mctx = NULL;
}

static isc_result_t
fromstruct_txt(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_txt);

	return (generic_fromstruct_txt(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_txt(ARGS_TOSTRUCT) {
	dns_rdata_txt_t *txt = target;

	REQUIRE(rdata->type == dns_rdatatype_txt);
	REQUIRE(txt != NULL);

	txt->common.rdclass = rdata->rdclass;
	txt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&txt->common, link);

	return (generic_tostruct_txt(CALL_TOSTRUCT));
}

static void
freestruct_txt(ARGS_FREESTRUCT) {
	dns_rdata_txt_t *txt = source;

	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_txt);

	generic_freestruct_txt(source);
}

static isc_result_t
additionaldata_txt(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_txt);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_txt(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_txt);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_txt(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_txt);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_txt(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_txt);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_txt(ARGS_COMPARE) {
	return (compare_txt(rdata1, rdata2));
}

static isc_result_t
generic_txt_first(dns_rdata_txt_t *txt) {
	REQUIRE(txt != NULL);
	REQUIRE(txt->txt != NULL || txt->txt_len == 0);

	if (txt->txt_len == 0) {
		return (ISC_R_NOMORE);
	}

	txt->offset = 0;
	return (ISC_R_SUCCESS);
}

static isc_result_t
generic_txt_next(dns_rdata_txt_t *txt) {
	isc_region_t r;
	uint8_t length;

	REQUIRE(txt != NULL);
	REQUIRE(txt->txt != NULL && txt->txt_len != 0);

	INSIST(txt->offset + 1 <= txt->txt_len);
	r.base = txt->txt + txt->offset;
	r.length = txt->txt_len - txt->offset;
	length = uint8_fromregion(&r);
	INSIST(txt->offset + 1 + length <= txt->txt_len);
	txt->offset = txt->offset + 1 + length;
	if (txt->offset == txt->txt_len) {
		return (ISC_R_NOMORE);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
generic_txt_current(dns_rdata_txt_t *txt, dns_rdata_txt_string_t *string) {
	isc_region_t r;

	REQUIRE(txt != NULL);
	REQUIRE(string != NULL);
	REQUIRE(txt->txt != NULL);
	REQUIRE(txt->offset < txt->txt_len);

	INSIST(txt->offset + 1 <= txt->txt_len);
	r.base = txt->txt + txt->offset;
	r.length = txt->txt_len - txt->offset;

	string->length = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	string->data = r.base;
	INSIST(txt->offset + 1 + string->length <= txt->txt_len);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdata_txt_first(dns_rdata_txt_t *txt) {
	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_txt);

	return (generic_txt_first(txt));
}

isc_result_t
dns_rdata_txt_next(dns_rdata_txt_t *txt) {
	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_txt);

	return (generic_txt_next(txt));
}

isc_result_t
dns_rdata_txt_current(dns_rdata_txt_t *txt, dns_rdata_txt_string_t *string) {
	REQUIRE(txt != NULL);
	REQUIRE(txt->common.rdtype == dns_rdatatype_txt);

	return (generic_txt_current(txt, string));
}
#endif /* RDATA_GENERIC_TXT_16_C */
