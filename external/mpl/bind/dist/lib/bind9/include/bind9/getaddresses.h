/*	$NetBSD: getaddresses.h,v 1.5.2.1 2024/02/25 15:46:47 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

/*! \file bind9/getaddresses.h */

#include <isc/lang.h>
#include <isc/net.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
bind9_getaddresses(const char *hostname, in_port_t port, isc_sockaddr_t *addrs,
		   int addrsize, int *addrcount);
/*%<
 * Use the system resolver to get the addresses associated with a hostname.
 * If successful, the number of addresses found is returned in 'addrcount'.
 * If a hostname lookup is performed and addresses of an unknown family is
 * seen, it is ignored.  If more than 'addrsize' addresses are seen, the
 * first 'addrsize' are returned and the remainder silently truncated.
 *
 * This routine may block.  If called by a program using the isc_app
 * framework, it should be surrounded by isc_app_block()/isc_app_unblock().
 *
 *  Requires:
 *\li	'hostname' is not NULL.
 *\li	'addrs' is not NULL.
 *\li	'addrsize' > 0
 *\li	'addrcount' is not NULL.
 *
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOTFOUND
 *\li	#ISC_R_FAMILYNOSUPPORT - 'hostname' is an IPv6 address, and IPv6 is
 *		not supported.
 */

ISC_LANG_ENDDECLS
