/*	$NetBSD: quota.h,v 1.11 2025/01/26 16:25:42 christos Exp $	*/

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
#include <isc/job.h>
#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/mutex.h>
#include <isc/os.h>
#include <isc/refcount.h>
#include <isc/types.h>
#include <isc/urcu.h>

/*****
***** Types.
*****/

/* Add -DISC_QUOTA_TRACE=1 to CFLAGS for detailed reference tracing */

ISC_LANG_BEGINDECLS

/*%
 * isc_quota structure
 *
 * NOTE: We are using struct cds_wfcq_head which has an internal
 * mutex, because we are using enqueue and dequeue, and dequeues need
 * synchronization between multiple threads (see urcu/wfcqueue.h for
 * detailed description).
 */
STATIC_ASSERT(ISC_OS_CACHELINE_SIZE >= sizeof(struct __cds_wfcq_head),
	      "ISC_OS_CACHELINE_SIZE smaller than "
	      "sizeof(struct __cds_wfcq_head)");
struct isc_quota {
	int		     magic;
	atomic_uint_fast32_t max;
	atomic_uint_fast32_t used;
	atomic_uint_fast32_t soft;
	struct {
		struct cds_wfcq_head head;
		uint8_t		     __padding[ISC_OS_CACHELINE_SIZE -
				       sizeof(struct __cds_wfcq_head)];
		struct cds_wfcq_tail tail;
	} jobs;
	ISC_LINK(isc_quota_t) link;
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

#define isc_quota_acquire(quota) isc_quota_acquire_cb(quota, NULL, NULL, NULL)
isc_result_t
isc_quota_acquire_cb(isc_quota_t *quota, isc_job_t *job, isc_job_cb cb,
		     void *cbarg);
/*%<
 *
 * Attempt to reserve one unit of 'quota', if there's no quota left then
 * cb->cb(cb->cbarg) will be called when there's quota again.
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
isc_quota_release(isc_quota_t *quota);
/*%<
 * Release one unit of quota.
 */

ISC_LANG_ENDDECLS
