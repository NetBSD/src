/*	$NetBSD: loc_29.c,v 1.9 2023/01/25 21:43:30 christos Exp $	*/

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

/* RFC1876 */

#ifndef RDATA_GENERIC_LOC_29_C
#define RDATA_GENERIC_LOC_29_C

#define RRTYPE_LOC_ATTRIBUTES (0)

static isc_result_t
loc_getdecimal(const char *str, unsigned long max, size_t precision, char units,
	       unsigned long *valuep) {
	bool ok;
	char *e;
	size_t i;
	long tmp;
	unsigned long value;

	value = strtoul(str, &e, 10);
	if (*e != 0 && *e != '.' && *e != units) {
		return (DNS_R_SYNTAX);
	}
	if (value > max) {
		return (ISC_R_RANGE);
	}
	ok = e != str;
	if (*e == '.') {
		e++;
		for (i = 0; i < precision; i++) {
			if (*e == 0 || *e == units) {
				break;
			}
			if ((tmp = decvalue(*e++)) < 0) {
				return (DNS_R_SYNTAX);
			}
			ok = true;
			value *= 10;
			value += tmp;
		}
		for (; i < precision; i++) {
			value *= 10;
		}
	} else {
		for (i = 0; i < precision; i++) {
			value *= 10;
		}
	}
	if (*e != 0 && *e == units) {
		e++;
	}
	if (!ok || *e != 0) {
		return (DNS_R_SYNTAX);
	}
	*valuep = value;
	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getprecision(const char *str, unsigned char *valuep) {
	unsigned long poweroften[8] = { 1,     10,     100,	1000,
					10000, 100000, 1000000, 10000000 };
	unsigned long m, cm;
	bool ok;
	char *e;
	size_t i;
	long tmp;
	int man;
	int exp;

	m = strtoul(str, &e, 10);
	if (*e != 0 && *e != '.' && *e != 'm') {
		return (DNS_R_SYNTAX);
	}
	if (m > 90000000) {
		return (ISC_R_RANGE);
	}
	cm = 0;
	ok = e != str;
	if (*e == '.') {
		e++;
		for (i = 0; i < 2; i++) {
			if (*e == 0 || *e == 'm') {
				break;
			}
			if ((tmp = decvalue(*e++)) < 0) {
				return (DNS_R_SYNTAX);
			}
			ok = true;
			cm *= 10;
			cm += tmp;
		}
		for (; i < 2; i++) {
			cm *= 10;
		}
	}
	if (*e == 'm') {
		e++;
	}
	if (!ok || *e != 0) {
		return (DNS_R_SYNTAX);
	}

	/*
	 * We don't just multiply out as we will overflow.
	 */
	if (m > 0) {
		for (exp = 0; exp < 7; exp++) {
			if (m < poweroften[exp + 1]) {
				break;
			}
		}
		man = m / poweroften[exp];
		exp += 2;
	} else if (cm >= 10) {
		man = cm / 10;
		exp = 1;
	} else {
		man = cm;
		exp = 0;
	}
	*valuep = (man << 4) + exp;
	return (ISC_R_SUCCESS);
}

static isc_result_t
get_degrees(isc_lex_t *lexer, isc_token_t *token, unsigned long *d) {
	RETERR(isc_lex_getmastertoken(lexer, token, isc_tokentype_number,
				      false));
	*d = token->value.as_ulong;

	return (ISC_R_SUCCESS);
}

static isc_result_t
check_coordinate(unsigned long d, unsigned long m, unsigned long s,
		 unsigned long maxd) {
	if (d > maxd || m > 59U) {
		return (ISC_R_RANGE);
	}
	if (d == maxd && (m != 0 || s != 0)) {
		return (ISC_R_RANGE);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
get_minutes(isc_lex_t *lexer, isc_token_t *token, unsigned long *m) {
	RETERR(isc_lex_getmastertoken(lexer, token, isc_tokentype_number,
				      false));

	*m = token->value.as_ulong;

	return (ISC_R_SUCCESS);
}

static isc_result_t
get_seconds(isc_lex_t *lexer, isc_token_t *token, unsigned long *s) {
	RETERR(isc_lex_getmastertoken(lexer, token, isc_tokentype_string,
				      false));
	RETERR(loc_getdecimal(DNS_AS_STR(*token), 59, 3, '\0', s));

	return (ISC_R_SUCCESS);
}

static isc_result_t
get_direction(isc_lex_t *lexer, isc_token_t *token, const char *directions,
	      int *direction) {
	RETERR(isc_lex_getmastertoken(lexer, token, isc_tokentype_string,
				      false));
	if (DNS_AS_STR(*token)[0] == directions[1] &&
	    DNS_AS_STR(*token)[1] == 0)
	{
		*direction = DNS_AS_STR(*token)[0];
		return (ISC_R_SUCCESS);
	}

	if (DNS_AS_STR(*token)[0] == directions[0] &&
	    DNS_AS_STR(*token)[1] == 0)
	{
		*direction = DNS_AS_STR(*token)[0];
		return (ISC_R_SUCCESS);
	}

	*direction = 0;
	isc_lex_ungettoken(lexer, token);
	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getcoordinate(isc_lex_t *lexer, unsigned long *dp, unsigned long *mp,
		  unsigned long *sp, const char *directions, int *directionp,
		  unsigned long maxd) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_token_t token;
	unsigned long d, m, s;
	int direction = 0;

	m = 0;
	s = 0;

	/*
	 * Degrees.
	 */
	RETERR(get_degrees(lexer, &token, &d));
	RETTOK(check_coordinate(d, m, s, maxd));

	/*
	 * Minutes.
	 */
	RETERR(get_direction(lexer, &token, directions, &direction));
	if (direction > 0) {
		goto done;
	}

	RETERR(get_minutes(lexer, &token, &m));
	RETTOK(check_coordinate(d, m, s, maxd));

	/*
	 * Seconds.
	 */
	RETERR(get_direction(lexer, &token, directions, &direction));
	if (direction > 0) {
		goto done;
	}

	result = get_seconds(lexer, &token, &s);
	if (result == ISC_R_RANGE || result == DNS_R_SYNTAX) {
		RETTOK(result);
	}
	RETERR(result);
	RETTOK(check_coordinate(d, m, s, maxd));

	/*
	 * Direction.
	 */
	RETERR(get_direction(lexer, &token, directions, &direction));
	if (direction == 0) {
		RETERR(DNS_R_SYNTAX);
	}
done:

	*directionp = direction;
	*dp = d;
	*mp = m;
	*sp = s;

	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getlatitude(isc_lex_t *lexer, unsigned long *latitude) {
	unsigned long d1 = 0, m1 = 0, s1 = 0;
	int direction = 0;

	RETERR(loc_getcoordinate(lexer, &d1, &m1, &s1, "SN", &direction, 90U));

	switch (direction) {
	case 'N':
		*latitude = 0x80000000 + (d1 * 3600 + m1 * 60) * 1000 + s1;
		break;
	case 'S':
		*latitude = 0x80000000 - (d1 * 3600 + m1 * 60) * 1000 - s1;
		break;
	default:
		UNREACHABLE();
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getlongitude(isc_lex_t *lexer, unsigned long *longitude) {
	unsigned long d2 = 0, m2 = 0, s2 = 0;
	int direction = 0;

	RETERR(loc_getcoordinate(lexer, &d2, &m2, &s2, "WE", &direction, 180U));

	switch (direction) {
	case 'E':
		*longitude = 0x80000000 + (d2 * 3600 + m2 * 60) * 1000 + s2;
		break;
	case 'W':
		*longitude = 0x80000000 - (d2 * 3600 + m2 * 60) * 1000 - s2;
		break;
	default:
		UNREACHABLE();
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getaltitude(isc_lex_t *lexer, unsigned long *altitude) {
	isc_token_t token;
	unsigned long cm;
	const char *str;

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      false));
	str = DNS_AS_STR(token);
	if (DNS_AS_STR(token)[0] == '-') {
		RETTOK(loc_getdecimal(str + 1, 100000, 2, 'm', &cm));
		if (cm > 10000000UL) {
			RETTOK(ISC_R_RANGE);
		}
		*altitude = 10000000 - cm;
	} else {
		RETTOK(loc_getdecimal(str, 42849672, 2, 'm', &cm));
		if (cm > 4284967295UL) {
			RETTOK(ISC_R_RANGE);
		}
		*altitude = 10000000 + cm;
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getoptionalprecision(isc_lex_t *lexer, unsigned char *valuep) {
	isc_token_t token;

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      true));
	if (token.type == isc_tokentype_eol || token.type == isc_tokentype_eof)
	{
		isc_lex_ungettoken(lexer, &token);
		return (ISC_R_NOMORE);
	}
	RETTOK(loc_getprecision(DNS_AS_STR(token), valuep));

	return (ISC_R_SUCCESS);
}

static isc_result_t
loc_getsize(isc_lex_t *lexer, unsigned char *sizep) {
	return (loc_getoptionalprecision(lexer, sizep));
}

static isc_result_t
loc_gethorizontalprecision(isc_lex_t *lexer, unsigned char *hpp) {
	return (loc_getoptionalprecision(lexer, hpp));
}

static isc_result_t
loc_getverticalprecision(isc_lex_t *lexer, unsigned char *vpp) {
	return (loc_getoptionalprecision(lexer, vpp));
}

/* The LOC record is expressed in a master file in the following format:
 *
 * <owner> <TTL> <class> LOC ( d1 [m1 [s1]] {"N"|"S"} d2 [m2 [s2]]
 *                             {"E"|"W"} alt["m"] [siz["m"] [hp["m"]
 *                             [vp["m"]]]] )
 *
 * (The parentheses are used for multi-line data as specified in [RFC
 * 1035] section 5.1.)
 *
 * where:
 *
 *     d1:     [0 .. 90]            (degrees latitude)
 *     d2:     [0 .. 180]           (degrees longitude)
 *     m1, m2: [0 .. 59]            (minutes latitude/longitude)
 *     s1, s2: [0 .. 59.999]        (seconds latitude/longitude)
 *     alt:    [-100000.00 .. 42849672.95] BY .01 (altitude in meters)
 *     siz, hp, vp: [0 .. 90000000.00] (size/precision in meters)
 *
 * If omitted, minutes and seconds default to zero, size defaults to 1m,
 * horizontal precision defaults to 10000m, and vertical precision
 * defaults to 10m.  These defaults are chosen to represent typical
 * ZIP/postal code area sizes, since it is often easy to find
 * approximate geographical location by ZIP/postal code.
 */
static isc_result_t
fromtext_loc(ARGS_FROMTEXT) {
	isc_result_t result = ISC_R_SUCCESS;
	unsigned long latitude = 0;
	unsigned long longitude = 0;
	unsigned long altitude = 0;
	unsigned char size = 0x12; /* Default: 1.00m */
	unsigned char hp = 0x16;   /* Default: 10000.00 m */
	unsigned char vp = 0x13;   /* Default: 10.00 m */
	unsigned char version = 0;

	REQUIRE(type == dns_rdatatype_loc);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	RETERR(loc_getlatitude(lexer, &latitude));
	RETERR(loc_getlongitude(lexer, &longitude));
	RETERR(loc_getaltitude(lexer, &altitude));
	result = loc_getsize(lexer, &size);
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
		goto encode;
	}
	RETERR(result);
	result = loc_gethorizontalprecision(lexer, &hp);
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
		goto encode;
	}
	RETERR(result);
	result = loc_getverticalprecision(lexer, &vp);
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
		goto encode;
	}
	RETERR(result);
encode:
	RETERR(mem_tobuffer(target, &version, 1));
	RETERR(mem_tobuffer(target, &size, 1));
	RETERR(mem_tobuffer(target, &hp, 1));
	RETERR(mem_tobuffer(target, &vp, 1));

	RETERR(uint32_tobuffer(latitude, target));
	RETERR(uint32_tobuffer(longitude, target));
	RETERR(uint32_tobuffer(altitude, target));

	return (result);
}

static isc_result_t
totext_loc(ARGS_TOTEXT) {
	int d1, m1, s1, fs1;
	int d2, m2, s2, fs2;
	unsigned long latitude;
	unsigned long longitude;
	unsigned long altitude;
	bool north;
	bool east;
	bool below;
	isc_region_t sr;
	char sbuf[sizeof("90000000m")];
	char hbuf[sizeof("90000000m")];
	char vbuf[sizeof("90000000m")];
	/* "89 59 59.999 N 179 59 59.999 E " */
	/* "-42849672.95m 90000000m 90000000m 90000000m"; */
	char buf[8 * 6 + 12 * 1 + 2 * 10 + sizeof(sbuf) + sizeof(hbuf) +
		 sizeof(vbuf)];
	unsigned char size, hp, vp;
	unsigned long poweroften[8] = { 1,     10,     100,	1000,
					10000, 100000, 1000000, 10000000 };

	UNUSED(tctx);

	REQUIRE(rdata->type == dns_rdatatype_loc);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	if (sr.base[0] != 0) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	REQUIRE(rdata->length == 16);

	size = sr.base[1];
	INSIST((size & 0x0f) < 10 && (size >> 4) < 10);
	if ((size & 0x0f) > 1) {
		snprintf(sbuf, sizeof(sbuf), "%lum",
			 (size >> 4) * poweroften[(size & 0x0f) - 2]);
	} else {
		snprintf(sbuf, sizeof(sbuf), "0.%02lum",
			 (size >> 4) * poweroften[(size & 0x0f)]);
	}
	hp = sr.base[2];
	INSIST((hp & 0x0f) < 10 && (hp >> 4) < 10);
	if ((hp & 0x0f) > 1) {
		snprintf(hbuf, sizeof(hbuf), "%lum",
			 (hp >> 4) * poweroften[(hp & 0x0f) - 2]);
	} else {
		snprintf(hbuf, sizeof(hbuf), "0.%02lum",
			 (hp >> 4) * poweroften[(hp & 0x0f)]);
	}
	vp = sr.base[3];
	INSIST((vp & 0x0f) < 10 && (vp >> 4) < 10);
	if ((vp & 0x0f) > 1) {
		snprintf(vbuf, sizeof(vbuf), "%lum",
			 (vp >> 4) * poweroften[(vp & 0x0f) - 2]);
	} else {
		snprintf(vbuf, sizeof(vbuf), "0.%02lum",
			 (vp >> 4) * poweroften[(vp & 0x0f)]);
	}
	isc_region_consume(&sr, 4);

	latitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (latitude >= 0x80000000) {
		north = true;
		latitude -= 0x80000000;
	} else {
		north = false;
		latitude = 0x80000000 - latitude;
	}
	fs1 = (int)(latitude % 1000);
	latitude /= 1000;
	s1 = (int)(latitude % 60);
	latitude /= 60;
	m1 = (int)(latitude % 60);
	latitude /= 60;
	d1 = (int)latitude;
	INSIST(latitude <= 90U);

	longitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (longitude >= 0x80000000) {
		east = true;
		longitude -= 0x80000000;
	} else {
		east = false;
		longitude = 0x80000000 - longitude;
	}
	fs2 = (int)(longitude % 1000);
	longitude /= 1000;
	s2 = (int)(longitude % 60);
	longitude /= 60;
	m2 = (int)(longitude % 60);
	longitude /= 60;
	d2 = (int)longitude;
	INSIST(longitude <= 180U);

	altitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (altitude < 10000000U) {
		below = true;
		altitude = 10000000 - altitude;
	} else {
		below = false;
		altitude -= 10000000;
	}

	snprintf(buf, sizeof(buf),
		 "%d %d %d.%03d %s %d %d %d.%03d %s %s%lu.%02lum %s %s %s", d1,
		 m1, s1, fs1, north ? "N" : "S", d2, m2, s2, fs2,
		 east ? "E" : "W", below ? "-" : "", altitude / 100,
		 altitude % 100, sbuf, hbuf, vbuf);

	return (str_totext(buf, target));
}

static isc_result_t
fromwire_loc(ARGS_FROMWIRE) {
	isc_region_t sr;
	unsigned char c;
	unsigned long latitude;
	unsigned long longitude;

	REQUIRE(type == dns_rdatatype_loc);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 1) {
		return (ISC_R_UNEXPECTEDEND);
	}
	if (sr.base[0] != 0) {
		/* Treat as unknown. */
		isc_buffer_forward(source, sr.length);
		return (mem_tobuffer(target, sr.base, sr.length));
	}
	if (sr.length < 16) {
		return (ISC_R_UNEXPECTEDEND);
	}

	/*
	 * Size.
	 */
	c = sr.base[1];
	if (c != 0) {
		if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 ||
		    ((c >> 4) & 0xf) == 0)
		{
			return (ISC_R_RANGE);

			/*
			 * Horizontal precision.
			 */
		}
	}

	/*
	 * Horizontal precision.
	 */
	c = sr.base[2];
	if (c != 0) {
		if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 ||
		    ((c >> 4) & 0xf) == 0)
		{
			return (ISC_R_RANGE);

			/*
			 * Vertical precision.
			 */
		}
	}

	/*
	 * Vertical precision.
	 */
	c = sr.base[3];
	if (c != 0) {
		if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 ||
		    ((c >> 4) & 0xf) == 0)
		{
			return (ISC_R_RANGE);
		}
	}
	isc_region_consume(&sr, 4);

	/*
	 * Latitude.
	 */
	latitude = uint32_fromregion(&sr);
	if (latitude < (0x80000000UL - 90 * 3600000) ||
	    latitude > (0x80000000UL + 90 * 3600000))
	{
		return (ISC_R_RANGE);
	}
	isc_region_consume(&sr, 4);

	/*
	 * Longitude.
	 */
	longitude = uint32_fromregion(&sr);
	if (longitude < (0x80000000UL - 180 * 3600000) ||
	    longitude > (0x80000000UL + 180 * 3600000))
	{
		return (ISC_R_RANGE);
	}

	/*
	 * Altitude.
	 * All values possible.
	 */

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, 16);
	return (mem_tobuffer(target, sr.base, 16));
}

static isc_result_t
towire_loc(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == dns_rdatatype_loc);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static int
compare_loc(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == dns_rdatatype_loc);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static isc_result_t
fromstruct_loc(ARGS_FROMSTRUCT) {
	dns_rdata_loc_t *loc = source;
	uint8_t c;

	REQUIRE(type == dns_rdatatype_loc);
	REQUIRE(loc != NULL);
	REQUIRE(loc->common.rdtype == type);
	REQUIRE(loc->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	if (loc->v.v0.version != 0) {
		return (ISC_R_NOTIMPLEMENTED);
	}
	RETERR(uint8_tobuffer(loc->v.v0.version, target));

	c = loc->v.v0.size;
	if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 || ((c >> 4) & 0xf) == 0) {
		return (ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(loc->v.v0.size, target));

	c = loc->v.v0.horizontal;
	if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 || ((c >> 4) & 0xf) == 0) {
		return (ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(loc->v.v0.horizontal, target));

	c = loc->v.v0.vertical;
	if ((c & 0xf) > 9 || ((c >> 4) & 0xf) > 9 || ((c >> 4) & 0xf) == 0) {
		return (ISC_R_RANGE);
	}
	RETERR(uint8_tobuffer(loc->v.v0.vertical, target));

	if (loc->v.v0.latitude < (0x80000000UL - 90 * 3600000) ||
	    loc->v.v0.latitude > (0x80000000UL + 90 * 3600000))
	{
		return (ISC_R_RANGE);
	}
	RETERR(uint32_tobuffer(loc->v.v0.latitude, target));

	if (loc->v.v0.longitude < (0x80000000UL - 180 * 3600000) ||
	    loc->v.v0.longitude > (0x80000000UL + 180 * 3600000))
	{
		return (ISC_R_RANGE);
	}
	RETERR(uint32_tobuffer(loc->v.v0.longitude, target));
	return (uint32_tobuffer(loc->v.v0.altitude, target));
}

static isc_result_t
tostruct_loc(ARGS_TOSTRUCT) {
	dns_rdata_loc_t *loc = target;
	isc_region_t r;
	uint8_t version;

	REQUIRE(rdata->type == dns_rdatatype_loc);
	REQUIRE(loc != NULL);
	REQUIRE(rdata->length != 0);

	UNUSED(mctx);

	dns_rdata_toregion(rdata, &r);
	version = uint8_fromregion(&r);
	if (version != 0) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	loc->common.rdclass = rdata->rdclass;
	loc->common.rdtype = rdata->type;
	ISC_LINK_INIT(&loc->common, link);

	loc->v.v0.version = version;
	isc_region_consume(&r, 1);
	loc->v.v0.size = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.horizontal = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.vertical = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.latitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	loc->v.v0.longitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	loc->v.v0.altitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	return (ISC_R_SUCCESS);
}

static void
freestruct_loc(ARGS_FREESTRUCT) {
	dns_rdata_loc_t *loc = source;

	REQUIRE(loc != NULL);
	REQUIRE(loc->common.rdtype == dns_rdatatype_loc);

	UNUSED(source);
	UNUSED(loc);
}

static isc_result_t
additionaldata_loc(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == dns_rdatatype_loc);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static isc_result_t
digest_loc(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == dns_rdatatype_loc);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static bool
checkowner_loc(ARGS_CHECKOWNER) {
	REQUIRE(type == dns_rdatatype_loc);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (true);
}

static bool
checknames_loc(ARGS_CHECKNAMES) {
	REQUIRE(rdata->type == dns_rdatatype_loc);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (true);
}

static int
casecompare_loc(ARGS_COMPARE) {
	return (compare_loc(rdata1, rdata2));
}

#endif /* RDATA_GENERIC_LOC_29_C */
