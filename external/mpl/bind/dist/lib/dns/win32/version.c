/*	$NetBSD: version.c,v 1.1.1.1 2018/08/12 12:08:23 christos Exp $	*/

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


#include <versions.h>

#include <dns/version.h>

LIBDNS_EXTERNAL_DATA const char dns_version[] = VERSION;
LIBDNS_EXTERNAL_DATA const char dns_major[] = MAJOR;
LIBDNS_EXTERNAL_DATA const char dns_mapapi[] = MAPAPI;

LIBDNS_EXTERNAL_DATA const unsigned int dns_libinterface = LIBINTERFACE;
LIBDNS_EXTERNAL_DATA const unsigned int dns_librevision = LIBREVISION;
LIBDNS_EXTERNAL_DATA const unsigned int dns_libage = LIBAGE;
