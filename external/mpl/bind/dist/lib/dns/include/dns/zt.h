/*	$NetBSD: zt.h,v 1.7 2023/01/25 21:43:30 christos Exp $	*/

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

#ifndef DNS_ZT_H
#define DNS_ZT_H 1

/*! \file dns/zt.h */

#include <stdbool.h>

#include <isc/lang.h>

#include <dns/types.h>

#define DNS_ZTFIND_NOEXACT 0x01
#define DNS_ZTFIND_MIRROR  0x02

ISC_LANG_BEGINDECLS

typedef isc_result_t (*dns_zt_allloaded_t)(void *arg);
/*%<
 * Method prototype: when all pending zone loads are complete,
 * the zone table can inform the caller via a callback function with
 * this signature.
 */

typedef isc_result_t (*dns_zt_zoneloaded_t)(dns_zt_t *zt, dns_zone_t *zone,
					    isc_task_t *task);
/*%<
 * Method prototype: when a zone finishes loading, the zt object
 * can be informed via a callback function with this signature.
 */

isc_result_t
dns_zt_create(isc_mem_t *mctx, dns_rdataclass_t rdclass, dns_zt_t **zt);
/*%<
 * Creates a new zone table.
 *
 * Requires:
 * \li	'mctx' to be initialized.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS on success.
 * \li	#ISC_R_NOMEMORY
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
 * \li	#ISC_R_NOSPACE
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_zt_unmount(dns_zt_t *zt, dns_zone_t *zone);
/*%<
 * Unmount the given zone from the table.
 *
 * Requires:
 * 	'zt' to be valid
 * \li	'zone' to be valid
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_zt_find(dns_zt_t *zt, const dns_name_t *name, unsigned int options,
	    dns_name_t *foundname, dns_zone_t **zone);
/*%<
 * Find the best match for 'name' in 'zt'.  If foundname is non NULL
 * then the name of the zone found is returned.
 *
 * Notes:
 * \li	If the DNS_ZTFIND_NOEXACT is set, the best partial match (if any)
 *	to 'name' will be returned.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'name' to be valid
 * \li	'foundname' to be initialized and associated with a fixedname or NULL
 * \li	'zone' to be non NULL and '*zone' to be NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_PARTIALMATCH
 * \li	#ISC_R_NOTFOUND
 * \li	#ISC_R_NOSPACE
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
dns_zt_flushanddetach(dns_zt_t **ztp);
/*%<
 * Detach the given zonetable, if the reference count goes to zero the
 * zonetable will be flushed and then freed.  In either case 'ztp' is
 * set to NULL.
 *
 * Requires:
 * \li	'*ztp' to be valid
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
dns_zt_asyncload(dns_zt_t *zt, bool newonly, dns_zt_allloaded_t alldone,
		 void *arg);
/*%<
 * Load all zones in the table.  If 'stop' is true,
 * stop on the first error and return it.  If 'stop'
 * is false, ignore errors.
 *
 * if newonly is set only zones that were never loaded are loaded.
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
 * Freeze/thaw updates to master zones.
 * Any pending updates will be flushed.
 * Zones will be reloaded on thaw.
 */

isc_result_t
dns_zt_apply(dns_zt_t *zt, isc_rwlocktype_t lock, bool stop, isc_result_t *sub,
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
 *	reported by 'action' is returned in '*sub';
 *	any error code from 'action'.
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
 *\li	'view' to be valid.
 */

void
dns_zt_setviewrevert(dns_zt_t *zt);
/*%<
 * Revert dns_zone_setview() calls previously made for all zones in this
 * zone table.
 *
 * Requires:
 *\li	'view' to be valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ZT_H */
