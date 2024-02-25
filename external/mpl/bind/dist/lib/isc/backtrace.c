/*	$NetBSD: backtrace.c,v 1.6.2.1 2024/02/25 15:47:15 martin Exp $	*/

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

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_BACKTRACE_SYMBOLS
#include <execinfo.h>
#endif /* HAVE_BACKTRACE_SYMBOLS */

#include <isc/backtrace.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/util.h>

#if HAVE_BACKTRACE_SYMBOLS
int
isc_backtrace(void **addrs, int maxaddrs) {
	int n;

	/*
	 * Validate the arguments: intentionally avoid using REQUIRE().
	 * See notes in backtrace.h.
	 */
	if (addrs == NULL || maxaddrs <= 0) {
		return (-1);
	}

	/*
	 * backtrace(3) includes this function itself in the address array,
	 * which should be eliminated from the returned sequence.
	 */
	n = backtrace(addrs, maxaddrs);
	if (n < 2) {
		return (-1);
	}
	n--;
	memmove(addrs, &addrs[1], sizeof(addrs[0]) * n);

	return (n);
}

char **
isc_backtrace_symbols(void *const *buffer, int size) {
	return (backtrace_symbols(buffer, size));
}

void
isc_backtrace_symbols_fd(void *const *buffer, int size, int fd) {
	backtrace_symbols_fd(buffer, size, fd);
}

#else /* HAVE_BACKTRACE_SYMBOLS */

int
isc_backtrace(void **addrs, int maxaddrs) {
	UNUSED(addrs);
	UNUSED(maxaddrs);

	return (-1);
}

char **
isc_backtrace_symbols(void *const *buffer, int size) {
	UNUSED(buffer);
	UNUSED(size);

	return (NULL);
}

void
isc_backtrace_symbols_fd(void *const *buffer, int size, int fd) {
	UNUSED(buffer);
	UNUSED(size);
	UNUSED(fd);
}

#endif /* HAVE_BACKTRACE_SYMBOLS */
