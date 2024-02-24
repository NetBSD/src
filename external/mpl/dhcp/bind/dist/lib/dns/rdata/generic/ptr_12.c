/*	$NetBSD: ptr_12.c,v 1.1.2.2 2024/02/24 13:07:13 martin Exp $	*/

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

#ifndef RDATA_GENERIC_PTR_12_C
#define RDATA_GENERIC_PTR_12_C

#define RRTYPE_PTR_ATTRIBUTES (0)

static isc_result_t
fromtext_ptr(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == dns_rdatatype_ptr);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL) {
		origin = dns_rootname;
	}
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	if (rdclass == dns_rdataclass_in &&
	    (options & DNS_RDATA_CHECKNAMES) != 0 &&
	    (options & DNS_RDATA_CHECKREVERSE) != 0)
	{
		bool ok;
		ok = dns_name_ishostname(&name, false);
		if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0) {
			RETTOK(DNS_R_BADNAME);
		}
		if (!ok && callbacks != NULL) {
			warn_badname(&name, lexer, callbacks);
		}
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_ptr(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;

	REQUIRE(rdata->type == dns_rdatatype_ptr);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static isc_result_t
fromwire_ptr(ARGS_FROMWIRE) {
	dns_name_t name;

	REQUIRE(type == dns_rdatatype_ptr);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, NULL);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static isc_result_t
towire_ptr(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_ptr);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static int
compare_ptr(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_ptr);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static isc_result_t
fromstruct_ptr(ARGS_FROMSTRUCT) {
	dns_rdata_ptr_t *ptr = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_ptr);
	REQUIRE(ptr != NULL);
	REQUIRE(ptr->common.rdtype == type);
	REQUIRE(ptr->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&ptr->ptr, &region);
	return (isc_buffer_copyregion(target, &region));
}

static isc_result_t
tostruct_ptr(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_ptr_t *ptr = target;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_ptr);
	REQUIRE(ptr != NULL);
	REQUIRE(rdata->length != 0);

	ptr->common.rdclass = rdata->rdclass;
	ptr->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ptr->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&ptr->ptr, NULL);
	RETERR(name_duporclone(&name, mctx, &ptr->ptr));
	ptr->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_ptr(ARGS_FREESTRUCT) {
	dns_rdata_ptr_t *ptr = source;

	REQUIRE(ptr != NULL);
	REQUIRE(ptr->common.rdtype == dns_rdatatype_ptr);

	if (ptr->mctx == NULL) {
		return;
	}

	dns_name_free(&ptr->ptr, ptr->mctx);
	ptr->mctx = NULL;
}

static isc_result_t
additionaldata_ptr(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_ptr);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_ptr(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_ptr);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static bool
checkowner_ptr(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_ptr);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static unsigned char ip6_arpa_data[] = "\003IP6\004ARPA";
static unsigned char ip6_arpa_offsets[] = { 0, 4, 9 };
static const dns_name_t ip6_arpa = DNS_NAME_INITABSOLUTE(ip6_arpa_data,
							 ip6_arpa_offsets);

static unsigned char ip6_int_data[] = "\003IP6\003INT";
static unsigned char ip6_int_offsets[] = { 0, 4, 8 };
static const dns_name_t ip6_int = DNS_NAME_INITABSOLUTE(ip6_int_data,
							ip6_int_offsets);

static unsigned char in_addr_arpa_data[] = "\007IN-ADDR\004ARPA";
static unsigned char in_addr_arpa_offsets[] = { 0, 8, 13 };
static const dns_name_t in_addr_arpa =
	DNS_NAME_INITABSOLUTE(in_addr_arpa_data, in_addr_arpa_offsets);

static bool
checknames_ptr(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_ptr);

	if (rdata->rdclass != dns_rdataclass_in) {
		return (true);
	}

	if (dns_name_isdnssd(owner)) {
		return (true);
	}

	if (dns_name_issubdomain(owner, &in_addr_arpa) ||
	    dns_name_issubdomain(owner, &ip6_arpa) ||
	    dns_name_issubdomain(owner, &ip6_int))
	{
		dns_rdata_toregion(rdata, &region);
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &region);
		if (!dns_name_ishostname(&name, false)) {
			if (bad != NULL) {
				dns_name_clone(&name, bad);
			}
			return (false);
		}
	}
	return (true);
}

static int
casecompare_ptr(ARGS_COMPARE) {
	return (compare_ptr(rdata1, rdata2));
}
#endif /* RDATA_GENERIC_PTR_12_C */
