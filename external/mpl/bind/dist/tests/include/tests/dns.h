/*	$NetBSD: dns.h,v 1.2.2.2 2024/02/25 15:47:49 martin Exp $	*/

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

#pragma once

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/diff.h>
#include <dns/zone.h>

#include <tests/isc.h>

extern dns_zonemgr_t *zonemgr;

typedef struct {
	dns_diffop_t op;
	const char  *owner;
	dns_ttl_t    ttl;
	const char  *type;
	const char  *rdata;
} zonechange_t;

#define ZONECHANGE_SENTINEL            \
	{                              \
		0, NULL, 0, NULL, NULL \
	}

isc_result_t
dns_test_makeview(const char *name, bool with_cache, dns_view_t **viewp);

/*%
 * Create a zone with origin 'name', return a pointer to the zone object in
 * 'zonep'.
 *
 * If 'view' is set, the returned zone will be assigned to the passed view.
 * 'createview' must be set to false when 'view' is non-NULL.
 *
 * If 'view' is not set and 'createview' is true, a new view is also created
 * and the returned zone is assigned to it.  This imposes two requirements on
 * the caller: 1) the returned zone has to be subsequently assigned to a zone
 * manager, otherwise its cleanup will fail, 2) the created view has to be
 * cleaned up by the caller.
 *
 * If 'view' is not set and 'createview' is false, the returned zone will not
 * be assigned to any view.
 */
isc_result_t
dns_test_makezone(const char *name, dns_zone_t **zonep, dns_view_t *view,
		  bool createview);

isc_result_t
dns_test_setupzonemgr(void);

isc_result_t
dns_test_managezone(dns_zone_t *zone);

void
dns_test_releasezone(dns_zone_t *zone);

void
dns_test_closezonemgr(void);

void
dns_test_nap(uint32_t usec);

isc_result_t
dns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
		const char *testfile);

isc_result_t
dns_test_getdata(const char *file, unsigned char *buf, size_t bufsiz,
		 size_t *sizep);

char *
dns_test_tohex(const unsigned char *data, size_t len, char *buf, size_t buflen);

/*%
 * Try parsing text form RDATA in "src" (of class "rdclass" and type "rdtype")
 * into a structure representing that RDATA at "rdata", storing the
 * uncompressed wire form of that RDATA at "dst", which is "dstlen" bytes long.
 * Set 'warnings' to true to print logged warnings from dns_rdata_fromtext().
 */
isc_result_t
dns_test_rdatafromstring(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
			 dns_rdatatype_t rdtype, unsigned char *dst,
			 size_t dstlen, const char *src, bool warnings);

void
dns_test_namefromstring(const char *namestr, dns_fixedname_t *fname);

/*%
 * Given a pointer to an uninitialized dns_diff_t structure in 'diff', make it
 * contain diff tuples representing zone database changes listed in 'changes'.
 * Set 'warnings' to true to print logged warnings from dns_rdata_fromtext().
 */
isc_result_t
dns_test_difffromchanges(dns_diff_t *diff, const zonechange_t *changes,
			 bool warnings);
