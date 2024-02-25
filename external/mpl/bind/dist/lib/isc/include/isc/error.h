/*	$NetBSD: error.h,v 1.5.2.1 2024/02/25 15:47:20 martin Exp $	*/

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

/*! \file isc/error.h */

#include <stdarg.h>

#include <isc/attributes.h>
#include <isc/formatcheck.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

typedef void (*isc_errorcallback_t)(const char *, int, const char *,
				    const char *, va_list);

/*% set unexpected error */
void isc_error_setunexpected(isc_errorcallback_t);

/*% set fatal error */
void isc_error_setfatal(isc_errorcallback_t);

/*% unexpected error */
void
isc_error_unexpected(const char *, int, const char *, const char *, ...)
	ISC_FORMAT_PRINTF(4, 5);

/*% fatal error */
noreturn void
isc_error_fatal(const char *, int, const char *, const char *, ...)
	ISC_FORMAT_PRINTF(4, 5);

ISC_LANG_ENDDECLS
