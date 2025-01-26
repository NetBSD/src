/*	$NetBSD: quota.c,v 1.10 2025/01/26 16:25:38 christos Exp $	*/

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

/*! \file */

#include <stddef.h>

#include <isc/atomic.h>
#include <isc/quota.h>
#include <isc/urcu.h>
#include <isc/util.h>

#define QUOTA_MAGIC    ISC_MAGIC('Q', 'U', 'O', 'T')
#define VALID_QUOTA(p) ISC_MAGIC_VALID(p, QUOTA_MAGIC)

void
isc_quota_init(isc_quota_t *quota, unsigned int max) {
	atomic_init(&quota->max, max);
	atomic_init(&quota->used, 0);
	atomic_init(&quota->soft, 0);
	cds_wfcq_init(&quota->jobs.head, &quota->jobs.tail);
	ISC_LINK_INIT(quota, link);
	quota->magic = QUOTA_MAGIC;
}

void
isc_quota_soft(isc_quota_t *quota, unsigned int soft) {
	REQUIRE(VALID_QUOTA(quota));
	atomic_store_relaxed(&quota->soft, soft);
}

void
isc_quota_max(isc_quota_t *quota, unsigned int max) {
	REQUIRE(VALID_QUOTA(quota));
	atomic_store_relaxed(&quota->max, max);
}

unsigned int
isc_quota_getmax(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return atomic_load_relaxed(&quota->max);
}

unsigned int
isc_quota_getsoft(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return atomic_load_relaxed(&quota->soft);
}

unsigned int
isc_quota_getused(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return atomic_load_relaxed(&quota->used);
}

void
isc_quota_release(isc_quota_t *quota) {
	/*
	 * We are using the cds_wfcq_dequeue_blocking() variant here that
	 * has an internal mutex because we need synchronization on
	 * multiple dequeues running from different threads.
	 */
	struct cds_wfcq_node *node =
		cds_wfcq_dequeue_blocking(&quota->jobs.head, &quota->jobs.tail);
	if (node == NULL) {
		uint_fast32_t used = atomic_fetch_sub_relaxed(&quota->used, 1);
		INSIST(used > 0);
		return;
	}

	isc_job_t *job = caa_container_of(node, isc_job_t, wfcq_node);
	job->cb(job->cbarg);
}

isc_result_t
isc_quota_acquire_cb(isc_quota_t *quota, isc_job_t *job, isc_job_cb cb,
		     void *cbarg) {
	REQUIRE(VALID_QUOTA(quota));
	REQUIRE(job == NULL || cb != NULL);

	uint_fast32_t used = atomic_fetch_add_relaxed(&quota->used, 1);
	uint_fast32_t max = atomic_load_relaxed(&quota->max);
	if (max != 0 && used >= max) {
		(void)atomic_fetch_sub_relaxed(&quota->used, 1);
		if (job != NULL) {
			job->cb = cb;
			job->cbarg = cbarg;
			cds_wfcq_node_init(&job->wfcq_node);

			/*
			 * The cds_wfcq_enqueue() is non-blocking (no internal
			 * mutex involved), so it offers a slight advantage.
			 */
			cds_wfcq_enqueue(&quota->jobs.head, &quota->jobs.tail,
					 &job->wfcq_node);
		}
		return ISC_R_QUOTA;
	}

	uint_fast32_t soft = atomic_load_relaxed(&quota->soft);
	if (soft != 0 && used >= soft) {
		return ISC_R_SOFTQUOTA;
	}

	return ISC_R_SUCCESS;
}

void
isc_quota_destroy(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	quota->magic = 0;

	INSIST(atomic_load(&quota->used) == 0);
	INSIST(cds_wfcq_empty(&quota->jobs.head, &quota->jobs.tail));

	cds_wfcq_destroy(&quota->jobs.head, &quota->jobs.tail);
}
