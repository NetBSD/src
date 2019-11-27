/*	$NetBSD: srv_33.c,v 1.4 2019/11/27 05:48:42 christos Exp $	*/

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

/* RFC2782 */

#ifndef RDATA_IN_1_SRV_33_C
#define RDATA_IN_1_SRV_33_C

#define RRTYPE_SRV_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_srv(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	bool ok;

	REQUIRE(type == dns_rdatatype_srv);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Priority.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Weight.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Port.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Target.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL)
		origin = dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	ok = true;
	if ((options & DNS_RDATA_CHECKNAMES) != 0)
		ok = dns_name_ishostname(&name, false);
	if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
		RETTOK(DNS_R_BADNAME);
	if (!ok && callbacks != NULL)
		warn_badname(&name, lexer, callbacks);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_srv(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;
	char buf[sizeof("64000")];
	unsigned short num;

	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	/*
	 * Priority.
	 */
	dns_rdata_toregion(rdata, &region);
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	snprintf(buf, sizeof(buf), "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Weight.
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	snprintf(buf, sizeof(buf), "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Port.
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	snprintf(buf, sizeof(buf), "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Target.
	 */
	dns_name_fromregion(&name, &region);
	sub = name_prefix(&name, tctx->origin, &prefix);
	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_in_srv(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t sr;

	REQUIRE(type == dns_rdatatype_srv);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);

	/*
	 * Priority, weight, port.
	 */
	isc_buffer_activeregion(source, &sr);
	if (sr.length < 6)
		return (ISC_R_UNEXPECTEDEND);
	RETERR(mem_tobuffer(target, sr.base, 6));
	isc_buffer_forward(source, 6);

	/*
	 * Target.
	 */
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_in_srv(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	/*
	 * Priority, weight, port.
	 */
	dns_rdata_toregion(rdata, &sr);
	RETERR(mem_tobuffer(target, sr.base, 6));
	isc_region_consume(&sr, 6);

	/*
	 * Target.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &sr);
	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_in_srv(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_srv);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	/*
	 * Priority, weight, port.
	 */
	order = memcmp(rdata1->data, rdata2->data, 6);
	if (order != 0)
		return (order < 0 ? -1 : 1);

	/*
	 * Target.
	 */
	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	isc_region_consume(&region1, 6);
	isc_region_consume(&region2, 6);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_in_srv(ARGS_FROMSTRUCT) {
	dns_rdata_in_srv_t *srv = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_srv);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(srv != NULL);
	REQUIRE(srv->common.rdtype == type);
	REQUIRE(srv->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(srv->priority, target));
	RETERR(uint16_tobuffer(srv->weight, target));
	RETERR(uint16_tobuffer(srv->port, target));
	dns_name_toregion(&srv->target, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_in_srv(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_in_srv_t *srv = target;
	dns_name_t name;

	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(srv != NULL);
	REQUIRE(rdata->length != 0);

	srv->common.rdclass = rdata->rdclass;
	srv->common.rdtype = rdata->type;
	ISC_LINK_INIT(&srv->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	srv->priority = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	srv->weight = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	srv->port = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	dns_name_fromregion(&name, &region);
	dns_name_init(&srv->target, NULL);
	RETERR(name_duporclone(&name, mctx, &srv->target));
	srv->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_srv(ARGS_FREESTRUCT) {
	dns_rdata_in_srv_t *srv = source;

	REQUIRE(srv != NULL);
	REQUIRE(srv->common.rdclass == dns_rdataclass_in);
	REQUIRE(srv->common.rdtype == dns_rdatatype_srv);

	if (srv->mctx == NULL)
		return;

	dns_name_free(&srv->target, srv->mctx);
	srv->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_srv(ARGS_ADDLDATA) {
	char buf[sizeof("_65000._tcp")];
	dns_fixedname_t fixed;
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;
	uint16_t port;
	isc_result_t result;

	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	isc_region_consume(&region, 4);
	port = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	dns_name_fromregion(&name, &region);

	if (dns_name_equal(&name, dns_rootname))
		return (ISC_R_SUCCESS);

	result = (add)(arg, &name, dns_rdatatype_a);
	if (result != ISC_R_SUCCESS)
		return (result);

	dns_fixedname_init(&fixed);
	snprintf(buf, sizeof(buf), "_%u._tcp", port);
	result = dns_name_fromstring2(dns_fixedname_name(&fixed), buf, NULL,
				      0, NULL);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	result = dns_name_concatenate(dns_fixedname_name(&fixed), &name,
				      dns_fixedname_name(&fixed), NULL);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	return ((add)(arg, dns_fixedname_name(&fixed), dns_rdatatype_tlsa));
}

static inline isc_result_t
digest_in_srv(ARGS_DIGEST) {
	isc_region_t r1, r2;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r1);
	r2 = r1;
	isc_region_consume(&r2, 6);
	r1.length = 6;
	RETERR((digest)(arg, &r1));
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);
	return (dns_name_digest(&name, digest, arg));
}

static inline bool
checkowner_in_srv(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_srv);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_in_srv(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_srv);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	isc_region_consume(&region, 6);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ishostname(&name, false)) {
		if (bad != NULL)
			dns_name_clone(&name, bad);
		return (false);
	}
	return (true);
}

static inline int
casecompare_in_srv(ARGS_COMPARE) {
	return (compare_in_srv(rdata1, rdata2));
}

#endif	/* RDATA_IN_1_SRV_33_C */
