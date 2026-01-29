/*	$NetBSD: dsync_66.c,v 1.1.1.1 2026/01/29 18:19:55 christos Exp $	*/

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

#ifndef RDATA_GENERIC_DSYNC_66_C
#define RDATA_GENERIC_DSYNC_66_C

#include <string.h>

#include <isc/net.h>

#include <dns/dsync.h>
#include <dns/fixedname.h>

#define RRTYPE_DSYNC_ATTRIBUTES (0)

static isc_result_t
fromtext_dsync(ARGS_FROMTEXT) {
	isc_token_t token;
	isc_result_t result;
	dns_fixedname_t fn;
	dns_name_t *name = dns_fixedname_initname(&fn);
	isc_buffer_t buffer;
	dns_rdatatype_t rrtype;
	dns_dsyncscheme_t scheme;
	bool ok = true;

	REQUIRE(type == dns_rdatatype_dsync);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * RRtype
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	result = dns_rdatatype_fromtext(&rrtype, &token.value.as_textregion);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTIMPLEMENTED) {
		char *e = NULL;
		long i = strtol(DNS_AS_STR(token), &e, 10);
		if (i < 0 || i > 65535) {
			RETTOK(ISC_R_RANGE);
		}
		if (*e != 0) {
			RETTOK(result);
		}
		rrtype = (dns_rdatatype_t)i;
	}
	RETERR(uint16_tobuffer(rrtype, target));

	/*
	 * Scheme
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	RETERR(dns_dsyncscheme_fromtext(&scheme, &token.value.as_textregion));
	RETERR(uint8_tobuffer(scheme, target));

	/*
	 * Port
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU) {
		RETTOK(ISC_R_RANGE);
	}
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Target
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));

	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL) {
		origin = dns_rootname;
	}
	RETTOK(dns_name_fromtext(name, &buffer, origin, options, target));
	if ((options & DNS_RDATA_CHECKNAMES) != 0) {
		ok = dns_name_ishostname(name, false);
	}
	if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0) {
		RETTOK(DNS_R_BADNAME);
	}
	if (!ok && callbacks != NULL) {
		warn_badname(name, lexer, callbacks);
	}
	return ISC_R_SUCCESS;
}

static isc_result_t
totext_dsync(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	unsigned int opts;
	char buf[sizeof("TYPE64000")];
	unsigned short num;
	dns_rdatatype_t type;
	dns_dsyncscheme_t scheme;

	REQUIRE(rdata->type == dns_rdatatype_dsync);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);

	/*
	 * Type.
	 */
	type = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	/*
	 * XXXAG We should have something like dns_rdatatype_isknown()
	 * that does the right thing with type 0.
	 */
	if (dns_rdatatype_isknown(type) && type != 0) {
		RETERR(dns_rdatatype_totext(type, target));
	} else {
		snprintf(buf, sizeof(buf), "TYPE%u", type);
		RETERR(str_totext(buf, target));
	}
	RETERR(str_totext(" ", target));

	/*
	 * Scheme.
	 */
	scheme = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	RETERR(dns_dsyncscheme_totext(scheme, target));

	RETERR(str_totext(" ", target));

	/*
	 * Port
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	snprintf(buf, sizeof(buf), "%u", num);
	RETERR(str_totext(buf, target));

	RETERR(str_totext(" ", target));

	/*
	 * Target
	 */
	dns_name_fromregion(&name, &region);
	opts = name_prefix(&name, tctx->origin, &prefix) ? DNS_NAME_OMITFINALDOT
							 : 0;
	return dns_name_totext(&prefix, opts, target);
}

static isc_result_t
fromwire_dsync(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t sregion;

	REQUIRE(type == dns_rdatatype_dsync);

	UNUSED(type);
	UNUSED(rdclass);

	dctx = dns_decompress_setpermitted(dctx, false);

	dns_name_init(&name, NULL);

	isc_buffer_activeregion(source, &sregion);
	if (sregion.length < 5) {
		return ISC_R_UNEXPECTEDEND;
	}
	RETERR(mem_tobuffer(target, sregion.base, 5));
	isc_buffer_forward(source, 5);
	return dns_name_fromwire(&name, source, dctx, target);
}

static isc_result_t
towire_dsync(ARGS_TOWIRE) {
	dns_name_t name;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_dsync);
	REQUIRE(rdata->length != 0);

	dns_compress_setpermitted(cctx, false);

	dns_rdata_toregion(rdata, &region);
	RETERR(mem_tobuffer(target, region.base, 5));
	isc_region_consume(&region, 5);

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);

	return dns_name_towire(&name, cctx, target, NULL);
}

static int
compare_dsync(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_dsync);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);
	return isc_region_compare(&region1, &region2);
}

static isc_result_t
fromstruct_dsync(ARGS_FROMSTRUCT) {
	dns_rdata_dsync_t *dsync = source;
	isc_region_t region;

	REQUIRE(type == dns_rdatatype_dsync);
	REQUIRE(dsync != NULL);
	REQUIRE(dsync->common.rdtype == type);
	REQUIRE(dsync->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(dsync->type, target));
	RETERR(uint16_tobuffer(dsync->scheme, target));
	RETERR(uint16_tobuffer(dsync->port, target));
	dns_name_toregion(&dsync->target, &region);
	return isc_buffer_copyregion(target, &region);
}

static isc_result_t
tostruct_dsync(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_dsync_t *dsync = target;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_dsync);
	REQUIRE(dsync != NULL);
	REQUIRE(rdata->length != 0);

	DNS_RDATACOMMON_INIT(dsync, rdata->type, rdata->rdclass);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dsync->type = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	dsync->scheme = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	dsync->port = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	dns_name_fromregion(&name, &region);
	dns_name_init(&dsync->target, NULL);
	name_duporclone(&name, mctx, &dsync->target);
	dsync->mctx = mctx;
	return ISC_R_SUCCESS;
}

static void
freestruct_dsync(ARGS_FREESTRUCT) {
	dns_rdata_dsync_t *dsync = source;

	REQUIRE(dsync != NULL);
	REQUIRE(dsync->common.rdtype == dns_rdatatype_dsync);

	if (dsync->mctx == NULL) {
		return;
	}

	dns_name_free(&dsync->target, dsync->mctx);
	dsync->mctx = NULL;
}

static isc_result_t
additionaldata_dsync(ARGS_ADDLDATA) {
	dns_name_t name;
	isc_region_t region;

	REQUIRE(rdata->type == dns_rdatatype_dsync);

	UNUSED(owner);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	isc_region_consume(&region, 5);
	dns_name_fromregion(&name, &region);

	if (dns_name_equal(&name, dns_rootname)) {
		return ISC_R_SUCCESS;
	}

	return (add)(arg, &name, dns_rdatatype_a, NULL DNS__DB_FILELINE);
}

static isc_result_t
digest_dsync(ARGS_DIGEST) {
	isc_region_t r1;

	REQUIRE(rdata->type == dns_rdatatype_dsync);

	dns_rdata_toregion(rdata, &r1);
	return (digest)(arg, &r1);
}

static bool
checkowner_dsync(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_dsync);

	UNUSED(name);
	UNUSED(rdclass);
	UNUSED(type);
	UNUSED(wildcard);

	return true;
}

static bool
checknames_dsync(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == dns_rdatatype_dsync);
	REQUIRE(rdata->length > 5);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	isc_region_consume(&region, 5);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ishostname(&name, false)) {
		if (bad != NULL) {
			dns_name_clone(&name, bad);
		}
		return false;
	}
	return true;
}

static int
casecompare_dsync(ARGS_COMPARE) {
	return compare_dsync(rdata1, rdata2);
}

#endif /* RDATA_GENERIC_DSYNC_66_C */
