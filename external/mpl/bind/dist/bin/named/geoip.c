/*	$NetBSD: geoip.c,v 1.2.4.3 2020/04/13 08:02:36 martin Exp $	*/

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

#if defined(HAVE_GEOIP2)
#include <maxminddb.h>
#elif defined(HAVE_GEOIP)
#include <GeoIP.h>
#include <GeoIPCity.h>
#endif

#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/geoip.h>

#include <named/log.h>
#include <named/geoip.h>

static dns_geoip_databases_t geoip_table;

#if defined(HAVE_GEOIP)
static void
init_geoip_db(void **dbp, GeoIPDBTypes edition, GeoIPDBTypes fallback,
	      GeoIPOptions method, const char *name)
{
	char *info;
	GeoIP *db;

	REQUIRE(dbp != NULL);

	db = (GeoIP *)*dbp;

	if (db != NULL) {
		GeoIP_delete(db);
		db = *dbp = NULL;
	}

	if (! GeoIP_db_avail(edition)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			"GeoIP %s (type %d) DB not available", name, edition);
		goto fail;
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		"initializing GeoIP %s (type %d) DB", name, edition);

	db = GeoIP_open_type(edition, method);
	if (db == NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			"failed to initialize GeoIP %s (type %d) DB%s",
			name, edition, fallback == 0
			 ? "geoip matches using this database will fail" : "");
		goto fail;
	}

	info = GeoIP_database_info(db);
	if (info != NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "%s", info);
		free(info);
	}

	*dbp = db;
	return;

 fail:
	if (fallback != 0) {
		init_geoip_db(dbp, fallback, 0, method, name);
	}

}
#elif defined(HAVE_GEOIP2)
static MMDB_s geoip_country, geoip_city, geoip_as, geoip_isp, geoip_domain;

static MMDB_s *
open_geoip2(const char *dir, const char *dbfile, MMDB_s *mmdb) {
	char pathbuf[PATH_MAX];
	unsigned int n;
	int ret;

	n = snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, dbfile);
	if (n >= sizeof(pathbuf)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "GeoIP2 database '%s/%s': path too long",
			      dir, dbfile);
		return (NULL);
	}

	ret = MMDB_open(pathbuf, MMDB_MODE_MMAP, mmdb);
	if (ret == MMDB_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "opened GeoIP2 database '%s'", pathbuf);
		return (mmdb);
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_DEBUG(1),
		      "unable to open GeoIP2 database '%s' (status %d)",
		      pathbuf, ret);

	return (NULL);
}
#endif /* HAVE_GEOIP2 */


void
named_geoip_init(void) {
#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)
	if (named_g_geoip == NULL) {
		named_g_geoip = &geoip_table;
	}
#if defined(HAVE_GEOIP)
	GeoIP_cleanup();
#endif
#else
	return;
#endif
}

void
named_geoip_load(char *dir) {
#if defined(HAVE_GEOIP2)
	REQUIRE(dir != NULL);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "looking for GeoIP2 databases in '%s'", dir);

	named_g_geoip->country = open_geoip2(dir, "GeoIP2-Country.mmdb",
					     &geoip_country);
	if (named_g_geoip->country == NULL) {
		named_g_geoip->country = open_geoip2(dir,
						     "GeoLite2-Country.mmdb",
						     &geoip_country);
	}

	named_g_geoip->city = open_geoip2(dir, "GeoIP2-City.mmdb",
					  &geoip_city);
	if (named_g_geoip->city == NULL) {
		named_g_geoip->city = open_geoip2(dir, "GeoLite2-City.mmdb",
						  &geoip_city);
	}

	named_g_geoip->as = open_geoip2(dir, "GeoIP2-ASN.mmdb", &geoip_as);
	if (named_g_geoip->as == NULL) {
		named_g_geoip->as = open_geoip2(dir, "GeoLite2-ASN.mmdb",
						&geoip_as);
	}

	named_g_geoip->isp = open_geoip2(dir, "GeoIP2-ISP.mmdb", &geoip_isp);
	named_g_geoip->domain = open_geoip2(dir, "GeoIP2-Domain.mmdb",
					    &geoip_domain);
#elif defined(HAVE_GEOIP)
	GeoIPOptions method;

#ifdef _WIN32
	method = GEOIP_STANDARD;
#else
	method = GEOIP_MMAP_CACHE;
#endif

	named_geoip_init();
	if (dir != NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "using \"%s\" as GeoIP directory", dir);
		GeoIP_setup_custom_directory(dir);
	}

	init_geoip_db(&named_g_geoip->country_v4, GEOIP_COUNTRY_EDITION, 0,
		      method, "Country (IPv4)");
#ifdef HAVE_GEOIP_V6
	init_geoip_db(&named_g_geoip->country_v6, GEOIP_COUNTRY_EDITION_V6, 0,
		      method, "Country (IPv6)");
#endif

	init_geoip_db(&named_g_geoip->city_v4, GEOIP_CITY_EDITION_REV1,
		      GEOIP_CITY_EDITION_REV0, method, "City (IPv4)");
#if defined(HAVE_GEOIP_V6) && defined(HAVE_GEOIP_CITY_V6)
	init_geoip_db(&named_g_geoip->city_v6, GEOIP_CITY_EDITION_REV1_V6,
		      GEOIP_CITY_EDITION_REV0_V6, method, "City (IPv6)");
#endif

	init_geoip_db(&named_g_geoip->region, GEOIP_REGION_EDITION_REV1,
		      GEOIP_REGION_EDITION_REV0, method, "Region");

	init_geoip_db(&named_g_geoip->isp, GEOIP_ISP_EDITION, 0,
		      method, "ISP");
	init_geoip_db(&named_g_geoip->org, GEOIP_ORG_EDITION, 0,
		      method, "Org");
	init_geoip_db(&named_g_geoip->as, GEOIP_ASNUM_EDITION, 0,
		      method, "AS");
	init_geoip_db(&named_g_geoip->domain, GEOIP_DOMAIN_EDITION, 0,
		      method, "Domain");
	init_geoip_db(&named_g_geoip->netspeed, GEOIP_NETSPEED_EDITION, 0,
		      method, "NetSpeed");
#else
	UNUSED(dir);

	return;
#endif
}

void
named_geoip_shutdown(void) {
#ifdef HAVE_GEOIP2
	if (named_g_geoip->country != NULL) {
		MMDB_close(named_g_geoip->country);
		named_g_geoip->country = NULL;
	}
	if (named_g_geoip->city != NULL) {
		MMDB_close(named_g_geoip->city);
		named_g_geoip->city = NULL;
	}
	if (named_g_geoip->as != NULL) {
		MMDB_close(named_g_geoip->as);
		named_g_geoip->as = NULL;
	}
	if (named_g_geoip->isp != NULL) {
		MMDB_close(named_g_geoip->isp);
		named_g_geoip->isp = NULL;
	}
	if (named_g_geoip->domain != NULL) {
		MMDB_close(named_g_geoip->domain);
		named_g_geoip->domain = NULL;
	}
#endif /* HAVE_GEOIP2 */

	dns_geoip_shutdown();
}
