/*	$NetBSD: geoip_test.c,v 1.3.2.3 2020/04/13 08:02:57 martin Exp $	*/

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

#include <sched.h> /* IWYU pragma: keep */
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

#if defined(HAVE_GEOIP2)
#include <maxminddb.h>

/* Use GeoIP2 databases from the 'geoip2' system test */
#define TEST_GEOIP_DATA "../../../bin/tests/system/geoip2/data"
#elif defined(HAVE_GEOIP)
#include <GeoIP.h>

/* Use GeoIP databases from the 'geoip' system test */
#define TEST_GEOIP_DATA "../../../bin/tests/system/geoip/data"
#endif

#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)
static void load_geoip(const char *dir);
static void close_geoip(void);

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	close_geoip();

	dns_test_end();

	return (0);
}

static dns_geoip_databases_t geoip;
#endif /* HAVE_GEOIP || HAVE_GEOIP2 */

#if defined(HAVE_GEOIP2)
static MMDB_s geoip_country, geoip_city, geoip_as, geoip_isp, geoip_domain;

static MMDB_s *
open_geoip2(const char *dir, const char *dbfile, MMDB_s *mmdb) {
	char pathbuf[PATH_MAX];
	int ret;

	snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, dbfile);
	ret = MMDB_open(pathbuf, MMDB_MODE_MMAP, mmdb);
	if (ret == MMDB_SUCCESS) {
		return (mmdb);
	}

	return (NULL);
}

static void
load_geoip(const char *dir) {
	geoip.country = open_geoip2(dir, "GeoIP2-Country.mmdb",
				     &geoip_country);
	geoip.city = open_geoip2(dir, "GeoIP2-City.mmdb", &geoip_city);
	geoip.as = open_geoip2(dir, "GeoLite2-ASN.mmdb", &geoip_as);
	geoip.isp = open_geoip2(dir, "GeoIP2-ISP.mmdb", &geoip_isp);
	geoip.domain = open_geoip2(dir, "GeoIP2-Domain.mmdb", &geoip_domain);
}

static void
close_geoip(void) {
	MMDB_close(&geoip_country);
	MMDB_close(&geoip_city);
	MMDB_close(&geoip_as);
	MMDB_close(&geoip_isp);
	MMDB_close(&geoip_domain);
}

#elif defined(HAVE_GEOIP)
/*
 * Helper functions (mostly copied from bin/named/geoip.c)
 */
static void
init_geoip_db(void **dbp, GeoIPDBTypes edition, GeoIPDBTypes fallback,
	      GeoIPOptions method, const char *name)
{
	GeoIP *db;

	REQUIRE(dbp != NULL);

	db = (GeoIP *)*dbp;

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

static void
close_geoip(void) {
	GeoIP_cleanup();
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

#endif /* HAVE_GEOIP */

#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)
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

/* GeoIP country matching */
static void
country(void **state) {
	bool match;

	UNUSED(state);

#ifdef HAVE_GEOIP2
	if (geoip.country == NULL) {
		skip();
	}
#else /* HAVE_GEOIP */
	if (geoip.country_v4 == NULL) {
		skip();
	}
#endif /* HAVE_GEOIP */

	match = do_lookup_string("10.53.0.1", dns_geoip_country_code, "AU");
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

#ifdef HAVE_GEOIP2
	if (geoip.country == NULL) {
		skip();
	}
#else /* HAVE_GEOIP */
	if (geoip.country_v6 == NULL) {
		skip();
	}
#endif /* HAVE_GEOIP */

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_code, "AU");
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

#ifdef HAVE_GEOIP2
	if (geoip.city == NULL) {
		skip();
	}
#else /* HAVE_GEOIP */
	if (geoip.city_v4 == NULL) {
		skip();
	}
#endif /* HAVE_GEOIP */

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_continentcode, "NA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1",
				 dns_geoip_city_countrycode, "US");
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

#ifdef HAVE_GEOIP
	match = do_lookup_int("10.53.0.1", dns_geoip_city_areacode, 650);
	assert_true(match);

	match = do_lookup_int("10.53.0.1", dns_geoip_city_metrocode, 807);
	assert_true(match);
#endif
}

/* GeoIP city (ipv6) matching */
static void
city_v6(void **state) {
	bool match;

	UNUSED(state);

#ifdef HAVE_GEOIP2
	if (geoip.city == NULL) {
		skip();
	}
#else /* HAVE_GEOIP */
	if (geoip.city_v6 == NULL) {
		skip();
	}
#endif /* HAVE_GEOIP */

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_continentcode, "NA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countrycode, "US");
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

/* GeoIP asnum matching */
static void
asnum(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.as == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.3", dns_geoip_as_asnum, "AS100003");
	assert_true(match);
}

/* GeoIP isp matching */
static void
isp(void **state) {
	bool match;

	UNUSED(state);

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

#ifdef HAVE_GEOIP2
	if (geoip.as == NULL) {
		skip();
	}
#else /* HAVE_GEOIP */
	if (geoip.org == NULL) {
		skip();
	}
#endif /* HAVE_GEOIP */

	match = do_lookup_string("10.53.0.2", dns_geoip_org_name,
				 "Two Technology Ltd.");
	assert_true(match);
}

/* GeoIP domain matching */
static void
domain(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.domain == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.5",
				 dns_geoip_domain_name, "five.es");
	assert_true(match);
}
#endif /* HAVE_GEOIP || HAVE_GEOIP2 */

#ifdef HAVE_GEOIP
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
#endif /* HAVE_GEOIP */

int
main(void) {
#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(country, _setup, _teardown),
		cmocka_unit_test_setup_teardown(country_v6, _setup, _teardown),
		cmocka_unit_test_setup_teardown(city, _setup, _teardown),
		cmocka_unit_test_setup_teardown(city_v6, _setup, _teardown),
		cmocka_unit_test_setup_teardown(asnum, _setup, _teardown),
		cmocka_unit_test_setup_teardown(isp, _setup, _teardown),
		cmocka_unit_test_setup_teardown(org, _setup, _teardown),
		cmocka_unit_test_setup_teardown(domain, _setup, _teardown),
#ifdef HAVE_GEOIP
		cmocka_unit_test_setup_teardown(region, _setup, _teardown),
		cmocka_unit_test_setup_teardown(netspeed, _setup, _teardown),
		cmocka_unit_test_setup_teardown(best, _setup, _teardown),
#endif /* HAVE_GEOIP */
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else
	print_message("1..0 # Skip GeoIP not enabled\n");
#endif
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* HAVE_CMOCKA */
