/*	$NetBSD: error.c,v 1.6.2.1 2024/02/25 15:47:16 martin Exp $	*/

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

/*! \file */

#include <stdio.h>
#include <stdlib.h>

#include <isc/error.h>
#include <isc/print.h>

/*% Default unexpected callback. */
static void
default_unexpected_callback(const char *, int, const char *, const char *,
			    va_list) ISC_FORMAT_PRINTF(4, 0);

/*% Default fatal callback. */
static void
default_fatal_callback(const char *, int, const char *, const char *, va_list)
	ISC_FORMAT_PRINTF(3, 0);

/*% unexpected_callback */
static isc_errorcallback_t unexpected_callback = default_unexpected_callback;
static isc_errorcallback_t fatal_callback = default_fatal_callback;

void
isc_error_setunexpected(isc_errorcallback_t cb) {
	if (cb == NULL) {
		unexpected_callback = default_unexpected_callback;
	} else {
		unexpected_callback = cb;
	}
}

void
isc_error_setfatal(isc_errorcallback_t cb) {
	if (cb == NULL) {
		fatal_callback = default_fatal_callback;
	} else {
		fatal_callback = cb;
	}
}

void
isc_error_unexpected(const char *file, int line, const char *func,
		     const char *format, ...) {
	va_list args;

	va_start(args, format);
	(unexpected_callback)(file, line, func, format, args);
	va_end(args);
}

void
isc_error_fatal(const char *file, int line, const char *func,
		const char *format, ...) {
	va_list args;

	va_start(args, format);
	(fatal_callback)(file, line, func, format, args);
	va_end(args);
	abort();
}

static void
default_unexpected_callback(const char *file, int line, const char *func,
			    const char *format, va_list args) {
	fprintf(stderr, "%s:%d:%s(): ", file, line, func);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
}

static void
default_fatal_callback(const char *file, int line, const char *func,
		       const char *format, va_list args) {
	fprintf(stderr, "%s:%d:%s(): fatal error: ", file, line, func);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
}
