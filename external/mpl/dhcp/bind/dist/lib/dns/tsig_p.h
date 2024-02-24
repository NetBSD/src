/*	$NetBSD: tsig_p.h,v 1.1.2.2 2024/02/24 13:07:02 martin Exp $	*/

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

#ifndef DNS_TSIG_P_H
#define DNS_TSIG_P_H

/*! \file */

#include <stdbool.h>

#include <isc/result.h>

#include <dns/types.h>

/*%
 *     These functions must not be used outside this module and
 *     its associated unit tests.
 */

ISC_LANG_BEGINDECLS

bool
dns__tsig_algvalid(unsigned int alg);
unsigned int
dns__tsig_algfromname(const dns_name_t *algorithm);
const dns_name_t *
dns__tsig_algnamefromname(const dns_name_t *algorithm);
bool
dns__tsig_algallocated(const dns_name_t *algorithm);

ISC_LANG_ENDDECLS

#endif /* DNS_TSIG_P_H */
