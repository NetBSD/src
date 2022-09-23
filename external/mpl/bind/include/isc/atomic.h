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

#ifdef ISC_MUTEX_ATOMICS
#include <isc/mutexatomic.h>
#else /* ifdef ISC_MUTEX_ATOMICS */
#if HAVE_STDATOMIC_H
#include <stdatomic.h>
#else /* if HAVE_STDATOMIC_H */
#include <isc/stdatomic.h>
#endif /* if HAVE_STDATOMIC_H */
#endif /* ifdef ISC_MUTEX_ATOMICS */

/*
 * We define a few additional macros to make things easier
 */

/* Relaxed Memory Ordering */

#define atomic_store_relaxed(o, v) \
	atomic_store_explicit((o), (v), memory_order_relaxed)
#define atomic_load_relaxed(o) atomic_load_explicit((o), memory_order_relaxed)
#define atomic_fetch_add_relaxed(o, v) \
	atomic_fetch_add_explicit((o), (v), memory_order_relaxed)
#define atomic_fetch_sub_relaxed(o, v) \
	atomic_fetch_sub_explicit((o), (v), memory_order_relaxed)
#define atomic_fetch_or_relaxed(o, v) \
	atomic_fetch_or_explicit((o), (v), memory_order_relaxed)
#define atomic_fetch_and_relaxed(o, v) \
	atomic_fetch_and_explicit((o), (v), memory_order_relaxed)
#define atomic_exchange_relaxed(o, v) \
	atomic_exchange_explicit((o), (v), memory_order_relaxed)
#define atomic_compare_exchange_weak_relaxed(o, e, d) \
	atomic_compare_exchange_weak_explicit(        \
		(o), (e), (d), memory_order_relaxed, memory_order_relaxed)
#define atomic_compare_exchange_strong_relaxed(o, e, d) \
	atomic_compare_exchange_strong_explicit(        \
		(o), (e), (d), memory_order_relaxed, memory_order_relaxed)
#define atomic_compare_exchange_strong_acq_rel(o, e, d) \
	atomic_compare_exchange_strong_explicit(        \
		(o), (e), (d), memory_order_acq_rel, memory_order_acquire)

/* Acquire-Release Memory Ordering */

#define atomic_store_release(o, v) \
	atomic_store_explicit((o), (v), memory_order_release)
#define atomic_load_acquire(o) atomic_load_explicit((o), memory_order_acquire)
#define atomic_fetch_add_release(o, v) \
	atomic_fetch_add_explicit((o), (v), memory_order_release)
#define atomic_fetch_sub_release(o, v) \
	atomic_fetch_sub_explicit((o), (v), memory_order_release)
#define atomic_fetch_and_release(o, v) \
	atomic_fetch_and_explicit((o), (v), memory_order_release)
#define atomic_fetch_or_release(o, v) \
	atomic_fetch_or_explicit((o), (v), memory_order_release)
#define atomic_exchange_acq_rel(o, v) \
	atomic_exchange_explicit((o), (v), memory_order_acq_rel)
#define atomic_fetch_sub_acq_rel(o, v) \
	atomic_fetch_sub_explicit((o), (v), memory_order_acq_rel)
#define atomic_compare_exchange_weak_acq_rel(o, e, d) \
	atomic_compare_exchange_weak_explicit(        \
		(o), (e), (d), memory_order_acq_rel, memory_order_acquire)
#define atomic_compare_exchange_strong_acq_rel(o, e, d) \
	atomic_compare_exchange_strong_explicit(        \
		(o), (e), (d), memory_order_acq_rel, memory_order_acquire)
