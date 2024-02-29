/*	$NetBSD: error.h,v 1.1.4.2 2024/02/29 11:39:04 martin Exp $	*/

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

#ifndef ISC_ERROR_H
#define ISC_ERROR_H 1

/*! \file isc/error.h */

#include <stdarg.h>

#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/likely.h>
#include <isc/platform.h>

ISC_LANG_BEGINDECLS

typedef void (*isc_errorcallback_t)(const char *, int, const char *, va_list);

/*% set unexpected error */
void isc_error_setunexpected(isc_errorcallback_t);

/*% set fatal error */
void isc_error_setfatal(isc_errorcallback_t);

/*% unexpected error */
void
isc_error_unexpected(const char *, int, const char *, ...)
	ISC_FORMAT_PRINTF(3, 4);

/*% fatal error */
ISC_PLATFORM_NORETURN_PRE void
isc_error_fatal(const char *, int, const char *, ...)
	ISC_FORMAT_PRINTF(3, 4) ISC_PLATFORM_NORETURN_POST;

/*% runtimecheck error */
ISC_PLATFORM_NORETURN_PRE void
isc_error_runtimecheck(const char *, int,
		       const char *) ISC_PLATFORM_NORETURN_POST;

#define ISC_ERROR_RUNTIMECHECK(cond) \
	((void)(ISC_LIKELY(cond) ||  \
		((isc_error_runtimecheck)(__FILE__, __LINE__, #cond), 0)))

ISC_LANG_ENDDECLS

#endif /* ISC_ERROR_H */
