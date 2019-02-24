/*	$NetBSD: util.h,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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

#ifndef ISC_UTIL_H
#define ISC_UTIL_H 1

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

/***
 *** General Macros.
 ***/

/*%
 * Use this to hide unused function arguments.
 * \code
 * int
 * foo(char *bar)
 * {
 *	UNUSED(bar);
 * }
 * \endcode
 */
#define UNUSED(x)      (void)(x)

/*%
 * The opposite: silent warnings about stored values which are never read.
 */
#define POST(x)        (void)(x)

#define ISC_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define ISC_MIN(a, b)  ((a) < (b) ? (a) : (b))

#define ISC_CLAMP(v, x, y) ((v) < (x) ? (x) : ((v) > (y) ? (y) : (v)))

/*%
 * Use this to remove the const qualifier of a variable to assign it to
 * a non-const variable or pass it as a non-const function argument ...
 * but only when you are sure it won't then be changed!
 * This is necessary to sometimes shut up some compilers
 * (as with gcc -Wcast-qual) when there is just no other good way to avoid the
 * situation.
 */
#define DE_CONST(konst, var) \
	do { \
		union { const void *k; void *v; } _u; \
		_u.k = konst; \
		var = _u.v; \
	} while (/*CONSTCOND*/0)

/*%
 * Use this in translation units that would otherwise be empty, to
 * suppress compiler warnings.
 */
#define EMPTY_TRANSLATION_UNIT extern int isc__empty;

/*%
 * We use macros instead of calling the routines directly because
 * the capital letters make the locking stand out.
 * We RUNTIME_CHECK for success since in general there's no way
 * for us to continue if they fail.
 */

#ifdef ISC_UTIL_TRACEON
#define ISC_UTIL_TRACE(a) a
#include <stdio.h>		/* Required for fprintf/stderr when tracing. */
#else
#define ISC_UTIL_TRACE(a)
#endif

#include <isc/result.h>		/* Contractual promise. */

#define LOCK(lp) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "LOCKING %p %s %d\n", \
			       (lp), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_mutex_lock((lp)) == ISC_R_SUCCESS); \
	ISC_UTIL_TRACE(fprintf(stderr, "LOCKED %p %s %d\n", \
			       (lp), __FILE__, __LINE__)); \
	} while (/*CONSTCOND*/0)
#define UNLOCK(lp) do { \
	RUNTIME_CHECK(isc_mutex_unlock((lp)) == ISC_R_SUCCESS); \
	ISC_UTIL_TRACE(fprintf(stderr, "UNLOCKED %p %s %d\n", \
			       (lp), __FILE__, __LINE__)); \
	} while (/*CONSTCOND*/0)

#define BROADCAST(cvp) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "BROADCAST %p %s %d\n", \
			       (cvp), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_condition_broadcast((cvp)) == ISC_R_SUCCESS); \
	} while (/*CONSTCOND*/0)
#define SIGNAL(cvp) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "SIGNAL %p %s %d\n", \
			       (cvp), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_condition_signal((cvp)) == ISC_R_SUCCESS); \
	} while (/*CONSTCOND*/0)
#define WAIT(cvp, lp) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "WAIT %p LOCK %p %s %d\n", \
			       (cvp), \
			       (lp), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_condition_wait((cvp), (lp)) == ISC_R_SUCCESS); \
	ISC_UTIL_TRACE(fprintf(stderr, "WAITED %p LOCKED %p %s %d\n", \
			       (cvp), \
			       (lp), __FILE__, __LINE__)); \
	} while (/*CONSTCOND*/0)

/*
 * isc_condition_waituntil can return ISC_R_TIMEDOUT, so we
 * don't RUNTIME_CHECK the result.
 *
 *  XXX Also, can't really debug this then...
 */

#define WAITUNTIL(cvp, lp, tp) \
	isc_condition_waituntil((cvp), (lp), (tp))

#define RWLOCK(lp, t) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "RWLOCK %p, %d %s %d\n", \
			       (lp), (t), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_rwlock_lock((lp), (t)) == ISC_R_SUCCESS); \
	ISC_UTIL_TRACE(fprintf(stderr, "RWLOCKED %p, %d %s %d\n", \
			       (lp), (t), __FILE__, __LINE__)); \
	} while (/*CONSTCOND*/0)
#define RWUNLOCK(lp, t) do { \
	ISC_UTIL_TRACE(fprintf(stderr, "RWUNLOCK %p, %d %s %d\n", \
			       (lp), (t), __FILE__, __LINE__)); \
	RUNTIME_CHECK(isc_rwlock_unlock((lp), (t)) == ISC_R_SUCCESS); \
	} while (/*CONSTCOND*/0)

/*
 * List Macros.
 */
#include <isc/list.h>		/* Contractual promise. */

#define LIST(type)			ISC_LIST(type)
#define INIT_LIST(type)			ISC_LIST_INIT(type)
#define LINK(type)			ISC_LINK(type)
#define INIT_LINK(elt, link)		ISC_LINK_INIT(elt, link)
#define HEAD(list)			ISC_LIST_HEAD(list)
#define TAIL(list)			ISC_LIST_TAIL(list)
#define EMPTY(list)			ISC_LIST_EMPTY(list)
#define PREV(elt, link)			ISC_LIST_PREV(elt, link)
#define NEXT(elt, link)			ISC_LIST_NEXT(elt, link)
#define APPEND(list, elt, link)		ISC_LIST_APPEND(list, elt, link)
#define PREPEND(list, elt, link)	ISC_LIST_PREPEND(list, elt, link)
#define UNLINK(list, elt, link)		ISC_LIST_UNLINK(list, elt, link)
#define ENQUEUE(list, elt, link)	ISC_LIST_APPEND(list, elt, link)
#define DEQUEUE(list, elt, link)	ISC_LIST_UNLINK(list, elt, link)
#define INSERTBEFORE(li, b, e, ln)	ISC_LIST_INSERTBEFORE(li, b, e, ln)
#define INSERTAFTER(li, a, e, ln)	ISC_LIST_INSERTAFTER(li, a, e, ln)
#define APPENDLIST(list1, list2, link)	ISC_LIST_APPENDLIST(list1, list2, link)

/*%
 * Performance
 */
#include <isc/likely.h>

#ifdef HAVE_BUILTIN_UNREACHABLE
#define ISC_UNREACHABLE() __builtin_unreachable();
#else
#define ISC_UNREACHABLE()
#endif

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

/* GCC defines __SANITIZE_ADDRESS__, so reuse the macro for clang */
#if __has_feature(address_sanitizer)
#define __SANITIZE_ADDRESS__ 1
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR >= 6)
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif __has_feature(c_static_assert)
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond, msg) INSIST(cond)
#endif

#ifdef UNIT_TESTING
extern void mock_assert(const int result, const char* const expression,
			const char * const file, const int line);
#define REQUIRE(expression)						\
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)
#define ENSURE(expression)						\
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)
#define INSIST(expression)						\
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)
#define INVARIANT(expression)						\
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)

#else /* UNIT_TESTING */
/*
 * Assertions
 */
#include <isc/assertions.h>	/* Contractual promise. */

/*% Require Assertion */
#define REQUIRE(e)			ISC_REQUIRE(e)
/*% Ensure Assertion */
#define ENSURE(e)			ISC_ENSURE(e)
/*% Insist Assertion */
#define INSIST(e)			ISC_INSIST(e)
/*% Invariant Assertion */
#define INVARIANT(e)			ISC_INVARIANT(e)

#endif /* UNIT_TESTING */

/*
 * Errors
 */
#include <isc/error.h>		/* Contractual promise. */

/*% Unexpected Error */
#define UNEXPECTED_ERROR		isc_error_unexpected
/*% Fatal Error */
#define FATAL_ERROR			isc_error_fatal

#ifdef UNIT_TESTING

#define RUNTIME_CHECK(expression)					\
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)

#else /* UNIT_TESTING */

/*% Runtime Check */
#define RUNTIME_CHECK(cond)		ISC_ERROR_RUNTIMECHECK(cond)

#endif /* UNIT_TESTING */

/*%
 * Time
 */
#define TIME_NOW(tp) 	RUNTIME_CHECK(isc_time_now((tp)) == ISC_R_SUCCESS)

/*%
 * Alignment
 */
#define ISC_ALIGN(x, a) (((x) + (a) - 1) & ~((typeof(x))(a)-1))

/*%
 * Misc
 */
#include <isc/deprecated.h>

#endif /* ISC_UTIL_H */
