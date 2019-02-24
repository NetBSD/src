/*	$NetBSD: amtrelay_260.c,v 1.1.1.1 2019/02/24 18:56:52 christos Exp $	*/

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


#ifndef RDATA_GENERIC_AMTRELAY_260_C
#define RDATA_GENERIC_AMTRELAY_260_C

#include <string.h>

#include <isc/net.h>

#define RRTYPE_AMTRELAY_ATTRIBUTES (0)

static inline isc_result_t
fromtext_amtrelay(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	unsigned int discovery;
	unsigned int gateway;
	struct in_addr addr;
	unsigned char addr6[16];
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_amtrelay);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Precedence.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffU) {
		RETTOK(ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Discovery.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 1U) {
		RETTOK(ISC_R_RANGE);
	}
	discovery = token.value.as_ulong;

	/*
	 * Gateway type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0x7fU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong | (discovery << 7), target));
	gateway = token.value.as_ulong;

	if (gateway == 0) {
		return (ISC_R_SUCCESS);
	}

	if (gateway > 3) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	/*
	 * Gateway.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	switch (gateway) {
	case 1:
		if (inet_pton(AF_INET, DNS_AS_STR(token), &addr) != 1) {
			RETTOK(DNS_R_BADDOTTEDQUAD);
		}
		isc_buffer_availableregion(target, &region);
		if (region.length < 4) {
			return (ISC_R_NOSPACE);
		}
		memmove(region.base, &addr, 4);
		isc_buffer_add(target, 4);
		return (ISC_R_SUCCESS);

	case 2:
		if (inet_pton(AF_INET6, DNS_AS_STR(token), addr6) != 1) {
			RETTOK(DNS_R_BADAAAA);
		}
		isc_buffer_availableregion(target, &region);
		if (region.length < 16) {
			return (ISC_R_NOSPACE);
		}
		memmove(region.base, addr6, 16);
		isc_buffer_add(target, 16);
		return (ISC_R_SUCCESS);

	case 3:
		dns_name_init(&name, NULL);
		buffer_fromregion(&buffer, &token.value.as_region);
		if (origin == NULL) {
			origin = dns_rootname;
		}
		return (dns_name_fromtext(&name, &buffer, origin, options,
					  target));
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static inline isc_result_t
totext_amtrelay(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	char buf[sizeof("0 255 ")];
	unsigned char precedence;
	unsigned char discovery;
	unsigned char gateway;
	const char *space;

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);
	REQUIRE(rdata->length >= 2);

	if ((rdata->data[1] & 0x7f) > 3U)
		return (ISC_R_NOTIMPLEMENTED);

	/*
	 * Precedence.
	 */
	dns_rdata_toregion(rdata, &region);
	precedence = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	snprintf(buf, sizeof(buf), "%u ", precedence);
	RETERR(str_totext(buf, target));

	/*
	 * Discovery and Gateway type.
	 */
	gateway = uint8_fromregion(&region);
	discovery = gateway >> 7;
	gateway &= 0x7f;
	space = (gateway != 0U) ? " " : "";
	isc_region_consume(&region, 1);
	snprintf(buf, sizeof(buf), "%u %u%s", discovery, gateway, space);
	RETERR(str_totext(buf, target));

	/*
	 * Gateway.
	 */
	switch (gateway) {
	case 0:
		break;
	case 1:
		return (inet_totext(AF_INET, &region, target));

	case 2:
		return (inet_totext(AF_INET6, &region, target));

	case 3:
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &region);
		return (dns_name_totext(&name, false, target));

	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_amtrelay(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_amtrelay);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	isc_buffer_activeregion(source, &region);
	if (region.length < 2)
		return (ISC_R_UNEXPECTEDEND);

	switch (region.base[1] & 0x7f) {
	case 0:
		if (region.length != 2) {
			return (DNS_R_FORMERR);
		}
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 1:
		if (region.length != 6) {
			return (DNS_R_FORMERR);
		}
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 2:
		if (region.length != 18) {
			return (DNS_R_FORMERR);
		}
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 3:
		RETERR(mem_tobuffer(target, region.base, 2));
		isc_buffer_forward(source, 2);
		dns_name_init(&name, NULL);
		return (dns_name_fromwire(&name, source, dctx, options,
					  target));

	default:
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));
	}
}

static inline isc_result_t
towire_amtrelay(ARGS_TOWIRE) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &region);
	return (mem_tobuffer(target, region.base, region.length));
}

static inline int
compare_amtrelay(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_amtrelay);
	REQUIRE(rdata1->length >= 2);
	REQUIRE(rdata2->length >= 2);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	return (isc_region_compare(&region1, &region2));
}

static inline isc_result_t
fromstruct_amtrelay(ARGS_FROMSTRUCT) {
	dns_rdata_amtrelay_t *amtrelay = source;
	isc_region_t region;
	uint32_t n;

	REQUIRE(type == dns_rdatatype_amtrelay);
	REQUIRE(source != NULL);
	REQUIRE(amtrelay->common.rdtype == type);
	REQUIRE(amtrelay->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(amtrelay->precedence, target));
	n = (amtrelay->discovery ? 0x80 : 0) | amtrelay->gateway_type;
	RETERR(uint8_tobuffer(n, target));

	switch  (amtrelay->gateway_type) {
	case 0:
		return (ISC_R_SUCCESS);

	case 1:
		n = ntohl(amtrelay->in_addr.s_addr);
		return (uint32_tobuffer(n, target));

	case 2:
		return (mem_tobuffer(target, amtrelay->in6_addr.s6_addr, 16));
		break;

	case 3:
		dns_name_toregion(&amtrelay->gateway, &region);
		return (isc_buffer_copyregion(target, &region));
		break;

	default:
		return (mem_tobuffer(target, amtrelay->data, amtrelay->length));
	}
}

static inline isc_result_t
tostruct_amtrelay(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_amtrelay_t *amtrelay = target;
	dns_name_t name;
	uint32_t n;

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length >= 2);

	amtrelay->common.rdclass = rdata->rdclass;
	amtrelay->common.rdtype = rdata->type;
	ISC_LINK_INIT(&amtrelay->common, link);

	dns_name_init(&amtrelay->gateway, NULL);
	amtrelay->data = NULL;

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);

	amtrelay->precedence = uint8_fromregion(&region);
	isc_region_consume(&region, 1);

	amtrelay->gateway_type = uint8_fromregion(&region);
	amtrelay->discovery = (amtrelay->gateway_type & 0x80) != 0;
	amtrelay->gateway_type &= 0x7f;
	isc_region_consume(&region, 1);

	switch (amtrelay->gateway_type) {
	case 0:
		break;

	case 1:
		n = uint32_fromregion(&region);
		amtrelay->in_addr.s_addr = htonl(n);
		isc_region_consume(&region, 4);
		break;

	case 2:
		memmove(amtrelay->in6_addr.s6_addr, region.base, 16);
		isc_region_consume(&region, 16);
		break;

	case 3:
		dns_name_fromregion(&name, &region);
		RETERR(name_duporclone(&name, mctx, &amtrelay->gateway));
		isc_region_consume(&region, name_length(&name));
		break;

	default:
		if (region.length != 0) {
			amtrelay->data = mem_maybedup(mctx, region.base,
						      region.length);
			if (amtrelay->data == NULL) {
				return (ISC_R_NOMEMORY);
			}
		}
		amtrelay->length = region.length;
	}
	amtrelay->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_amtrelay(ARGS_FREESTRUCT) {
	dns_rdata_amtrelay_t *amtrelay = source;

	REQUIRE(source != NULL);
	REQUIRE(amtrelay->common.rdtype == dns_rdatatype_amtrelay);

	if (amtrelay->mctx == NULL)
		return;

	if (amtrelay->gateway_type == 3)
		dns_name_free(&amtrelay->gateway, amtrelay->mctx);

	if (amtrelay->data != NULL)
		isc_mem_free(amtrelay->mctx, amtrelay->data);

	amtrelay->mctx = NULL;
}

static inline isc_result_t
additionaldata_amtrelay(ARGS_ADDLDATA) {

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_amtrelay(ARGS_DIGEST) {
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);

	dns_rdata_toregion(rdata, &region);
	return ((digest)(arg, &region));
}

static inline bool
checkowner_amtrelay(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_amtrelay);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static inline bool
checknames_amtrelay(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_amtrelay);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_amtrelay(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;
	dns_name_t name1;
	dns_name_t name2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_amtrelay);
	REQUIRE(rdata1->length >= 2);
	REQUIRE(rdata2->length >= 2);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	if (memcmp(region1.base, region2.base, 2) != 0 ||
	    (region1.base[1] & 0x7f) != 3)
		return (isc_region_compare(&region1, &region2));

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	isc_region_consume(&region1, 2);
	isc_region_consume(&region2, 2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

#endif	/* RDATA_GENERIC_AMTRELAY_260_C */
