/*	$NetBSD: geoip_test.c,v 1.3 2019/01/09 16:55:13 christos Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/print.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/geoip.h>

#include "dnstest.h"

#ifdef HAVE_GEOIP
#include <GeoIP.h>

/* We use GeoIP databases from the 'geoip' system test */
#define TEST_GEOIP_DATA "../../../bin/tests/system/geoip/data"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

/*
 * Helper functions
 * (Mostly copied from bin/named/geoip.c)
 */
static dns_geoip_databases_t geoip = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static void
init_geoip_db(GeoIP **dbp, GeoIPDBTypes edition, GeoIPDBTypes fallback,
	      GeoIPOptions method, const char *name)
{
	GeoIP *db;

	REQUIRE(dbp != NULL);

	db = *dbp;

	if (db != NULL) {
		GeoIP_delete(db);
		db = *dbp = NULL;
	}

	if (! GeoIP_db_avail(edition)) {
		goto fail;
	}

	db = GeoIP_open_type(edition, method);
	if (db == NULL) {
		goto fail;
	}

	*dbp = db;
	return;

 fail:
	if (fallback != 0) {
		init_geoip_db(dbp, fallback, 0, method, name);
	}
}

static void
load_geoip(const char *dir) {
	GeoIPOptions method;

#ifdef _WIN32
	method = GEOIP_STANDARD;
#else
	method = GEOIP_MMAP_CACHE;
#endif

	if (dir != NULL) {
		char *p;
		DE_CONST(dir, p);
		GeoIP_setup_custom_directory(p);
	}

	init_geoip_db(&geoip.country_v4, GEOIP_COUNTRY_EDITION, 0,
		      method, "Country (IPv4)");
#ifdef HAVE_GEOIP_V6
	init_geoip_db(&geoip.country_v6, GEOIP_COUNTRY_EDITION_V6, 0,
		      method, "Country (IPv6)");
#endif

	init_geoip_db(&geoip.city_v4, GEOIP_CITY_EDITION_REV1,
		      GEOIP_CITY_EDITION_REV0, method, "City (IPv4)");
#if defined(HAVE_GEOIP_V6) && defined(HAVE_GEOIP_CITY_V6)
	init_geoip_db(&geoip.city_v6, GEOIP_CITY_EDITION_REV1_V6,
		      GEOIP_CITY_EDITION_REV0_V6, method, "City (IPv6)");
#endif

	init_geoip_db(&geoip.region, GEOIP_REGION_EDITION_REV1,
		      GEOIP_REGION_EDITION_REV0, method, "Region");
	init_geoip_db(&geoip.isp, GEOIP_ISP_EDITION, 0,
		      method, "ISP");
	init_geoip_db(&geoip.org, GEOIP_ORG_EDITION, 0,
		      method, "Org");
	init_geoip_db(&geoip.as, GEOIP_ASNUM_EDITION, 0,
		      method, "AS");
	init_geoip_db(&geoip.domain, GEOIP_DOMAIN_EDITION, 0,
		      method, "Domain");
	init_geoip_db(&geoip.netspeed, GEOIP_NETSPEED_EDITION, 0,
		      method, "NetSpeed");
}

static bool
do_lookup_string(const char *addr, dns_geoip_subtype_t subtype,
		 const char *string)
{
	dns_geoip_elem_t elt;
	struct in_addr in4;
	isc_netaddr_t na;

	inet_pton(AF_INET, addr, &in4);
	isc_netaddr_fromin(&na, &in4);

	elt.subtype = subtype;
	strlcpy(elt.as_string, string, sizeof(elt.as_string));

	return (dns_geoip_match(&na, &geoip, &elt));
}

static bool
do_lookup_string_v6(const char *addr, dns_geoip_subtype_t subtype,
		    const char *string)
{
	dns_geoip_elem_t elt;
	struct in6_addr in6;
	isc_netaddr_t na;

	inet_pton(AF_INET6, addr, &in6);
	isc_netaddr_fromin6(&na, &in6);

	elt.subtype = subtype;
	strlcpy(elt.as_string, string, sizeof(elt.as_string));

	return (dns_geoip_match(&na, &geoip, &elt));
}

static bool
do_lookup_int(const char *addr, dns_geoip_subtype_t subtype, int id) {
	dns_geoip_elem_t elt;
	struct in_addr in4;
	isc_netaddr_t na;

	inet_pton(AF_INET, addr, &in4);
	isc_netaddr_fromin(&na, &in4);

	elt.subtype = subtype;
	elt.as_int = id;

	return (dns_geoip_match(&na, &geoip, &elt));
}

/* GeoIP country matching */
static void
country(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.country_v4 == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1", dns_geoip_country_code, "AU");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_country_code3, "AUS");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_country_name, "Australia");
	assert_true(match);

	match = do_lookup_string("192.0.2.128", dns_geoip_country_code, "O1");
	assert_true(match);

	match = do_lookup_string("192.0.2.128",
				 dns_geoip_country_name, "Other");
	assert_true(match);
}

/* GeoIP country (ipv6) matching */
static void
country_v6(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.country_v6 == NULL) {
		skip();
	}

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_code, "AU");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_code3, "AUS");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_name, "Australia");
	assert_true(match);
}

/* GeoIP city (ipv4) matching */
static void
city(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.city_v4 == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_continentcode, "NA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_countrycode, "US");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_countrycode3, "USA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_countryname, "United States");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_region, "CA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_regionname, "California");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_name, "Redwood City");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_postalcode, "94063");
	assert_true(match);

	match = do_lookup_int("10.53.0.1", dns_geoip_city_areacode, 650);
	assert_true(match);

	match = do_lookup_int("10.53.0.1", dns_geoip_city_metrocode, 807);
	assert_true(match);
}

/* GeoIP city (ipv6) matching */
static void
city_v6(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.city_v6 == NULL) {
		skip();
	}

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_continentcode, "NA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countrycode, "US");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countrycode3, "USA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countryname,
				    "United States");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_region, "CA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_regionname, "California");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_name, "Redwood City");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_postalcode, "94063");
	assert_true(match);
}


/* GeoIP region matching */
static void
region(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.region == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_region_code, "CA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_region_name, "California");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_region_countrycode, "US");
	assert_true(match);
}

/*
 * GeoIP best-database matching
 * (With no specified databse and a city database available, answers
 * should come from city database.  With city database unavailable, region
 * database.  Region database unavailable, country database.)
 */
static void
best(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.region == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode, "US");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode3, "USA");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countryname, "United States");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_regionname, "Virginia");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_region, "VA");
	assert_true(match);

	GeoIP_delete(geoip.city_v4);
	geoip.city_v4 = NULL;

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode, "AU");
	assert_true(match);

	/*
	 * Note, region doesn't support code3 or countryname, so
	 * the next two would be answered from the country database instead
	 */
	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode3, "CAN");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countryname, "Canada");
	assert_true(match);

	GeoIP_delete(geoip.region);
	geoip.region = NULL;

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode, "CA");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countrycode3, "CAN");
	assert_true(match);

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_countryname, "Canada");
	assert_true(match);
}


/* GeoIP asnum matching */
static void
asnum(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.as == NULL) {
		skip();
	}


	match = do_lookup_string("10.53.0.3", dns_geoip_as_asnum,
				 "AS100003 Three Network Labs");
	assert_true(match);
}

/* GeoIP isp matching */
static void
isp(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.isp == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1", dns_geoip_isp_name,
				 "One Systems, Inc.");
	assert_true(match);
}

/* GeoIP org matching */
static void
org(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.org == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.2", dns_geoip_org_name,
				 "Two Technology Ltd.");
	assert_true(match);
}

/* GeoIP domain matching */
static void
domain(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.domain == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.4",
				 dns_geoip_domain_name, "four.com");
	assert_true(match);
}

/* GeoIP netspeed matching */
static void
netspeed(void **state) {
	bool match;

	UNUSED(state);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	if (geoip.netspeed == NULL) {
		skip();
	}

	match = do_lookup_int("10.53.0.1", dns_geoip_netspeed_id, 0);
	assert_true(match);

	match = do_lookup_int("10.53.0.2", dns_geoip_netspeed_id, 1);
	assert_true(match);

	match = do_lookup_int("10.53.0.3", dns_geoip_netspeed_id, 2);
	assert_true(match);

	match = do_lookup_int("10.53.0.4", dns_geoip_netspeed_id, 3);
	assert_true(match);
}
#endif /* HAVE_GEOIP */

int
main(void) {
#ifdef HAVE_GEOIP
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(country, _setup, _teardown),
		cmocka_unit_test_setup_teardown(country_v6, _setup, _teardown),
		cmocka_unit_test_setup_teardown(city, _setup, _teardown),
		cmocka_unit_test_setup_teardown(city_v6, _setup, _teardown),
		cmocka_unit_test_setup_teardown(region, _setup, _teardown),
		cmocka_unit_test_setup_teardown(best, _setup, _teardown),
		cmocka_unit_test_setup_teardown(asnum, _setup, _teardown),
		cmocka_unit_test_setup_teardown(isp, _setup, _teardown),
		cmocka_unit_test_setup_teardown(org, _setup, _teardown),
		cmocka_unit_test_setup_teardown(domain, _setup, _teardown),
		cmocka_unit_test_setup_teardown(netspeed, _setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else
	print_message("1..0 # Skip geoip not enabled\n");
#endif /* HAVE_GEOIP */
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* HAVE_CMOCKA */
