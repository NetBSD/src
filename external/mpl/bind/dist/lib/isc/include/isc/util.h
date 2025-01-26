/*	$NetBSD: util.h,v 1.17 2025/01/26 16:25:43 christos Exp $	*/

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

/*! \file isc/util.h
 * NOTE:
 *
 * This file is not to be included from any <isc/???.h> (or other) library
 * files.
 *
 * \brief
 * Including this file puts several macros in your name space that are
 * not protected (as all the other ISC functions/macros do) by prepending
 * ISC_ or isc_ to the name.
 */

#include <isc/attributes.h>

/***
 *** Clang Compatibility Macros
 ***/

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif /* if !defined(__has_feature) */

/***
 *** General Macros.
 ***/

/*%
 * Legacy way how to hide unused function arguments, don't use in
 * the new code, rather use the ISC_ATTR_UNUSED macro that expands
 * to either C23's [[maybe_unused]] or __attribute__((__unused__)).
 *
 * \code
 * int
 * foo(ISC_ATTR_UNUSED char *bar) {
 *         ...;
 * }
 * \endcode
 */
#define UNUSED(x) (void)(x)

#if __GNUC__ >= 8 && !defined(__clang__)
#define ISC_NONSTRING __attribute__((nonstring))
#else /* if __GNUC__ >= 8 && !defined(__clang__) */
#define ISC_NONSTRING
#endif /* __GNUC__ */

#if HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR && HAVE_FUNC_ATTRIBUTE_DESTRUCTOR
#define ISC_CONSTRUCTOR __attribute__((constructor))
#define ISC_DESTRUCTOR	__attribute__((destructor))
#else
#define ISC_CONSTRUCTOR
#define ISC_DESTRUCTOR
#endif

/*%
 * The opposite: silent warnings about stored values which are never read.
 */
#define POST(x) (void)(x)

#define ISC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define ISC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define ISC_CLAMP(v, x, y) ((v) < (x) ? (x) : ((v) > (y) ? (y) : (v)))

/*%
 * The UNCONST() macro can be used to omit warnings produced by certain
 * compilers when operating with pointers declared with the const type qual-
 * ifier in a context without such qualifier.  Examples include passing a
 * pointer declared with the const qualifier to a function without such
 * qualifier, and variable assignment from a const pointer to a non-const
 * pointer.
 *
 * As the macro may hide valid errors, their usage is not recommended
 * unless there is a well-thought reason for a cast.  A typical use case for
 * __UNCONST() involve an API that does not follow the so-called ``const
 * correctness'' even if it would be appropriate.
 */
#define UNCONST(ptr) ((void *)(uintptr_t)(ptr))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/*
 * Optional return values, or out-arguments
 */
#define SET_IF_NOT_NULL(obj, val) \
	if ((obj) != NULL) {      \
		*(obj) = (val);   \
	}

/*%
 * Get the allocation size for a struct with a flexible array member
 * containing `count` elements. The struct is identified by a pointer,
 * typically the one that points to (or will point to) the allocation.
 */
#define STRUCT_FLEX_SIZE(pointer, member, count) \
	(sizeof(*(pointer)) + sizeof(*(pointer)->member) * (count))

/*%
 * Use this in translation units that would otherwise be empty, to
 * suppress compiler warnings.
 */
#define EMPTY_TRANSLATION_UNIT extern int isc__empty;

/*%
 * We use macros instead of calling the routines directly because
 * the capital letters make the locking stand out.
 */

#ifdef ISC_UTIL_TRACEON
#define ISC_UTIL_TRACE(a) a
#include <stdio.h> /* Required for fprintf/stderr when tracing. */
#else		   /* ifdef ISC_UTIL_TRACEON */
#define ISC_UTIL_TRACE(a)
#endif /* ifdef ISC_UTIL_TRACEON */

#include <isc/result.h> /* Contractual promise. */

#define SPINLOCK(sp)                                                           \
	{                                                                      \
		ISC_UTIL_TRACE(fprintf(stderr, "SPINLOCKING %p %s %d\n", (sp), \
				       __FILE__, __LINE__));                   \
		isc_spinlock_lock((sp));                                       \
		ISC_UTIL_TRACE(fprintf(stderr, "SPINLOCKED %p %s %d\n", (sp),  \
				       __FILE__, __LINE__));                   \
	}
#define SPINUNLOCK(sp)                                                    \
	{                                                                 \
		isc_spinlock_unlock((sp));                                \
		ISC_UTIL_TRACE(fprintf(stderr, "SPINUNLOCKED %p %s %d\n", \
				       (sp), __FILE__, __LINE__));        \
	}

#define LOCK(lp)                                                           \
	{                                                                  \
		ISC_UTIL_TRACE(fprintf(stderr, "LOCKING %p %s %d\n", (lp), \
				       __FILE__, __LINE__));               \
		isc_mutex_lock((lp));                                      \
		ISC_UTIL_TRACE(fprintf(stderr, "LOCKED %p %s %d\n", (lp),  \
				       __FILE__, __LINE__));               \
	}
#define UNLOCK(lp)                                                          \
	{                                                                   \
		isc_mutex_unlock((lp));                                     \
		ISC_UTIL_TRACE(fprintf(stderr, "UNLOCKED %p %s %d\n", (lp), \
				       __FILE__, __LINE__));                \
	}

#define BROADCAST(cvp)                                                        \
	{                                                                     \
		ISC_UTIL_TRACE(fprintf(stderr, "BROADCAST %p %s %d\n", (cvp), \
				       __FILE__, __LINE__));                  \
		isc_condition_broadcast((cvp));                               \
	}
#define SIGNAL(cvp)                                                        \
	{                                                                  \
		ISC_UTIL_TRACE(fprintf(stderr, "SIGNAL %p %s %d\n", (cvp), \
				       __FILE__, __LINE__));               \
		isc_condition_signal((cvp));                               \
	}
#define WAIT(cvp, lp)                                                         \
	{                                                                     \
		ISC_UTIL_TRACE(fprintf(stderr, "WAIT %p LOCK %p %s %d\n",     \
				       (cvp), (lp), __FILE__, __LINE__));     \
		isc_condition_wait((cvp), (lp));                              \
		ISC_UTIL_TRACE(fprintf(stderr, "WAITED %p LOCKED %p %s %d\n", \
				       (cvp), (lp), __FILE__, __LINE__));     \
	}

/*
 * isc_condition_waituntil can return ISC_R_TIMEDOUT, so we
 * don't RUNTIME_CHECK the result.
 *
 *  XXX Also, can't really debug this then...
 */

#define WAITUNTIL(cvp, lp, tp) isc_condition_waituntil((cvp), (lp), (tp))

#define RWLOCK(lp, t)                                                         \
	{                                                                     \
		ISC_UTIL_TRACE(fprintf(stderr, "RWLOCK %p, %d %s %d\n", (lp), \
				       (t), __FILE__, __LINE__));             \
		isc_rwlock_lock((lp), (t));                                   \
		ISC_UTIL_TRACE(fprintf(stderr, "RWLOCKED %p, %d %s %d\n",     \
				       (lp), (t), __FILE__, __LINE__));       \
	}
#define RWUNLOCK(lp, t)                                                   \
	{                                                                 \
		ISC_UTIL_TRACE(fprintf(stderr, "RWUNLOCK %p, %d %s %d\n", \
				       (lp), (t), __FILE__, __LINE__));   \
		isc_rwlock_unlock((lp), (t));                             \
	}

#define RDLOCK(lp)   RWLOCK(lp, isc_rwlocktype_read)
#define RDUNLOCK(lp) RWUNLOCK(lp, isc_rwlocktype_read)
#define WRLOCK(lp)   RWLOCK(lp, isc_rwlocktype_write)
#define WRUNLOCK(lp) RWUNLOCK(lp, isc_rwlocktype_write)

#define UPGRADELOCK(lock, locktype)                                         \
	{                                                                   \
		if (locktype == isc_rwlocktype_read) {                      \
			if (isc_rwlock_tryupgrade(lock) == ISC_R_SUCCESS) { \
				locktype = isc_rwlocktype_write;            \
			} else {                                            \
				RWUNLOCK(lock, locktype);                   \
				locktype = isc_rwlocktype_write;            \
				RWLOCK(lock, locktype);                     \
			}                                                   \
		}                                                           \
		INSIST(locktype == isc_rwlocktype_write);                   \
	}

/*
 * List Macros.
 */
#include <isc/list.h> /* Contractual promise. */

#define LIST(type)		       ISC_LIST(type)
#define INIT_LIST(type)		       ISC_LIST_INIT(type)
#define LINK(type)		       ISC_LINK(type)
#define INIT_LINK(elt, link)	       ISC_LINK_INIT(elt, link)
#define HEAD(list)		       ISC_LIST_HEAD(list)
#define TAIL(list)		       ISC_LIST_TAIL(list)
#define EMPTY(list)		       ISC_LIST_EMPTY(list)
#define PREV(elt, link)		       ISC_LIST_PREV(elt, link)
#define NEXT(elt, link)		       ISC_LIST_NEXT(elt, link)
#define APPEND(list, elt, link)	       ISC_LIST_APPEND(list, elt, link)
#define PREPEND(list, elt, link)       ISC_LIST_PREPEND(list, elt, link)
#define UNLINK(list, elt, link)	       ISC_LIST_UNLINK(list, elt, link)
#define ENQUEUE(list, elt, link)       ISC_LIST_APPEND(list, elt, link)
#define DEQUEUE(list, elt, link)       ISC_LIST_UNLINK(list, elt, link)
#define INSERTBEFORE(li, b, e, ln)     ISC_LIST_INSERTBEFORE(li, b, e, ln)
#define INSERTAFTER(li, a, e, ln)      ISC_LIST_INSERTAFTER(li, a, e, ln)
#define APPENDLIST(list1, list2, link) ISC_LIST_APPENDLIST(list1, list2, link)

/*%
 * Performance
 */

/* GCC defines __SANITIZE_ADDRESS__, so reuse the macro for clang */
#if __has_feature(address_sanitizer)
#define __SANITIZE_ADDRESS__ 1
#endif /* if __has_feature(address_sanitizer) */

#if __SANITIZE_ADDRESS__
#define ISC_NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
#else /* if __SANITIZE_ADDRESS__ */
#define ISC_NO_SANITIZE_ADDRESS
#endif /* if __SANITIZE_ADDRESS__ */

#if __has_feature(thread_sanitizer)
#define __SANITIZE_THREAD__ 1
#endif /* if __has_feature(thread_sanitizer) */

#if __SANITIZE_THREAD__
#define ISC_NO_SANITIZE_THREAD __attribute__((no_sanitize("thread")))
#else /* if __SANITIZE_THREAD__ */
#define ISC_NO_SANITIZE_THREAD
#endif /* if __SANITIZE_THREAD__ */

#ifndef __lint__
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR >= 6)
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif __has_feature(c_static_assert)
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else /* if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR >= 6) */

/* Courtesy of Joseph Quinsey: https://godbolt.org/z/K9RvWS */
#define TOKENPASTE(a, b)	a##b /* "##" is the "Token Pasting Operator" */
#define EXPAND_THEN_PASTE(a, b) TOKENPASTE(a, b) /* expand then paste */
#define STATIC_ASSERT(x, msg) \
	enum { EXPAND_THEN_PASTE(ASSERT_line_, __LINE__) = 1 / ((msg) && (x)) }
#endif /* if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR >= 6) */
#else
#define STATIC_ASSERT(cond, msg)
#endif

#ifdef UNIT_TESTING
extern void
mock_assert(const int result, const char *const expression,
	    const char *const file, const int line);
/*
 *	Allow clang to determine that the following code is not reached
 *	by calling abort() if the condition fails.  The abort() will
 *	never be executed as mock_assert() and _assert_true() longjmp
 *	or exit if the condition is false.
 */
#define REQUIRE(expression)                                                   \
	((!(expression))                                                      \
		 ? (mock_assert(0, #expression, __FILE__, __LINE__), abort()) \
		 : (void)0)
#define ENSURE(expression)                                                    \
	((!(int)(expression))                                                 \
		 ? (mock_assert(0, #expression, __FILE__, __LINE__), abort()) \
		 : (void)0)
#define INSIST(expression)                                                    \
	((!(expression))                                                      \
		 ? (mock_assert(0, #expression, __FILE__, __LINE__), abort()) \
		 : (void)0)
#define INVARIANT(expression)                                                 \
	((!(expression))                                                      \
		 ? (mock_assert(0, #expression, __FILE__, __LINE__), abort()) \
		 : (void)0)
#define UNREACHABLE() \
	(mock_assert(0, "unreachable", __FILE__, __LINE__), abort())
#define _assert_true(c, e, f, l) \
	((c) ? (void)0 : (_assert_true(0, e, f, l), abort()))
#define _assert_int_equal(a, b, f, l) \
	(((a) == (b)) ? (void)0 : (_assert_int_equal(a, b, f, l), abort()))
#define _assert_int_not_equal(a, b, f, l) \
	(((a) != (b)) ? (void)0 : (_assert_int_not_equal(a, b, f, l), abort()))
#else			    /* UNIT_TESTING */

/*
 * Assertions
 */
#include <isc/assertions.h> /* Contractual promise. */

/*% Require Assertion */
#define REQUIRE(e)   ISC_REQUIRE(e)
/*% Ensure Assertion */
#define ENSURE(e)    ISC_ENSURE(e)
/*% Insist Assertion */
#define INSIST(e)    ISC_INSIST(e)
/*% Invariant Assertion */
#define INVARIANT(e) ISC_INVARIANT(e)

#define UNREACHABLE() ISC_UNREACHABLE()

#endif /* UNIT_TESTING */

/*
 * Errors
 */
#include <errno.h> /* for errno */

#include <isc/error.h>	/* Contractual promise. */
#include <isc/strerr.h> /* for ISC_STRERRORSIZE */

#define UNEXPECTED_ERROR(...) \
	isc_error_unexpected(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define FATAL_ERROR(...) \
	isc_error_fatal(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define REPORT_SYSERROR(report, err, fmt, ...)                        \
	{                                                             \
		char strerr[ISC_STRERRORSIZE];                        \
		strerror_r(err, strerr, sizeof(strerr));              \
		report(__FILE__, __LINE__, __func__, fmt ": %s (%d)", \
		       ##__VA_ARGS__, strerr, err);                   \
	}

#define UNEXPECTED_SYSERROR(err, ...) \
	REPORT_SYSERROR(isc_error_unexpected, err, __VA_ARGS__)

#define FATAL_SYSERROR(err, ...) \
	REPORT_SYSERROR(isc_error_fatal, err, __VA_ARGS__)

#ifdef UNIT_TESTING

#define RUNTIME_CHECK(cond) \
	((cond) ? (void)0   \
		: (mock_assert(0, #cond, __FILE__, __LINE__), abort()))

#else /* UNIT_TESTING */

#define RUNTIME_CHECK(cond) \
	((cond) ? (void)0 : FATAL_ERROR("RUNTIME_CHECK(%s) failed", #cond))

#endif /* UNIT_TESTING */

/*%
 * Runtime check which logs the error value returned by a POSIX Threads
 * function and the error string that corresponds to it
 */
#define PTHREADS_RUNTIME_CHECK(func, ret)           \
	if ((ret) != 0) {                           \
		FATAL_SYSERROR(ret, "%s()", #func); \
	}

/*%
 * Alignment
 */
#ifdef __GNUC__
#define ISC_ALIGN(x, a) (((x) + (a) - 1) & ~((typeof(x))(a) - 1))
#else /* ifdef __GNUC__ */
#define ISC_ALIGN(x, a) (((x) + (a) - 1) & ~((uintmax_t)(a) - 1))
#endif /* ifdef __GNUC__ */

/*%
 * Swap
 */
#define ISC_SWAP(a, b)                    \
	{                                 \
		typeof(a) __tmp_swap = a; \
		a = b;                    \
		b = __tmp_swap;           \
	}
