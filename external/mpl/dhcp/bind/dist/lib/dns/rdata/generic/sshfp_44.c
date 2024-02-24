/*	$NetBSD: sshfp_44.c,v 1.1.2.2 2024/02/24 13:07:14 martin Exp $	*/

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

/* RFC 4255 */

#ifndef RDATA_GENERIC_SSHFP_44_C
#define RDATA_GENERIC_SSHFP_44_C

#define RRTYPE_SSHFP_ATTRIBUTES (0)

static isc_result_t
fromtext_sshfp(ARGS_FROMTEXT) {
	isc_token_t token;
	int len = -1;

	REQUIRE(type == dns_rdatatype_sshfp);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffU) {
		RETTOK(ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Digest type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffU) {
		RETTOK(ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Enforce known digest lengths.
	 */
	switch (token.value.as_ulong) {
	case 1:
		len = ISC_SHA1_DIGESTLENGTH;
		break;
	case 2:
		len = ISC_SHA256_DIGESTLENGTH;
		break;
	default:
		break;
	}

	/*
	 * Digest.
	 */
	return (isc_hex_tobuffer(lexer, target, len));
}

static isc_result_t
totext_sshfp(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->type == dns_rdatatype_sshfp);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Algorithm.
	 */
	n = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%u ", n);
	RETERR(str_totext(buf, target));

	/*
	 * Digest type.
	 */
	n = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%u", n);
	RETERR(str_totext(buf, target));

	if (sr.length == 0U) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * Digest.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" (", target));
	}
	RETERR(str_totext(tctx->linebreak, target));
	if (tctx->width == 0) { /* No splitting */
		RETERR(isc_hex_totext(&sr, 0, "", target));
	} else {
		RETERR(isc_hex_totext(&sr, tctx->width - 2, tctx->linebreak,
				      target));
	}
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" )", target));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
fromwire_sshfp(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == dns_rdatatype_sshfp);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 2) {
		return (ISC_R_UNEXPECTEDEND);
	}

	if ((sr.base[1] == 1 && sr.length != ISC_SHA1_DIGESTLENGTH + 2) ||
	    (sr.base[1] == 2 && sr.length != ISC_SHA256_DIGESTLENGTH + 2))
	{
		return (DNS_R_FORMERR);
	}

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static isc_result_t
towire_sshfp(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_sshfp);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_sshfp(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_sshfp);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_sshfp(ARGS_FROMSTRUCT) {
	dns_rdata_sshfp_t *sshfp = source;

	REQUIRE(type == dns_rdatatype_sshfp);
	REQUIRE(sshfp != NULL);
	REQUIRE(sshfp->common.rdtype == type);
	REQUIRE(sshfp->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(sshfp->algorithm, target));
	RETERR(uint8_tobuffer(sshfp->digest_type, target));

	return (mem_tobuffer(target, sshfp->digest, sshfp->length));
}

static isc_result_t
tostruct_sshfp(ARGS_TOSTRUCT) {
	dns_rdata_sshfp_t *sshfp = target;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_sshfp);
	REQUIRE(sshfp != NULL);
	REQUIRE(rdata->length != 0);

	sshfp->common.rdclass = rdata->rdclass;
	sshfp->common.rdtype = rdata->type;
	ISC_LINK_INIT(&sshfp->common, link);

	dns_rdata_toregion(rdata, &region);

	sshfp->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sshfp->digest_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sshfp->length = region.length;

	sshfp->digest = mem_maybedup(mctx, region.base, region.length);
	if (sshfp->digest == NULL) {
		return (ISC_R_NOMEMORY);
	}

	sshfp->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_sshfp(ARGS_FREESTRUCT) {
	dns_rdata_sshfp_t *sshfp = source;

	REQUIRE(sshfp != NULL);
	REQUIRE(sshfp->common.rdtype == dns_rdatatype_sshfp);

	if (sshfp->mctx == NULL) {
		return;
	}

	if (sshfp->digest != NULL) {
		isc_mem_free(sshfp->mctx, sshfp->digest);
	}
	sshfp->mctx = NULL;
}

static isc_result_t
additionaldata_sshfp(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_sshfp);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_sshfp(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_sshfp);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_sshfp(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_sshfp);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_sshfp(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_sshfp);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_sshfp(ARGS_COMPARE) {
	return (compare_sshfp(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_SSHFP_44_C */
