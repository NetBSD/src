/*	$NetBSD: version.c,v 1.3 2019/01/09 16:55:14 christos Exp $	*/

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

#include <config.h>

#include <versions.h>

#include <irs/version.h>

LIBIRS_EXTERNAL_DATA const char irs_version[] = VERSION;

LIBIRS_EXTERNAL_DATA const unsigned int irs_libinterface = LIBINTERFACE;
LIBIRS_EXTERNAL_DATA const unsigned int irs_librevision = LIBREVISION;
LIBIRS_EXTERNAL_DATA const unsigned int irs_libage = LIBAGE;
