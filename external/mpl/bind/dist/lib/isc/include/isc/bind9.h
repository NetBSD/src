/*	$NetBSD: bind9.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_BIND9_H
#define ISC_BIND9_H 1

#include <isc/boolean.h>
#include <isc/platform.h>

/*
 * This determines whether we are using the libisc/libdns libraries
 * in BIND9 or in some other application.  For BIND9 (named and related
 * tools) it must be set to ISC_TRUE at runtime.  Export library clients
 * will call isc_lib_register(), which will set it to ISC_FALSE.
 */
LIBISC_EXTERNAL_DATA extern isc_boolean_t isc_bind9;

#endif /* ISC_BIND9_H */
