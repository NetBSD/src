/*	$NetBSD: quota.c,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stddef.h>

#include <isc/atomic.h>
#include <isc/quota.h>
#include <isc/util.h>


void
isc_quota_init(isc_quota_t *quota, unsigned int max) {
	atomic_store(&quota->max, max);
	atomic_store(&quota->used, 0);
	atomic_store(&quota->soft, 0);
}

void
isc_quota_destroy(isc_quota_t *quota) {
	INSIST(atomic_load(&quota->used) == 0);
	atomic_store(&quota->max, 0);
	atomic_store(&quota->used, 0);
	atomic_store(&quota->soft, 0);
}

void
isc_quota_soft(isc_quota_t *quota, unsigned int soft) {
	atomic_store(&quota->soft, soft);
}

void
isc_quota_max(isc_quota_t *quota, unsigned int max) {
	atomic_store(&quota->max, max);
}

unsigned int
isc_quota_getmax(isc_quota_t *quota) {
	return (atomic_load_relaxed(&quota->max));
}

unsigned int
isc_quota_getsoft(isc_quota_t *quota) {
	return (atomic_load_relaxed(&quota->soft));
}

unsigned int
isc_quota_getused(isc_quota_t *quota) {
	return (atomic_load_relaxed(&quota->used));
}

isc_result_t
isc_quota_reserve(isc_quota_t *quota) {
	isc_result_t result;
	uint32_t max = atomic_load(&quota->max);
	uint32_t soft = atomic_load(&quota->soft);
	uint32_t used = atomic_fetch_add(&quota->used, 1);
	if (max == 0 || used < max) {
		if (soft == 0 || used < soft) {
			result = ISC_R_SUCCESS;
		} else {
			result = ISC_R_SOFTQUOTA;
		}
	} else {
		INSIST(atomic_fetch_sub(&quota->used, 1) > 0);
		result = ISC_R_QUOTA;
	}
	return (result);
}

void
isc_quota_release(isc_quota_t *quota) {
	INSIST(atomic_fetch_sub(&quota->used, 1) > 0);
}

isc_result_t
isc_quota_attach(isc_quota_t *quota, isc_quota_t **p)
{
	isc_result_t result;
	INSIST(p != NULL && *p == NULL);
	result = isc_quota_reserve(quota);
	if (result == ISC_R_SUCCESS || result == ISC_R_SOFTQUOTA)
		*p = quota;
	return (result);
}

void
isc_quota_detach(isc_quota_t **p)
{
	INSIST(p != NULL && *p != NULL);
	isc_quota_release(*p);
	*p = NULL;
}
