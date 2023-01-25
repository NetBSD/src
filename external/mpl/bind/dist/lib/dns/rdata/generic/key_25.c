/*	$NetBSD: key_25.c,v 1.11 2023/01/25 21:43:30 christos Exp $	*/

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

/* RFC2535 */

#ifndef RDATA_GENERIC_KEY_25_C
#define RDATA_GENERIC_KEY_25_C

#include <dst/dst.h>

#define RRTYPE_KEY_ATTRIBUTES \
	(DNS_RDATATYPEATTR_ATCNAME | DNS_RDATATYPEATTR_ZONECUTAUTH)

/*
 * RFC 2535 section 3.1.2 says that if bits 0-1 of the Flags field are
 * both set, it means there is no key information and the RR stops after
 * the algorithm octet.  However, this only applies to KEY records, as
 * indicated by the specifications of the RR types based on KEY:
 *
 *     CDNSKEY - RFC 7344
 *     DNSKEY - RFC 4034
 *     RKEY - draft-reid-dnsext-rkey-00
 */
static bool
generic_key_nokey(dns_rdatatype_t type, unsigned int flags) {
	switch (type) {
	case dns_rdatatype_cdnskey:
	case dns_rdatatype_dnskey:
	case dns_rdatatype_rkey:
		return (false);
	case dns_rdatatype_key:
	default:
		return ((flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY);
	}
}

static isc_result_t
generic_fromtext_key(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_secalg_t alg;
	dns_secproto_t proto;
	dns_keyflags_t flags;

	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/* flags */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_keyflags_fromtext(&flags, &token.value.as_textregion));
	if (type == dns_rdatatype_rkey && flags != 0U) {
		RETTOK(DNS_R_FORMERR);
	}
	RETERR(uint16_tobuffer(flags, target));

	/* protocol */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_secproto_fromtext(&proto, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &proto, 1));

	/* algorithm */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETTOK(dns_secalg_fromtext(&alg, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &alg, 1));

	/* No Key? */
	if (generic_key_nokey(type, flags)) {
		return (ISC_R_SUCCESS);
	}

	return (isc_base64_tobuffer(lexer, target, -2));
}

static isc_result_t
generic_totext_key(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("[key id = 64000]")];
	unsigned int flags;
	unsigned char algorithm;
	char algbuf[DNS_NAME_FORMATSIZE];
	const char *keyinfo;
	isc_region_t tmpr;

	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	/* flags */
	flags = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	snprintf(buf, sizeof(buf), "%u", flags);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));
	if ((flags & DNS_KEYFLAG_KSK) != 0) {
		if (flags & DNS_KEYFLAG_REVOKE) {
			keyinfo = "revoked KSK";
		} else {
			keyinfo = "KSK";
		}
	} else {
		keyinfo = "ZSK";
	}

	/* protocol */
	snprintf(buf, sizeof(buf), "%u", sr.base[0]);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/* algorithm */
	algorithm = sr.base[0];
	snprintf(buf, sizeof(buf), "%u", algorithm);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));

	/* No Key? */
	if (generic_key_nokey(rdata->type, flags)) {
		return (ISC_R_SUCCESS);
	}

	if ((tctx->flags & DNS_STYLEFLAG_RRCOMMENT) != 0 &&
	    algorithm == DNS_KEYALG_PRIVATEDNS)
	{
		dns_name_t name;
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &sr);
		dns_name_format(&name, algbuf, sizeof(algbuf));
	} else {
		dns_secalg_format((dns_secalg_t)algorithm, algbuf,
				  sizeof(algbuf));
	}

	/* key */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" (", target));
	}
	RETERR(str_totext(tctx->linebreak, target));

	if ((tctx->flags & DNS_STYLEFLAG_NOCRYPTO) == 0) {
		if (tctx->width == 0) { /* No splitting */
			RETERR(isc_base64_totext(&sr, 60, "", target));
		} else {
			RETERR(isc_base64_totext(&sr, tctx->width - 2,
						 tctx->linebreak, target));
		}
	} else {
		dns_rdata_toregion(rdata, &tmpr);
		snprintf(buf, sizeof(buf), "[key id = %u]",
			 dst_region_computeid(&tmpr));
		RETERR(str_totext(buf, target));
	}

	if ((tctx->flags & DNS_STYLEFLAG_RRCOMMENT) != 0) {
		RETERR(str_totext(tctx->linebreak, target));
	} else if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(" ", target));
	}

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(")", target));
	}

	if ((tctx->flags & DNS_STYLEFLAG_RRCOMMENT) != 0) {
		if (rdata->type == dns_rdatatype_dnskey ||
		    rdata->type == dns_rdatatype_cdnskey)
		{
			RETERR(str_totext(" ; ", target));
			RETERR(str_totext(keyinfo, target));
		}
		RETERR(str_totext("; alg = ", target));
		RETERR(str_totext(algbuf, target));
		RETERR(str_totext(" ; key id = ", target));
		dns_rdata_toregion(rdata, &tmpr);
		snprintf(buf, sizeof(buf), "%u", dst_region_computeid(&tmpr));
		RETERR(str_totext(buf, target));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
generic_fromwire_key(ARGS_FROMWIRE) {
	unsigned char algorithm;
	uint16_t flags;
	isc_region_t sr;

	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 4) {
		return (ISC_R_UNEXPECTEDEND);
	}
	flags = (sr.base[0] << 8) | sr.base[1];

	if (type == dns_rdatatype_rkey && flags != 0U) {
		return (DNS_R_FORMERR);
	}

	algorithm = sr.base[3];
	RETERR(mem_tobuffer(target, sr.base, 4));
	isc_region_consume(&sr, 4);
	isc_buffer_forward(source, 4);

	if (generic_key_nokey(type, flags)) {
		return (ISC_R_SUCCESS);
	}
	if (sr.length == 0) {
		return (ISC_R_UNEXPECTEDEND);
	}

	if (algorithm == DNS_KEYALG_PRIVATEDNS) {
		dns_name_t name;
		dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);
		dns_name_init(&name, NULL);
		RETERR(dns_name_fromwire(&name, source, dctx, options, target));
	}

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static isc_result_t
fromtext_key(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_key);

	return (generic_fromtext_key(CALL_FROMTEXT));
}

static isc_result_t
totext_key(ARGS_TOTEXT) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);

	return (generic_totext_key(CALL_TOTEXT));
}

static isc_result_t
fromwire_key(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_key);

	return (generic_fromwire_key(CALL_FROMWIRE));
}

static isc_result_t
towire_key(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static int
compare_key(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1 != NULL);
	REQUIRE(rdata2 != NULL);
	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_key);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
generic_fromstruct_key(ARGS_FROMSTRUCT) {
	dns_rdata_key_t *key = source;

	REQUIRE(key != NULL);
	REQUIRE(key->common.rdtype == type);
	REQUIRE(key->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	if (type == dns_rdatatype_rkey) {
		INSIST(key->flags == 0U);
	}

	/* Flags */
	RETERR(uint16_tobuffer(key->flags, target));

	/* Protocol */
	RETERR(uint8_tobuffer(key->protocol, target));

	/* Algorithm */
	RETERR(uint8_tobuffer(key->algorithm, target));

	/* Data */
	return (mem_tobuffer(target, key->data, key->datalen));
}

static isc_result_t
generic_tostruct_key(ARGS_TOSTRUCT) {
	dns_rdata_key_t *key = target;
	isc_region_t sr;

	REQUIRE(key != NULL);
	REQUIRE(rdata->length != 0);

	REQUIRE(key != NULL);
	REQUIRE(key->common.rdclass == rdata->rdclass);
	REQUIRE(key->common.rdtype == rdata->type);
	REQUIRE(!ISC_LINK_LINKED(&key->common, link));

	dns_rdata_toregion(rdata, &sr);

	/* Flags */
	if (sr.length < 2) {
		return (ISC_R_UNEXPECTEDEND);
	}
	key->flags = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);

	/* Protocol */
	if (sr.length < 1) {
		return (ISC_R_UNEXPECTEDEND);
	}
	key->protocol = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/* Algorithm */
	if (sr.length < 1) {
		return (ISC_R_UNEXPECTEDEND);
	}
	key->algorithm = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/* Data */
	key->datalen = sr.length;
	key->data = mem_maybedup(mctx, sr.base, key->datalen);
	if (key->data == NULL) {
		return (ISC_R_NOMEMORY);
	}

	key->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
generic_freestruct_key(ARGS_FREESTRUCT) {
	dns_rdata_key_t *key = (dns_rdata_key_t *)source;

	REQUIRE(key != NULL);

	if (key->mctx == NULL) {
		return;
	}

	if (key->data != NULL) {
		isc_mem_free(key->mctx, key->data);
	}
	key->mctx = NULL;
}

static isc_result_t
fromstruct_key(ARGS_FROMSTRUCT) {
	REQUIRE(type == dns_rdatatype_key);

	return (generic_fromstruct_key(CALL_FROMSTRUCT));
}

static isc_result_t
tostruct_key(ARGS_TOSTRUCT) {
	dns_rdata_key_t *key = target;

	REQUIRE(key != NULL);
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);

	key->common.rdclass = rdata->rdclass;
	key->common.rdtype = rdata->type;
	ISC_LINK_INIT(&key->common, link);

	return (generic_tostruct_key(CALL_TOSTRUCT));
}

static void
freestruct_key(ARGS_FREESTRUCT) {
	dns_rdata_key_t *key = (dns_rdata_key_t *)source;

	REQUIRE(key != NULL);
	REQUIRE(key->common.rdtype == dns_rdatatype_key);

	generic_freestruct_key(source);
}

static isc_result_t
additionaldata_key(ARGS_ADDLDATA) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_key(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_key(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_key);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_key(ARGS_CHECKNAMES) {
	REQUIRE(rdata != NULL);
	REQUIRE(rdata->type == dns_rdatatype_key);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_key(ARGS_COMPARE) {
	return (compare_key(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_KEY_25_C */
