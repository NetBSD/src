/*	$NetBSD: geoip.c,v 1.1.1.1 2018/08/12 12:07:41 christos Exp $	*/

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

#include <isc/util.h>

#include <named/log.h>
#include <named/geoip.h>

#include <dns/geoip.h>

#ifdef HAVE_GEOIP
static dns_geoip_databases_t geoip_table = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static void
init_geoip_db(GeoIP **dbp, GeoIPDBTypes edition, GeoIPDBTypes fallback,
	      GeoIPOptions method, const char *name)
{
	char *info;
	GeoIP *db;

	REQUIRE(dbp != NULL);

	db = *dbp;

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
	if (fallback != 0)
		init_geoip_db(dbp, fallback, 0, method, name);

}
#endif /* HAVE_GEOIP */

void
named_geoip_init(void) {
#ifndef HAVE_GEOIP
	return;
#else
	GeoIP_cleanup();
	if (named_g_geoip == NULL)
		named_g_geoip = &geoip_table;
#endif
}

void
named_geoip_load(char *dir) {
#ifndef HAVE_GEOIP

	UNUSED(dir);

	return;
#else
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
#endif /* HAVE_GEOIP */
}
