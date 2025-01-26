/*	$NetBSD: zt.h,v 1.10 2025/01/26 16:25:29 christos Exp $	*/

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

/*! \file dns/zt.h */

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/rwlock.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

typedef enum dns_ztfind {
	DNS_ZTFIND_EXACT = 1 << 0,
	DNS_ZTFIND_NOEXACT = 1 << 1,
	DNS_ZTFIND_MIRROR = 1 << 2,
} dns_ztfind_t;

typedef isc_result_t
dns_zt_callback_t(void *arg);

void
dns_zt_create(isc_mem_t *mctx, dns_view_t *view, dns_zt_t **ztp);
/*%<
 * Creates a new zone table for a view.
 *
 * Requires:
 * \li	'mctx' to be initialized.
 * \li	'view' is non-NULL
 * \li	'ztp' is non-NULL
 * \li	'*ztp' is NULL
 */

void
dns_zt_compact(dns_zt_t *zt);
/*%<
 * Reclaim unused memory in the zone table
 *
 * Requires:
 * \li	'zt' to be valid
 */

isc_result_t
dns_zt_mount(dns_zt_t *zt, dns_zone_t *zone);
/*%<
 * Mounts the zone on the zone table.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'zone' to be valid
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_EXISTS
 */

isc_result_t
dns_zt_unmount(dns_zt_t *zt, dns_zone_t *zone);
/*%<
 * Unmount the given zone from the table.
 *
 * Requires:
 *	'zt' to be valid
 * \li	'zone' to be valid
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 */

isc_result_t
dns_zt_find(dns_zt_t *zt, const dns_name_t *name, dns_ztfind_t options,
	    dns_zone_t **zone);
/*%<
 * Find the best match for 'name' in 'zt'.
 *
 * Notes:
 * \li	If the DNS_ZTFIND_EXACT option is set, only an exact match is
 *	returned.
 *
 * \li	If the DNS_ZTFIND_NOEXACT option is set, the closest matching
 *      parent domain is returned, even when there is an exact match
 *      in the tree.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'name' to be valid
 * \li	'zone' to be non NULL and '*zone' to be NULL
 * \li	DNS_ZTFIND_EXACT and DNS_ZTFIND_NOEXACT are not both set
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_PARTIALMATCH (if DNS_ZTFIND_EXACT is not set)
 * \li	#ISC_R_NOTFOUND
 */

void
dns_zt_detach(dns_zt_t **ztp);
/*%<
 * Detach the given zonetable, if the reference count goes to zero the
 * zonetable will be freed.  In either case 'ztp' is set to NULL.
 *
 * Requires:
 * \li	'*ztp' to be valid
 */

void
dns_zt_flush(dns_zt_t *ztp);
/*%<
 * Schedule flushing of the given zonetable, when reference count goes
 * to zero.
 *
 * Requires:
 * \li	'ztp' to be valid
 */

void
dns_zt_attach(dns_zt_t *zt, dns_zt_t **ztp);
/*%<
 * Attach 'zt' to '*ztp'.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'*ztp' to be NULL
 */

isc_result_t
dns_zt_load(dns_zt_t *zt, bool stop, bool newonly);

isc_result_t
dns_zt_asyncload(dns_zt_t *zt, bool newonly, dns_zt_callback_t alldone,
		 void *arg);
/*%<
 * Load all zones in the table. If 'stop' is true, stop on the first
 * error and return it. If 'stop' is false, ignore errors.
 *
 * If newonly is set only zones that were never loaded are loaded.
 *
 * dns_zt_asyncload() loads zones asynchronously; when all
 * zones in the zone table have finished loaded (or failed due
 * to errors), the caller is informed by calling 'alldone'
 * with an argument of 'arg'.
 *
 * Requires:
 * \li	'zt' to be valid
 */

isc_result_t
dns_zt_freezezones(dns_zt_t *zt, dns_view_t *view, bool freeze);
/*%<
 * Freeze/thaw updates to primary zones.
 * Any pending updates will be flushed.
 * Zones will be reloaded on thaw.
 */

isc_result_t
dns_zt_apply(dns_zt_t *zt, bool stop, isc_result_t *sub,
	     isc_result_t (*action)(dns_zone_t *, void *), void *uap);
/*%<
 * Apply a given 'action' to all zone zones in the table.
 * If 'stop' is 'true' then walking the zone tree will stop if
 * 'action' does not return ISC_R_SUCCESS.
 *
 * Requires:
 * \li	'zt' to be valid.
 * \li	'action' to be non NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS if action was applied to all nodes.  If 'stop' is
 *	false and 'sub' is non NULL then the first error (if any)
 *	reported by 'action' is returned in '*sub'. If 'stop' is true,
 *	the first error code from 'action' is returned.
 */

bool
dns_zt_loadspending(dns_zt_t *zt);
/*%<
 * Returns true if and only if there are zones still waiting to
 * be loaded in zone table 'zt'.
 *
 * Requires:
 * \li	'zt' to be valid.
 */

void
dns_zt_setviewcommit(dns_zt_t *zt);
/*%<
 * Commit dns_zone_setview() calls previously made for all zones in this
 * zone table.
 *
 * Requires:
 *\li	'zt' to be valid.
 */

void
dns_zt_setviewrevert(dns_zt_t *zt);
/*%<
 * Revert dns_zone_setview() calls previously made for all zones in this
 * zone table.
 *
 * Requires:
 *\li	'zt' to be valid.
 */

ISC_LANG_ENDDECLS
