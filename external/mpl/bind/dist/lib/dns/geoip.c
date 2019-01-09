/*	$NetBSD: geoip.c,v 1.1.1.2 2019/01/09 16:48:20 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>

#include <isc/util.h>

#include <isc/mem.h>
#include <isc/once.h>
#include <isc/string.h>

#include <dns/acl.h>
#include <dns/geoip.h>

#include <isc/thread.h>
#include <math.h>
#ifndef WIN32
#include <netinet/in.h>
#else
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <winsock2.h>
#endif	/* WIN32 */
#include <dns/log.h>

#ifdef HAVE_GEOIP
#include <GeoIP.h>
#include <GeoIPCity.h>

/*
 * This structure preserves state from the previous GeoIP lookup,
 * so that successive lookups for the same data from the same IP
 * address will not require repeated calls into the GeoIP library
 * to look up data in the database. This should improve performance
 * somewhat.
 *
 * For lookups in the City and Region databases, we preserve pointers
 * to the GeoIPRecord and GeoIPregion structures; these will need to be
 * freed by GeoIPRecord_delete() and GeoIPRegion_delete().
 *
 * for lookups in ISP, AS, Org and Domain we prserve a pointer to
 * the returned name; these must be freed by free().
 *
 * For lookups in Country we preserve a pointer to the text of
 * the country code, name, etc (we use a different pointer for this
 * than for the names returned by Org, ISP, etc, because those need
 * to be freed but country lookups do not).
 *
 * For lookups in Netspeed we preserve the returned ID.
 *
 * XXX: Currently this mechanism is only used for IPv4 lookups; the
 * family and addr6 fields are to be used IPv6 is added.
 */
typedef struct geoip_state {
	uint16_t subtype;
	unsigned int family;
	uint32_t ipnum;
	geoipv6_t ipnum6;
	GeoIPRecord *record;
	GeoIPRegion *region;
	const char *text;
	char *name;
	int id;
	isc_mem_t *mctx;
} geoip_state_t;

static isc_mutex_t key_mutex;
static bool state_key_initialized = false;
static isc_thread_key_t state_key;
static isc_once_t mutex_once = ISC_ONCE_INIT;
static isc_mem_t *state_mctx = NULL;

static void
key_mutex_init(void) {
	isc_mutex_init(&key_mutex);
}

static void
free_state(void *arg) {
	geoip_state_t *state = arg;
	if (state != NULL && state->record != NULL)
		GeoIPRecord_delete(state->record);
	if (state != NULL)
		isc_mem_putanddetach(&state->mctx,
				     state, sizeof(geoip_state_t));
	isc_thread_key_setspecific(state_key, NULL);
}

static isc_result_t
state_key_init(void) {
	isc_result_t result;

	result = isc_once_do(&mutex_once, key_mutex_init);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (!state_key_initialized) {
		LOCK(&key_mutex);
		if (!state_key_initialized) {
			int ret;

			if (state_mctx == NULL)
				result = isc_mem_create(0, 0, &state_mctx);
			if (result != ISC_R_SUCCESS)
				goto unlock;
			isc_mem_setname(state_mctx, "geoip_state", NULL);
			isc_mem_setdestroycheck(state_mctx, false);

			ret = isc_thread_key_create(&state_key, free_state);
			if (ret == 0)
				state_key_initialized = true;
			else
				result = ISC_R_FAILURE;
		}
 unlock:
		UNLOCK(&key_mutex);
	}

	return (result);
}

static void
clean_state(geoip_state_t *state) {
	if (state == NULL)
		return;

	if (state->record != NULL) {
		GeoIPRecord_delete(state->record);
		state->record = NULL;
	}
	if (state->region != NULL) {
		GeoIPRegion_delete(state->region);
		state->region = NULL;
	}
	if (state->name != NULL) {
		free (state->name);
		state->name = NULL;
	}
	state->ipnum = 0;
	state->text = NULL;
	state->id = 0;
}

static isc_result_t
set_state(unsigned int family, uint32_t ipnum, const geoipv6_t *ipnum6,
	  dns_geoip_subtype_t subtype, GeoIPRecord *record,
	  GeoIPRegion *region, char *name, const char *text, int id)
{
	geoip_state_t *state = NULL;
	isc_result_t result;

	result = state_key_init();
	if (result != ISC_R_SUCCESS)
		return (result);

	state = (geoip_state_t *) isc_thread_key_getspecific(state_key);
	if (state == NULL) {
		state = (geoip_state_t *) isc_mem_get(state_mctx,
						      sizeof(geoip_state_t));
		if (state == NULL)
			return (ISC_R_NOMEMORY);
		memset(state, 0, sizeof(*state));

		result = isc_thread_key_setspecific(state_key, state);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(state_mctx, state, sizeof(geoip_state_t));
			return (result);
		}

		isc_mem_attach(state_mctx, &state->mctx);
	} else
		clean_state(state);

	if (family == AF_INET) {
		state->ipnum = ipnum;
	} else {
		INSIST(ipnum6 != NULL);
		state->ipnum6 = *ipnum6;
	}

	state->family = family;
	state->subtype = subtype;
	state->record = record;
	state->region = region;
	state->name = name;
	state->text = text;
	state->id = id;

	return (ISC_R_SUCCESS);
}

static geoip_state_t *
get_state_for(unsigned int family, uint32_t ipnum,
	      const geoipv6_t *ipnum6)
{
	geoip_state_t *state;

	isc_result_t result;

	result = state_key_init();
	if (result != ISC_R_SUCCESS)
		return (NULL);

	state = (geoip_state_t *) isc_thread_key_getspecific(state_key);
	if (state == NULL)
		return (NULL);

	if (state->family == family &&
	    ((state->family == AF_INET && state->ipnum == ipnum) ||
	     (state->family == AF_INET6 && ipnum6 != NULL &&
	      memcmp(state->ipnum6.s6_addr, ipnum6->s6_addr, 16) == 0)))
		return (state);

	return (NULL);
}

/*
 * Country lookups are performed if the previous lookup was from a
 * different IP address than the current, or was for a search of a
 * different subtype.
 */
static const char *
country_lookup(GeoIP *db, dns_geoip_subtype_t subtype,
	       unsigned int family,
	       uint32_t ipnum, const geoipv6_t *ipnum6)
{
	geoip_state_t *prev_state = NULL;
	const char *text = NULL;

	REQUIRE(db != NULL);

#ifndef HAVE_GEOIP_V6
	/* no IPv6 support? give up now */
	if (family == AF_INET6)
		return (NULL);
#endif

	prev_state = get_state_for(family, ipnum, ipnum6);
	if (prev_state != NULL && prev_state->subtype == subtype) {
		text = prev_state->text;
	}

	if (text == NULL) {
		switch (subtype) {
		case dns_geoip_country_code:
			if (family == AF_INET)
				text = GeoIP_country_code_by_ipnum(db, ipnum);
#ifdef HAVE_GEOIP_V6
			else
				text = GeoIP_country_code_by_ipnum_v6(db,
								      *ipnum6);
#endif
			break;
		case dns_geoip_country_code3:
			if (family == AF_INET)
				text = GeoIP_country_code3_by_ipnum(db, ipnum);
#ifdef HAVE_GEOIP_V6
			else
				text = GeoIP_country_code3_by_ipnum_v6(db,
								       *ipnum6);
#endif
			break;
		case dns_geoip_country_name:
			if (family == AF_INET)
				text = GeoIP_country_name_by_ipnum(db, ipnum);
#ifdef HAVE_GEOIP_V6
			else
				text = GeoIP_country_name_by_ipnum_v6(db,
								      *ipnum6);
#endif
			break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}

		if (text == NULL)
			return (NULL);

		set_state(family, ipnum, ipnum6, subtype,
			  NULL, NULL, NULL, text, 0);
	}

	return (text);
}

static char *
city_string(GeoIPRecord *record, dns_geoip_subtype_t subtype, int *maxlen) {
	const char *s;
	char *deconst;

	REQUIRE(record != NULL);
	REQUIRE(maxlen != NULL);

	/* Set '*maxlen' to the maximum length of this subtype, if any */
	switch (subtype) {
	case dns_geoip_city_countrycode:
	case dns_geoip_city_region:
	case dns_geoip_city_continentcode:
		*maxlen = 2;
		break;

	case dns_geoip_city_countrycode3:
		*maxlen = 3;
		break;

	default:
		/* No fixed length; just use strcasecmp() for comparison */
		*maxlen = 255;
	}

	switch (subtype) {
	case dns_geoip_city_countrycode:
		return (record->country_code);
	case dns_geoip_city_countrycode3:
		return (record->country_code3);
	case dns_geoip_city_countryname:
		return (record->country_name);
	case dns_geoip_city_region:
		return (record->region);
	case dns_geoip_city_regionname:
		s = GeoIP_region_name_by_code(record->country_code,
					      record->region);
		DE_CONST(s, deconst);
		return (deconst);
	case dns_geoip_city_name:
		return (record->city);
	case dns_geoip_city_postalcode:
		return (record->postal_code);
	case dns_geoip_city_continentcode:
		return (record->continent_code);
	case dns_geoip_city_timezonecode:
		s = GeoIP_time_zone_by_country_and_region(record->country_code,
							  record->region);
		DE_CONST(s, deconst);
		return (deconst);
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static bool
is_city(dns_geoip_subtype_t subtype) {
	switch (subtype) {
	case dns_geoip_city_countrycode:
	case dns_geoip_city_countrycode3:
	case dns_geoip_city_countryname:
	case dns_geoip_city_region:
	case dns_geoip_city_regionname:
	case dns_geoip_city_name:
	case dns_geoip_city_postalcode:
	case dns_geoip_city_continentcode:
	case dns_geoip_city_timezonecode:
	case dns_geoip_city_metrocode:
	case dns_geoip_city_areacode:
		return (true);
	default:
		return (false);
	}
}

/*
 * GeoIPRecord lookups are performed if the previous lookup was
 * from a different IP address than the current, or was for a search
 * outside the City database.
 */
static GeoIPRecord *
city_lookup(GeoIP *db, dns_geoip_subtype_t subtype,
	    unsigned int family, uint32_t ipnum,
	    const geoipv6_t *ipnum6)
{
	GeoIPRecord *record = NULL;
	geoip_state_t *prev_state = NULL;

	REQUIRE(db != NULL);

#ifndef HAVE_GEOIP_V6
	/* no IPv6 support? give up now */
	if (family == AF_INET6)
		return (NULL);
#endif

	prev_state = get_state_for(family, ipnum, ipnum6);
	if (prev_state != NULL && is_city(prev_state->subtype)) {
		record = prev_state->record;
	}

	if (record == NULL) {
		if (family == AF_INET)
			record = GeoIP_record_by_ipnum(db, ipnum);
#ifdef HAVE_GEOIP_V6
		else
			record = GeoIP_record_by_ipnum_v6(db, *ipnum6);
#endif
		if (record == NULL)
			return (NULL);

		set_state(family, ipnum, ipnum6, subtype, record,
			  NULL, NULL, NULL, 0);
	}

	return (record);
}

static char * region_string(GeoIPRegion *region, dns_geoip_subtype_t subtype, int *maxlen) {
	const char *s;
	char *deconst;

	REQUIRE(region != NULL);
	REQUIRE(maxlen != NULL);

	switch (subtype) {
	case dns_geoip_region_countrycode:
		*maxlen = 2;
		return (region->country_code);
	case dns_geoip_region_code:
		*maxlen = 2;
		return (region->region);
	case dns_geoip_region_name:
		*maxlen = 255;
		s = GeoIP_region_name_by_code(region->country_code,
					      region->region);
		DE_CONST(s, deconst);
		return (deconst);
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static bool
is_region(dns_geoip_subtype_t subtype) {
	switch (subtype) {
	case dns_geoip_region_countrycode:
	case dns_geoip_region_code:
		return (true);
	default:
		return (false);
	}
}

/*
 * GeoIPRegion lookups are performed if the previous lookup was
 * from a different IP address than the current, or was for a search
 * outside the Region database.
 */
static GeoIPRegion *
region_lookup(GeoIP *db, dns_geoip_subtype_t subtype, uint32_t ipnum) {
	GeoIPRegion *region = NULL;
	geoip_state_t *prev_state = NULL;

	REQUIRE(db != NULL);

	prev_state = get_state_for(AF_INET, ipnum, NULL);
	if (prev_state != NULL && is_region(prev_state->subtype)) {
		region = prev_state->region;
	}

	if (region == NULL) {
		region = GeoIP_region_by_ipnum(db, ipnum);
		if (region == NULL)
			return (NULL);

		set_state(AF_INET, ipnum, NULL, subtype, NULL,
			  region, NULL, NULL, 0);
	}

	return (region);
}

/*
 * ISP, Organization, AS Number and Domain lookups are performed if
 * the previous lookup was from a different IP address than the current,
 * or was for a search of a different subtype.
 */
static char *
name_lookup(GeoIP *db, dns_geoip_subtype_t subtype, uint32_t ipnum) {
	char *name = NULL;
	geoip_state_t *prev_state = NULL;

	REQUIRE(db != NULL);

	prev_state = get_state_for(AF_INET, ipnum, NULL);
	if (prev_state != NULL && prev_state->subtype == subtype) {
		name = prev_state->name;
	}

	if (name == NULL) {
		name = GeoIP_name_by_ipnum(db, ipnum);
		if (name == NULL)
			return (NULL);

		set_state(AF_INET, ipnum, NULL, subtype, NULL, NULL,
			  name, NULL, 0);
	}

	return (name);
}

/*
 * Netspeed lookups are performed if the previous lookup was from a
 * different IP address than the current, or was for a search of a
 * different subtype.
 */
static int
netspeed_lookup(GeoIP *db, dns_geoip_subtype_t subtype, uint32_t ipnum) {
	geoip_state_t *prev_state = NULL;
	bool found = false;
	int id = -1;

	REQUIRE(db != NULL);

	prev_state = get_state_for(AF_INET, ipnum, NULL);
	if (prev_state != NULL && prev_state->subtype == subtype) {
		id = prev_state->id;
		found = true;
	}

	if (!found) {
		id = GeoIP_id_by_ipnum(db, ipnum);
		if (id == 0)
			return (0);

		set_state(AF_INET, ipnum, NULL, subtype, NULL,
			  NULL, NULL, NULL, id);
	}

	return (id);
}
#endif /* HAVE_GEOIP */

#define DB46(addr, geoip, name) \
	((addr->family == AF_INET) ? (geoip->name##_v4) : (geoip->name##_v6))

#ifdef HAVE_GEOIP
/*
 * Find the best database to answer a generic subtype
 */
static dns_geoip_subtype_t
fix_subtype(const isc_netaddr_t *reqaddr, const dns_geoip_databases_t *geoip,
	    dns_geoip_subtype_t subtype)
{
	dns_geoip_subtype_t ret = subtype;

	switch (subtype) {
	case dns_geoip_countrycode:
		if (DB46(reqaddr, geoip, city) != NULL)
			ret = dns_geoip_city_countrycode;
		else if (reqaddr->family == AF_INET && geoip->region != NULL)
			ret = dns_geoip_region_countrycode;
		else if (DB46(reqaddr, geoip, country) != NULL)
			ret = dns_geoip_country_code;
		break;
	case dns_geoip_countrycode3:
		if (DB46(reqaddr, geoip, city) != NULL)
			ret = dns_geoip_city_countrycode3;
		else if (DB46(reqaddr, geoip, country) != NULL)
			ret = dns_geoip_country_code3;
		break;
	case dns_geoip_countryname:
		if (DB46(reqaddr, geoip, city) != NULL)
			ret = dns_geoip_city_countryname;
		else if (DB46(reqaddr, geoip, country) != NULL)
			ret = dns_geoip_country_name;
		break;
	case dns_geoip_region:
		if (DB46(reqaddr, geoip, city) != NULL)
			ret = dns_geoip_city_region;
		else if (reqaddr->family == AF_INET && geoip->region != NULL)
			ret = dns_geoip_region_code;
		break;
	case dns_geoip_regionname:
		if (DB46(reqaddr, geoip, city) != NULL)
			ret = dns_geoip_city_regionname;
		else if (reqaddr->family == AF_INET && geoip->region != NULL)
			ret = dns_geoip_region_name;
		break;
	default:
		break;
	}

	return (ret);
}
#endif /* HAVE_GEOIP */

bool
dns_geoip_match(const isc_netaddr_t *reqaddr,
		const dns_geoip_databases_t *geoip,
		const dns_geoip_elem_t *elt)
{
#ifndef HAVE_GEOIP
	UNUSED(reqaddr);
	UNUSED(geoip);
	UNUSED(elt);

	return (false);
#else
	GeoIP *db;
	GeoIPRecord *record;
	GeoIPRegion *region;
	dns_geoip_subtype_t subtype;
	uint32_t ipnum = 0;
	int maxlen = 0, id, family;
	const char *cs;
	char *s;
#ifdef HAVE_GEOIP_V6
	const geoipv6_t *ipnum6 = NULL;
#else
	const void *ipnum6 = NULL;
#endif

	INSIST(geoip != NULL);

	family = reqaddr->family;
	switch (family) {
	case AF_INET:
		ipnum = ntohl(reqaddr->type.in.s_addr);
		break;
	case AF_INET6:
#ifdef HAVE_GEOIP_V6
		ipnum6 = &reqaddr->type.in6;
		break;
#else
		return (false);
#endif
	default:
		return (false);
	}

	subtype = fix_subtype(reqaddr, geoip, elt->subtype);

	switch (subtype) {
	case dns_geoip_country_code:
		maxlen = 2;
		goto getcountry;

	case dns_geoip_country_code3:
		maxlen = 3;
		goto getcountry;

	case dns_geoip_country_name:
		maxlen = 255;
 getcountry:
		db = DB46(reqaddr, geoip, country);
		if (db == NULL)
			return (false);

		INSIST(elt->as_string != NULL);

		cs = country_lookup(db, subtype, family, ipnum, ipnum6);
		if (cs != NULL && strncasecmp(elt->as_string, cs, maxlen) == 0)
			return (true);
		break;

	case dns_geoip_city_countrycode:
	case dns_geoip_city_countrycode3:
	case dns_geoip_city_countryname:
	case dns_geoip_city_region:
	case dns_geoip_city_regionname:
	case dns_geoip_city_name:
	case dns_geoip_city_postalcode:
	case dns_geoip_city_continentcode:
	case dns_geoip_city_timezonecode:
		INSIST(elt->as_string != NULL);

		db = DB46(reqaddr, geoip, city);
		if (db == NULL)
			return (false);

		record = city_lookup(db, subtype, family,
				     ipnum, ipnum6);
		if (record == NULL)
			break;

		s = city_string(record, subtype, &maxlen);
		INSIST(maxlen != 0);
		if (s != NULL && strncasecmp(elt->as_string, s, maxlen) == 0)
			return (true);
		break;

	case dns_geoip_city_metrocode:
		db = DB46(reqaddr, geoip, city);
		if (db == NULL)
			return (false);

		record = city_lookup(db, subtype, family,
				     ipnum, ipnum6);
		if (record == NULL)
			break;

		if (elt->as_int == record->metro_code)
			return (true);
		break;

	case dns_geoip_city_areacode:
		db = DB46(reqaddr, geoip, city);
		if (db == NULL)
			return (false);

		record = city_lookup(db, subtype, family,
				     ipnum, ipnum6);
		if (record == NULL)
			break;

		if (elt->as_int == record->area_code)
			return (true);
		break;

	case dns_geoip_region_countrycode:
	case dns_geoip_region_code:
	case dns_geoip_region_name:
	case dns_geoip_region:
		if (geoip->region == NULL)
			return (false);

		INSIST(elt->as_string != NULL);

		/* Region DB is not supported for IPv6 */
		if (family == AF_INET6)
			return (false);

		region = region_lookup(geoip->region, subtype, ipnum);
		if (region == NULL)
			break;

		s = region_string(region, subtype, &maxlen);
		INSIST(maxlen != 0);
		if (s != NULL && strncasecmp(elt->as_string, s, maxlen) == 0)
			return (true);
		break;

	case dns_geoip_isp_name:
		db = geoip->isp;
		goto getname;

	case dns_geoip_org_name:
		db = geoip->org;
		goto getname;

	case dns_geoip_as_asnum:
		db = geoip->as;
		goto getname;

	case dns_geoip_domain_name:
		db = geoip->domain;

 getname:
		if (db == NULL)
			return (false);

		INSIST(elt->as_string != NULL);
		/* ISP, Org, AS, and Domain are not supported for IPv6 */
		if (family == AF_INET6)
			return (false);

		s = name_lookup(db, subtype, ipnum);
		if (s != NULL) {
			size_t l;
			if (strcasecmp(elt->as_string, s) == 0)
				return (true);
			if (subtype != dns_geoip_as_asnum)
				break;
			/*
			 * Just check if the ASNNNN value matches.
			 */
			l = strlen(elt->as_string);
			if (l > 0U && strchr(elt->as_string, ' ') == NULL &&
			    strncasecmp(elt->as_string, s, l) == 0 &&
			    s[l] == ' ')
				return (true);
		}
		break;

	case dns_geoip_netspeed_id:
		INSIST(geoip->netspeed != NULL);

		/* Netspeed DB is not supported for IPv6 */
		if (family == AF_INET6)
			return (false);

		id = netspeed_lookup(geoip->netspeed, subtype, ipnum);
		if (id == elt->as_int)
			return (true);
		break;

	case dns_geoip_countrycode:
	case dns_geoip_countrycode3:
	case dns_geoip_countryname:
	case dns_geoip_regionname:
		/*
		 * If these were not remapped by fix_subtype(),
		 * the database was unavailable. Always return false.
		 */
		break;

	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	return (false);
#endif
}

void
dns_geoip_shutdown(void) {
#ifdef HAVE_GEOIP
	GeoIP_cleanup();
	if (state_mctx != NULL)
		isc_mem_detach(&state_mctx);
#else
	return;
#endif
}
