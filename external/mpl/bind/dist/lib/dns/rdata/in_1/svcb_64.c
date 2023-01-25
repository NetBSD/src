/*	$NetBSD: svcb_64.c,v 1.3 2023/01/25 21:43:30 christos Exp $	*/

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

/* draft-ietf-dnsop-svcb-https-02 */

#ifndef RDATA_IN_1_SVCB_64_C
#define RDATA_IN_1_SVCB_64_C

#define RRTYPE_SVCB_ATTRIBUTES 0

#define SVCB_MAN_KEY		 0
#define SVCB_ALPN_KEY		 1
#define SVCB_NO_DEFAULT_ALPN_KEY 2

/*
 * Service Binding Parameter Registry
 */
enum encoding {
	sbpr_text,
	sbpr_port,
	sbpr_ipv4s,
	sbpr_ipv6s,
	sbpr_base64,
	sbpr_empty,
	sbpr_alpn,
	sbpr_keylist,
	sbpr_dohpath
};
static const struct {
	const char *name; /* Restricted to lowercase LDH by registry. */
	unsigned int value;
	enum encoding encoding;
	bool initial; /* Part of the first defined set of encodings. */
} sbpr[] = {
	{ "mandatory", 0, sbpr_keylist, true },
	{ "alpn", 1, sbpr_alpn, true },
	{ "no-default-alpn", 2, sbpr_empty, true },
	{ "port", 3, sbpr_port, true },
	{ "ipv4hint", 4, sbpr_ipv4s, true },
	{ "ech", 5, sbpr_base64, true },
	{ "ipv6hint", 6, sbpr_ipv6s, true },
	{ "dohpath", 7, sbpr_dohpath, false },
};

static isc_result_t
alpn_fromtxt(isc_textregion_t *source, isc_buffer_t *target) {
	isc_textregion_t source0 = *source;
	do {
		RETERR(commatxt_fromtext(&source0, true, target));
	} while (source0.length != 0);
	return (ISC_R_SUCCESS);
}

static int
svckeycmp(const void *a1, const void *a2) {
	const unsigned char *u1 = a1, *u2 = a2;
	if (*u1 != *u2) {
		return (*u1 - *u2);
	}
	return (*(++u1) - *(++u2));
}

static isc_result_t
svcsortkeylist(isc_buffer_t *target, unsigned int used) {
	isc_region_t region;

	isc_buffer_usedregion(target, &region);
	isc_region_consume(&region, used);
	INSIST(region.length > 0U);
	qsort(region.base, region.length / 2, 2, svckeycmp);
	/* Reject duplicates. */
	while (region.length >= 4) {
		if (region.base[0] == region.base[2] &&
		    region.base[1] == region.base[3])
		{
			return (DNS_R_SYNTAX);
		}
		isc_region_consume(&region, 2);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
svcb_validate(uint16_t key, isc_region_t *region) {
	size_t i;

#ifndef ARRAYSIZE
/* defined in winnt.h */
#define ARRAYSIZE(x) (sizeof(x) / sizeof(*x))
#endif

	for (i = 0; i < ARRAYSIZE(sbpr); i++) {
		if (sbpr[i].value == key) {
			switch (sbpr[i].encoding) {
			case sbpr_port:
				if (region->length != 2) {
					return (DNS_R_FORMERR);
				}
				break;
			case sbpr_ipv4s:
				if ((region->length % 4) != 0 ||
				    region->length == 0)
				{
					return (DNS_R_FORMERR);
				}
				break;
			case sbpr_ipv6s:
				if ((region->length % 16) != 0 ||
				    region->length == 0)
				{
					return (DNS_R_FORMERR);
				}
				break;
			case sbpr_alpn: {
				if (region->length == 0) {
					return (DNS_R_FORMERR);
				}
				while (region->length != 0) {
					size_t l = *region->base + 1;
					if (l == 1U || l > region->length) {
						return (DNS_R_FORMERR);
					}
					isc_region_consume(region, l);
				}
				break;
			}
			case sbpr_keylist: {
				if ((region->length % 2) != 0 ||
				    region->length == 0)
				{
					return (DNS_R_FORMERR);
				}
				/* In order? */
				while (region->length >= 4) {
					if (region->base[0] > region->base[2] ||
					    (region->base[0] ==
						     region->base[2] &&
					     region->base[1] >=
						     region->base[3]))
					{
						return (DNS_R_FORMERR);
					}
					isc_region_consume(region, 2);
				}
				break;
			}
			case sbpr_text:
			case sbpr_base64:
				break;
			case sbpr_dohpath:
				/*
				 * Minimum valid dohpath is "/{?dns}" as
				 * it MUST be relative (leading "/") and
				 * MUST contain "{?dns}".
				 */
				if (region->length < 7) {
					return (DNS_R_FORMERR);
				}
				/* MUST be relative */
				if (region->base[0] != '/') {
					return (DNS_R_FORMERR);
				}
				/* MUST be UTF8 */
				if (!isc_utf8_valid(region->base,
						    region->length))
				{
					return (DNS_R_FORMERR);
				}
				/* MUST contain "{?dns}" */
				if (strnstr((char *)region->base, "{?dns}",
					    region->length) == NULL)
				{
					return (DNS_R_FORMERR);
				}
				break;
			case sbpr_empty:
				if (region->length != 0) {
					return (DNS_R_FORMERR);
				}
				break;
			}
		}
	}
	return (ISC_R_SUCCESS);
}

/*
 * Parse keyname from region.
 */
static isc_result_t
svc_keyfromregion(isc_textregion_t *region, char sep, uint16_t *value,
		  isc_buffer_t *target) {
	char *e = NULL;
	size_t i;
	unsigned long ul;

	/* Look for known key names.  */
	for (i = 0; i < ARRAYSIZE(sbpr); i++) {
		size_t len = strlen(sbpr[i].name);
		if (strncasecmp(region->base, sbpr[i].name, len) != 0 ||
		    (region->base[len] != 0 && region->base[len] != sep))
		{
			continue;
		}
		isc_textregion_consume(region, len);
		ul = sbpr[i].value;
		goto finish;
	}
	/* Handle keyXXXXX form. */
	if (strncmp(region->base, "key", 3) != 0) {
		return (DNS_R_SYNTAX);
	}
	isc_textregion_consume(region, 3);
	/* Disallow [+-]XXXXX which is allowed by strtoul. */
	if (region->length == 0 || *region->base == '-' || *region->base == '+')
	{
		return (DNS_R_SYNTAX);
	}
	/* No zero padding. */
	if (region->length > 1 && *region->base == '0' &&
	    region->base[1] != sep)
	{
		return (DNS_R_SYNTAX);
	}
	ul = strtoul(region->base, &e, 10);
	/* Valid number? */
	if (e == region->base || (*e != sep && *e != 0)) {
		return (DNS_R_SYNTAX);
	}
	if (ul > 0xffff) {
		return (ISC_R_RANGE);
	}
	isc_textregion_consume(region, e - region->base);
finish:
	if (sep == ',' && region->length == 1) {
		return (DNS_R_SYNTAX);
	}
	/* Consume separator. */
	if (region->length != 0) {
		isc_textregion_consume(region, 1);
	}
	RETERR(uint16_tobuffer(ul, target));
	if (value != NULL) {
		*value = ul;
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
svc_fromtext(isc_textregion_t *region, isc_buffer_t *target) {
	char *e = NULL;
	char abuf[16];
	char tbuf[sizeof("aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:255.255.255.255,")];
	isc_buffer_t sb;
	isc_region_t keyregion;
	size_t len;
	uint16_t key;
	unsigned int i;
	unsigned int used;
	unsigned long ul;

	for (i = 0; i < ARRAYSIZE(sbpr); i++) {
		len = strlen(sbpr[i].name);
		if (strncmp(region->base, sbpr[i].name, len) != 0 ||
		    (region->base[len] != 0 && region->base[len] != '='))
		{
			continue;
		}

		if (region->base[len] == '=') {
			len++;
		}

		RETERR(uint16_tobuffer(sbpr[i].value, target));
		isc_textregion_consume(region, len);

		sb = *target;
		RETERR(uint16_tobuffer(0, target)); /* length */

		switch (sbpr[i].encoding) {
		case sbpr_text:
		case sbpr_dohpath:
			RETERR(multitxt_fromtext(region, target));
			break;
		case sbpr_alpn:
			RETERR(alpn_fromtxt(region, target));
			break;
		case sbpr_port:
			if (!isdigit((unsigned char)*region->base)) {
				return (DNS_R_SYNTAX);
			}
			ul = strtoul(region->base, &e, 10);
			if (*e != '\0') {
				return (DNS_R_SYNTAX);
			}
			if (ul > 0xffff) {
				return (ISC_R_RANGE);
			}
			RETERR(uint16_tobuffer(ul, target));
			break;
		case sbpr_ipv4s:
			do {
				snprintf(tbuf, sizeof(tbuf), "%*s",
					 (int)(region->length), region->base);
				e = strchr(tbuf, ',');
				if (e != NULL) {
					*e++ = 0;
					isc_textregion_consume(region,
							       e - tbuf);
				}
				if (inet_pton(AF_INET, tbuf, abuf) != 1) {
					return (DNS_R_SYNTAX);
				}
				mem_tobuffer(target, abuf, 4);
			} while (e != NULL);
			break;
		case sbpr_ipv6s:
			do {
				snprintf(tbuf, sizeof(tbuf), "%*s",
					 (int)(region->length), region->base);
				e = strchr(tbuf, ',');
				if (e != NULL) {
					*e++ = 0;
					isc_textregion_consume(region,
							       e - tbuf);
				}
				if (inet_pton(AF_INET6, tbuf, abuf) != 1) {
					return (DNS_R_SYNTAX);
				}
				mem_tobuffer(target, abuf, 16);
			} while (e != NULL);
			break;
		case sbpr_base64:
			RETERR(isc_base64_decodestring(region->base, target));
			break;
		case sbpr_empty:
			if (region->length != 0) {
				return (DNS_R_SYNTAX);
			}
			break;
		case sbpr_keylist:
			if (region->length == 0) {
				return (DNS_R_SYNTAX);
			}
			used = isc_buffer_usedlength(target);
			while (region->length != 0) {
				RETERR(svc_keyfromregion(region, ',', NULL,
							 target));
			}
			RETERR(svcsortkeylist(target, used));
			break;
		default:
			UNREACHABLE();
		}

		len = isc_buffer_usedlength(target) -
		      isc_buffer_usedlength(&sb) - 2;
		RETERR(uint16_tobuffer(len, &sb)); /* length */
		switch (sbpr[i].encoding) {
		case sbpr_dohpath:
			/*
			 * Apply constraints not applied by multitxt_fromtext.
			 */
			keyregion.base = isc_buffer_used(&sb);
			keyregion.length = isc_buffer_usedlength(target) -
					   isc_buffer_usedlength(&sb);
			RETERR(svcb_validate(sbpr[i].value, &keyregion));
			break;
		default:
			break;
		}
		return (ISC_R_SUCCESS);
	}

	RETERR(svc_keyfromregion(region, '=', &key, target));
	if (region->length == 0) {
		RETERR(uint16_tobuffer(0, target)); /* length */
		/* Sanity check keyXXXXX form. */
		keyregion.base = isc_buffer_used(target);
		keyregion.length = 0;
		return (svcb_validate(key, &keyregion));
	}
	sb = *target;
	RETERR(uint16_tobuffer(0, target)); /* dummy length */
	RETERR(multitxt_fromtext(region, target));
	len = isc_buffer_usedlength(target) - isc_buffer_usedlength(&sb) - 2;
	RETERR(uint16_tobuffer(len, &sb)); /* length */
	/* Sanity check keyXXXXX form. */
	keyregion.base = isc_buffer_used(&sb);
	keyregion.length = len;
	return (svcb_validate(key, &keyregion));
}

static const char *
svcparamkey(unsigned short value, enum encoding *encoding, char *buf,
	    size_t len) {
	size_t i;
	int n;

	for (i = 0; i < ARRAYSIZE(sbpr); i++) {
		if (sbpr[i].value == value && sbpr[i].initial) {
			*encoding = sbpr[i].encoding;
			return (sbpr[i].name);
		}
	}
	n = snprintf(buf, len, "key%u", value);
	INSIST(n > 0 && (unsigned)n < len);
	*encoding = sbpr_text;
	return (buf);
}

static isc_result_t
svcsortkeys(isc_buffer_t *target, unsigned int used) {
	isc_region_t r1, r2, man = { .base = NULL, .length = 0 };
	unsigned char buf[1024];
	uint16_t mankey = 0;
	bool have_alpn = false;

	if (isc_buffer_usedlength(target) == used) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * Get the parameters into r1.
	 */
	isc_buffer_usedregion(target, &r1);
	isc_region_consume(&r1, used);

	while (1) {
		uint16_t key1, len1, key2, len2;
		unsigned char *base1, *base2;

		r2 = r1;

		/*
		 * Get the first parameter.
		 */
		base1 = r1.base;
		key1 = uint16_fromregion(&r1);
		isc_region_consume(&r1, 2);
		len1 = uint16_fromregion(&r1);
		isc_region_consume(&r1, 2);
		isc_region_consume(&r1, len1);

		/*
		 * Was there only one key left?
		 */
		if (r1.length == 0) {
			if (mankey != 0) {
				/* Is this the last mandatory key? */
				if (key1 != mankey || man.length != 0) {
					return (DNS_R_INCONSISTENTRR);
				}
			} else if (key1 == SVCB_MAN_KEY) {
				/* Lone mandatory field. */
				return (DNS_R_DISALLOWED);
			} else if (key1 == SVCB_NO_DEFAULT_ALPN_KEY &&
				   !have_alpn)
			{
				/* Missing required ALPN field. */
				return (DNS_R_DISALLOWED);
			}
			return (ISC_R_SUCCESS);
		}

		/*
		 * Find the smallest parameter.
		 */
		while (r1.length != 0) {
			base2 = r1.base;
			key2 = uint16_fromregion(&r1);
			isc_region_consume(&r1, 2);
			len2 = uint16_fromregion(&r1);
			isc_region_consume(&r1, 2);
			isc_region_consume(&r1, len2);
			if (key2 == key1) {
				return (DNS_R_DUPLICATE);
			}
			if (key2 < key1) {
				base1 = base2;
				key1 = key2;
				len1 = len2;
			}
		}

		/*
		 * Do we need to move the smallest parameter to the start?
		 */
		if (base1 != r2.base) {
			size_t offset = 0;
			size_t bytes = len1 + 4;
			size_t length = base1 - r2.base;

			/*
			 * Move the smallest parameter to the start.
			 */
			while (bytes > 0) {
				size_t count;

				if (bytes > sizeof(buf)) {
					count = sizeof(buf);
				} else {
					count = bytes;
				}
				memmove(buf, base1, count);
				memmove(r2.base + offset + count,
					r2.base + offset, length);
				memmove(r2.base + offset, buf, count);
				base1 += count;
				bytes -= count;
				offset += count;
			}
		}

		/*
		 * Check ALPN is present when NO-DEFAULT-ALPN is set.
		 */
		if (key1 == SVCB_ALPN_KEY) {
			have_alpn = true;
		} else if (key1 == SVCB_NO_DEFAULT_ALPN_KEY && !have_alpn) {
			/* Missing required ALPN field. */
			return (DNS_R_DISALLOWED);
		}

		/*
		 * Check key against mandatory key list.
		 */
		if (mankey != 0) {
			if (key1 > mankey) {
				return (DNS_R_INCONSISTENTRR);
			}
			if (key1 == mankey) {
				if (man.length >= 2) {
					mankey = uint16_fromregion(&man);
					isc_region_consume(&man, 2);
				} else {
					mankey = 0;
				}
			}
		}

		/*
		 * Is this the mandatory key?
		 */
		if (key1 == SVCB_MAN_KEY) {
			man = r2;
			man.length = len1 + 4;
			isc_region_consume(&man, 4);
			if (man.length >= 2) {
				mankey = uint16_fromregion(&man);
				isc_region_consume(&man, 2);
				if (mankey == SVCB_MAN_KEY) {
					return (DNS_R_DISALLOWED);
				}
			} else {
				return (DNS_R_SYNTAX);
			}
		}

		/*
		 * Consume the smallest parameter.
		 */
		isc_region_consume(&r2, len1 + 4);
		r1 = r2;
	}
}

static isc_result_t
generic_fromtext_in_svcb(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	bool alias;
	bool ok = true;
	unsigned int used;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * SvcPriority.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      false));
	if (token.value.as_ulong > 0xffffU) {
		RETTOK(ISC_R_RANGE);
	}
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	alias = token.value.as_ulong == 0;

	/*
	 * TargetName.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      false));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	if (origin == NULL) {
		origin = dns_rootname;
	}
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	if (!alias && (options & DNS_RDATA_CHECKNAMES) != 0) {
		ok = dns_name_ishostname(&name, false);
	}
	if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0) {
		RETTOK(DNS_R_BADNAME);
	}
	if (!ok && callbacks != NULL) {
		warn_badname(&name, lexer, callbacks);
	}

	if (alias) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * SvcParams
	 */
	used = isc_buffer_usedlength(target);
	while (1) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_qvpair, true));
		if (token.type == isc_tokentype_eol ||
		    token.type == isc_tokentype_eof)
		{
			isc_lex_ungettoken(lexer, &token);
			return (svcsortkeys(target, used));
		}

		if (token.type != isc_tokentype_string && /* key only */
		    token.type != isc_tokentype_qvpair &&
		    token.type != isc_tokentype_vpair)
		{
			RETTOK(DNS_R_SYNTAX);
		}
		RETTOK(svc_fromtext(&token.value.as_textregion, target));
	}
}

static isc_result_t
fromtext_in_svcb(ARGS_FROMTEXT) {
	REQUIRE(type == dns_rdatatype_svcb);
	REQUIRE(rdclass == dns_rdataclass_in);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	return (generic_fromtext_in_svcb(CALL_FROMTEXT));
}

static isc_result_t
generic_totext_in_svcb(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	bool sub;
	char buf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255")];
	unsigned short num;
	int n;

	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);

	/*
	 * SvcPriority.
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	n = snprintf(buf, sizeof(buf), "%u ", num);
	INSIST(n > 0 && (unsigned)n < sizeof(buf));
	RETERR(str_totext(buf, target));

	/*
	 * TargetName.
	 */
	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));
	sub = name_prefix(&name, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));

	while (region.length > 0) {
		isc_region_t r;
		enum encoding encoding;

		RETERR(str_totext(" ", target));

		INSIST(region.length >= 2);
		num = uint16_fromregion(&region);
		isc_region_consume(&region, 2);
		RETERR(str_totext(svcparamkey(num, &encoding, buf, sizeof(buf)),
				  target));

		INSIST(region.length >= 2);
		num = uint16_fromregion(&region);
		isc_region_consume(&region, 2);

		INSIST(region.length >= num);
		r = region;
		r.length = num;
		isc_region_consume(&region, num);
		if (num == 0) {
			continue;
		}
		if (encoding != sbpr_empty) {
			RETERR(str_totext("=", target));
		}
		switch (encoding) {
		case sbpr_text:
			RETERR(multitxt_totext(&r, target));
			break;
		case sbpr_port:
			num = uint16_fromregion(&r);
			isc_region_consume(&r, 2);
			n = snprintf(buf, sizeof(buf), "%u", num);
			INSIST(n > 0 && (unsigned)n < sizeof(buf));
			RETERR(str_totext(buf, target));
			INSIST(r.length == 0U);
			break;
		case sbpr_ipv4s:
			while (r.length > 0U) {
				INSIST(r.length >= 4U);
				inet_ntop(AF_INET, r.base, buf, sizeof(buf));
				RETERR(str_totext(buf, target));
				isc_region_consume(&r, 4);
				if (r.length != 0U) {
					RETERR(str_totext(",", target));
				}
			}
			break;
		case sbpr_ipv6s:
			while (r.length > 0U) {
				INSIST(r.length >= 16U);
				inet_ntop(AF_INET6, r.base, buf, sizeof(buf));
				RETERR(str_totext(buf, target));
				isc_region_consume(&r, 16);
				if (r.length != 0U) {
					RETERR(str_totext(",", target));
				}
			}
			break;
		case sbpr_base64:
			RETERR(isc_base64_totext(&r, 0, "", target));
			break;
		case sbpr_alpn:
			INSIST(r.length != 0U);
			RETERR(str_totext("\"", target));
			while (r.length != 0) {
				commatxt_totext(&r, false, true, target);
				if (r.length != 0) {
					RETERR(str_totext(",", target));
				}
			}
			RETERR(str_totext("\"", target));
			break;
		case sbpr_empty:
			INSIST(r.length == 0U);
			break;
		case sbpr_keylist:
			while (r.length > 0) {
				num = uint16_fromregion(&r);
				isc_region_consume(&r, 2);
				RETERR(str_totext(svcparamkey(num, &encoding,
							      buf, sizeof(buf)),
						  target));
				if (r.length != 0) {
					RETERR(str_totext(",", target));
				}
			}
			break;
		default:
			UNREACHABLE();
		}
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
totext_in_svcb(ARGS_TOTEXT) {
	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->length != 0);

	return (generic_totext_in_svcb(CALL_TOTEXT));
}

static isc_result_t
generic_fromwire_in_svcb(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t region, man = { .base = NULL, .length = 0 };
	bool alias, first = true, have_alpn = false;
	uint16_t lastkey = 0, mankey = 0;

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);

	/*
	 * SvcPriority.
	 */
	isc_buffer_activeregion(source, &region);
	if (region.length < 2) {
		return (ISC_R_UNEXPECTEDEND);
	}
	RETERR(mem_tobuffer(target, region.base, 2));
	alias = uint16_fromregion(&region) == 0;
	isc_buffer_forward(source, 2);

	/*
	 * TargetName.
	 */
	RETERR(dns_name_fromwire(&name, source, dctx, options, target));

	if (alias) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * SvcParams.
	 */
	isc_buffer_activeregion(source, &region);
	while (region.length > 0U) {
		isc_region_t keyregion;
		uint16_t key, len;

		/*
		 * SvcParamKey
		 */
		if (region.length < 2U) {
			return (ISC_R_UNEXPECTEDEND);
		}
		RETERR(mem_tobuffer(target, region.base, 2));
		key = uint16_fromregion(&region);
		isc_region_consume(&region, 2);

		/*
		 * Keys must be unique and in order.
		 */
		if (!first && key <= lastkey) {
			return (DNS_R_FORMERR);
		}

		/*
		 * Check mandatory keys.
		 */
		if (mankey != 0) {
			/* Missing mandatory key? */
			if (key > mankey) {
				return (DNS_R_FORMERR);
			}
			if (key == mankey) {
				/* Get next mandatory key. */
				if (man.length >= 2) {
					mankey = uint16_fromregion(&man);
					isc_region_consume(&man, 2);
				} else {
					mankey = 0;
				}
			}
		}

		/*
		 * Check alpn present when no-default-alpn is set.
		 */
		if (key == SVCB_ALPN_KEY) {
			have_alpn = true;
		} else if (key == SVCB_NO_DEFAULT_ALPN_KEY && !have_alpn) {
			return (DNS_R_FORMERR);
		}

		first = false;
		lastkey = key;

		/*
		 * SvcParamValue length.
		 */
		if (region.length < 2U) {
			return (ISC_R_UNEXPECTEDEND);
		}
		RETERR(mem_tobuffer(target, region.base, 2));
		len = uint16_fromregion(&region);
		isc_region_consume(&region, 2);

		/*
		 * SvcParamValue.
		 */
		if (region.length < len) {
			return (ISC_R_UNEXPECTEDEND);
		}

		/*
		 * Remember manatory key.
		 */
		if (key == SVCB_MAN_KEY) {
			man = region;
			man.length = len;
			/* Get first mandatory key */
			if (man.length >= 2) {
				mankey = uint16_fromregion(&man);
				isc_region_consume(&man, 2);
				if (mankey == SVCB_MAN_KEY) {
					return (DNS_R_FORMERR);
				}
			} else {
				return (DNS_R_FORMERR);
			}
		}
		keyregion = region;
		keyregion.length = len;
		RETERR(svcb_validate(key, &keyregion));
		RETERR(mem_tobuffer(target, region.base, len));
		isc_region_consume(&region, len);
		isc_buffer_forward(source, len + 4);
	}

	/*
	 * Do we have an outstanding mandatory key?
	 */
	if (mankey != 0) {
		return (DNS_R_FORMERR);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
fromwire_in_svcb(ARGS_FROMWIRE) {
	REQUIRE(type == dns_rdatatype_svcb);
	REQUIRE(rdclass == dns_rdataclass_in);

	return (generic_fromwire_in_svcb(CALL_FROMWIRE));
}

static isc_result_t
generic_towire_in_svcb(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);

	/*
	 * SvcPriority.
	 */
	dns_rdata_toregion(rdata, &region);
	RETERR(mem_tobuffer(target, region.base, 2));
	isc_region_consume(&region, 2);

	/*
	 * TargetName.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &region);
	RETERR(dns_name_towire(&name, cctx, target));
	isc_region_consume(&region, name_length(&name));

	/*
	 * SvcParams.
	 */
	return (mem_tobuffer(target, region.base, region.length));
}

static isc_result_t
towire_in_svcb(ARGS_TOWIRE) {
	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(rdata->length != 0);

	return (generic_towire_in_svcb(CALL_TOWIRE));
}

static int
compare_in_svcb(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_svcb);
	REQUIRE(rdata1->rdclass == dns_rdataclass_in);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	return (isc_region_compare(&region1, &region2));
}

static isc_result_t
generic_fromstruct_in_svcb(ARGS_FROMSTRUCT) {
	dns_rdata_in_svcb_t *svcb = source;
	isc_region_t region;

	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdtype == type);
	REQUIRE(svcb->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(svcb->priority, target));
	dns_name_toregion(&svcb->svcdomain, &region);
	RETERR(isc_buffer_copyregion(target, &region));

	return (mem_tobuffer(target, svcb->svc, svcb->svclen));
}

static isc_result_t
fromstruct_in_svcb(ARGS_FROMSTRUCT) {
	dns_rdata_in_svcb_t *svcb = source;

	REQUIRE(type == dns_rdatatype_svcb);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdtype == type);
	REQUIRE(svcb->common.rdclass == rdclass);

	return (generic_fromstruct_in_svcb(CALL_FROMSTRUCT));
}

static isc_result_t
generic_tostruct_in_svcb(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_in_svcb_t *svcb = target;
	dns_name_t name;

	REQUIRE(svcb != NULL);
	REQUIRE(rdata->length != 0);

	svcb->common.rdclass = rdata->rdclass;
	svcb->common.rdtype = rdata->type;
	ISC_LINK_INIT(&svcb->common, link);

	dns_rdata_toregion(rdata, &region);

	svcb->priority = uint16_fromregion(&region);
	isc_region_consume(&region, 2);

	dns_name_init(&svcb->svcdomain, NULL);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));

	RETERR(name_duporclone(&name, mctx, &svcb->svcdomain));
	svcb->svclen = region.length;
	svcb->svc = mem_maybedup(mctx, region.base, region.length);

	if (svcb->svc == NULL) {
		if (mctx != NULL) {
			dns_name_free(&svcb->svcdomain, svcb->mctx);
		}
		return (ISC_R_NOMEMORY);
	}

	svcb->offset = 0;
	svcb->mctx = mctx;

	return (ISC_R_SUCCESS);
}

static isc_result_t
tostruct_in_svcb(ARGS_TOSTRUCT) {
	dns_rdata_in_svcb_t *svcb = target;

	REQUIRE(rdata->rdclass == dns_rdataclass_in);
	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(svcb != NULL);
	REQUIRE(rdata->length != 0);

	return (generic_tostruct_in_svcb(CALL_TOSTRUCT));
}

static void
generic_freestruct_in_svcb(ARGS_FREESTRUCT) {
	dns_rdata_in_svcb_t *svcb = source;

	REQUIRE(svcb != NULL);

	if (svcb->mctx == NULL) {
		return;
	}

	dns_name_free(&svcb->svcdomain, svcb->mctx);
	isc_mem_free(svcb->mctx, svcb->svc);
	svcb->mctx = NULL;
}

static void
freestruct_in_svcb(ARGS_FREESTRUCT) {
	dns_rdata_in_svcb_t *svcb = source;

	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdclass == dns_rdataclass_in);
	REQUIRE(svcb->common.rdtype == dns_rdatatype_svcb);

	generic_freestruct_in_svcb(CALL_FREESTRUCT);
}

static isc_result_t
generic_additionaldata_in_svcb(ARGS_ADDLDATA) {
	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
additionaldata_in_svcb(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	return (generic_additionaldata_in_svcb(CALL_ADDLDATA));
}

static isc_result_t
digest_in_svcb(ARGS_DIGEST) {
	isc_region_t region1;

	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	dns_rdata_toregion(rdata, &region1);
	return ((digest)(arg, &region1));
}

static bool
checkowner_in_svcb(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_svcb);
	REQUIRE(rdclass == dns_rdataclass_in);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
generic_checknames_in_svcb(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;
	bool alias;

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	INSIST(region.length > 1);
	alias = uint16_fromregion(&region) == 0;
	isc_region_consume(&region, 2);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!alias && !dns_name_ishostname(&name, false)) {
		if (bad != NULL) {
			dns_name_clone(&name, bad);
		}
		return (false);
	}
	return (true);
}

static bool
checknames_in_svcb(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_svcb);
	REQUIRE(rdata->rdclass == dns_rdataclass_in);

	return (generic_checknames_in_svcb(CALL_CHECKNAMES));
}

static int
casecompare_in_svcb(ARGS_COMPARE) {
	return (compare_in_svcb(rdata1, rdata2));
}

static isc_result_t
generic_rdata_in_svcb_first(dns_rdata_in_svcb_t *svcb) {
	if (svcb->svclen == 0) {
		return (ISC_R_NOMORE);
	}
	svcb->offset = 0;
	return (ISC_R_SUCCESS);
}

static isc_result_t
generic_rdata_in_svcb_next(dns_rdata_in_svcb_t *svcb) {
	isc_region_t region;
	size_t len;

	if (svcb->offset >= svcb->svclen) {
		return (ISC_R_NOMORE);
	}

	region.base = svcb->svc + svcb->offset;
	region.length = svcb->svclen - svcb->offset;
	INSIST(region.length >= 4);
	isc_region_consume(&region, 2);
	len = uint16_fromregion(&region);
	INSIST(region.length >= len + 2);
	svcb->offset += len + 4;
	return (svcb->offset >= svcb->svclen ? ISC_R_NOMORE : ISC_R_SUCCESS);
}

static void
generic_rdata_in_svcb_current(dns_rdata_in_svcb_t *svcb, isc_region_t *region) {
	size_t len;

	INSIST(svcb->offset <= svcb->svclen);

	region->base = svcb->svc + svcb->offset;
	region->length = svcb->svclen - svcb->offset;
	INSIST(region->length >= 4);
	isc_region_consume(region, 2);
	len = uint16_fromregion(region);
	INSIST(region->length >= len + 2);
	region->base = svcb->svc + svcb->offset;
	region->length = len + 4;
}

isc_result_t
dns_rdata_in_svcb_first(dns_rdata_in_svcb_t *svcb) {
	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdtype == dns_rdatatype_svcb);
	REQUIRE(svcb->common.rdclass == dns_rdataclass_in);

	return (generic_rdata_in_svcb_first(svcb));
}

isc_result_t
dns_rdata_in_svcb_next(dns_rdata_in_svcb_t *svcb) {
	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdtype == dns_rdatatype_svcb);
	REQUIRE(svcb->common.rdclass == dns_rdataclass_in);

	return (generic_rdata_in_svcb_next(svcb));
}

void
dns_rdata_in_svcb_current(dns_rdata_in_svcb_t *svcb, isc_region_t *region) {
	REQUIRE(svcb != NULL);
	REQUIRE(svcb->common.rdtype == dns_rdatatype_svcb);
	REQUIRE(svcb->common.rdclass == dns_rdataclass_in);
	REQUIRE(region != NULL);

	generic_rdata_in_svcb_current(svcb, region);
}

#endif /* RDATA_IN_1_SVCB_64_C */
