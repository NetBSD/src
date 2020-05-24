/*	$NetBSD: zoneverify.h,v 1.3 2020/05/24 19:46:23 christos Exp $	*/

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

#pragma once

/*! \file dns/zoneverify.h */

#include <stdbool.h>

#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*%
 * Verify that certain things are sane:
 *
 *   The apex has a DNSKEY record with at least one KSK, and at least
 *   one ZSK if the -x flag was not used.
 *
 *   The DNSKEY record was signed with at least one of the KSKs in this
 *   set.
 *
 *   The rest of the zone was signed with at least one of the ZSKs
 *   present in the DNSKEY RRSET.
 *
 * Mark all RRsets correctly signed by one of the keys in the DNSKEY RRset at
 * zone apex as secure.
 *
 * If 'secroots' is not NULL, mark the DNSKEY RRset as secure if it is
 * correctly signed by at least one key present in 'secroots'.
 */
isc_result_t
dns_zoneverify_dnssec(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver,
		      dns_name_t *origin, dns_keytable_t *secroots,
		      isc_mem_t *mctx, bool ignore_kskflag, bool keyset_kskonly,
		      void (*report)(const char *, ...));

ISC_LANG_ENDDECLS
