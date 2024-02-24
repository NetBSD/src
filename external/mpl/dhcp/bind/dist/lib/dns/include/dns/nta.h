/*	$NetBSD: nta.h,v 1.1.2.2 2024/02/24 13:07:06 martin Exp $	*/

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

#ifndef DNS_NTA_H
#define DNS_NTA_H 1

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * The NTA module provides services for storing and retrieving negative
 * trust anchors, and determine whether a given domain is subject to
 * DNSSEC validation.
 */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/stdtime.h>
#include <isc/task.h>
#include <isc/timer.h>

#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/types.h>
#include <dns/view.h>

ISC_LANG_BEGINDECLS

struct dns_ntatable {
	/* Unlocked. */
	unsigned int	magic;
	dns_view_t     *view;
	isc_rwlock_t	rwlock;
	isc_taskmgr_t  *taskmgr;
	isc_timermgr_t *timermgr;
	isc_task_t     *task;
	/* Protected by atomics */
	isc_refcount_t references;
	/* Locked by rwlock. */
	dns_rbt_t *table;
	bool	   shuttingdown;
};

#define NTATABLE_MAGIC	   ISC_MAGIC('N', 'T', 'A', 't')
#define VALID_NTATABLE(nt) ISC_MAGIC_VALID(nt, NTATABLE_MAGIC)

isc_result_t
dns_ntatable_create(dns_view_t *view, isc_taskmgr_t *taskmgr,
		    isc_timermgr_t *timermgr, dns_ntatable_t **ntatablep);
/*%<
 * Create an NTA table in view 'view'.
 *
 * Requires:
 *
 *\li	'view' is a valid view.
 *
 *\li	'tmgr' is a valid timer manager.
 *
 *\li	ntatablep != NULL && *ntatablep == NULL
 *
 * Ensures:
 *
 *\li	On success, *ntatablep is a valid, empty NTA table.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	Any other result indicates failure.
 */

void
dns_ntatable_attach(dns_ntatable_t *source, dns_ntatable_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 *\li	'source' is a valid ntatable.
 *
 *\li	'targetp' points to a NULL dns_ntatable_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to source.
 */

void
dns_ntatable_detach(dns_ntatable_t **ntatablep);
/*%<
 * Detach *ntatablep from its ntatable.
 *
 * Requires:
 *
 *\li	'ntatablep' points to a valid ntatable.
 *
 * Ensures:
 *
 *\li	*ntatablep is NULL.
 *
 *\li	If '*ntatablep' is the last reference to the ntatable,
 *		all resources used by the ntatable will be freed
 */

isc_result_t
dns_ntatable_add(dns_ntatable_t *ntatable, const dns_name_t *name, bool force,
		 isc_stdtime_t now, uint32_t lifetime);
/*%<
 * Add a negative trust anchor to 'ntatable' for name 'name',
 * which will expire at time 'now' + 'lifetime'.  If 'force' is true,
 * then the NTA will persist for the entire specified lifetime.
 * If it is false, then the name will be queried periodically and
 * validation will be attempted to see whether it's still bogus;
 * if validation is successful, the NTA will be allowed to expire
 * early and validation below the NTA will resume.
 *
 * Notes:
 *
 *\li   If an NTA already exists in the table, its expiry time
 *      is updated.
 *
 * Requires:
 *
 *\li	'ntatable' points to a valid ntatable.
 *
 *\li	'name' points to a valid name.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_ntatable_delete(dns_ntatable_t *ntatable, const dns_name_t *keyname);
/*%<
 * Delete node(s) from 'ntatable' matching name 'keyname'
 *
 * Requires:
 *
 *\li	'ntatable' points to a valid ntatable.
 *
 *\li	'name' is not NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

bool
dns_ntatable_covered(dns_ntatable_t *ntatable, isc_stdtime_t now,
		     const dns_name_t *name, const dns_name_t *anchor);
/*%<
 * Return true if 'name' is below a non-expired negative trust
 * anchor which in turn is at or below 'anchor'.
 *
 * If 'ntatable' has not been initialized, return false.
 *
 * Requires:
 *
 *\li	'ntatable' is NULL or is a valid ntatable.
 *
 *\li	'name' is a valid absolute name.
 */

isc_result_t
dns_ntatable_totext(dns_ntatable_t *ntatable, const char *view,
		    isc_buffer_t **buf);
/*%<
 * Dump the NTA table to buffer at 'buf', with view names
 *
 * Requires:
 * \li   "ntatable" is a valid table.
 *
 * \li   "*buf" is a valid buffer.
 */

isc_result_t
dns_ntatable_dump(dns_ntatable_t *ntatable, FILE *fp);
/*%<
 * Dump the NTA table to the file opened as 'fp'.
 */

isc_result_t
dns_ntatable_save(dns_ntatable_t *ntatable, FILE *fp);
/*%<
 * Save the NTA table to the file opened as 'fp', for later loading.
 */

void
dns_ntatable_shutdown(dns_ntatable_t *ntatable);
/*%<
 * Cancel future checks to see if NTAs can be removed.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NTA_H */
