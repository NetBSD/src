/*	$NetBSD: atma_34.c,v 1.7.2.1 2024/02/25 15:47:06 martin Exp $	*/

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

/* http://www.broadband-forum.org/ftp/pub/approved-specs/af-dans-0152.000.pdf */

#ifndef RDATA_IN_1_ATMA_22_C
#define RDATA_IN_1_ATMA_22_C

#define RRTYPE_ATMA_ATTRIBUTES (0)

static isc_result_t
fromtext_in_atma(ARGS_FROMTEXT) {
	isc_token_t token;
	isc_textregion_t *sr;
	int n;
	bool valid = false;
	bool lastwasperiod = true; /* leading periods not allowed */
	int digits = 0;
	unsigned char c = 0;

	REQUIRE(type == dns_rdatatype_atma);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/* ATM End System Address (AESA) format or E.164 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	sr = &token.value.as_textregion;
	if (sr->length < 1) {
		RETTOK(ISC_R_UNEXPECTEDEND);
	}

	if (sr->base[0] != '+') {
		/*
		 * Format 0: ATM End System Address (AESA) format.
		 */
		c = 0;
		RETERR(mem_tobuffer(target, &c, 1));
		while (sr->length > 0) {
			if (sr->base[0] == '.') {
				if (lastwasperiod) {
					RETTOK(DNS_R_SYNTAX);
				}
				isc_textregion_consume(sr, 1);
				lastwasperiod = true;
				continue;
			}
			if ((n = hexvalue(sr->base[0])) == -1) {
				RETTOK(DNS_R_SYNTAX);
			}
			c <<= 4;
			c += n;
			if (++digits == 2) {
				RETERR(mem_tobuffer(target, &c, 1));
				valid = true;
				digits = 0;
				c = 0;
			}
			isc_textregion_consume(sr, 1);
			lastwasperiod = false;
		}
		if (digits != 0 || !valid || lastwasperiod) {
			RETTOK(ISC_R_UNEXPECTEDEND);
		}
	} else {
		/*
		 * Format 1:  E.164.
		 */
		c = 1;
		RETERR(mem_tobuffer(target, &c, 1));
		isc_textregion_consume(sr, 1);
		while (sr->length > 0) {
			if (sr->base[0] == '.') {
				if (lastwasperiod) {
					RETTOK(DNS_R_SYNTAX);
				}
				isc_textregion_consume(sr, 1);
				lastwasperiod = true;
				continue;
			}
			if (!isdigit((unsigned char)sr->base[0])) {
				RETTOK(DNS_R_SYNTAX);
			}
			RETERR(mem_tobuffer(target, sr->base, 1));
			isc_textregion_consume(sr, 1);
			lastwasperiod = false;
		}
		if (lastwasperiod) {
			RETTOK(ISC_R_UNEXPECTEDEND);
		}
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_in_atma(ARGS_TOTEXT) {
	isc_region_t region;
	char buf[sizeof("xx")];

	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	INSIST(region.length > 1);
	switch (region.base[0]) {
	case 0:
		isc_region_consume(&region, 1);
		while (region.length != 0) {
			snprintf(buf, sizeof(buf), "%02x", region.base[0]);
			isc_region_consume(&region, 1);
			RETERR(str_totext(buf, target));
		}
		break;
	case 1:
		RETERR(str_totext("+", target));
		isc_region_consume(&region, 1);
		RETERR(mem_tobuffer(target, region.base, region.length));
		break;
	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
fromwire_in_atma(ARGS_FROMWIRE) {
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_atma);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &region);
	if (region.length < 2) {
		return (ISC_R_UNEXPECTEDEND);
	}
	if (region.base[0] == 1) {
		unsigned int i;
		for (i = 1; i < region.length; i++) {
			if (!isdigit((unsigned char)region.base[i])) {
				return (DNS_R_FORMERR);
			}
		}
	}
	RETERR(mem_tobuffer(target, region.base, region.length));
	isc_buffer_forward(source, region.length);
	return (ISC_R_SUCCESS);
}

static isc_result_t
towire_in_atma(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_in_atma(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_atma);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_in_atma(ARGS_FROMSTRUCT) {
	dns_rdata_in_atma_t *atma = source;

	REQUIRE(type == dns_rdatatype_atma);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(atma != NULL);
	REQUIRE(atma->common.rdtype == type);
	REQUIRE(atma->common.rdclass == rdclass);
	REQUIRE(atma->atma != NULL || atma->atma_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(mem_tobuffer(target, &atma->format, 1));
	return (mem_tobuffer(target, atma->atma, atma->atma_len));
}

static isc_result_t
tostruct_in_atma(ARGS_TOSTRUCT) {
	dns_rdata_in_atma_t *atma = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(atma != NULL);
	REQUIRE(rdata->length != 0);

	atma->common.rdclass = rdata->rdclass;
	atma->common.rdtype = rdata->type;
	ISC_LINK_INIT(&atma->common, link);

	dns_rdata_toregion(rdata, &r);
	atma->format = r.base[0];
	isc_region_consume(&r, 1);
	atma->atma_len = r.length;
	atma->atma = mem_maybedup(mctx, r.base, r.length);
	if (atma->atma == NULL) {
		return (ISC_R_NOMEMORY);
	}

	atma->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static void
freestruct_in_atma(ARGS_FREESTRUCT) {
	dns_rdata_in_atma_t *atma = source;

	REQUIRE(atma != NULL);
	REQUIRE(atma->common.rdclass == dns_rdataclass_in);
	REQUIRE(atma->common.rdtype == dns_rdatatype_atma);

	if (atma->mctx == NULL) {
		return;
	}

	if (atma->atma != NULL) {
		isc_mem_free(atma->mctx, atma->atma);
	}
	atma->mctx = NULL;
}

static isc_result_t
additionaldata_in_atma(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_in_atma(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_in_atma(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_atma);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_in_atma(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_atma);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_in_atma(ARGS_COMPARE) {
	return (compare_in_atma(rdata1, rdata2));
}

#endif /* RDATA_IN_1_atma_22_C */
