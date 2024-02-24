/*	$NetBSD: quota.h,v 1.1.2.2 2024/02/24 13:07:26 martin Exp $	*/

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
#include <isc/magic.h>
#include <isc/mutex.h>
#include <isc/types.h>

/*****
***** Types.
*****/

ISC_LANG_BEGINDECLS

/*% isc_quota_cb - quota callback structure */
typedef struct isc_quota_cb isc_quota_cb_t;
typedef void (*isc_quota_cb_func_t)(isc_quota_t *quota, void *data);
struct isc_quota_cb {
	int		    magic;
	isc_quota_cb_func_t cb_func;
	void		   *data;
	ISC_LINK(isc_quota_cb_t) link;
};

/*% isc_quota structure */
struct isc_quota {
	int		     magic;
	atomic_uint_fast32_t max;
	atomic_uint_fast32_t used;
	atomic_uint_fast32_t soft;
	atomic_uint_fast32_t waiting;
	isc_mutex_t	     cblock;
	ISC_LIST(isc_quota_cb_t) cbs;
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
isc_quota_attach(isc_quota_t *quota, isc_quota_t **p);
/*%<
 *
 * Attempt to reserve one unit of 'quota', and also attaches '*p' to the quota
 * if successful (ISC_R_SUCCESS or ISC_R_SOFTQUOTA).
 *
 * Returns:
 * \li	#ISC_R_SUCCESS		Success
 * \li	#ISC_R_SOFTQUOTA	Success soft quota reached
 * \li	#ISC_R_QUOTA		Quota is full
 */

isc_result_t
isc_quota_attach_cb(isc_quota_t *quota, isc_quota_t **p, isc_quota_cb_t *cb);
/*%<
 *
 * Like isc_quota_attach(), but if there's no quota left then cb->cb_func will
 * be called when we are attached to quota.
 *
 * Note: It's the caller's responsibility to make sure that we don't end up
 * with a huge number of callbacks waiting, making it easy to create a
 * resource exhaustion attack. For example, in the case of TCP listening,
 * we simply don't accept new connections when the quota is exceeded, so
 * the number of callbacks waiting in the queue will be limited by the
 * listen() backlog.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS		Success
 * \li	#ISC_R_SOFTQUOTA	Success soft quota reached
 * \li	#ISC_R_QUOTA		Quota is full
 */

void
isc_quota_cb_init(isc_quota_cb_t *cb, isc_quota_cb_func_t cb_func, void *data);
/*%<
 * Initialize isc_quota_cb_t - setup the list, set the callback and data.
 */

void
isc_quota_detach(isc_quota_t **p);
/*%<
 * Release one unit of quota, and also detaches '*p' from the quota.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_QUOTA_H */
