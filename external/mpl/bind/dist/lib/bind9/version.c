/*	$NetBSD: version.c,v 1.3.2.2 2019/06/10 22:04:34 christos Exp $	*/

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

#include <bind9/version.h>

const char bind9_version[] = VERSION;

const unsigned int bind9_libinterface = LIBINTERFACE;
const unsigned int bind9_librevision = LIBREVISION;
const unsigned int bind9_libage = LIBAGE;
