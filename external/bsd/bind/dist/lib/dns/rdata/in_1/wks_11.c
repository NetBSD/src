/*	$NetBSD: wks_11.c,v 1.8.4.1 2018/04/16 01:57:57 pgoyette Exp $	*/

/*
 * Copyright (C) 2004, 2007, 2009, 2011-2017  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* Reviewed: Fri Mar 17 15:01:49 PST 2000 by explorer */

#ifndef RDATA_IN_1_WKS_11_C
#define RDATA_IN_1_WKS_11_C

#include <limits.h>
#include <stdlib.h>

#include <isc/net.h>
#include <isc/netdb.h>
#include <isc/once.h>

#define RRTYPE_WKS_ATTRIBUTES (0)

static isc_mutex_t wks_lock;

static void init_lock(void) {
	RUNTIME_CHECK(isc_mutex_init(&wks_lock) == ISC_R_SUCCESS);
}

static isc_boolean_t
mygetprotobyname(const char *name, long *proto) {
	struct protoent *pe;

	LOCK(&wks_lock);
	pe = getprotobyname(name);
	if (pe != NULL)
		*proto = pe->p_proto;
	UNLOCK(&wks_lock);
	return (ISC_TF(pe != NULL));
}

static isc_boolean_t
mygetservbyname(const char *name, const char *proto, long *port) {
	struct servent *se;

	LOCK(&wks_lock);
	se = getservbyname(name, proto);
	if (se != NULL)
		*port = ntohs(se->s_port);
	UNLOCK(&wks_lock);
	return (ISC_TF(se != NULL));
}

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

static inline isc_result_t
fromtext_in_wks(ARGS_FROMTEXT) {
	static isc_once_t once = ISC_ONCE_INIT;
	isc_token_t token;
	isc_region_t region;
	struct in_addr addr;
	char *e;
	long proto;
	unsigned char bm[8*1024]; /* 64k bits */
	long port;
	long maxport = -1;
	const char *ps = NULL;
	unsigned int n;
	char service[32];
	int i;
	isc_result_t result;

	REQUIRE(type == dns_rdatatype_wks);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);

	RUNTIME_CHECK(isc_once_do(&once, init_lock) == ISC_R_SUCCESS);

#ifdef _WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 0);

		err = WSAStartup(wVersionRequested, &wsaData );
		if (err != 0)
			return (ISC_R_FAILURE);
	}
#endif

	/*
	 * IPv4 dotted quad.
	 */
	CHECK(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	isc_buffer_availableregion(target, &region);
	if (getquad(DNS_AS_STR(token), &addr, lexer, callbacks) != 1)
		CHECKTOK(DNS_R_BADDOTTEDQUAD);
	if (region.length < 4)
		return (ISC_R_NOSPACE);
	memmove(region.base, &addr, 4);
	isc_buffer_add(target, 4);

	/*
	 * Protocol.
	 */
	CHECK(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	proto = strtol(DNS_AS_STR(token), &e, 10);
	if (*e == 0)
		;
	else if (!mygetprotobyname(DNS_AS_STR(token), &proto))
		CHECKTOK(DNS_R_UNKNOWNPROTO);

	if (proto < 0 || proto > 0xff)
		CHECKTOK(ISC_R_RANGE);

	if (proto == IPPROTO_TCP)
		ps = "tcp";
	else if (proto == IPPROTO_UDP)
		ps = "udp";

	CHECK(uint8_tobuffer(proto, target));

	memset(bm, 0, sizeof(bm));
	do {
		CHECK(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string, ISC_TRUE));
		if (token.type != isc_tokentype_string)
			break;

		/*
		 * Lowercase the service string as some getservbyname() are
		 * case sensitive and the database is usually in lowercase.
		 */
		strncpy(service, DNS_AS_STR(token), sizeof(service));
		service[sizeof(service)-1] = '\0';
		for (i = strlen(service) - 1; i >= 0; i--)
			if (isupper(service[i]&0xff))
				service[i] = tolower(service[i]&0xff);

		port = strtol(DNS_AS_STR(token), &e, 10);
		if (*e == 0)
			;
		else if (!mygetservbyname(service, ps, &port) &&
			 !mygetservbyname(DNS_AS_STR(token), ps, &port))
			CHECKTOK(DNS_R_UNKNOWNSERVICE);
		if (port < 0 || port > 0xffff)
			CHECKTOK(ISC_R_RANGE);
		if (port > maxport)
			maxport = port;
		bm[port / 8] |= (0x80 >> (port % 8));
	} while (1);

	/*
	 * Let upper layer handle eol/eof.
	 */
	isc_lex_ungettoken(lexer, &token);

	n = (maxport + 8) / 8;
	result = mem_tobuffer(target, bm, n);

 cleanup:
#ifdef _WIN32
	WSACleanup();
#endif

	return (result);
}

static inline isc_result_t
totext_in_wks(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned short proto;
	char buf[sizeof("65535")];
	unsigned int i, j;

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length >= 5);

	dns_rdata_toregion(rdata, &sr);
	RETERR(inet_totext(AF_INET, &sr, target));
	isc_region_consume(&sr, 4);

	proto = uint8_fromregion(&sr);
	snprintf(buf, sizeof(buf), "%u", proto);
	RETERR(str_totext(" ", target));
	RETERR(str_totext(buf, target));
	isc_region_consume(&sr, 1);

	INSIST(sr.length <= 8*1024);
	for (i = 0; i < sr.length; i++) {
		if (sr.base[i] != 0)
			for (j = 0; j < 8; j++)
				if ((sr.base[i] & (0x80 >> j)) != 0) {
					snprintf(buf, sizeof(buf),
						 "%u", i * 8 + j);
					RETERR(str_totext(" ", target));
					RETERR(str_totext(buf, target));
				}
	}

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_in_wks(ARGS_FROMWIRE) {
	isc_region_t sr;
	isc_region_t tr;

	REQUIRE(type == dns_rdatatype_wks);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &sr);
	isc_buffer_availableregion(target, &tr);

	if (sr.length < 5)
		return (ISC_R_UNEXPECTEDEND);
	if (sr.length > 8 * 1024 + 5)
		return (DNS_R_EXTRADATA);
	if (tr.length < sr.length)
		return (ISC_R_NOSPACE);

	memmove(tr.base, sr.base, sr.length);
	isc_buffer_add(target, sr.length);
	isc_buffer_forward(source, sr.length);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_in_wks(ARGS_TOWIRE) {
	isc_region_t sr;

	UNUSED(cctx);

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_in_wks(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_wks);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_in_wks(ARGS_FROMSTRUCT) {
	dns_rdata_in_wks_t *wks = source;
	isc_uint32_t a;

	REQUIRE(type == dns_rdatatype_wks);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(source != NULL);
	REQUIRE(wks->common.rdtype == type);
	REQUIRE(wks->common.rdclass == rdclass);
	REQUIRE((wks->map != NULL && wks->map_len <= 8*1024) ||
		 wks->map_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	a = ntohl(wks->in_addr.s_addr);
	RETERR(uint32_tobuffer(a, target));
	RETERR(uint8_tobuffer(wks->protocol, target));
	return (mem_tobuffer(target, wks->map, wks->map_len));
}

static inline isc_result_t
tostruct_in_wks(ARGS_TOSTRUCT) {
	dns_rdata_in_wks_t *wks = target;
	isc_uint32_t n;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	wks->common.rdclass = rdata->rdclass;
	wks->common.rdtype = rdata->type;
	ISC_LINK_INIT(&wks->common, link);

	dns_rdata_toregion(rdata, &region);
	n = uint32_fromregion(&region);
	wks->in_addr.s_addr = htonl(n);
	isc_region_consume(&region, 4);
	wks->protocol = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	wks->map_len = region.length;
	wks->map = mem_maybedup(mctx, region.base, region.length);
	if (wks->map == NULL)
		return (ISC_R_NOMEMORY);
	wks->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_wks(ARGS_FREESTRUCT) {
	dns_rdata_in_wks_t *wks = source;

	REQUIRE(source != NULL);
	REQUIRE(wks->common.rdtype == dns_rdatatype_wks);
	REQUIRE(wks->common.rdclass == dns_rdataclass_in);

	if (wks->mctx == NULL)
		return;

	if (wks->map != NULL)
		isc_mem_free(wks->mctx, wks->map);
	wks->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_wks(ARGS_ADDLDATA) {
	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_wks(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_in_wks(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_wks);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(rdclass);

	return (dns_name_ishostname(name, wildcard));
}

static inline isc_boolean_t
checknames_in_wks(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_wks);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

static inline int
casecompare_in_wks(ARGS_COMPARE) {
	return (compare_in_wks(rdata1, rdata2));
}

#endif	/* RDATA_IN_1_WKS_11_C */
