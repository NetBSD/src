/*	$NetBSD: ds_43.c,v 1.4.4.1 2019/09/12 19:18:15 martin Exp $	*/

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


/* RFC3658 */

#ifndef RDATA_GENERIC_DS_43_C
#define RDATA_GENERIC_DS_43_C

#define RRTYPE_DS_ATTRIBUTES \
	( DNS_RDATATYPEATTR_DNSSEC | DNS_RDATATYPEATTR_ZONECUTAUTH | \
	  DNS_RDATATYPEATTR_ATPARENT )

#include <isc/md.h>

#include <dns/ds.h>

static inline isc_result_t
generic_fromtext_ds(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char c;
	int length;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

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
	RETTOK(dns_secalg_fromtext(&c, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &c, 1));

	/*
	 * Digest type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_dsdigest_fromtext(&c, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &c, 1));

	/*
	 * Digest.
	 */
	switch (c) {
	case DNS_DSDIGEST_SHA1:
		length = ISC_SHA1_DIGESTLENGTH;
		break;
	case DNS_DSDIGEST_SHA256:
		length = ISC_SHA256_DIGESTLENGTH;
		break;
	case DNS_DSDIGEST_SHA384:
		length = ISC_SHA384_DIGESTLENGTH;
		break;
	default:
		length = -2;
		break;
	}
	return (isc_hex_tobuffer(lexer, target, length));
}

static inline isc_result_t
fromtext_ds(ARGS_FROMTEXT) {

	REQUIRE(type == dns_rdatatype_ds);

	return (generic_fromtext_ds(rdclass, type, lexer, origin, options,
				    target, callbacks));
}

static inline isc_result_t
generic_totext_ds(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

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

	/*
	 * Digest.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_NOCRYPTO) == 0) {
		if (tctx->width == 0) /* No splitting */
			RETERR(isc_hex_totext(&sr, 0, "", target));
		else
			RETERR(isc_hex_totext(&sr, tctx->width - 2,
					      tctx->linebreak, target));
	} else
		RETERR(str_totext("[omitted]", target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_ds(ARGS_TOTEXT) {

	REQUIRE(rdata->type == dns_rdatatype_ds);

	return (generic_totext_ds(rdata, tctx, target));
}

static inline isc_result_t
generic_fromwire_ds(ARGS_FROMWIRE) {
	isc_region_t sr;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);

	/*
	 * Check digest lengths if we know them.
	 */
	if (sr.length < 5 ||
	    (sr.base[3] == DNS_DSDIGEST_SHA1 &&
	     sr.length < 4 + ISC_SHA1_DIGESTLENGTH) ||
	    (sr.base[3] == DNS_DSDIGEST_SHA256 &&
	     sr.length < 4 + ISC_SHA256_DIGESTLENGTH) ||
	    (sr.base[3] == DNS_DSDIGEST_SHA384 &&
	     sr.length < 4 + ISC_SHA384_DIGESTLENGTH))
		return (ISC_R_UNEXPECTEDEND);

	/*
	 * Only copy digest lengths if we know them.
	 * If there is extra data dns_rdata_fromwire() will
	 * detect that.
	 */
	if (sr.base[3] == DNS_DSDIGEST_SHA1)
		sr.length = 4 + ISC_SHA1_DIGESTLENGTH;
	else if (sr.base[3] == DNS_DSDIGEST_SHA256)
		sr.length = 4 + ISC_SHA256_DIGESTLENGTH;
	else if (sr.base[3] == DNS_DSDIGEST_SHA384)
		sr.length = 4 + ISC_SHA384_DIGESTLENGTH;

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
fromwire_ds(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_ds);

	return (generic_fromwire_ds(rdclass, type, source, dctx, options,
				    target));
}

static inline isc_result_t
towire_ds(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_ds);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_ds(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_ds);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
generic_fromstruct_ds(ARGS_FROMSTRUCT) {
	dns_rdata_ds_t *ds = source;

	REQUIRE(source != NULL);
	REQUIRE(ds->common.rdtype == type);
	REQUIRE(ds->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	switch (ds->digest_type) {
	case DNS_DSDIGEST_SHA1:
		REQUIRE(ds->length == ISC_SHA1_DIGESTLENGTH);
		break;
	case DNS_DSDIGEST_SHA256:
		REQUIRE(ds->length == ISC_SHA256_DIGESTLENGTH);
		break;
	case DNS_DSDIGEST_SHA384:
		REQUIRE(ds->length == ISC_SHA384_DIGESTLENGTH);
		break;
	}

	RETERR(uint16_tobuffer(ds->key_tag, target));
	RETERR(uint8_tobuffer(ds->algorithm, target));
	RETERR(uint8_tobuffer(ds->digest_type, target));

	return (mem_tobuffer(target, ds->digest, ds->length));
}

static inline isc_result_t
fromstruct_ds(ARGS_FROMSTRUCT) {

	REQUIRE(type == dns_rdatatype_ds);

	return (generic_fromstruct_ds(rdclass, type, source, target));
}

static inline isc_result_t
generic_tostruct_ds(ARGS_TOSTRUCT) {
	dns_rdata_ds_t *ds = target;
	isc_region_t region;

	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);
	REQUIRE(ds->common.rdtype == rdata->type);
	REQUIRE(ds->common.rdclass == rdata->rdclass);
	REQUIRE(!ISC_LINK_LINKED(&ds->common, link));

	dns_rdata_toregion(rdata, &region);

	ds->key_tag = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	ds->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	ds->digest_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	ds->length = region.length;

	ds->digest = mem_maybedup(mctx, region.base, region.length);
	if (ds->digest == NULL)
		return (ISC_R_NOMEMORY);

	ds->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
tostruct_ds(ARGS_TOSTRUCT) {
	dns_rdata_ds_t *ds = target;

	REQUIRE(rdata->type == dns_rdatatype_ds);
	REQUIRE(target != NULL);

	ds->common.rdclass = rdata->rdclass;
	ds->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ds->common, link);

	return (generic_tostruct_ds(rdata, target, mctx));
}

static inline void
freestruct_ds(ARGS_FREESTRUCT) {
	dns_rdata_ds_t *ds = source;

	REQUIRE(ds != NULL);
	REQUIRE(ds->common.rdtype == dns_rdatatype_ds);

	if (ds->mctx == NULL)
		return;

	if (ds->digest != NULL)
		isc_mem_free(ds->mctx, ds->digest);
	ds->mctx = NULL;
}

static inline isc_result_t
additionaldata_ds(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_ds);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_ds(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_ds);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline bool
checkowner_ds(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_ds);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_ds(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_ds);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_ds(ARGS_COMPARE) {
	return (compare_ds(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_DS_43_C */
