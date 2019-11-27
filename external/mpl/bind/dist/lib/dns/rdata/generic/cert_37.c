/*	$NetBSD: cert_37.c,v 1.5 2019/11/27 05:48:42 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/* RFC2538 */

#ifndef RDATA_GENERIC_CERT_37_C
#define RDATA_GENERIC_CERT_37_C

#define RRTYPE_CERT_ATTRIBUTES (0)

static inline isc_result_t
fromtext_cert(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_secalg_t secalg;
	dns_cert_t cert;

	REQUIRE(type == dns_rdatatype_cert);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/*
	 * Cert type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_cert_fromtext(&cert, &token.value.as_textregion));
	RETERR(uint16_tobuffer(cert, target));

	/*
	 * Key tag.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_secalg_fromtext(&secalg, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &secalg, 1));

	return (isc_base64_tobuffer(lexer, target, -2));
}

static inline isc_result_t
totext_cert(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->type == dns_rdatatype_cert);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Type.
	 */
	n = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	RETERR(dns_cert_totext((dns_cert_t)n, target));
	RETERR(str_totext(" ", target));

	/*
	 * Key tag.
	 */
	n = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	snprintf(buf, sizeof(buf), "%u ", n);
	RETERR(str_totext(buf, target));

	/*
	 * Algorithm.
	 */
	RETERR(dns_secalg_totext(sr.base[0], target));
	isc_region_consume(&sr, 1);

	/*
	 * Cert.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));
	if (tctx->width == 0)   /* No splitting */
		RETERR(isc_base64_totext(&sr, 60, "", target));
	else
		RETERR(isc_base64_totext(&sr, tctx->width - 2,
					 tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_cert(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == dns_rdatatype_cert);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 5)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_cert(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_cert);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_cert(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_cert);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_cert(ARGS_FROMSTRUCT) {
	dns_rdata_cert_t *cert = source;

	REQUIRE(type == dns_rdatatype_cert);
	REQUIRE(cert != NULL);
	REQUIRE(cert->common.rdtype == type);
	REQUIRE(cert->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(cert->type, target));
	RETERR(uint16_tobuffer(cert->key_tag, target));
	RETERR(uint8_tobuffer(cert->algorithm, target));

	return (mem_tobuffer(target, cert->certificate, cert->length));
}

static inline isc_result_t
tostruct_cert(ARGS_TOSTRUCT) {
	dns_rdata_cert_t *cert = target;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_cert);
	REQUIRE(cert != NULL);
	REQUIRE(rdata->length != 0);

	cert->common.rdclass = rdata->rdclass;
	cert->common.rdtype = rdata->type;
	ISC_LINK_INIT(&cert->common, link);

	dns_rdata_toregion(rdata, &region);

	cert->type = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	cert->key_tag = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	cert->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	cert->length = region.length;

	cert->certificate = mem_maybedup(mctx, region.base, region.length);
	if (cert->certificate == NULL)
		return (ISC_R_NOMEMORY);

	cert->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_cert(ARGS_FREESTRUCT) {
	dns_rdata_cert_t *cert = source;

	REQUIRE(cert != NULL);
	REQUIRE(cert->common.rdtype == dns_rdatatype_cert);

	if (cert->mctx == NULL)
		return;

	if (cert->certificate != NULL)
		isc_mem_free(cert->mctx, cert->certificate);
	cert->mctx = NULL;
}

static inline isc_result_t
additionaldata_cert(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_cert);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_cert(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_cert);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline bool
checkowner_cert(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_cert);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_cert(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_cert);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}


static inline int
casecompare_cert(ARGS_COMPARE) {
	return (compare_cert(rdata1, rdata2));
}
#endif	/* RDATA_GENERIC_CERT_37_C */
