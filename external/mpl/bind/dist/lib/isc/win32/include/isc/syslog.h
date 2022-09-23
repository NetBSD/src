/*	$NetBSD: syslog.h,v 1.5 2022/09/23 12:15:35 christos Exp $	*/

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

#ifndef ISC_SYSLOG_H
#define ISC_SYSLOG_H 1

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
isc_syslog_facilityfromstring(const char *str, int *facilityp);
/*
 * Convert 'str' to the appropriate syslog facility constant.
 *
 * Requires:
 *
 *	'str' is not NULL
 *	'facilityp' is not NULL
 *
 * Returns:
 * 	ISC_R_SUCCESS
 * 	ISC_R_NOTFOUND
 */

ISC_LANG_ENDDECLS

#endif /* ISC_SYSLOG_H */
