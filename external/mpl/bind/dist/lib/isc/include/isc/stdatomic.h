/*	$NetBSD: stdatomic.h,v 1.4.2.2 2024/02/25 15:47:23 martin Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#if HAVE_UCHAR_H
#include <uchar.h>
#endif /* HAVE_UCHAR_H */

/* GCC 4.7.0 introduced __atomic builtins, but not the __GNUC_ATOMICS define */
#if !defined(__GNUC_ATOMICS) && __GNUC__ == 4 && __GNUC_MINOR__ >= 7
#define __GNUC_ATOMICS
#endif

#if !defined(__GNUC_ATOMICS)
#error "isc/stdatomic.h does not support your compiler"
#endif /* if !defined(__GNUC_ATOMICS) */

typedef enum memory_order {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#ifndef HAVE_UCHAR_H
typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;
#endif /* HAVE_UCHAR_H */

typedef bool		   atomic_bool;
typedef char		   atomic_char;
typedef signed char	   atomic_schar;
typedef unsigned char	   atomic_uchar;
typedef short		   atomic_short;
typedef unsigned short	   atomic_ushort;
typedef int		   atomic_int;
typedef unsigned int	   atomic_uint;
typedef long		   atomic_long;
typedef unsigned long	   atomic_ulong;
typedef long long	   atomic_llong;
typedef unsigned long long atomic_ullong;
typedef char16_t	   atomic_char16_t;
typedef char32_t	   atomic_char32_t;
typedef wchar_t		   atomic_wchar_t;
typedef int_least8_t	   atomic_int_least8_t;
typedef uint_least8_t	   atomic_uint_least8_t;
typedef int_least16_t	   atomic_int_least16_t;
typedef uint_least16_t	   atomic_uint_least16_t;
typedef int_least32_t	   atomic_int_least32_t;
typedef uint_least32_t	   atomic_uint_least32_t;
typedef int_least64_t	   atomic_int_least64_t;
typedef uint_least64_t	   atomic_uint_least64_t;
typedef int_fast8_t	   atomic_int_fast8_t;
typedef uint_fast8_t	   atomic_uint_fast8_t;
typedef int_fast16_t	   atomic_int_fast16_t;
typedef uint_fast16_t	   atomic_uint_fast16_t;
typedef int_fast32_t	   atomic_int_fast32_t;
typedef uint_fast32_t	   atomic_uint_fast32_t;
typedef int_fast64_t	   atomic_int_fast64_t;
typedef uint_fast64_t	   atomic_uint_fast64_t;
typedef intptr_t	   atomic_intptr_t;
typedef uintptr_t	   atomic_uintptr_t;
typedef size_t		   atomic_size_t;
typedef ptrdiff_t	   atomic_ptrdiff_t;
typedef intmax_t	   atomic_intmax_t;
typedef uintmax_t	   atomic_uintmax_t;

#define atomic_init(obj, desired)	 (*obj = desired)
#define atomic_load_explicit(obj, order) __atomic_load_n(obj, order)
#define atomic_store_explicit(obj, desired, order) \
	__atomic_store_n(obj, desired, order)
#define atomic_fetch_add_explicit(obj, arg, order) \
	__atomic_fetch_add(obj, arg, order)
#define atomic_fetch_sub_explicit(obj, arg, order) \
	__atomic_fetch_sub(obj, arg, order)
#define atomic_fetch_and_explicit(obj, arg, order) \
	__atomic_fetch_and(obj, arg, order)
#define atomic_fetch_or_explicit(obj, arg, order) \
	__atomic_fetch_or(obj, arg, order)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, \
						fail)                         \
	__atomic_compare_exchange_n(obj, expected, desired, 0, succ, fail)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, \
					      fail)                         \
	__atomic_compare_exchange_n(obj, expected, desired, 1, succ, fail)
#define atomic_exchange_explicit(obj, desired, order) \
	__atomic_exchange_n(obj, desired, order)

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
#define atomic_exchange(obj, desired) \
	atomic_exchange_explicit(obj, desired, memory_order_seq_cst)
