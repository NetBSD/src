/*	$NetBSD: ipv6.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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


#include <isc/net.h>
#include <isc/platform.h>

LIBISC_EXTERNAL_DATA const struct in6_addr isc_in6addr_any =
	IN6ADDR_ANY_INIT;

LIBISC_EXTERNAL_DATA const struct in6_addr isc_in6addr_loopback =
	IN6ADDR_LOOPBACK_INIT;
