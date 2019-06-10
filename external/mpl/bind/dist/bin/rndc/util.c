/*	$NetBSD: util.c,v 1.3.2.2 2019/06/10 22:03:03 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <isc/print.h>

#include "util.h"

extern bool verbose;
extern const char *progname;

void
notify(const char *fmt, ...) {
	va_list ap;

	if (verbose) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputs("\n", stderr);
	}
}

void
fatal(const char *format, ...) {
	va_list args;

	fprintf(stderr, "%s: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
