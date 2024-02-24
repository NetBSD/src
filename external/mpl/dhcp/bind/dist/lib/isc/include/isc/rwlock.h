/*	$NetBSD: rwlock.h,v 1.1.2.2 2024/02/24 13:07:27 martin Exp $	*/

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

#ifndef ISC_RWLOCK_H
#define ISC_RWLOCK_H 1

#include <inttypes.h>

/*! \file isc/rwlock.h */

#include <isc/atomic.h>
#include <isc/condition.h>
#include <isc/lang.h>
#include <isc/platform.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

typedef enum {
	isc_rwlocktype_none = 0,
	isc_rwlocktype_read,
	isc_rwlocktype_write
} isc_rwlocktype_t;

#if USE_PTHREAD_RWLOCK
#include <pthread.h>

struct isc_rwlock {
	pthread_rwlock_t rwlock;
	atomic_bool	 downgrade;
};

#else /* USE_PTHREAD_RWLOCK */

struct isc_rwlock {
	/* Unlocked. */
	unsigned int	    magic;
	isc_mutex_t	    lock;
	atomic_int_fast32_t spins;

	/*
	 * When some atomic instructions with hardware assistance are
	 * available, rwlock will use those so that concurrent readers do not
	 * interfere with each other through mutex as long as no writers
	 * appear, massively reducing the lock overhead in the typical case.
	 *
	 * The basic algorithm of this approach is the "simple
	 * writer-preference lock" shown in the following URL:
	 * http://www.cs.rochester.edu/u/scott/synchronization/pseudocode/rw.html
	 * but our implementation does not rely on the spin lock unlike the
	 * original algorithm to be more portable as a user space application.
	 */

	/* Read or modified atomically. */
	atomic_int_fast32_t write_requests;
	atomic_int_fast32_t write_completions;
	atomic_int_fast32_t cnt_and_flag;

	/* Locked by lock. */
	isc_condition_t readable;
	isc_condition_t writeable;
	unsigned int	readers_waiting;

	/* Locked by rwlock itself. */
	atomic_uint_fast32_t write_granted;

	/* Unlocked. */
	unsigned int write_quota;
};

#endif /* USE_PTHREAD_RWLOCK */

void
isc_rwlock_init(isc_rwlock_t *rwl, unsigned int read_quota,
		unsigned int write_quota);

isc_result_t
isc_rwlock_lock(isc_rwlock_t *rwl, isc_rwlocktype_t type);

isc_result_t
isc_rwlock_trylock(isc_rwlock_t *rwl, isc_rwlocktype_t type);

isc_result_t
isc_rwlock_unlock(isc_rwlock_t *rwl, isc_rwlocktype_t type);

isc_result_t
isc_rwlock_tryupgrade(isc_rwlock_t *rwl);

void
isc_rwlock_downgrade(isc_rwlock_t *rwl);

void
isc_rwlock_destroy(isc_rwlock_t *rwl);

ISC_LANG_ENDDECLS

#endif /* ISC_RWLOCK_H */
