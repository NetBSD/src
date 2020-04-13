/*	$NetBSD: geoip.h,v 1.3.2.3 2020/04/13 08:02:57 martin Exp $	*/

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

#ifndef DNS_GEOIP_H
#define DNS_GEOIP_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/geoip.h
 * \brief
 * GeoIP/GeoIP2 data types and function prototypes.
 */

#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/netaddr.h>
#include <isc/refcount.h>

#include <dns/name.h>
#include <dns/types.h>
#include <dns/iptable.h>

/***
 *** Types
 ***/

typedef enum {
	dns_geoip_countrycode,
	dns_geoip_countrycode3,
	dns_geoip_countryname,
	dns_geoip_continentcode,
	dns_geoip_continent,
	dns_geoip_region,
	dns_geoip_regionname,
	dns_geoip_country_code,
	dns_geoip_country_code3,
	dns_geoip_country_name,
	dns_geoip_country_continentcode,
	dns_geoip_country_continent,
	dns_geoip_region_countrycode,
	dns_geoip_region_code,
	dns_geoip_region_name,
	dns_geoip_city_countrycode,
	dns_geoip_city_countrycode3,
	dns_geoip_city_countryname,
	dns_geoip_city_region,
	dns_geoip_city_regionname,
	dns_geoip_city_name,
	dns_geoip_city_postalcode,
	dns_geoip_city_metrocode,
	dns_geoip_city_areacode,
	dns_geoip_city_continentcode,
	dns_geoip_city_continent,
	dns_geoip_city_timezonecode,
	dns_geoip_isp_name,
	dns_geoip_org_name,
	dns_geoip_as_asnum,
	dns_geoip_domain_name,
	dns_geoip_netspeed_id
} dns_geoip_subtype_t;

typedef struct dns_geoip_elem {
	dns_geoip_subtype_t subtype;
	void *db;
	union {
		char as_string[256];
		int as_int;
	};
} dns_geoip_elem_t;

struct dns_geoip_databases {
#ifdef HAVE_GEOIP2
	void *country;		/* GeoIP2-Country or GeoLite2-Country */
	void *city;		/* GeoIP2-CIty or GeoLite2-City */
	void *domain;		/* GeoIP2-Domain */
	void *isp;		/* GeoIP2-ISP */
	void *as;		/* GeoIP2-ASN or GeoLite2-ASN */
#else /* HAVE_GEOIP */
	void *country_v4;	/* GeoIP DB 1 */
	void *city_v4;		/* GeoIP DB 2 or 6 */
	void *region;		/* GeoIP DB 3 or 7 */
	void *isp;		/* GeoIP DB 4 */
	void *org;		/* GeoIP DB 5 */
	void *as;		/* GeoIP DB 9 */
	void *netspeed;		/* GeoIP DB 10 */
	void *domain;		/* GeoIP DB 11 */
	void *country_v6;	/* GeoIP DB 12 */
	void *city_v6;		/* GeoIP DB 30 or 31 */
#endif /* HAVE_GEOIP */
};

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

bool
dns_geoip_match(const isc_netaddr_t *reqaddr,
		const dns_geoip_databases_t *geoip,
		const dns_geoip_elem_t *elt);

void
dns_geoip_shutdown(void);

ISC_LANG_ENDDECLS

#endif /* HAVE_GEOIP | HAVE_GEOIP2 */

#endif /* DNS_GEOIP_H */
