/*	$NetBSD: assertions.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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

/*! \file isc/assertions.h
 */

#ifndef ISC_ASSERTIONS_H
#define ISC_ASSERTIONS_H 1

#include <isc/lang.h>
#include <isc/likely.h>
#include <isc/platform.h>

ISC_LANG_BEGINDECLS

/*% isc assertion type */
typedef enum {
	isc_assertiontype_require,
	isc_assertiontype_ensure,
	isc_assertiontype_insist,
	isc_assertiontype_invariant
} isc_assertiontype_t;

typedef void (*isc_assertioncallback_t)(const char *, int, isc_assertiontype_t,
					const char *);

/* coverity[+kill] */
ISC_PLATFORM_NORETURN_PRE
void isc_assertion_failed(const char *, int, isc_assertiontype_t,
			  const char *) ISC_PLATFORM_NORETURN_POST;

void
isc_assertion_setcallback(isc_assertioncallback_t);

const char *
isc_assertion_typetotext(isc_assertiontype_t type);

#if defined(ISC_CHECK_ALL) || defined(__COVERITY__)
#define ISC_CHECK_REQUIRE		1
#define ISC_CHECK_ENSURE		1
#define ISC_CHECK_INSIST		1
#define ISC_CHECK_INVARIANT		1
#endif

#if defined(ISC_CHECK_NONE) && !defined(__COVERITY__)
#define ISC_CHECK_REQUIRE		0
#define ISC_CHECK_ENSURE		0
#define ISC_CHECK_INSIST		0
#define ISC_CHECK_INVARIANT		0
#endif

#ifndef ISC_CHECK_REQUIRE
#define ISC_CHECK_REQUIRE		1
#endif

#ifndef ISC_CHECK_ENSURE
#define ISC_CHECK_ENSURE		1
#endif

#ifndef ISC_CHECK_INSIST
#define ISC_CHECK_INSIST		1
#endif

#ifndef ISC_CHECK_INVARIANT
#define ISC_CHECK_INVARIANT		1
#endif

#if ISC_CHECK_REQUIRE != 0
#define ISC_REQUIRE(cond) \
	((void) (/*CONSTCOND*/ISC_LIKELY(cond) || \
		 ((isc_assertion_failed)(__FILE__, __LINE__, \
					 isc_assertiontype_require, \
					 #cond), 0)))
#else
#define ISC_REQUIRE(cond)	((void) 0)
#endif /* ISC_CHECK_REQUIRE */

#if ISC_CHECK_ENSURE != 0
#define ISC_ENSURE(cond) \
	((void) (/*CONSTCOND*/ISC_LIKELY(cond) || \
		 ((isc_assertion_failed)(__FILE__, __LINE__, \
					 isc_assertiontype_ensure, \
					 #cond), 0)))
#else
#define ISC_ENSURE(cond)	((void) 0)
#endif /* ISC_CHECK_ENSURE */

#if ISC_CHECK_INSIST != 0
#define ISC_INSIST(cond) \
	((void) (/*CONSTCOND*/ISC_LIKELY(cond) || \
		 ((isc_assertion_failed)(__FILE__, __LINE__, \
					 isc_assertiontype_insist, \
					 #cond), 0)))
#else
#define ISC_INSIST(cond)	((void) 0)
#endif /* ISC_CHECK_INSIST */

#if ISC_CHECK_INVARIANT != 0
#define ISC_INVARIANT(cond) \
	((void) (/*CONSTCOND*/ISC_LIKELY(cond) || \
		 ((isc_assertion_failed)(__FILE__, __LINE__, \
					 isc_assertiontype_invariant, \
					 #cond), 0)))
#else
#define ISC_INVARIANT(cond)	((void) 0)
#endif /* ISC_CHECK_INVARIANT */

ISC_LANG_ENDDECLS

#endif /* ISC_ASSERTIONS_H */
