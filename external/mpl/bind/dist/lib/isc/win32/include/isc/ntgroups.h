/*	$NetBSD: ntgroups.h,v 1.1.1.4 2022/09/23 12:09:23 christos Exp $	*/

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

#ifndef ISC_NTGROUPS_H
#define ISC_NTGROUPS_H 1

#include <isc/lang.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

isc_result_t
isc_ntsecurity_getaccountgroups(char *name, char **Groups,
				unsigned int maxgroups, unsigned int *total);

ISC_LANG_ENDDECLS

#endif /* ISC_NTGROUPS_H */
