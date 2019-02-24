/*	$NetBSD: quota.h,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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


#ifndef ISC_QUOTA_H
#define ISC_QUOTA_H 1

/*****
 ***** Module Info
 *****/

/*! \file isc/quota.h
 *
 * \brief The isc_quota_t object is a simple helper object for implementing
 * quotas on things like the number of simultaneous connections to
 * a server.  It keeps track of the amount of quota in use, and
 * encapsulates the locking necessary to allow multiple tasks to
 * share a quota.
 */

/***
 *** Imports.
 ***/

#include <isc/atomic.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/types.h>

/*****
 ***** Types.
 *****/

ISC_LANG_BEGINDECLS

/*% isc_quota structure */
struct isc_quota {
	atomic_uint_fast32_t 		max;
	atomic_uint_fast32_t 		used;
	atomic_uint_fast32_t		soft;
};


void
isc_quota_init(isc_quota_t *quota, unsigned int max);
/*%<
 * Initialize a quota object.
 */

void
isc_quota_destroy(isc_quota_t *quota);
/*%<
 * Destroy a quota object.
 */

void
isc_quota_soft(isc_quota_t *quota, unsigned int soft);
/*%<
 * Set a soft quota.
 */

void
isc_quota_max(isc_quota_t *quota, unsigned int max);
/*%<
 * Re-set a maximum quota.
 */

unsigned int
isc_quota_getmax(isc_quota_t *quota);
/*%<
 * Get the maximum quota.
 */

unsigned int
isc_quota_getsoft(isc_quota_t *quota);
/*%<
 * Get the soft quota.
 */

unsigned int
isc_quota_getused(isc_quota_t *quota);
/*%<
 * Get the current usage of quota.
 */

isc_result_t
isc_quota_reserve(isc_quota_t *quota);
/*%<
 * Attempt to reserve one unit of 'quota'.
 *
 * Returns:
 * \li 	#ISC_R_SUCCESS		Success
 * \li	#ISC_R_SOFTQUOTA	Success soft quota reached
 * \li	#ISC_R_QUOTA		Quota is full
 */

void
isc_quota_release(isc_quota_t *quota);
/*%<
 * Release one unit of quota.
 */

isc_result_t
isc_quota_attach(isc_quota_t *quota, isc_quota_t **p);
/*%<
 * Like isc_quota_reserve, and also attaches '*p' to the
 * quota if successful (ISC_R_SUCCESS or ISC_R_SOFTQUOTA).
 */

void
isc_quota_detach(isc_quota_t **p);
/*%<
 * Like isc_quota_release, and also detaches '*p' from the
 * quota.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_QUOTA_H */
