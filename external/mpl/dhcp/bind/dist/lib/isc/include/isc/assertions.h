/*	$NetBSD: assertions.h,v 1.1.2.2 2024/02/24 13:07:23 martin Exp $	*/

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
void
isc_assertion_failed(const char *, int, isc_assertiontype_t,
		     const char *) ISC_PLATFORM_NORETURN_POST;

void isc_assertion_setcallback(isc_assertioncallback_t);

const char *
isc_assertion_typetotext(isc_assertiontype_t type);

#define ISC_REQUIRE(cond)                                                  \
	((void)(ISC_LIKELY(cond) ||                                        \
		((isc_assertion_failed)(__FILE__, __LINE__,                \
					isc_assertiontype_require, #cond), \
		 0)))

#define ISC_ENSURE(cond)                                                  \
	((void)(ISC_LIKELY(cond) ||                                       \
		((isc_assertion_failed)(__FILE__, __LINE__,               \
					isc_assertiontype_ensure, #cond), \
		 0)))

#define ISC_INSIST(cond)                                                  \
	((void)(/*CONSTCOND*/ISC_LIKELY(cond) ||                          \
		((isc_assertion_failed)(__FILE__, __LINE__,               \
					isc_assertiontype_insist, #cond), \
		 0)))

#define ISC_INVARIANT(cond)                                                  \
	((void)(ISC_LIKELY(cond) ||                                          \
		((isc_assertion_failed)(__FILE__, __LINE__,                  \
					isc_assertiontype_invariant, #cond), \
		 0)))

#define ISC_UNREACHABLE()                                                   \
	(isc_assertion_failed(__FILE__, __LINE__, isc_assertiontype_insist, \
			      "unreachable"),                               \
	 __builtin_unreachable())

ISC_LANG_ENDDECLS

#endif /* ISC_ASSERTIONS_H */
