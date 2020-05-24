/*	$NetBSD: mutexatomic.h,v 1.2 2020/05/24 19:46:26 christos Exp $	*/

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

#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include <isc/mutex.h>

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif /* if !defined(__has_feature) */

#if !defined(__has_extension)
#define __has_extension(x) __has_feature(x)
#endif /* if !defined(__has_extension) */

#if !defined(__GNUC_PREREQ__)
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define __GNUC_PREREQ__(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else /* if defined(__GNUC__) && defined(__GNUC_MINOR__) */
#define __GNUC_PREREQ__(maj, min) 0
#endif /* if defined(__GNUC__) && defined(__GNUC_MINOR__) */
#endif /* if !defined(__GNUC_PREREQ__) */

#if !defined(__CLANG_ATOMICS) && !defined(__GNUC_ATOMICS)
#if __has_extension(c_atomic) || __has_extension(cxx_atomic)
#define __CLANG_ATOMICS
#elif __GNUC_PREREQ__(4, 7)
#define __GNUC_ATOMICS
#elif !defined(__GNUC__)
#error "isc/stdatomic.h does not support your compiler"
#endif /* if __has_extension(c_atomic) || __has_extension(cxx_atomic) */
#endif /* if !defined(__CLANG_ATOMICS) && !defined(__GNUC_ATOMICS) */

#ifndef __ATOMIC_RELAXED
#define __ATOMIC_RELAXED 0
#endif /* ifndef __ATOMIC_RELAXED */
#ifndef __ATOMIC_CONSUME
#define __ATOMIC_CONSUME 1
#endif /* ifndef __ATOMIC_CONSUME */
#ifndef __ATOMIC_ACQUIRE
#define __ATOMIC_ACQUIRE 2
#endif /* ifndef __ATOMIC_ACQUIRE */
#ifndef __ATOMIC_RELEASE
#define __ATOMIC_RELEASE 3
#endif /* ifndef __ATOMIC_RELEASE */
#ifndef __ATOMIC_ACQ_REL
#define __ATOMIC_ACQ_REL 4
#endif /* ifndef __ATOMIC_ACQ_REL */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif /* ifndef __ATOMIC_SEQ_CST */

enum memory_order {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
};

typedef enum memory_order memory_order;

typedef struct atomic_int_fast32 {
	isc_mutex_t  m;
	int_fast32_t v;
} atomic_int_fast32_t;

typedef struct atomic_int_fast64 {
	isc_mutex_t  m;
	int_fast64_t v;
} atomic_int_fast64_t;

typedef struct atomic_uint_fast32 {
	isc_mutex_t   m;
	uint_fast32_t v;
} atomic_uint_fast32_t;

typedef struct atomic_uint_fast64 {
	isc_mutex_t   m;
	uint_fast64_t v;
} atomic_uint_fast64_t;

typedef struct atomic_uintptr {
	isc_mutex_t m;
	uintptr_t   v;
} atomic_uintptr_t;

typedef struct atomic_bool_s {
	isc_mutex_t m;
	bool	    v;
} atomic_bool;

#define ATOMIC_VAR_INIT(arg)                             \
	{                                                \
		.m = PTHREAD_MUTEX_INITIALIZER, .v = arg \
	}

#define atomic_init(obj, desired)          \
	{                                  \
		isc_mutex_init(&(obj)->m); \
		(obj)->v = desired;        \
	}
#define atomic_load_explicit(obj, order)                               \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_store_explicit(obj, desired, order)                     \
	{                                                              \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		(obj)->v = desired;                                    \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
	}
#define atomic_fetch_add_explicit(obj, arg, order)                     \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		(obj)->v += arg;                                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_fetch_sub_explicit(obj, arg, order)                     \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		(obj)->v -= arg;                                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_fetch_and_explicit(obj, arg, order)                     \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		(obj)->v &= arg;                                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_fetch_or_explicit(obj, arg, order)                      \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		(obj)->v |= arg;                                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, \
						fail)                         \
	({                                                                    \
		bool ___v;                                                    \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);          \
		___v = ((obj)->v == *expected);                               \
		*expected = (obj)->v;                                         \
		(obj)->v = ___v ? desired : (obj)->v;                         \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS);        \
		___v;                                                         \
	})
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, \
					      fail)                         \
	({                                                                  \
		bool ___v;                                                  \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);        \
		___v = ((obj)->v == *expected);                             \
		*expected = (obj)->v;                                       \
		(obj)->v = ___v ? desired : (obj)->v;                       \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS);      \
		___v;                                                       \
	})

#define atomic_load(obj) atomic_load_explicit(obj, memory_order_seq_cst)
#define atomic_store(obj, arg) \
	atomic_store_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_add(obj, arg) \
	atomic_fetch_add_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_sub(obj, arg) \
	atomic_fetch_sub_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_and(obj, arg) \
	atomic_fetch_and_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_or(obj, arg) \
	atomic_fetch_or_explicit(obj, arg, memory_order_seq_cst)
#define atomic_compare_exchange_strong(obj, expected, desired)          \
	atomic_compare_exchange_strong_explicit(obj, expected, desired, \
						memory_order_seq_cst,   \
						memory_order_seq_cst)
#define atomic_compare_exchange_weak(obj, expected, desired)          \
	atomic_compare_exchange_weak_explicit(obj, expected, desired, \
					      memory_order_seq_cst,   \
					      memory_order_seq_cst)
#define atomic_exchange_explicit(obj, desired, order)                  \
	({                                                             \
		typeof((obj)->v) ___v;                                 \
		REQUIRE(isc_mutex_lock(&(obj)->m) == ISC_R_SUCCESS);   \
		___v = (obj)->v;                                       \
		(obj)->v = desired;                                    \
		REQUIRE(isc_mutex_unlock(&(obj)->m) == ISC_R_SUCCESS); \
		___v;                                                  \
	})
#define atomic_exchange(obj, desired) \
	atomic_exchange_explicit(obj, desired, memory_order_seq_cst)
