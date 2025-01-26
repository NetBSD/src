/*	$NetBSD: byaddr.c,v 1.10 2025/01/26 16:25:21 christos Exp $	*/

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

/*! \file */

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/db.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/view.h>

/*
 * XXXRTH  We could use a static event...
 */

static char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
			     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

isc_result_t
dns_byaddr_createptrname(const isc_netaddr_t *address, dns_name_t *name) {
	char textname[128];
	const unsigned char *bytes;
	int i;
	char *cp;
	isc_buffer_t buffer;
	unsigned int len;

	REQUIRE(address != NULL);

	/*
	 * We create the text representation and then convert to a
	 * dns_name_t.  This is not maximally efficient, but it keeps all
	 * of the knowledge of wire format in the dns_name_ routines.
	 */

	bytes = (const unsigned char *)(&address->type);
	if (address->family == AF_INET) {
		(void)snprintf(textname, sizeof(textname),
			       "%u.%u.%u.%u.in-addr.arpa.",
			       ((unsigned int)bytes[3] & 0xffU),
			       ((unsigned int)bytes[2] & 0xffU),
			       ((unsigned int)bytes[1] & 0xffU),
			       ((unsigned int)bytes[0] & 0xffU));
	} else if (address->family == AF_INET6) {
		size_t remaining;

		cp = textname;
		for (i = 15; i >= 0; i--) {
			*cp++ = hex_digits[bytes[i] & 0x0f];
			*cp++ = '.';
			*cp++ = hex_digits[(bytes[i] >> 4) & 0x0f];
			*cp++ = '.';
		}
		remaining = sizeof(textname) - (cp - textname);
		strlcpy(cp, "ip6.arpa.", remaining);
	} else {
		return ISC_R_NOTIMPLEMENTED;
	}

	len = (unsigned int)strlen(textname);
	isc_buffer_init(&buffer, textname, len);
	isc_buffer_add(&buffer, len);
	return dns_name_fromtext(name, &buffer, dns_rootname, 0, NULL);
}
