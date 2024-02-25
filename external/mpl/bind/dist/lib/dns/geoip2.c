/*	$NetBSD: geoip2.c,v 1.6.2.1 2024/02/25 15:46:49 martin Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 * This file is only built and linked if GeoIP2 has been configured.
 */
#include <math.h>
#include <maxminddb.h>
#include <netinet/in.h>

#include <isc/mem.h>
#include <isc/once.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/geoip.h>
#include <dns/log.h>

/*
 * This structure preserves state from the previous GeoIP lookup,
 * so that successive lookups for the same data from the same IP
 * address will not require repeated database lookups.
 * This should improve performance somewhat.
 *
 * For all lookups we preserve pointers to the MMDB_lookup_result_s
 * and MMDB_entry_s structures, a pointer to the database from which
 * the lookup was answered, and a copy of the request address.
 *
 * If the next geoip ACL lookup is for the same database and from the
 * same address, we can reuse the MMDB entry without repeating the lookup.
 * This is for the case when a single query has to process multiple
 * geoip ACLs: for example, when there are multiple views with
 * match-clients statements that search for different countries.
 *
 * (XXX: Currently the persistent state is stored in thread specific
 * memory, but it could more simply be stored in the client object.
 * Also multiple entries could be stored in case the ACLs require
 * searching in more than one GeoIP database.)
 */

typedef struct geoip_state {
	uint16_t subtype;
	const MMDB_s *db;
	isc_netaddr_t addr;
	MMDB_lookup_result_s mmresult;
	MMDB_entry_s entry;
} geoip_state_t;

static thread_local geoip_state_t geoip_state = { 0 };

static void
set_state(const MMDB_s *db, const isc_netaddr_t *addr,
	  MMDB_lookup_result_s mmresult, MMDB_entry_s entry) {
	geoip_state.db = db;
	geoip_state.addr = *addr;
	geoip_state.mmresult = mmresult;
	geoip_state.entry = entry;
}

static geoip_state_t *
get_entry_for(MMDB_s *const db, const isc_netaddr_t *addr) {
	isc_sockaddr_t sa;
	MMDB_lookup_result_s match;
	int err;

	if (db == geoip_state.db && isc_netaddr_equal(addr, &geoip_state.addr))
	{
		return (&geoip_state);
	}

	isc_sockaddr_fromnetaddr(&sa, addr, 0);
	match = MMDB_lookup_sockaddr(db, &sa.type.sa, &err);
	if (err != MMDB_SUCCESS || !match.found_entry) {
		return (NULL);
	}

	set_state(db, addr, match, match.entry);

	return (&geoip_state);
}

static dns_geoip_subtype_t
fix_subtype(const dns_geoip_databases_t *geoip, dns_geoip_subtype_t subtype) {
	dns_geoip_subtype_t ret = subtype;

	switch (subtype) {
	case dns_geoip_countrycode:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_countrycode;
		} else if (geoip->country != NULL) {
			ret = dns_geoip_country_code;
		}
		break;
	case dns_geoip_countryname:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_countryname;
		} else if (geoip->country != NULL) {
			ret = dns_geoip_country_name;
		}
		break;
	case dns_geoip_continentcode:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_continentcode;
		} else if (geoip->country != NULL) {
			ret = dns_geoip_country_continentcode;
		}
		break;
	case dns_geoip_continent:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_continent;
		} else if (geoip->country != NULL) {
			ret = dns_geoip_country_continent;
		}
		break;
	case dns_geoip_region:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_region;
		}
		break;
	case dns_geoip_regionname:
		if (geoip->city != NULL) {
			ret = dns_geoip_city_regionname;
		}
	default:
		break;
	}

	return (ret);
}

static MMDB_s *
geoip2_database(const dns_geoip_databases_t *geoip,
		dns_geoip_subtype_t subtype) {
	switch (subtype) {
	case dns_geoip_country_code:
	case dns_geoip_country_name:
	case dns_geoip_country_continentcode:
	case dns_geoip_country_continent:
		return (geoip->country);

	case dns_geoip_city_countrycode:
	case dns_geoip_city_countryname:
	case dns_geoip_city_continentcode:
	case dns_geoip_city_continent:
	case dns_geoip_city_region:
	case dns_geoip_city_regionname:
	case dns_geoip_city_name:
	case dns_geoip_city_postalcode:
	case dns_geoip_city_timezonecode:
	case dns_geoip_city_metrocode:
	case dns_geoip_city_areacode:
		return (geoip->city);

	case dns_geoip_isp_name:
		return (geoip->isp);

	case dns_geoip_as_asnum:
	case dns_geoip_org_name:
		return (geoip->as);

	case dns_geoip_domain_name:
		return (geoip->domain);

	default:
		/*
		 * All other subtypes are unavailable in GeoIP2.
		 */
		return (NULL);
	}
}

static bool
match_string(MMDB_entry_data_s *value, const char *str) {
	REQUIRE(str != NULL);

	if (value == NULL || !value->has_data ||
	    value->type != MMDB_DATA_TYPE_UTF8_STRING ||
	    value->utf8_string == NULL)
	{
		return (false);
	}

	return (strncasecmp(value->utf8_string, str, value->data_size) == 0);
}

static bool
match_int(MMDB_entry_data_s *value, const uint32_t ui32) {
	if (value == NULL || !value->has_data ||
	    (value->type != MMDB_DATA_TYPE_UINT32 &&
	     value->type != MMDB_DATA_TYPE_UINT16))
	{
		return (false);
	}

	return (value->uint32 == ui32);
}

bool
dns_geoip_match(const isc_netaddr_t *reqaddr,
		const dns_geoip_databases_t *geoip,
		const dns_geoip_elem_t *elt) {
	MMDB_s *db = NULL;
	MMDB_entry_data_s value;
	geoip_state_t *state = NULL;
	dns_geoip_subtype_t subtype;
	const char *s = NULL;
	int ret;

	REQUIRE(reqaddr != NULL);
	REQUIRE(elt != NULL);
	REQUIRE(geoip != NULL);

	subtype = fix_subtype(geoip, elt->subtype);
	db = geoip2_database(geoip, subtype);
	if (db == NULL) {
		return (false);
	}

	state = get_entry_for(db, reqaddr);
	if (state == NULL) {
		return (false);
	}

	switch (subtype) {
	case dns_geoip_country_code:
	case dns_geoip_city_countrycode:
		ret = MMDB_get_value(&state->entry, &value, "country",
				     "iso_code", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_country_name:
	case dns_geoip_city_countryname:
		ret = MMDB_get_value(&state->entry, &value, "country", "names",
				     "en", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_country_continentcode:
	case dns_geoip_city_continentcode:
		ret = MMDB_get_value(&state->entry, &value, "continent", "code",
				     (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_country_continent:
	case dns_geoip_city_continent:
		ret = MMDB_get_value(&state->entry, &value, "continent",
				     "names", "en", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_region:
	case dns_geoip_city_region:
		ret = MMDB_get_value(&state->entry, &value, "subdivisions", "0",
				     "iso_code", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_regionname:
	case dns_geoip_city_regionname:
		ret = MMDB_get_value(&state->entry, &value, "subdivisions", "0",
				     "names", "en", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_city_name:
		ret = MMDB_get_value(&state->entry, &value, "city", "names",
				     "en", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_city_postalcode:
		ret = MMDB_get_value(&state->entry, &value, "postal", "code",
				     (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_city_timezonecode:
		ret = MMDB_get_value(&state->entry, &value, "location",
				     "time_zone", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_city_metrocode:
		ret = MMDB_get_value(&state->entry, &value, "location",
				     "metro_code", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_isp_name:
		ret = MMDB_get_value(&state->entry, &value, "isp", (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_as_asnum:
		INSIST(elt->as_string != NULL);

		ret = MMDB_get_value(&state->entry, &value,
				     "autonomous_system_number", (char *)0);
		if (ret == MMDB_SUCCESS) {
			int i;
			s = elt->as_string;
			if (strncasecmp(s, "AS", 2) == 0) {
				s += 2;
			}
			i = strtol(s, NULL, 10);
			return (match_int(&value, i));
		}
		break;

	case dns_geoip_org_name:
		ret = MMDB_get_value(&state->entry, &value,
				     "autonomous_system_organization",
				     (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	case dns_geoip_domain_name:
		ret = MMDB_get_value(&state->entry, &value, "domain",
				     (char *)0);
		if (ret == MMDB_SUCCESS) {
			return (match_string(&value, elt->as_string));
		}
		break;

	default:
		/*
		 * For any other subtype, we assume the database was
		 * unavailable and return false.
		 */
		return (false);
	}

	/*
	 * No database matched: return false.
	 */
	return (false);
}
