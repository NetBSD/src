/*	$NetBSD: urcu.h,v 1.3 2026/01/29 18:37:55 christos Exp $	*/

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

#include <isc/util.h>

/* when urcu is not installed in a system header location */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if defined(RCU_MEMBARRIER) || defined(RCU_MB) || defined(RCU_SIGNAL)
#include <urcu.h>
#elif defined(RCU_QSBR)
#include <urcu-qsbr.h>
#elif defined(RCU_BP)
#include <urcu-bp.h>
#endif

#include <urcu-pointer.h>

#include <urcu/compiler.h>
#include <urcu/rculfhash.h>
#include <urcu/rculist.h>
#include <urcu/wfstack.h>

#pragma GCC diagnostic pop

#if defined(RCU_QSBR)

/*
 * Define wrappers that allows us to make the thread online without any extra
 * heavy tooling around libuv callbacks.
 */

#define isc_qsbr_read_lock()                       \
	{                                          \
		if (!urcu_qsbr_read_ongoing()) {   \
			urcu_qsbr_thread_online(); \
		}                                  \
		urcu_qsbr_read_lock();             \
	}

#undef rcu_read_lock
#define rcu_read_lock() isc_qsbr_read_lock()

#define isc_qsbr_call_rcu(rcu_head, func)           \
	{                                           \
		if (!urcu_qsbr_read_ongoing()) {    \
			urcu_qsbr_thread_online();  \
		}                                   \
		urcu_qsbr_call_rcu(rcu_head, func); \
	}

#undef call_rcu
#define call_rcu(rcu_head, func) isc_qsbr_call_rcu(rcu_head, func)

#define isc_qsbr_synchronize_rcu()                 \
	{                                          \
		if (!urcu_qsbr_read_ongoing()) {   \
			urcu_qsbr_thread_online(); \
		}                                  \
		urcu_qsbr_synchronize_rcu();       \
	}

#undef synchronize_rcu
#define synchronize_rcu() isc_qsbr_synchronize_rcu()

#define isc_qsbr_rcu_dereference(ptr)              \
	({                                         \
		if (!urcu_qsbr_read_ongoing()) {   \
			urcu_qsbr_thread_online(); \
		}                                  \
		_rcu_dereference(ptr);             \
	})

#undef rcu_dereference
#define rcu_dereference(ptr) isc_qsbr_rcu_dereference(ptr)

#endif /* RCU_QSBR */

/* clang-format off */
/*
 * Following definitions were copied from liburcu development branch to help
 * with AddressSanitizer complaining about calling caa_container_of on NULL.
 */

#if !defined(caa_container_of_check_null)
/*
 * caa_container_of_check_null - Get the address of an object containing a field.
 *
 * @ptr: pointer to the field.
 * @type: type of the object.
 * @member: name of the field within the object.
 *
 * Return the address of the object containing the field. Return NULL if
 * @ptr is NULL.
 */
#define caa_container_of_check_null(ptr, type, member)			\
	__extension__							\
	({								\
		const __typeof__(((type *) NULL)->member) * __ptr = (ptr); \
		(__ptr) ? (type *)((char *)__ptr - offsetof(type, member)) : NULL; \
	})

#define cds_lfht_entry(ptr, type, member)				\
	caa_container_of_check_null(ptr, type, member)

#undef cds_lfht_for_each_entry
#define cds_lfht_for_each_entry(ht, iter, pos, member)			\
	for (cds_lfht_first(ht, iter),					\
			pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				__typeof__(*(pos)), member);		\
		pos != NULL;						\
		cds_lfht_next(ht, iter),				\
			pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				__typeof__(*(pos)), member))

#undef cds_lfht_for_each_entry_duplicate
#define cds_lfht_for_each_entry_duplicate(ht, hash, match, key,		\
				iter, pos, member)			\
	for (cds_lfht_lookup(ht, hash, match, key, iter),		\
			pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				__typeof__(*(pos)), member);		\
		pos != NULL;						\
		cds_lfht_next_duplicate(ht, match, key, iter),		\
			pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				__typeof__(*(pos)), member))

#endif /* !defined(caa_container_of_check_null) */
/* clang-format on */

#ifdef __SANITIZE_THREAD__

/*
 * Restore the behaviour removed in
 * https://github.com/urcu/userspace-rcu/commit/5cd787d0f953182a23d340669b20b150fd50c18c
 * as we only use CMM_LOAD_SHARED() and CMM_STORE_SHARED() with atomic types.
 */

#undef CMM_LOAD_SHARED
#define CMM_LOAD_SHARED(x) \
	__atomic_load_n(cmm_cast_volatile(&(x)), __ATOMIC_RELAXED)

#undef _CMM_LOAD_SHARED
#define _CMM_LOAD_SHARED(x) CMM_LOAD_SHARED(x)

#undef CMM_STORE_SHARED
#define CMM_STORE_SHARED(x, v)                                \
	__extension__({                                       \
		__typeof__(v) _v = (v);                       \
		__atomic_store_n(cmm_cast_volatile(&(x)), _v, \
				 __ATOMIC_RELAXED);           \
		_v;                                           \
	})

#undef _CMM_STORE_SHARED
#define _CMM_STORE_SHARED(x, v) CMM_STORE_SHARED(x, v)

#endif
