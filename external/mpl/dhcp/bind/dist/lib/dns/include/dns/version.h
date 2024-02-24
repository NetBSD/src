/*	$NetBSD: version.h,v 1.1.2.2 2024/02/24 13:07:08 martin Exp $	*/

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

/*! \file dns/version.h */

#ifndef DNS_VERSION_H
#define DNS_VERSION_H 1

#include <isc/platform.h>

LIBDNS_EXTERNAL_DATA extern const char dns_version[];
LIBDNS_EXTERNAL_DATA extern const char dns_major[];
LIBDNS_EXTERNAL_DATA extern const char dns_mapapi[];

#endif /* DNS_VERSION_H */
