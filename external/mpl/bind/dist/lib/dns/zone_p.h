/*	$NetBSD: zone_p.h,v 1.6.2.1 2024/02/25 15:46:54 martin Exp $	*/

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

#include <stdbool.h>

/*! \file */

/*%
 *     Types and functions below not be used outside this module and its
 *     associated unit tests.
 */

ISC_LANG_BEGINDECLS

typedef struct {
	dns_diff_t *diff;
	bool offline;
} dns__zonediff_t;

isc_result_t
dns__zone_findkeys(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver,
		   isc_stdtime_t now, isc_mem_t *mctx, unsigned int maxkeys,
		   dst_key_t **keys, unsigned int *nkeys);

isc_result_t
dns__zone_updatesigs(dns_diff_t *diff, dns_db_t *db, dns_dbversion_t *version,
		     dst_key_t *zone_keys[], unsigned int nkeys,
		     dns_zone_t *zone, isc_stdtime_t inception,
		     isc_stdtime_t expire, isc_stdtime_t keyxpire,
		     isc_stdtime_t now, bool check_ksk, bool keyset_kskonly,
		     dns__zonediff_t *zonediff);

isc_result_t
dns__zone_lookup_nsec3param(dns_zone_t *zone, dns_rdata_nsec3param_t *lookup,
			    dns_rdata_nsec3param_t *param,
			    unsigned char saltbuf[255], bool resalt);

ISC_LANG_ENDDECLS
