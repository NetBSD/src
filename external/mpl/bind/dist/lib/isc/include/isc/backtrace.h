/*	$NetBSD: backtrace.h,v 1.5.2.1 2024/02/25 15:47:19 martin Exp $	*/

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

/*! \file isc/backtrace.h
 * \brief provide a back trace of the running process to help debug problems.
 *
 * This module tries to get a back trace of the process using some platform
 * dependent way when available.  It also manages an internal symbol table
 * that maps function addresses used in the process to their textual symbols.
 * This module is expected to be used to help debug when some fatal error
 * happens.
 *
 * IMPORTANT NOTE: since the (major) intended use case of this module is
 * dumping a back trace on a fatal error, normally followed by self termination,
 * functions defined in this module generally doesn't employ assertion checks
 * (if it did, a program bug could cause infinite recursive calls to a
 * backtrace function).
 */

#pragma once

/***
 ***	Imports
 ***/
#include <isc/types.h>

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS
int
isc_backtrace(void **addrs, int maxaddrs);
/*%<
 * Get a back trace of the running process above this function itself.  On
 * success, addrs[i] will store the address of the call point of the i-th
 * stack frame (addrs[0] is the caller of this function).  *nframes will store
 * the total number of frames.
 *
 * Requires (note that these are not ensured by assertion checks, see above):
 *
 *\li	'addrs' is a valid array containing at least 'maxaddrs' void * entries.
 *
 *\li	'nframes' must be non NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_FAILURE
 *\li	#ISC_R_NOTFOUND
 *\li	#ISC_R_NOTIMPLEMENTED
 */

char **
isc_backtrace_symbols(void *const *buffer, int size);
/*
 * isc_backtrace_symbols() attempts to transform a call stack obtained by
 * backtrace() into an array of human-readable strings using dladdr().  The
 * array of strings returned has size elements.  It is allocated using
 * malloc() and should be released using free().  There is no need to free
 * the individual strings in the array.
 *
 * Notes:
 *
 *\li	On Windows, this is shim implementation using SymFromAddr()
 *\li	On systems with backtrace_symbols(), it's just a thin wrapper
 *\li	Otherwise, it returns NULL
 *\li	See platform NOTES for backtrace_symbols
 *
 * Returns:
 *
 *\li	On success, backtrace_symbols() returns a pointer to the array
 *\li	On error, NULL is returned.
 */

void
isc_backtrace_symbols_fd(void *const *buffer, int size, int fd);
/*
 * isc_backtrace_symbols_fd() performs the same operation as
 * isc_backtrace_symbols(), but the resulting strings are immediately written to
 * the file descriptor fd, and are not returned.  isc_backtrace_symbols_fd()
 * does not call malloc(3), and so can be employed in situations where the
 * latter function might fail.
 *
 * Notes:
 *
 *\li	See isc_backtrace_symbols() notes
 *\li	See platform NOTES for backtrace_symbols_fd for caveats
 */

ISC_LANG_ENDDECLS
