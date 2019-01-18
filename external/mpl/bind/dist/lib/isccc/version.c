/*	$NetBSD: version.c,v 1.2.2.3 2019/01/18 08:50:02 pgoyette Exp $	*/

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

#include <config.h>

#include <isccc/version.h>

const char isccc_version[] = VERSION;

const unsigned int isccc_libinterface = LIBINTERFACE;
const unsigned int isccc_librevision = LIBREVISION;
const unsigned int isccc_libage = LIBAGE;
