/*	$NetBSD: zone_p.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

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

#ifndef DNS_ZONE_P_H
#define DNS_ZONE_P_H

/*! \file */

/*%
 *     Types and functions below not be used outside this module and its
 *     associated unit tests.
 */

ISC_LANG_BEGINDECLS

typedef struct {
	dns_diff_t	*diff;
	isc_boolean_t	offline;
} dns__zonediff_t;

isc_result_t
dns__zone_findkeys(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver,
		   isc_stdtime_t now, isc_mem_t *mctx, unsigned int maxkeys,
		   dst_key_t **keys, unsigned int *nkeys);

isc_result_t
dns__zone_updatesigs(dns_diff_t *diff, dns_db_t *db, dns_dbversion_t *version,
		     dst_key_t *zone_keys[], unsigned int nkeys,
		     dns_zone_t *zone, isc_stdtime_t inception,
		     isc_stdtime_t expire, isc_stdtime_t now,
		     isc_boolean_t check_ksk, isc_boolean_t keyset_kskonly,
		     dns__zonediff_t *zonediff);

ISC_LANG_ENDDECLS

#endif /* DNS_ZONE_P_H */
