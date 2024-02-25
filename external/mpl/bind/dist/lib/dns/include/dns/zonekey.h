/*	$NetBSD: zonekey.h,v 1.6.2.1 2024/02/25 15:46:59 martin Exp $	*/

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

/*! \file dns/zonekey.h */

#include <stdbool.h>

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

bool
dns_zonekey_iszonekey(dns_rdata_t *keyrdata);
/*%<
 *	Determines if the key record contained in the rdata is a zone key.
 *
 *	Requires:
 *		'keyrdata' is not NULL.
 */

ISC_LANG_ENDDECLS
