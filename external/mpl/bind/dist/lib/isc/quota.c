/*	$NetBSD: quota.c,v 1.8.2.1 2024/02/25 15:47:18 martin Exp $	*/

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
#include <isc/util.h>

#define QUOTA_MAGIC    ISC_MAGIC('Q', 'U', 'O', 'T')
#define VALID_QUOTA(p) ISC_MAGIC_VALID(p, QUOTA_MAGIC)

#define QUOTA_CB_MAGIC	  ISC_MAGIC('Q', 'T', 'C', 'B')
#define VALID_QUOTA_CB(p) ISC_MAGIC_VALID(p, QUOTA_CB_MAGIC)

void
isc_quota_init(isc_quota_t *quota, unsigned int max) {
	atomic_init(&quota->max, max);
	atomic_init(&quota->used, 0);
	atomic_init(&quota->soft, 0);
	atomic_init(&quota->waiting, 0);
	ISC_LIST_INIT(quota->cbs);
	isc_mutex_init(&quota->cblock);
	ISC_LINK_INIT(quota, link);
	quota->magic = QUOTA_MAGIC;
}

void
isc_quota_destroy(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	quota->magic = 0;

	INSIST(atomic_load(&quota->used) == 0);
	INSIST(atomic_load(&quota->waiting) == 0);
	INSIST(ISC_LIST_EMPTY(quota->cbs));
	atomic_store_release(&quota->max, 0);
	atomic_store_release(&quota->used, 0);
	atomic_store_release(&quota->soft, 0);
	isc_mutex_destroy(&quota->cblock);
}

void
isc_quota_soft(isc_quota_t *quota, unsigned int soft) {
	REQUIRE(VALID_QUOTA(quota));
	atomic_store_release(&quota->soft, soft);
}

void
isc_quota_max(isc_quota_t *quota, unsigned int max) {
	REQUIRE(VALID_QUOTA(quota));
	atomic_store_release(&quota->max, max);
}

unsigned int
isc_quota_getmax(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return (atomic_load_relaxed(&quota->max));
}

unsigned int
isc_quota_getsoft(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return (atomic_load_relaxed(&quota->soft));
}

unsigned int
isc_quota_getused(isc_quota_t *quota) {
	REQUIRE(VALID_QUOTA(quota));
	return (atomic_load_relaxed(&quota->used));
}

static isc_result_t
quota_reserve(isc_quota_t *quota) {
	isc_result_t result;
	uint_fast32_t max = atomic_load_acquire(&quota->max);
	uint_fast32_t soft = atomic_load_acquire(&quota->soft);
	uint_fast32_t used = atomic_load_acquire(&quota->used);
	do {
		if (max != 0 && used >= max) {
			return (ISC_R_QUOTA);
		}
		if (soft != 0 && used >= soft) {
			result = ISC_R_SOFTQUOTA;
		} else {
			result = ISC_R_SUCCESS;
		}
	} while (!atomic_compare_exchange_weak_acq_rel(&quota->used, &used,
						       used + 1));
	return (result);
}

/* Must be quota->cbslock locked */
static void
enqueue(isc_quota_t *quota, isc_quota_cb_t *cb) {
	REQUIRE(cb != NULL);
	ISC_LIST_ENQUEUE(quota->cbs, cb, link);
	atomic_fetch_add_release(&quota->waiting, 1);
}

/* Must be quota->cbslock locked */
static isc_quota_cb_t *
dequeue(isc_quota_t *quota) {
	isc_quota_cb_t *cb = ISC_LIST_HEAD(quota->cbs);
	INSIST(cb != NULL);
	ISC_LIST_DEQUEUE(quota->cbs, cb, link);
	atomic_fetch_sub_relaxed(&quota->waiting, 1);
	return (cb);
}

static void
quota_release(isc_quota_t *quota) {
	uint_fast32_t used;

	/*
	 * This is opportunistic - we might race with a failing quota_attach_cb
	 * and not detect that something is waiting, but eventually someone will
	 * be releasing quota and will detect it, so we don't need to worry -
	 * and we're saving a lot by not locking cblock every time.
	 */

	if (atomic_load_acquire(&quota->waiting) > 0) {
		isc_quota_cb_t *cb = NULL;
		LOCK(&quota->cblock);
		if (atomic_load_relaxed(&quota->waiting) > 0) {
			cb = dequeue(quota);
		}
		UNLOCK(&quota->cblock);
		if (cb != NULL) {
			cb->cb_func(quota, cb->data);
			return;
		}
	}

	used = atomic_fetch_sub_release(&quota->used, 1);
	INSIST(used > 0);
}

static isc_result_t
doattach(isc_quota_t *quota, isc_quota_t **p) {
	isc_result_t result;
	REQUIRE(p != NULL && *p == NULL);

	result = quota_reserve(quota);
	if (result == ISC_R_SUCCESS || result == ISC_R_SOFTQUOTA) {
		*p = quota;
	}

	return (result);
}

isc_result_t
isc_quota_attach(isc_quota_t *quota, isc_quota_t **quotap) {
	REQUIRE(VALID_QUOTA(quota));
	REQUIRE(quotap != NULL && *quotap == NULL);

	return (isc_quota_attach_cb(quota, quotap, NULL));
}

isc_result_t
isc_quota_attach_cb(isc_quota_t *quota, isc_quota_t **quotap,
		    isc_quota_cb_t *cb) {
	REQUIRE(VALID_QUOTA(quota));
	REQUIRE(cb == NULL || VALID_QUOTA_CB(cb));
	REQUIRE(quotap != NULL && *quotap == NULL);

	isc_result_t result = doattach(quota, quotap);
	if (result == ISC_R_QUOTA && cb != NULL) {
		LOCK(&quota->cblock);
		enqueue(quota, cb);
		UNLOCK(&quota->cblock);
	}
	return (result);
}

void
isc_quota_cb_init(isc_quota_cb_t *cb, isc_quota_cb_func_t cb_func, void *data) {
	ISC_LINK_INIT(cb, link);
	cb->cb_func = cb_func;
	cb->data = data;
	cb->magic = QUOTA_CB_MAGIC;
}

void
isc_quota_detach(isc_quota_t **quotap) {
	REQUIRE(quotap != NULL && VALID_QUOTA(*quotap));
	isc_quota_t *quota = *quotap;
	*quotap = NULL;

	quota_release(quota);
}
