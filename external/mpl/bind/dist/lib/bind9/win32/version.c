/*	$NetBSD: version.c,v 1.2.2.3 2019/01/18 08:49:53 pgoyette Exp $	*/

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

#include <bind9/version.h>

LIBBIND9_EXTERNAL_DATA const char bind9_version[] = VERSION;

LIBBIND9_EXTERNAL_DATA const unsigned int bind9_libinterface = LIBINTERFACE;
LIBBIND9_EXTERNAL_DATA const unsigned int bind9_librevision = LIBREVISION;
LIBBIND9_EXTERNAL_DATA const unsigned int bind9_libage = LIBAGE;
