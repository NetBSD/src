/*	$NetBSD: version.c,v 1.1.1.1 2018/08/12 12:08:29 christos Exp $	*/

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

#include <isccc/version.h>

LIBISCCC_EXTERNAL_DATA const char isccc_version[] = VERSION;

LIBISCCC_EXTERNAL_DATA const unsigned int isccc_libinterface = LIBINTERFACE;
LIBISCCC_EXTERNAL_DATA const unsigned int isccc_librevision = LIBREVISION;
LIBISCCC_EXTERNAL_DATA const unsigned int isccc_libage = LIBAGE;
