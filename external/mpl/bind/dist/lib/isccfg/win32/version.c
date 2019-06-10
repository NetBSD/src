/*	$NetBSD: version.c,v 1.3.2.2 2019/06/10 22:04:49 christos Exp $	*/

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

#include <isccfg/version.h>

LIBISCCFG_EXTERNAL_DATA const char cfg_version[] = VERSION;

LIBISCCFG_EXTERNAL_DATA const unsigned int cfg_libinterface = LIBINTERFACE;
LIBISCCFG_EXTERNAL_DATA const unsigned int cfg_librevision = LIBREVISION;
LIBISCCFG_EXTERNAL_DATA const unsigned int cfg_libage = LIBAGE;

