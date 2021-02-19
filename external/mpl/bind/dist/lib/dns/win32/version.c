/*	$NetBSD: version.c,v 1.5 2021/02/19 16:42:18 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <versions.h>

#include <dns/version.h>

LIBDNS_EXTERNAL_DATA const char dns_version[] = VERSION;
LIBDNS_EXTERNAL_DATA const char dns_major[] = MAJOR;
LIBDNS_EXTERNAL_DATA const char dns_mapapi[] = MAPAPI;
