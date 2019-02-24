/*	$NetBSD: stdatomic.h,v 1.3 2019/02/24 20:01:32 christos Exp $	*/

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

#define WIN32_LEAN_AND_MEAN
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <intrin.h>

#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8
#define InterlockedCompareExchange8 _InterlockedCompareExchange8

#pragma intrinsic(_InterlockedCompareExchange8, _InterlockedExchangeAdd8)

#include <isc/util.h>

#ifndef __ATOMIC_RELAXED
#define __ATOMIC_RELAXED        0
#endif
#ifndef __ATOMIC_CONSUME
#define __ATOMIC_CONSUME        1
#endif
#ifndef __ATOMIC_ACQUIRE
#define __ATOMIC_ACQUIRE        2
#endif
#ifndef __ATOMIC_RELEASE
#define __ATOMIC_RELEASE        3
#endif
#ifndef __ATOMIC_ACQ_REL
#define __ATOMIC_ACQ_REL        4
#endif
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST        5
#endif

enum memory_order {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
};

typedef enum memory_order memory_order;

/*
 * If you add a type with different sizeof() length,
 * you need to implement atomic_<foo>_explicitNN macros.
 */

typedef bool volatile		atomic_bool;
typedef int_fast8_t volatile	atomic_int_fast8_t;
typedef uint_fast8_t volatile	atomic_uint_fast8_t;
typedef int_fast32_t volatile	atomic_int_fast32_t;
typedef uint_fast32_t volatile	atomic_uint_fast32_t;
typedef int_fast64_t volatile	atomic_int_fast64_t;
typedef uint_fast64_t volatile	atomic_uint_fast64_t;

#define atomic_init(obj, desired)				\
	(*(obj) = (desired))

#define atomic_store_explicit8(obj, desired, order) \
	(void)InterlockedExchange8((atomic_int_fast8_t *)obj, desired)

#define atomic_store_explicit32(obj, desired, order)		\
	(order == memory_order_relaxed				\
	 ? (void)InterlockedExchangeNoFence((atomic_int_fast32_t *)obj, desired) \
	 : (order == memory_order_acquire			\
	    ? (void)InterlockedExchangeAcquire((atomic_int_fast32_t *)obj, desired) \
	    : (void)InterlockedExchange((atomic_int_fast32_t *)obj, desired)))

#ifdef _WIN64
#define atomic_store_explicit64(obj, desired, order)		\
	(order == memory_order_relaxed				\
	 ? (void)InterlockedExchangeNoFence64((atomic_int_fast64_t *)obj, desired) \
	 : (order == memory_order_acquire			\
	    ? (void)InterlockedExchangeAcquire64((atomic_int_fast64_t *)obj, desired) \
	    : (void)InterlockedExchange64((atomic_int_fast64_t *)obj, desired)))
#else
#define atomic_store_explicit64(obj, desired, order)		\
	(void)InterlockedExchange64((atomic_int_fast64_t *)obj, desired)
#endif

static inline
void
atomic_store_abort() {
	INSIST(0);
	ISC_UNREACHABLE();
}

#define atomic_store_explicit(obj, desired, order) 		\
	(sizeof(*(obj)) == 8					\
	 ? atomic_store_explicit64(obj, desired, order)		\
	 : (sizeof(*(obj)) == 4					\
	    ? atomic_store_explicit32(obj, desired, order)	\
	    : (sizeof(*(obj)) == 1				\
	       ? atomic_store_explicit8(obj, desired, order)	\
	       : atomic_store_abort())))

#define atomic_store(obj, desired) \
	atomic_store_explicit(obj, desired, memory_order_seq_cst)

#define atomic_load_explicit8(obj, order)			\
	(int8_t)InterlockedOr8((atomic_int_fast8_t *)obj, 0)

#define atomic_load_explicit32(obj, order)				\
	(order == memory_order_relaxed					\
	 ? (int32_t)InterlockedOrNoFence((atomic_int_fast32_t *)obj, 0)	\
	 : (order == memory_order_acquire				\
	    ? (int32_t)InterlockedOrAcquire((atomic_int_fast32_t *)obj, 0) \
	    : (order == memory_order_release				\
	       ? (int32_t)InterlockedOrRelease((atomic_int_fast32_t *)obj, 0) \
	       : (int32_t)InterlockedOr((atomic_int_fast32_t *)obj, 0))))

#ifdef _WIN64
#define atomic_load_explicit64(obj, order)				\
	(order == memory_order_relaxed					\
	 ? InterlockedOr64NoFence((atomic_int_fast64_t *)obj, 0)	\
	 : (order == memory_order_acquire				\
	    ? InterlockedOr64Acquire((atomic_int_fast64_t *)obj, 0)	\
	    : (order == memory_order_release				\
	       ? InterlockedOr64Release((atomic_int_fast64_t *)obj, 0)	\
	       : InterlockedOr64((atomic_int_fast64_t *)obj, 0))))
#else
#define atomic_load_explicit64(obj, order)				\
	InterlockedOr64((atomic_int_fast64_t *)obj, 0)
#endif

static inline
int8_t
atomic_load_abort() {
	INSIST(0);
	ISC_UNREACHABLE();
}

#define atomic_load_explicit(obj, order)		\
	((sizeof(*(obj)) == 8				\
	 ? atomic_load_explicit64(obj, order)		\
	 : (sizeof(*(obj) == 4)				\
	    ? atomic_load_explicit32(obj, order)	\
	    : (sizeof(*(obj) == 1)			\
	       ? atomic_load_explicit8(obj, order)	\
	       : atomic_load_abort()))) & 		\
	 (sizeof(*(obj)) == 8 ? 0xffffffffffffffffULL : \
	  (sizeof(*(obj)) == 4 ? 0xffffffffULL :	\
	   (sizeof(*(obj)) == 1 ? 0xffULL : atomic_load_abort()))))


#define atomic_load(obj)					\
	atomic_load_explicit(obj, memory_order_seq_cst)

#define atomic_fetch_add_explicit8(obj, arg, order)	\
	InterlockedExchangeAdd8((atomic_int_fast8_t *)obj, arg)

#define atomic_fetch_add_explicit32(obj, arg, order)		\
	(order == memory_order_relaxed				\
	 ? InterlockedExchangeAddNoFence((atomic_int_fast32_t *)obj, arg)	\
	 : (order == memory_order_acquire			\
	    ? InterlockedExchangeAddAcquire((atomic_int_fast32_t *)obj, arg) \
	    : (order == memory_order_release			\
	       ? InterlockedExchangeAddRelease((atomic_int_fast32_t *)obj, arg) \
	       : InterlockedExchangeAdd((atomic_int_fast32_t *)obj, arg))))

#ifdef _WIN64
#define atomic_fetch_add_explicit64(obj, arg, order)		\
	(order == memory_order_relaxed				\
	 ? InterlockedExchangeAddNoFence64((atomic_int_fast64_t *)obj, arg) \
	 : (order == memory_order_acquire			\
	    ? InterlockedExchangeAddAcquire64((atomic_int_fast64_t *)obj, arg) \
	    : (order == memory_order_release			\
	       ? InterlockedExchangeAddRelease64((atomic_int_fast64_t *)obj, arg) \
	       : InterlockedExchangeAdd64((atomic_int_fast64_t *)obj, arg))))
#else
#define atomic_fetch_add_explicit64(obj, arg, order)	\
	InterlockedExchange64((atomic_int_fast64_t *)obj, arg)
#endif

static inline
int8_t
atomic_add_abort() {
	INSIST(0);
	ISC_UNREACHABLE();
}

#define atomic_fetch_add_explicit(obj, arg, order)		\
	(sizeof(*(obj)) == 8					\
	 ? atomic_fetch_add_explicit64(obj, arg, order)		\
	 : (sizeof(*(obj)) == 4					\
	    ? atomic_fetch_add_explicit32(obj, arg, order)	\
	    : (sizeof(*(obj)) == 1				\
	       ? atomic_fetch_add_explicit8(obj, arg, order)	\
	       : atomic_add_abort())))

#define atomic_fetch_add(obj, arg)				\
	atomic_fetch_add_explicit(obj, arg, memory_order_seq_cst)

#define atomic_fetch_sub_explicit(obj, arg, order)		\
	atomic_fetch_add_explicit(obj, -arg, order)

#define atomic_fetch_sub(obj, arg)				\
	atomic_fetch_sub_explicit(obj, arg, memory_order_seq_cst)

static inline bool
atomic_compare_exchange_strong_explicit8(atomic_int_fast8_t *obj,
					 int8_t *expected,
					 int8_t desired,
					 memory_order succ,
					 memory_order fail)
{
	bool __r;
	int8_t __v;
	REQUIRE(succ == fail);
	__v = InterlockedCompareExchange8((atomic_int_fast8_t *)obj, desired, *expected);
	__r = (*(expected) == __v);
	if (!__r) {
		*(expected) = __v;
	}
	return (__r);
}

static inline bool
atomic_compare_exchange_strong_explicit32(atomic_int_fast32_t *obj,
					  int32_t *expected,
					  int32_t desired,
					  memory_order succ,
					  memory_order fail) {
	bool __r;
	int32_t __v;
	REQUIRE(succ == fail);
	switch (succ) {
	case memory_order_relaxed:
		__v = InterlockedCompareExchangeNoFence((atomic_int_fast32_t *)obj, desired, *expected);
		break;
	case memory_order_acquire:
		__v = InterlockedCompareExchangeAcquire((atomic_int_fast32_t *)obj, desired, *expected);
		break;
	case memory_order_release:
		__v = InterlockedCompareExchangeRelease((atomic_int_fast32_t *)obj, desired, *expected);
		break;
	default:
		__v = InterlockedCompareExchange((atomic_int_fast32_t *)obj, desired, *expected);
		break;
	}
	__r = (*(expected) == __v);
	if (!__r) {
		*(expected) = __v;
	}
	return (__r);
}

static inline bool
atomic_compare_exchange_strong_explicit64(atomic_int_fast64_t *obj,
					  int64_t *expected,
					  int64_t desired,
					  memory_order succ,
					  memory_order fail) {
	bool __r;
	int64_t __v;
	REQUIRE(succ == fail);
#ifdef _WIN64
	switch (succ) {
	case memory_order_relaxed:
		__v = InterlockedCompareExchangeNoFence64((atomic_int_fast64_t *)obj, desired, *expected);
		break;
	case memory_order_acquire:
		__v = InterlockedCompareExchangeAcquire64((atomic_int_fast64_t *)obj, desired, *expected);
		break;
	case memory_order_release:
		__v = InterlockedCompareExchangeRelease64((atomic_int_fast64_t *)obj, desired, *expected);
		break;
	default:
		__v = InterlockedCompareExchange64((atomic_int_fast64_t *)obj, desired, *expected);
		break;
	}
#else
	__v = InterlockedCompareExchange64((atomic_int_fast64_t *)obj, desired, *expected);
#endif
	__r = (*(expected) == __v);
	if (!__r) {
		*(expected) = __v;
	}
	return (__r);
}

static inline
bool
atomic_compare_exchange_abort() {
	INSIST(0);
	ISC_UNREACHABLE();
}

#define atomic_compare_exchange_strong_explicit(obj, expected, desired, \
						succ, fail)		\
	(sizeof(*(obj)) == 8						\
	 ? atomic_compare_exchange_strong_explicit64(obj, expected,	\
						     desired,		\
						     succ, fail)	\
	 : (sizeof(*(obj)) == 4						\
	    ? atomic_compare_exchange_strong_explicit32(obj, expected,	\
							desired,	\
							succ, fail)	\
		: (sizeof(*(obj)) == 1					\
		? atomic_compare_exchange_strong_explicit8(obj, expected, \
							   desired,	\
							   succ, fail)	\
		    : atomic_compare_exchange_abort())))

#define atomic_compare_exchange_strong(obj, expected, desired,		\
				       succ, fail)			\
	atomic_compare_exchange_strong_explicit(obj, expected, desired, \
						memory_order_seq_cst,	\
						memory_order_seq_cst)

#define atomic_compare_exchange_weak_explicit(obj, expected, desired,	\
					      succ, fail)		\
	atomic_compare_exchange_strong_explicit(obj, expected, desired, \
						succ, fail)

#define atomic_compare_exchange_weak(obj, expected, desired)		\
	atomic_compare_exchange_weak_explicit(obj, expected, desired,	\
					      memory_order_seq_cst,	\
					      memory_order_seq_cst)
