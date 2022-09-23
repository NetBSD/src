/*	$NetBSD: ipv6.c,v 1.6 2022/09/23 12:15:34 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <isc/net.h>
#include <isc/platform.h>

LIBISC_EXTERNAL_DATA const struct in6_addr isc_in6addr_loopback =
	IN6ADDR_LOOPBACK_INIT;
