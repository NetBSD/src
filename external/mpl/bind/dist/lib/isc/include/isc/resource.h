/*	$NetBSD: resource.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


#ifndef ISC_RESOURCE_H
#define ISC_RESOURCE_H 1

/*! \file isc/resource.h */

#include <isc/lang.h>
#include <isc/types.h>

#define ISC_RESOURCE_UNLIMITED ((isc_resourcevalue_t)ISC_UINT64_MAX)

ISC_LANG_BEGINDECLS

isc_result_t
isc_resource_setlimit(isc_resource_t resource, isc_resourcevalue_t value);
/*%<
 * Set the maximum limit for a system resource.
 *
 * Notes:
 *\li	If 'value' exceeds the maximum possible on the operating system,
 *	it is silently limited to that maximum -- or to "infinity", if
 *	the operating system has that concept.  #ISC_RESOURCE_UNLIMITED
 *	can be used to explicitly ask for the maximum.
 *
 * Requires:
 *\li	'resource' is a valid member of the isc_resource_t enumeration.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	Success.
 *\li	#ISC_R_NOTIMPLEMENTED	'resource' is not a type known by the OS.
 *\li	#ISC_R_NOPERM	The calling process did not have adequate permission
 *			to change the resource limit.
 */

isc_result_t
isc_resource_getlimit(isc_resource_t resource, isc_resourcevalue_t *value);
/*%<
 * Get the maximum limit for a system resource.
 *
 * Notes:
 *\li	'value' is set to the maximum limit.
 *
 *\li	#ISC_RESOURCE_UNLIMITED is the maximum value of isc_resourcevalue_t.
 *
 *\li	On many (all?) Unix systems, RLIM_INFINITY is a valid value that is
 *	significantly less than #ISC_RESOURCE_UNLIMITED, but which in practice
 *	behaves the same.
 *
 *\li	The current ISC libdns configuration file parser assigns a value
 *	of ISC_UINT32_MAX for a size_spec of "unlimited" and ISC_UNIT32_MAX - 1
 *	for "default", the latter of which is supposed to represent "the
 *	limit that was in force when the server started".  Since these are
 *	valid values in the middle of the range of isc_resourcevalue_t,
 *	there is the possibility for confusion over what exactly those
 *	particular values are supposed to represent in a particular context --
 *	discrete integral values or generalized concepts.
 *
 * Requires:
 *\li	'resource' is a valid member of the isc_resource_t enumeration.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Success.
 *\li	#ISC_R_NOTIMPLEMENTED	'resource' is not a type known by the OS.
 */

isc_result_t
isc_resource_getcurlimit(isc_resource_t resource, isc_resourcevalue_t *value);
/*%<
 * Same as isc_resource_getlimit(), but returns the current (soft) limit.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Success.
 *\li	#ISC_R_NOTIMPLEMENTED	'resource' is not a type known by the OS.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_RESOURCE_H */

