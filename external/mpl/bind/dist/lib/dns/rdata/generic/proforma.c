/*	$NetBSD: proforma.c,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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


#ifndef RDATA_GENERIC_#_#_C
#define RDATA_GENERIC_#_#_C

#define RRTYPE_#_ATTRIBUTES (0)

static inline isc_result_t
fromtext_#(ARGS_FROMTEXT) {
	isc_token_t token;

	REQUIRE(type == dns_rdatatype_proforma.c#);
	REQUIRE(rdclass == #);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
totext_#(ARGS_TOTEXT) {

	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);
	REQUIRE(rdata->length != 0);	/* XXX */

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
fromwire_#(ARGS_FROMWIRE) {

	REQUIRE(type == dns_rdatatype_proforma.c#);
	REQUIRE(rdclass == #);

	/* NONE or GLOBAL14 */
	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
towire_#(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);
	REQUIRE(rdata->length != 0);	/* XXX */

	/* NONE or GLOBAL14 */
	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline int
compare_#(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == dns_rdatatype_proforma.crdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata1->rdclass == #);
	REQUIRE(rdata1->length != 0);	/* XXX */
	REQUIRE(rdata2->length != 0);	/* XXX */

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_#(ARGS_FROMSTRUCT) {
	dns_rdata_#_t *# = source;

	REQUIRE(type == dns_rdatatype_proforma.c#);
	REQUIRE(rdclass == #);
	REQUIRE(source != NULL);
	REQUIRE(#->common.rdtype == dns_rdatatype_proforma.ctype);
	REQUIRE(#->common.rdclass == rdclass);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
tostruct_#(ARGS_TOSTRUCT) {

	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);
	REQUIRE(rdata->length != 0);	/* XXX */

	return (ISC_R_NOTIMPLEMENTED);
}

static inline void
freestruct_#(ARGS_FREESTRUCT) {
	dns_rdata_#_t *# = source;

	REQUIRE(source != NULL);
	REQUIRE(#->common.rdtype == dns_rdatatype_proforma.c#);
	REQUIRE(#->common.rdclass == #);

}

static inline isc_result_t
additionaldata_#(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);

	(void)add;
	(void)arg;

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_#(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_#(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_proforma.c#);
	REQUIRE(rdclass == #);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_#(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata->rdclass == #);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_#(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == dns_rdatatype_proforma.crdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_proforma.c#);
	REQUIRE(rdata1->rdclass == #);
	REQUIRE(rdata1->length != 0);	/* XXX */
	REQUIRE(rdata2->length != 0);	/* XXX */

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

#endif	/* RDATA_GENERIC_#_#_C */
