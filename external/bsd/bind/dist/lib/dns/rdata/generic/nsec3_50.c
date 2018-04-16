/*	$NetBSD: nsec3_50.c,v 1.10.14.1 2018/04/16 01:57:57 pgoyette Exp $	*/

/*
 * Copyright (C) 2008, 2009, 2011, 2012, 2014, 2015, 2017  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

/*
 * Copyright (C) 2004  Nominet, Ltd.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINET DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* RFC 5155 */

#ifndef RDATA_GENERIC_NSEC3_50_C
#define RDATA_GENERIC_NSEC3_50_C

#include <isc/iterated_hash.h>
#include <isc/base32.h>

#define RRTYPE_NSEC3_ATTRIBUTES DNS_RDATATYPEATTR_DNSSEC

static inline isc_result_t
fromtext_nsec3(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned int flags;
	unsigned char hashalg;
	isc_buffer_t b;
	unsigned char buf[256];

	REQUIRE(type == dns_rdatatype_nsec3);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);
	UNUSED(origin);
	UNUSED(options);

	/* Hash. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_hashalg_fromtext(&hashalg, &token.value.as_textregion));
	RETERR(uint8_tobuffer(hashalg, target));

	/* Flags. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	flags = token.value.as_ulong;
	if (flags > 255U)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(flags, target));

	/* Iterations. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/* salt */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (token.value.as_textregion.length > (255*2))
		RETTOK(DNS_R_TEXTTOOLONG);
	if (strcmp(DNS_AS_STR(token), "-") == 0) {
		RETERR(uint8_tobuffer(0, target));
	} else {
		RETERR(uint8_tobuffer(strlen(DNS_AS_STR(token)) / 2, target));
		RETERR(isc_hex_decodestring(DNS_AS_STR(token), target));
	}

	/*
	 * Next hash a single base32hex word.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	isc_buffer_init(&b, buf, sizeof(buf));
	RETTOK(isc_base32hexnp_decodestring(DNS_AS_STR(token), &b));
	if (isc_buffer_usedlength(&b) > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(isc_buffer_usedlength(&b), target));
	RETERR(mem_tobuffer(target, &buf, isc_buffer_usedlength(&b)));

	return (typemap_fromtext(lexer, target, ISC_TRUE));
}

static inline isc_result_t
totext_nsec3(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned int i, j;
	unsigned char hash;
	unsigned char flags;
	char buf[sizeof("TYPE65535")];
	isc_uint32_t iterations;

	REQUIRE(rdata->type == dns_rdatatype_nsec3);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	/* Hash */
	hash = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%u ", hash);
	RETERR(str_totext(buf, target));

	/* Flags */
	flags = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	snprintf(buf, sizeof(buf), "%u ", flags);
	RETERR(str_totext(buf, target));

	/* Iterations */
	iterations = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	snprintf(buf, sizeof(buf), "%u ", iterations);
	RETERR(str_totext(buf, target));

	/* Salt */
	j = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	INSIST(j <= sr.length);

	if (j != 0) {
		i = sr.length;
		sr.length = j;
		RETERR(isc_hex_totext(&sr, 1, "", target));
		sr.length = i - j;
	} else
		RETERR(str_totext("-", target));

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));

	/* Next hash */
	j = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	INSIST(j <= sr.length);

	i = sr.length;
	sr.length = j;
	RETERR(isc_base32hexnp_totext(&sr, 1, "", target));
	sr.length = i - j;

	/*
	 * Don't leave a trailing space when there's no typemap present.
	 */
	if (((tctx->flags & DNS_STYLEFLAG_MULTILINE) == 0) && (sr.length > 0))
		RETERR(str_totext(" ", target));

	RETERR(typemap_totext(&sr, tctx, target));

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_nsec3(ARGS_FROMWIRE) {
	isc_region_t sr, rr;
	unsigned int saltlen, hashlen;

	REQUIRE(type == dns_rdatatype_nsec3);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(options);
	UNUSED(dctx);

	isc_buffer_activeregion(source, &sr);
	rr = sr;

	/* hash(1), flags(1), iteration(2), saltlen(1) */
	if (sr.length < 5U)
		RETERR(DNS_R_FORMERR);
	saltlen = sr.base[4];
	isc_region_consume(&sr, 5);

	if (sr.length < saltlen)
		RETERR(DNS_R_FORMERR);
	isc_region_consume(&sr, saltlen);

	if (sr.length < 1U)
		RETERR(DNS_R_FORMERR);
	hashlen = sr.base[0];
	isc_region_consume(&sr, 1);

	if (sr.length < hashlen)
		RETERR(DNS_R_FORMERR);
	isc_region_consume(&sr, hashlen);

	RETERR(typemap_test(&sr, ISC_TRUE));

	RETERR(mem_tobuffer(target, rr.base, rr.length));
	isc_buffer_forward(source, rr.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_nsec3(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == dns_rdatatype_nsec3);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_nsec3(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_nsec3);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_nsec3(ARGS_FROMSTRUCT) {
	dns_rdata_nsec3_t *nsec3 = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_nsec3);
	REQUIRE(source != NULL);
	REQUIRE(nsec3->common.rdtype == type);
	REQUIRE(nsec3->common.rdclass == rdclass);
	REQUIRE(nsec3->typebits != NULL || nsec3->len == 0);
	REQUIRE(nsec3->hash == dns_hash_sha1);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(nsec3->hash, target));
	RETERR(uint8_tobuffer(nsec3->flags, target));
	RETERR(uint16_tobuffer(nsec3->iterations, target));
	RETERR(uint8_tobuffer(nsec3->salt_length, target));
	RETERR(mem_tobuffer(target, nsec3->salt, nsec3->salt_length));
	RETERR(uint8_tobuffer(nsec3->next_length, target));
	RETERR(mem_tobuffer(target, nsec3->next, nsec3->next_length));

	region.base = nsec3->typebits;
	region.length = nsec3->len;
	RETERR(typemap_test(&region, ISC_TRUE));
	return (mem_tobuffer(target, nsec3->typebits, nsec3->len));
}

static inline isc_result_t
tostruct_nsec3(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_nsec3_t *nsec3 = target;

	REQUIRE(rdata->type == dns_rdatatype_nsec3);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nsec3->common.rdclass = rdata->rdclass;
	nsec3->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nsec3->common, link);

	region.base = rdata->data;
	region.length = rdata->length;
	nsec3->hash = uint8_consume_fromregion(&region);
	nsec3->flags = uint8_consume_fromregion(&region);
	nsec3->iterations = uint16_consume_fromregion(&region);

	nsec3->salt_length = uint8_consume_fromregion(&region);
	nsec3->salt = mem_maybedup(mctx, region.base, nsec3->salt_length);
	if (nsec3->salt == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&region, nsec3->salt_length);

	nsec3->next_length = uint8_consume_fromregion(&region);
	nsec3->next = mem_maybedup(mctx, region.base, nsec3->next_length);
	if (nsec3->next == NULL)
		goto cleanup;
	isc_region_consume(&region, nsec3->next_length);

	nsec3->len = region.length;
	nsec3->typebits = mem_maybedup(mctx, region.base, region.length);
	if (nsec3->typebits == NULL)
		goto cleanup;

	nsec3->mctx = mctx;
	return (ISC_R_SUCCESS);

  cleanup:
	if (nsec3->next != NULL)
		isc_mem_free(mctx, nsec3->next);
	isc_mem_free(mctx, nsec3->salt);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_nsec3(ARGS_FREESTRUCT) {
	dns_rdata_nsec3_t *nsec3 = source;

	REQUIRE(source != NULL);
	REQUIRE(nsec3->common.rdtype == dns_rdatatype_nsec3);

	if (nsec3->mctx == NULL)
		return;

	if (nsec3->salt != NULL)
		isc_mem_free(nsec3->mctx, nsec3->salt);
	if (nsec3->next != NULL)
		isc_mem_free(nsec3->mctx, nsec3->next);
	if (nsec3->typebits != NULL)
		isc_mem_free(nsec3->mctx, nsec3->typebits);
	nsec3->mctx = NULL;
}

static inline isc_result_t
additionaldata_nsec3(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_nsec3);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_nsec3(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_nsec3);

	dns_rdata_toregion(rdata, &r);
	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_nsec3(ARGS_CHECKOWNER) {
	unsigned char owner[NSEC3_MAX_HASH_LENGTH];
	isc_buffer_t buffer;
	dns_label_t label;

	REQUIRE(type == dns_rdatatype_nsec3);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	/*
	 * First label is a base32hex string without padding.
	 */
	dns_name_getlabel(name, 0, &label);
	isc_region_consume(&label, 1);
	isc_buffer_init(&buffer, owner, sizeof(owner));
	if (isc_base32hexnp_decoderegion(&label, &buffer) == ISC_R_SUCCESS)
		return (ISC_TRUE);

	return (ISC_FALSE);
}

static inline isc_boolean_t
checknames_nsec3(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_nsec3);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_nsec3(ARGS_COMPARE) {
	return (compare_nsec3(rdata1, rdata2));
}

#endif	/* RDATA_GENERIC_NSEC3_50_C */
