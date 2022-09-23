/*	$NetBSD: meminfo.h,v 1.1.1.5 2022/09/23 12:09:22 christos Exp $	*/

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

#ifndef ISC_MEMINFO_H
#define ISC_MEMINFO_H 1

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

uint64_t
isc_meminfo_totalphys(void);
/*%<
 * Return total available physical memory in bytes, or 0 if this cannot
 * be determined
 */

ISC_LANG_ENDDECLS

#endif /* ISC_MEMINFO_H */
