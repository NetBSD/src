/*	$NetBSD: meminfo.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#ifndef ISC_MEMINFO_H
#define ISC_MEMINFO_H 1

#include <isc/types.h>

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

isc_uint64_t
isc_meminfo_totalphys(void);
/*%<
 * Return total available physical memory in bytes, or 0 if this cannot
 * be determined
*/

ISC_LANG_ENDDECLS

#endif /* ISC_MEMINFO_H */
