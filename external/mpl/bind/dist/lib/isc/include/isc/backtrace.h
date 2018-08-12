/*	$NetBSD: backtrace.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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
 * backtrace function).  These functions still perform minimal checks and return
 * ISC_R_FAILURE if they detect an error, but the caller should therefore be
 * very careful about the use of these functions, and generally discouraged to
 * use them except in an exit path.  The exception is
 * isc_backtrace_getsymbolfromindex(), which is expected to be used in a
 * non-error-handling context and validates arguments with assertion checks.
 */

#ifndef ISC_BACKTRACE_H
#define ISC_BACKTRACE_H 1

/***
 ***	Imports
 ***/

#include <isc/types.h>

/***
 *** Types
 ***/
struct isc_backtrace_symmap {
	void		*addr;
	const char	*symbol;
};

LIBISC_EXTERNAL_DATA extern const int isc__backtrace_nsymbols;
LIBISC_EXTERNAL_DATA extern const
	isc_backtrace_symmap_t isc__backtrace_symtable[];

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS
isc_result_t
isc_backtrace_gettrace(void **addrs, int maxaddrs, int *nframes);
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

isc_result_t
isc_backtrace_getsymbolfromindex(int index, const void **addrp,
				 const char **symbolp);
/*%<
 * Returns the content of the internal symbol table of the given index.
 * On success, *addrsp and *symbolp point to the address and the symbol of
 * the 'index'th entry of the table, respectively.  If 'index' is not in the
 * range of the symbol table, ISC_R_RANGE will be returned.
 *
 * Requires
 *
 *\li	'addrp' must be non NULL && '*addrp' == NULL.
 *
 *\li	'symbolp' must be non NULL && '*symbolp' == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_RANGE
 */

isc_result_t
isc_backtrace_getsymbol(const void *addr, const char **symbolp,
			unsigned long *offsetp);
/*%<
 * Searches the internal symbol table for the symbol that most matches the
 * given 'addr'.  On success, '*symbolp' will point to the name of function
 * to which the address 'addr' belong, and '*offsetp' will store the offset
 * from the function's entry address to 'addr'.
 *
 * Requires (note that these are not ensured by assertion checks, see above):
 *
 *\li	'symbolp' must be non NULL && '*symbolp' == NULL.
 *
 *\li	'offsetp' must be non NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_FAILURE
 *\li	#ISC_R_NOTFOUND
 */
ISC_LANG_ENDDECLS

#endif	/* ISC_BACKTRACE_H */
