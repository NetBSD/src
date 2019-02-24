/*	$NetBSD: stdatomic.h,v 1.1.1.1 2019/02/24 18:56:48 christos Exp $	*/

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

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if !defined(__has_extension)
#define __has_extension(x) __has_feature(x)
#endif

#if !defined(__GNUC_PREREQ__)
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define __GNUC_PREREQ__(maj, min)                    \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define __GNUC_PREREQ__(maj, min) 0
#endif
#endif

#if !defined(__CLANG_ATOMICS) && !defined(__GNUC_ATOMICS)
#if __has_extension(c_atomic) || __has_extension(cxx_atomic)
#define __CLANG_ATOMICS
#elif __GNUC_PREREQ__(4, 7)
#define __GNUC_ATOMICS
#elif !defined(__GNUC__)
#error "isc/stdatomic.h does not support your compiler"
#endif
#endif

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

typedef int_fast32_t	atomic_int_fast32_t;
typedef uint_fast32_t	atomic_uint_fast32_t;
typedef int_fast64_t	atomic_int_fast64_t;
typedef uint_fast64_t	atomic_uint_fast64_t;
typedef bool		atomic_bool;

#if defined(__CLANG_ATOMICS) /* __c11_atomic builtins */
#define atomic_init(obj, desired)		\
	__c11_atomic_init(obj, desired)
#define atomic_load_explicit(obj, order)	\
	__c11_atomic_load(obj, order)
#define atomic_store_explicit(obj, desired, order)	\
	__c11_atomic_store(obj, desired, order)
#define atomic_fetch_add_explicit(obj, arg, order)	\
	__c11_atomic_fetch_add(obj, arg, order)
#define atomic_fetch_sub_explicit(obj, arg, order)	\
	__c11_atomic_fetch_sub(obj, arg, order)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)	\
	__c11_atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail)	\
	__c11_atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail)
#elif defined(__GNUC_ATOMICS) /* __atomic builtins */
#define atomic_init(obj, desired)			\
	(*obj = desired)
#define atomic_load_explicit(obj, order)		\
	__atomic_load_n(obj, order)
#define atomic_store_explicit(obj, desired, order)	\
	__atomic_store_n(obj, desired, order)
#define atomic_fetch_add_explicit(obj, arg, order)	\
	__atomic_fetch_add(obj, arg, order)
#define atomic_fetch_sub_explicit(obj, arg, order)	\
	__atomic_fetch_sub(obj, arg, order)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)	\
	__atomic_compare_exchange_n(obj, expected, desired, 0, succ, fail)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail)	\
	__atomic_compare_exchange_n(obj, expected, desired, 1, succ, fail)
#else /* __sync builtins */
#define atomic_init(obj, desired)			\
	(*obj = desired)
#define atomic_load_explicit(obj, order)		\
	__sync_fetch_and_add(obj, 0)
#define atomic_store_explicit(obj, desired, order)	\
	do {						\
		__sync_synchronize();			\
		*obj = desired;				\
		__sync_synchronize();			\
	} while (0);
#define atomic_fetch_add_explicit(obj, arg, order) \
	__sync_fetch_and_add(obj, arg)
#define atomic_fetch_sub_explicit(obj, arg, order) \
	__sync_fetch_and_sub(obj, arg, order)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)	\
	({									\
		__typeof__(obj) __v;						\
		_Bool __r;							\
		__v = (__typeof__(obj))__sync_val_compare_and_swap(obj, 	\
								   *(expected), \
								   desired);	\
		__r = ((__typeof__(obj))*(expected) == __v);            	\
		*(expected) = __v;						\
		__r;								\
	})
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail)	\
	atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)
#endif

#define atomic_load(obj) \
	atomic_load_explicit(obj, memory_order_seq_cst)
#define atomic_store(obj, arg) \
	atomic_store_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_add(obj, arg) \
	atomic_fetch_add_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_sub(obj, arg) \
	atomic_fetch_sub_explicit(obj, arg, memory_order_seq_cst)
#define atomic_compare_exchange_strong(obj, expected, desired)	\
	atomic_compare_exchange_strong_explicit(obj, expected, desired, memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_weak(obj, expected, desired)	\
	atomic_compare_exchange_weak_explicit(obj, expected, desired, memory_order_seq_cst, memory_order_seq_cst)
