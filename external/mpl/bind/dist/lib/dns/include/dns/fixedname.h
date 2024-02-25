/*	$NetBSD: fixedname.h,v 1.6.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file dns/fixedname.h
 * \brief
 * Fixed-size Names
 *
 * dns_fixedname_t is a convenience type containing a name, an offsets
 * table, and a dedicated buffer big enough for the longest possible
 * name. This is typically used for stack-allocated names.
 *
 * MP:
 *\li	The caller must ensure any required synchronization.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	Per dns_fixedname_t:
 *\code
 *		sizeof(dns_name_t) + sizeof(dns_offsets_t) +
 *		sizeof(isc_buffer_t) + 255 bytes + structure padding
 *\endcode
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	None.
 */

/*****
***** Imports
*****/

#include <isc/buffer.h>
#include <isc/lang.h>

#include <dns/name.h>

/*****
***** Types
*****/

struct dns_fixedname {
	dns_name_t    name;
	dns_offsets_t offsets;
	isc_buffer_t  buffer;
	unsigned char data[DNS_NAME_MAXWIRE];
};

ISC_LANG_BEGINDECLS

void
dns_fixedname_init(dns_fixedname_t *fixed);

void
dns_fixedname_invalidate(dns_fixedname_t *fixed);

dns_name_t *
dns_fixedname_name(dns_fixedname_t *fixed);

dns_name_t *
dns_fixedname_initname(dns_fixedname_t *fixed);

ISC_LANG_ENDDECLS
