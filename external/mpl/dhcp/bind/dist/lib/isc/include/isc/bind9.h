/*	$NetBSD: bind9.h,v 1.1.2.2 2024/02/24 13:07:23 martin Exp $	*/

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

#ifndef ISC_BIND9_H
#define ISC_BIND9_H 1

#include <stdbool.h>

#include <isc/platform.h>

/*
 * This determines whether we are using the libisc/libdns libraries
 * in BIND9 or in some other application.  For BIND9 (named and related
 * tools) it must be set to true at runtime.  Export library clients
 * will call isc_lib_register(), which will set it to false.
 */
LIBISC_EXTERNAL_DATA extern bool isc_bind9;

#endif /* ISC_BIND9_H */
