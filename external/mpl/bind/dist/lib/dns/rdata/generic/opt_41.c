/*	$NetBSD: opt_41.c,v 1.3.4.2 2019/10/17 19:34:20 martin Exp $	*/

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

/* RFC2671 */

#ifndef RDATA_GENERIC_OPT_41_C
#define RDATA_GENERIC_OPT_41_C

#define RRTYPE_OPT_ATTRIBUTES (DNS_RDATATYPEATTR_SINGLETON | \
			       DNS_RDATATYPEATTR_META | \
			       DNS_RDATATYPEATTR_NOTQUESTION)

static inline isc_result_t
fromtext_opt(ARGS_FROMTEXT) {
	/*
	 * OPT records do not have a text format.
	 */

	REQUIRE(type == dns_rdatatype_opt);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(lexer);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(target);
	UNUSED(callbacks);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
totext_opt(ARGS_TOTEXT) {
	isc_region_t r;
	isc_region_t or;
	uint16_t option;
	uint16_t length;
	char buf[sizeof("64000 64000")];

	/*
	 * OPT records do not have a text format.
	 */

	REQUIRE(rdata->type == dns_rdatatype_opt);

	dns_rdata_toregion(rdata, &r);
	while (r.length > 0) {
		option = uint16_fromregion(&r);
		isc_region_consume(&r, 2);
		length = uint16_fromregion(&r);
		isc_region_consume(&r, 2);
		snprintf(buf, sizeof(buf), "%u %u", option, length);
		RETERR(str_totext(buf, target));
		INSIST(r.length >= length);
		if (length > 0) {
			if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
				RETERR(str_totext(" (", target));
			RETERR(str_totext(tctx->linebreak, target));
			or = r;
			or.length = length;
			if (tctx->width == 0)   /* No splitting */
				RETERR(isc_base64_totext(&or, 60, "", target));
			else
				RETERR(isc_base64_totext(&or, tctx->width - 2,
							 tctx->linebreak,
							 target));
			isc_region_consume(&r, length);
			if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
				RETERR(str_totext(" )", target));
		}
		if (r.length > 0)
			RETERR(str_totext(" ", target));
	}

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_opt(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;
	uint16_t opt;
	uint16_t length;
	unsigned int total;

	REQUIRE(type == dns_rdatatype_opt);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sregion);
	if (sregion.length == 0) {
		return (ISC_R_SUCCESS);
	}
	total = 0;
	while (sregion.length != 0) {
		if (sregion.length < 4) {
			return (ISC_R_UNEXPECTEDEND);
		}
		opt = uint16_fromregion(&sregion);
		isc_region_consume(&sregion, 2);
		length = uint16_fromregion(&sregion);
		isc_region_consume(&sregion, 2);
		total += 4;
		if (sregion.length < length) {
			return (ISC_R_UNEXPECTEDEND);
		}
		switch (opt) {
		case DNS_OPT_LLQ:
			if (length != 18U) {
				return (DNS_R_OPTERR);
			}
			isc_region_consume(&sregion, length);
			break;
		case DNS_OPT_CLIENT_SUBNET: {
			uint16_t family;
			uint8_t addrlen;
			uint8_t scope;
			uint8_t addrbytes;

			if (length < 4) {
				return (DNS_R_OPTERR);
			}
			family = uint16_fromregion(&sregion);
			isc_region_consume(&sregion, 2);
			addrlen = uint8_fromregion(&sregion);
			isc_region_consume(&sregion, 1);
			scope = uint8_fromregion(&sregion);
			isc_region_consume(&sregion, 1);

			switch (family) {
			case 0:
				/*
				 * XXXMUKS: In queries and replies, if
				 * FAMILY is set to 0, SOURCE
				 * PREFIX-LENGTH and SCOPE PREFIX-LENGTH
				 * must be 0 and ADDRESS should not be
				 * present as the address and prefix
				 * lengths don't make sense because the
				 * family is unknown.
				 */
				if (addrlen != 0U || scope != 0U) {
					return (DNS_R_OPTERR);
				}
				break;
			case 1:
				if (addrlen > 32U || scope > 32U) {
					return (DNS_R_OPTERR);
				}
				break;
			case 2:
				if (addrlen > 128U || scope > 128U) {
					return (DNS_R_OPTERR);
				}
				break;
			default:
				return (DNS_R_OPTERR);
			}
			addrbytes = (addrlen + 7) / 8;
			if (addrbytes + 4 != length) {
				return (DNS_R_OPTERR);
			}

			if (addrbytes != 0U && (addrlen % 8) != 0) {
				uint8_t bits = ~0U << (8 - (addrlen % 8));
				bits &= sregion.base[addrbytes - 1];
				if (bits != sregion.base[addrbytes - 1]) {
					return (DNS_R_OPTERR);
				}
			}
			isc_region_consume(&sregion, addrbytes);
			break;
		}
		case DNS_OPT_EXPIRE:
			/*
			 * Request has zero length.  Response is 32 bits.
			 */
			if (length != 0 && length != 4) {
				return (DNS_R_OPTERR);
			}
			isc_region_consume(&sregion, length);
			break;
		case DNS_OPT_COOKIE:
			if (length != 8 && (length < 16 || length > 40)) {
				return (DNS_R_OPTERR);
			}
			isc_region_consume(&sregion, length);
			break;
		case DNS_OPT_KEY_TAG:
			if (length == 0 || (length % 2) != 0) {
				return (DNS_R_OPTERR);
			}
			isc_region_consume(&sregion, length);
			break;
		case DNS_OPT_CLIENT_TAG:
			/* FALLTHROUGH */
		case DNS_OPT_SERVER_TAG:
			if (length != 2) {
				return (DNS_R_OPTERR);
			}
			isc_region_consume(&sregion, length);
			break;
		default:
			isc_region_consume(&sregion, length);
			break;
		}
		total += length;
	}

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);
	if (tregion.length < total) {
		return (ISC_R_NOSPACE);
	}
	memmove(tregion.base, sregion.base, total);
	isc_buffer_forward(source, total);
	isc_buffer_add(target, total);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_opt(ARGS_TOWIRE) {

	REQUIRE(rdata->type == dns_rdatatype_opt);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_opt(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_opt);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_opt(ARGS_FROMSTRUCT) {
	dns_rdata_opt_t *opt = source;
	isc_region_t region;
	uint16_t length;

	REQUIRE(type == dns_rdatatype_opt);
	REQUIRE(source != NULL);
	REQUIRE(opt->common.rdtype == type);
	REQUIRE(opt->common.rdclass == rdclass);
	REQUIRE(opt->options != NULL || opt->length == 0);

	UNUSED(type);
	UNUSED(rdclass);

	region.base = opt->options;
	region.length = opt->length;
	while (region.length >= 4) {
		isc_region_consume(&region, 2);	/* opt */
		length = uint16_fromregion(&region);
		isc_region_consume(&region, 2);
		if (region.length < length)
			return (ISC_R_UNEXPECTEDEND);
		isc_region_consume(&region, length);
	}
	if (region.length != 0)
		return (ISC_R_UNEXPECTEDEND);

	return (mem_tobuffer(target, opt->options, opt->length));
}

static inline isc_result_t
tostruct_opt(ARGS_TOSTRUCT) {
	dns_rdata_opt_t *opt = target;
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_opt);
	REQUIRE(target != NULL);

	opt->common.rdclass = rdata->rdclass;
	opt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&opt->common, link);

	dns_rdata_toregion(rdata, &r);
	opt->length = r.length;
	opt->options = mem_maybedup(mctx, r.base, r.length);
	if (opt->options == NULL)
		return (ISC_R_NOMEMORY);

	opt->offset = 0;
	opt->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_opt(ARGS_FREESTRUCT) {
	dns_rdata_opt_t *opt = source;

	REQUIRE(source != NULL);
	REQUIRE(opt->common.rdtype == dns_rdatatype_opt);

	if (opt->mctx == NULL)
		return;

	if (opt->options != NULL)
		isc_mem_free(opt->mctx, opt->options);
	opt->mctx = NULL;
}

static inline isc_result_t
additionaldata_opt(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_opt);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_opt(ARGS_DIGEST) {

	/*
	 * OPT records are not digested.
	 */

	REQUIRE(rdata->type == dns_rdatatype_opt);

	UNUSED(rdata);
	UNUSED(digest);
	UNUSED(arg);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline bool
checkowner_opt(ARGS_CHECKOWNER) {

	REQUIRE(type == dns_rdatatype_opt);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (dns_name_equal(name, dns_rootname));
}

static inline bool
checknames_opt(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == dns_rdatatype_opt);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static inline int
casecompare_opt(ARGS_COMPARE) {
	return (compare_opt(rdata1, rdata2));
}

isc_result_t
dns_rdata_opt_first(dns_rdata_opt_t *opt) {

	REQUIRE(opt != NULL);
	REQUIRE(opt->common.rdtype == dns_rdatatype_opt);
	REQUIRE(opt->options != NULL || opt->length == 0);

	if (opt->length == 0)
		return (ISC_R_NOMORE);

	opt->offset = 0;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdata_opt_next(dns_rdata_opt_t *opt) {
	isc_region_t r;
	uint16_t length;

	REQUIRE(opt != NULL);
	REQUIRE(opt->common.rdtype == dns_rdatatype_opt);
	REQUIRE(opt->options != NULL && opt->length != 0);
	REQUIRE(opt->offset < opt->length);

	INSIST(opt->offset + 4 <= opt->length);
	r.base = opt->options + opt->offset + 2;
	r.length = opt->length - opt->offset - 2;
	length = uint16_fromregion(&r);
	INSIST(opt->offset + 4 + length <= opt->length);
	opt->offset = opt->offset + 4 + length;
	if (opt->offset == opt->length)
		return (ISC_R_NOMORE);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdata_opt_current(dns_rdata_opt_t *opt, dns_rdata_opt_opcode_t *opcode) {
	isc_region_t r;

	REQUIRE(opt != NULL);
	REQUIRE(opcode != NULL);
	REQUIRE(opt->common.rdtype == dns_rdatatype_opt);
	REQUIRE(opt->options != NULL);
	REQUIRE(opt->offset < opt->length);

	INSIST(opt->offset + 4 <= opt->length);
	r.base = opt->options + opt->offset;
	r.length = opt->length - opt->offset;

	opcode->opcode = uint16_fromregion(&r);
	isc_region_consume(&r, 2);
	opcode->length = uint16_fromregion(&r);
	isc_region_consume(&r, 2);
	opcode->data = r.base;
	INSIST(opt->offset + 4 + opcode->length <= opt->length);

	return (ISC_R_SUCCESS);
}

#endif	/* RDATA_GENERIC_OPT_41_C */
