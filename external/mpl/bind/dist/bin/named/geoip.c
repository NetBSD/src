/*	$NetBSD: geoip.c,v 1.6.2.1 2024/02/25 15:43:05 martin Exp $	*/

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

#if defined(HAVE_GEOIP2)
#include <maxminddb.h>
#endif /* if defined(HAVE_GEOIP2) */

#include <isc/dir.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/geoip.h>

#include <named/geoip.h>
#include <named/log.h>

static dns_geoip_databases_t geoip_table;

#if defined(HAVE_GEOIP2)
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
			      "GeoIP2 database '%s/%s': path too long", dir,
			      dbfile);
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
#if defined(HAVE_GEOIP2)
	if (named_g_geoip == NULL) {
		named_g_geoip = &geoip_table;
	}
#else  /* if defined(HAVE_GEOIP2) */
	return;
#endif /* if defined(HAVE_GEOIP2) */
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
		named_g_geoip->country = open_geoip2(
			dir, "GeoLite2-Country.mmdb", &geoip_country);
	}

	named_g_geoip->city = open_geoip2(dir, "GeoIP2-City.mmdb", &geoip_city);
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
#else  /* if defined(HAVE_GEOIP2) */
	UNUSED(dir);

	return;
#endif /* if defined(HAVE_GEOIP2) */
}

void
named_geoip_unload(void) {
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
#endif /* ifdef HAVE_GEOIP2 */
}

void
named_geoip_shutdown(void) {
#ifdef HAVE_GEOIP2
	named_geoip_unload();
#endif /* HAVE_GEOIP2 */
}
