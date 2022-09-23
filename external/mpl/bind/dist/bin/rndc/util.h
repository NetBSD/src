/*	$NetBSD: util.h,v 1.6 2022/09/23 12:15:23 christos Exp $	*/

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

#ifndef RNDC_UTIL_H
#define RNDC_UTIL_H 1

/*! \file */

#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/platform.h>

#define NS_CONTROL_PORT 953

#undef DO
#define DO(name, function)                                                \
	do {                                                              \
		result = function;                                        \
		if (result != ISC_R_SUCCESS)                              \
			fatal("%s: %s", name, isc_result_totext(result)); \
		else                                                      \
			notify("%s", name);                               \
	} while (0)

ISC_LANG_BEGINDECLS

void
notify(const char *fmt, ...) ISC_FORMAT_PRINTF(1, 2);

ISC_PLATFORM_NORETURN_PRE void
fatal(const char *format, ...)
	ISC_FORMAT_PRINTF(1, 2) ISC_PLATFORM_NORETURN_POST;

ISC_LANG_ENDDECLS

#endif /* RNDC_UTIL_H */
