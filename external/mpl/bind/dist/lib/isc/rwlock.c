/*	$NetBSD: rwlock.c,v 1.15 2025/01/26 16:25:38 christos Exp $	*/

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

/*
 * Modified C-RW-WP Implementation from NUMA-Aware Reader-Writer Locks paper:
 * http://dl.acm.org/citation.cfm?id=2442532
 *
 * This work is based on C++ code available from
 * https://github.com/pramalhe/ConcurrencyFreaks/
 *
 * Copyright (c) 2014-2016, Pedro Ramalhete, Andreia Correia
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Concurrency Freaks nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/atomic.h>
#include <isc/hash.h>
#include <isc/pause.h>
#include <isc/rwlock.h>
#include <isc/thread.h>
#include <isc/tid.h>
#include <isc/util.h>

#include "probes.h"

static atomic_uint_fast16_t isc__crwlock_workers = 128;

#define ISC_RWLOCK_UNLOCKED false
#define ISC_RWLOCK_LOCKED   true

/*
 * See https://csce.ucmss.com/cr/books/2017/LFS/CSREA2017/FCS3701.pdf for
 * guidance on patience level
 */
#ifndef RWLOCK_MAX_READER_PATIENCE
#define RWLOCK_MAX_READER_PATIENCE 500
#endif /* ifndef RWLOCK_MAX_READER_PATIENCE */

static void
read_indicator_wait_until_empty(isc_rwlock_t *rwl);

#include <stdio.h>

static void
read_indicator_arrive(isc_rwlock_t *rwl) {
	(void)atomic_fetch_add_release(&rwl->readers_ingress, 1);
}

static void
read_indicator_depart(isc_rwlock_t *rwl) {
	(void)atomic_fetch_add_release(&rwl->readers_egress, 1);
}

static bool
read_indicator_isempty(isc_rwlock_t *rwl) {
	return atomic_load_acquire(&rwl->readers_egress) ==
	       atomic_load_acquire(&rwl->readers_ingress);
}

static void
writers_barrier_raise(isc_rwlock_t *rwl) {
	(void)atomic_fetch_add_release(&rwl->writers_barrier, 1);
}

static void
writers_barrier_lower(isc_rwlock_t *rwl) {
	(void)atomic_fetch_sub_release(&rwl->writers_barrier, 1);
}

static bool
writers_barrier_israised(isc_rwlock_t *rwl) {
	return atomic_load_acquire(&rwl->writers_barrier) > 0;
}

static bool
writers_lock_islocked(isc_rwlock_t *rwl) {
	return atomic_load_acquire(&rwl->writers_lock) == ISC_RWLOCK_LOCKED;
}

static bool
writers_lock_acquire(isc_rwlock_t *rwl) {
	return atomic_compare_exchange_weak_acq_rel(
		&rwl->writers_lock, &(bool){ ISC_RWLOCK_UNLOCKED },
		ISC_RWLOCK_LOCKED);
}

static void
writers_lock_release(isc_rwlock_t *rwl) {
	REQUIRE(atomic_compare_exchange_strong_acq_rel(
		&rwl->writers_lock, &(bool){ ISC_RWLOCK_LOCKED },
		ISC_RWLOCK_UNLOCKED));
}

#define ran_out_of_patience(cnt) (cnt >= RWLOCK_MAX_READER_PATIENCE)

void
isc_rwlock_rdlock(isc_rwlock_t *rwl) {
	uint32_t cnt = 0;
	bool barrier_raised = false;

	LIBISC_RWLOCK_RDLOCK_REQ(rwl);

	while (true) {
		read_indicator_arrive(rwl);
		if (!writers_lock_islocked(rwl)) {
			/* Acquired lock in read-only mode */
			break;
		}

		/* Writer has acquired the lock, must reset to 0 and wait */
		read_indicator_depart(rwl);

		while (writers_lock_islocked(rwl)) {
			isc_pause();
			if (ran_out_of_patience(cnt++) && !barrier_raised) {
				writers_barrier_raise(rwl);
				barrier_raised = true;
			}
		}
	}
	if (barrier_raised) {
		writers_barrier_lower(rwl);
	}

	LIBISC_RWLOCK_RDLOCK_ACQ(rwl);
}

isc_result_t
isc_rwlock_tryrdlock(isc_rwlock_t *rwl) {
	read_indicator_arrive(rwl);
	if (writers_lock_islocked(rwl)) {
		/* Writer has acquired the lock, release the read lock */
		read_indicator_depart(rwl);

		LIBISC_RWLOCK_TRYRDLOCK(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	/* Acquired lock in read-only mode */
	LIBISC_RWLOCK_TRYRDLOCK(rwl, ISC_R_SUCCESS);
	return ISC_R_SUCCESS;
}

void
isc_rwlock_rdunlock(isc_rwlock_t *rwl) {
	read_indicator_depart(rwl);
	LIBISC_RWLOCK_RDUNLOCK(rwl);
}

isc_result_t
isc_rwlock_tryupgrade(isc_rwlock_t *rwl) {
	/* Write Barriers has been raised */
	if (writers_barrier_israised(rwl)) {
		LIBISC_RWLOCK_TRYUPGRADE(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	/* Try to acquire the write-lock */
	if (!writers_lock_acquire(rwl)) {
		LIBISC_RWLOCK_TRYUPGRADE(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	/* Unlock the read-lock */
	read_indicator_depart(rwl);

	if (!read_indicator_isempty(rwl)) {
		/* Re-acquire the read-lock back */
		read_indicator_arrive(rwl);

		/* Unlock the write-lock */
		writers_lock_release(rwl);
		LIBISC_RWLOCK_TRYUPGRADE(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}
	LIBISC_RWLOCK_TRYUPGRADE(rwl, ISC_R_SUCCESS);
	return ISC_R_SUCCESS;
}

static void
read_indicator_wait_until_empty(isc_rwlock_t *rwl) {
	/* Write-lock was acquired, now wait for running Readers to finish */
	while (true) {
		if (read_indicator_isempty(rwl)) {
			break;
		}
		isc_pause();
	}
}

void
isc_rwlock_wrlock(isc_rwlock_t *rwl) {
	LIBISC_RWLOCK_WRLOCK_REQ(rwl);

	/* Write Barriers has been raised, wait */
	while (writers_barrier_israised(rwl)) {
		isc_pause();
	}

	/* Try to acquire the write-lock */
	while (!writers_lock_acquire(rwl)) {
		isc_pause();
	}

	read_indicator_wait_until_empty(rwl);

	LIBISC_RWLOCK_WRLOCK_ACQ(rwl);
}

void
isc_rwlock_wrunlock(isc_rwlock_t *rwl) {
	writers_lock_release(rwl);
	LIBISC_RWLOCK_WRUNLOCK(rwl);
}

isc_result_t
isc_rwlock_trywrlock(isc_rwlock_t *rwl) {
	/* Write Barriers has been raised */
	if (writers_barrier_israised(rwl)) {
		LIBISC_RWLOCK_TRYWRLOCK(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	/* Try to acquire the write-lock */
	if (!writers_lock_acquire(rwl)) {
		LIBISC_RWLOCK_TRYWRLOCK(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	if (!read_indicator_isempty(rwl)) {
		/* Unlock the write-lock */
		writers_lock_release(rwl);

		LIBISC_RWLOCK_TRYWRLOCK(rwl, ISC_R_LOCKBUSY);
		return ISC_R_LOCKBUSY;
	}

	LIBISC_RWLOCK_TRYWRLOCK(rwl, ISC_R_SUCCESS);
	return ISC_R_SUCCESS;
}

void
isc_rwlock_downgrade(isc_rwlock_t *rwl) {
	read_indicator_arrive(rwl);

	writers_lock_release(rwl);

	LIBISC_RWLOCK_DOWNGRADE(rwl);
}

void
isc_rwlock_init(isc_rwlock_t *rwl) {
	REQUIRE(rwl != NULL);

	atomic_init(&rwl->writers_lock, ISC_RWLOCK_UNLOCKED);
	atomic_init(&rwl->writers_barrier, 0);
	atomic_init(&rwl->readers_ingress, 0);
	atomic_init(&rwl->readers_egress, 0);
	LIBISC_RWLOCK_INIT(rwl);
}

void
isc_rwlock_destroy(isc_rwlock_t *rwl) {
	LIBISC_RWLOCK_DESTROY(rwl);
	/* Check whether write lock has been unlocked */
	REQUIRE(atomic_load(&rwl->writers_lock) == ISC_RWLOCK_UNLOCKED);
	REQUIRE(read_indicator_isempty(rwl));
}

void
isc_rwlock_setworkers(uint16_t workers) {
	atomic_store(&isc__crwlock_workers, workers);
}
