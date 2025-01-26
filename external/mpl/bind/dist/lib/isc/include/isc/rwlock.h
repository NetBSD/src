/*	$NetBSD: rwlock.h,v 1.9 2025/01/26 16:25:42 christos Exp $	*/

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

#include <inttypes.h>
#include <stdlib.h>

/*! \file isc/rwlock.h */

#include <isc/lang.h>
#include <isc/types.h>
#include <isc/util.h>

ISC_LANG_BEGINDECLS

typedef enum {
	isc_rwlocktype_none = 0,
	isc_rwlocktype_read,
	isc_rwlocktype_write
} isc_rwlocktype_t;

#if USE_PTHREAD_RWLOCK
#include <pthread.h>

/*
 * We use macros instead of static inline functions so that the exact code
 * location can be reported when PTHREADS_RUNTIME_CHECK() fails or when mutrace
 * reports lock contention.
 */

#if ISC_TRACK_PTHREADS_OBJECTS

typedef pthread_rwlock_t *isc_rwlock_t;
typedef pthread_rwlock_t  isc__rwlock_t;

#define isc_rwlock_init(rwl)                  \
	{                                     \
		*rwl = malloc(sizeof(**rwl)); \
		isc__rwlock_init(*rwl);       \
	}
#define isc_rwlock_lock(rwl, type)    isc__rwlock_lock(*rwl, type)
#define isc_rwlock_trylock(rwl, type) isc__rwlock_trylock(*rwl, type)
#define isc_rwlock_unlock(rwl, type)  isc__rwlock_unlock(*rwl, type)
#define isc_rwlock_tryupgrade(rwl)    isc__rwlock_tryupgrade(*rwl)
#define isc_rwlock_destroy(rwl)            \
	{                                  \
		isc__rwlock_destroy(*rwl); \
		free(*rwl);                \
	}

#else /* ISC_TRACK_PTHREADS_OBJECTS */

typedef pthread_rwlock_t isc_rwlock_t;
typedef pthread_rwlock_t isc__rwlock_t;

#define isc_rwlock_init(rwl)	      isc__rwlock_init(rwl)
#define isc_rwlock_lock(rwl, type)    isc__rwlock_lock(rwl, type)
#define isc_rwlock_trylock(rwl, type) isc__rwlock_trylock(rwl, type)
#define isc_rwlock_unlock(rwl, type)  isc__rwlock_unlock(rwl, type)
#define isc_rwlock_tryupgrade(rwl)    isc__rwlock_tryupgrade(rwl)
#define isc_rwlock_destroy(rwl)	      isc__rwlock_destroy(rwl)

#endif /* ISC_TRACK_PTHREADS_OBJECTS */

#define isc__rwlock_init(rwl)                                      \
	{                                                          \
		int _ret = pthread_rwlock_init(rwl, NULL);         \
		PTHREADS_RUNTIME_CHECK(pthread_rwlock_init, _ret); \
	}

#define isc__rwlock_lock(rwl, type)                                          \
	{                                                                    \
		int _ret;                                                    \
		switch (type) {                                              \
		case isc_rwlocktype_read:                                    \
			_ret = pthread_rwlock_rdlock(rwl);                   \
			PTHREADS_RUNTIME_CHECK(pthread_rwlock_rdlock, _ret); \
			break;                                               \
		case isc_rwlocktype_write:                                   \
			_ret = pthread_rwlock_wrlock(rwl);                   \
			PTHREADS_RUNTIME_CHECK(pthread_rwlock_rwlock, _ret); \
			break;                                               \
		default:                                                     \
			UNREACHABLE();                                       \
		}                                                            \
	}

#define isc__rwlock_trylock(rwl, type)                                   \
	({                                                               \
		int	     _ret = 0;                                   \
		isc_result_t _res = ISC_R_UNSET;                         \
                                                                         \
		switch (type) {                                          \
		case isc_rwlocktype_read:                                \
			_ret = pthread_rwlock_tryrdlock(rwl);            \
			break;                                           \
		case isc_rwlocktype_write:                               \
			_ret = pthread_rwlock_trywrlock(rwl);            \
			break;                                           \
		default:                                                 \
			UNREACHABLE();                                   \
		}                                                        \
                                                                         \
		switch (_ret) {                                          \
		case 0:                                                  \
			_res = ISC_R_SUCCESS;                            \
			break;                                           \
		case EBUSY:                                              \
		case EAGAIN:                                             \
			_res = ISC_R_LOCKBUSY;                           \
			break;                                           \
		default:                                                 \
			switch (type) {                                  \
			case isc_rwlocktype_read:                        \
				PTHREADS_RUNTIME_CHECK(                  \
					pthread_rwlock_tryrdlock, _ret); \
				break;                                   \
			case isc_rwlocktype_write:                       \
				PTHREADS_RUNTIME_CHECK(                  \
					pthread_rwlock_trywrlock, _ret); \
				break;                                   \
			default:                                         \
				UNREACHABLE();                           \
			}                                                \
			UNREACHABLE();                                   \
		}                                                        \
		_res;                                                    \
	})

#define isc__rwlock_unlock(rwl, type)                                \
	{                                                            \
		int _ret = pthread_rwlock_unlock(rwl);               \
		UNUSED(type);                                        \
		PTHREADS_RUNTIME_CHECK(pthread_rwlock_rwlock, _ret); \
	}

#define isc__rwlock_tryupgrade(rwl) \
	({                          \
		UNUSED(rwl);        \
		ISC_R_LOCKBUSY;     \
	})

#define isc__rwlock_destroy(rwl)                                      \
	{                                                             \
		int _ret = pthread_rwlock_destroy(rwl);               \
		PTHREADS_RUNTIME_CHECK(pthread_rwlock_destroy, _ret); \
	}

#define isc_rwlock_setworkers(workers)

#else /* USE_PTHREAD_RWLOCK */

#include <isc/atomic.h>
#include <isc/os.h>

STATIC_ASSERT(ISC_OS_CACHELINE_SIZE >= sizeof(atomic_uint_fast32_t),
	      "ISC_OS_CACHELINE_SIZE smaller than "
	      "sizeof(atomic_uint_fast32_t)");
STATIC_ASSERT(ISC_OS_CACHELINE_SIZE >= sizeof(atomic_int_fast32_t),
	      "ISC_OS_CACHELINE_SIZE smaller than sizeof(atomic_int_fast32_t)");

struct isc_rwlock {
	atomic_uint_fast32_t readers_ingress;
	uint8_t __padding1[ISC_OS_CACHELINE_SIZE - sizeof(atomic_uint_fast32_t)];
	atomic_uint_fast32_t readers_egress;
	uint8_t __padding2[ISC_OS_CACHELINE_SIZE - sizeof(atomic_uint_fast32_t)];
	atomic_int_fast32_t writers_barrier;
	uint8_t __padding3[ISC_OS_CACHELINE_SIZE - sizeof(atomic_int_fast32_t)];
	atomic_bool writers_lock;
};

typedef struct isc_rwlock isc_rwlock_t;

void
isc_rwlock_init(isc_rwlock_t *rwl);

void
isc_rwlock_rdlock(isc_rwlock_t *rwl);

void
isc_rwlock_wrlock(isc_rwlock_t *rwl);

isc_result_t
isc_rwlock_tryrdlock(isc_rwlock_t *rwl);

isc_result_t
isc_rwlock_trywrlock(isc_rwlock_t *rwl);

void
isc_rwlock_rdunlock(isc_rwlock_t *rwl);

void
isc_rwlock_wrunlock(isc_rwlock_t *rwl);

isc_result_t
isc_rwlock_tryupgrade(isc_rwlock_t *rwl);

void
isc_rwlock_downgrade(isc_rwlock_t *rwl);

void
isc_rwlock_destroy(isc_rwlock_t *rwl);

void
isc_rwlock_setworkers(uint16_t workers);

#define isc_rwlock_lock(rwl, type)              \
	{                                       \
		switch (type) {                 \
		case isc_rwlocktype_read:       \
			isc_rwlock_rdlock(rwl); \
			break;                  \
		case isc_rwlocktype_write:      \
			isc_rwlock_wrlock(rwl); \
			break;                  \
		default:                        \
			UNREACHABLE();          \
		}                               \
	}

#define isc_rwlock_trylock(rwl, type)                         \
	({                                                    \
		int __result;                                 \
		switch (type) {                               \
		case isc_rwlocktype_read:                     \
			__result = isc_rwlock_tryrdlock(rwl); \
			break;                                \
		case isc_rwlocktype_write:                    \
			__result = isc_rwlock_trywrlock(rwl); \
			break;                                \
		default:                                      \
			UNREACHABLE();                        \
		}                                             \
		__result;                                     \
	})

#define isc_rwlock_unlock(rwl, type)              \
	{                                         \
		switch (type) {                   \
		case isc_rwlocktype_read:         \
			isc_rwlock_rdunlock(rwl); \
			break;                    \
		case isc_rwlocktype_write:        \
			isc_rwlock_wrunlock(rwl); \
			break;                    \
		default:                          \
			UNREACHABLE();            \
		}                                 \
	}

#endif /* USE_PTHREAD_RWLOCK */

ISC_LANG_ENDDECLS
