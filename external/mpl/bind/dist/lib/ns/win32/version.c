/*	$NetBSD: version.c,v 1.2 2018/08/12 13:02:41 christos Exp $	*/

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

#include <versions.h>

#include <ns/version.h>

LIBNS_EXTERNAL_DATA const char ns_version[] = VERSION;
LIBNS_EXTERNAL_DATA const char ns_major[] = MAJOR;
LIBNS_EXTERNAL_DATA const char ns_mapapi[] = MAPAPI;

LIBNS_EXTERNAL_DATA const unsigned int ns_libinterface = LIBINTERFACE;
LIBNS_EXTERNAL_DATA const unsigned int ns_librevision = LIBREVISION;
LIBNS_EXTERNAL_DATA const unsigned int ns_libage = LIBAGE;
