/*	$NetBSD: zonemd_63.c,v 1.5.2.1 2024/02/25 15:47:06 martin Exp $	*/

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

/* RFC 8976 */

#ifndef RDATA_GENERIC_ZONEMD_63_C
#define RDATA_GENERIC_ZONEMD_63_C

#define RRTYPE_ZONEMD_ATTRIBUTES 0

static isc_result_t
fromtext_zonemd(ARGS_FROMTEXT) {
	isc_token_t token;
	int digest_type, length;
	isc_buffer_t save;
	isc_result_t result;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/*
	 * Zone Serial.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	RETERR(uint32_tobuffer(token.value.as_ulong, target));

	/*
	 * Digest Scheme.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Digest Type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	digest_type = token.value.as_ulong;
	RETERR(uint8_tobuffer(digest_type, target));

	/*
	 * Digest.
	 */
	switch (digest_type) {
	case DNS_ZONEMD_DIGEST_SHA384:
		length = ISC_SHA384_DIGESTLENGTH;
		break;
	case DNS_ZONEMD_DIGEST_SHA512:
		length = ISC_SHA512_DIGESTLENGTH;
		break;
	default:
		length = -2;
		break;
	}

	save = *target;
	result = isc_hex_tobuffer(lexer, target, length);
	/* Minimum length of digest is 12 octets. */
	if (isc_buffer_usedlength(target) - isc_buffer_usedlength(&save) < 12) {
		return (ISC_R_UNEXPECTEDEND);
	}
	return (result);
}

static isc_result_t
totext_zonemd(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("0123456789")];
	unsigned long num;

	REQUIRE(rdata->length > 6);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Zone Serial.
	 */
	num = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	snprintf(buf, sizeof(buf), "%lu", num);
	RETERR(str_totext(buf, target));

	RETERR(str_totext(" ", target));

	/*
	 * Digest scheme.
	 */
	num = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%lu", num);
	RETERR(str_totext(buf, target));

	RETERR(str_totext(" ", target));

	/*
	 * Digest type.
	 */
	num = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%lu", num);
	RETERR(str_totext(buf, target));

	/*
	 * Digest.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" (", target));
	}
	RETERR(str_totext(tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_NOCRYPTO) == 0) {
		if (tctx->width == 0) { /* No splitting */
			RETERR(isc_hex_totext(&sr, 0, "", target));
		} else {
			RETERR(isc_hex_totext(&sr, tctx->width - 2,
					      tctx->linebreak, target));
		}
	} else {
		RETERR(str_totext("[omitted]", target));
	}
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" )", target));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
fromwire_zonemd(ARGS_FROMWIRE) {
	isc_region_t sr;
	size_t digestlen = 0;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);

	/*
	 * If we do not recognize the digest type, ensure that the digest
	 * meets minimum length (12).
	 *
	 * If we do recognize the digest type, ensure that the digest is of the
	 * correct length.
	 */
	if (sr.length < 18) {
		return (ISC_R_UNEXPECTEDEND);
	}

	switch (sr.base[5]) {
	case DNS_ZONEMD_DIGEST_SHA384:
		digestlen = ISC_SHA384_DIGESTLENGTH;
		break;
	case DNS_ZONEMD_DIGEST_SHA512:
		digestlen = ISC_SHA512_DIGESTLENGTH;
		break;
	default:
		break;
	}

	if (digestlen != 0 && sr.length < 6 + digestlen) {
		return (ISC_R_UNEXPECTEDEND);
	}

	/*
	 * Only specify the number of octets to consume if we recognize the
	 * digest type.
	 *
	 * If there is extra data, dns_rdata_fromwire() will detect that.
	 */
	if (digestlen != 0) {
		sr.length = 6 + digestlen;
	}

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static isc_result_t
towire_zonemd(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_zonemd);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_zonemd(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_zonemd);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_zonemd(ARGS_FROMSTRUCT) {
	dns_rdata_zonemd_t *zonemd = source;

	REQUIRE(zonemd != NULL);
	REQUIRE(zonemd->common.rdtype == type);
	REQUIRE(zonemd->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	switch (zonemd->digest_type) {
	case DNS_ZONEMD_DIGEST_SHA384:
		REQUIRE(zonemd->length == ISC_SHA384_DIGESTLENGTH);
		break;
	case DNS_ZONEMD_DIGEST_SHA512:
		REQUIRE(zonemd->length == ISC_SHA512_DIGESTLENGTH);
		break;
	}

	RETERR(uint32_tobuffer(zonemd->serial, target));
	RETERR(uint8_tobuffer(zonemd->scheme, target));
	RETERR(uint8_tobuffer(zonemd->digest_type, target));

	return (mem_tobuffer(target, zonemd->digest, zonemd->length));
}

static isc_result_t
tostruct_zonemd(ARGS_TOSTRUCT) {
	dns_rdata_zonemd_t *zonemd = target;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_zonemd);
	REQUIRE(zonemd != NULL);
	REQUIRE(rdata->length != 0);

	zonemd->common.rdclass = rdata->rdclass;
	zonemd->common.rdtype = rdata->type;
	ISC_LINK_INIT(&zonemd->common, link);

	dns_rdata_toregion(rdata, &region);

	zonemd->serial = uint32_fromregion(&region);
	isc_region_consume(&region, 4);
	zonemd->scheme = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	zonemd->digest_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	zonemd->length = region.length;

	zonemd->digest = mem_maybedup(mctx, region.base, region.length);
	if (zonemd->digest == NULL) {
		return (ISC_R_NOMEMORY);
	}

	zonemd->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_zonemd(ARGS_FREESTRUCT) {
	dns_rdata_zonemd_t *zonemd = source;

	REQUIRE(zonemd != NULL);
	REQUIRE(zonemd->common.rdtype == dns_rdatatype_zonemd);

	if (zonemd->mctx == NULL) {
		return;
	}

	if (zonemd->digest != NULL) {
		isc_mem_free(zonemd->mctx, zonemd->digest);
	}
	zonemd->mctx = NULL;
}

static isc_result_t
additionaldata_zonemd(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_zonemd);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_zonemd(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_zonemd);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_zonemd(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_zonemd);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_zonemd(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_zonemd);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_zonemd(ARGS_COMPARE) {
	return (compare_zonemd(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_ZONEMD_63_C */
