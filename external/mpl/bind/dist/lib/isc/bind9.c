/*	$NetBSD: bind9.c,v 1.1.1.1 2018/08/12 12:08:24 christos Exp $	*/

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

#include <isc/bind9.h>

/*
 * This determines whether we are using the libisc/libdns libraries
 * in BIND9 or in some other application. It is initialized to ISC_TRUE
 * and remains unchanged for BIND9 and related tools; export library
 * clients will run isc_lib_register(), which sets it to ISC_FALSE,
 * overriding certain BIND9 behaviors.
 */
LIBISC_EXTERNAL_DATA isc_boolean_t isc_bind9 = ISC_TRUE;
