/*	$NetBSD: lib.h,v 1.1.4.2 2024/02/29 11:38:46 martin Exp $	*/

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

#ifndef DNS_LIB_H
#define DNS_LIB_H 1

/*! \file dns/lib.h */

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/*%
 * Tuning: external query load in packets per seconds.
 */
LIBDNS_EXTERNAL_DATA extern unsigned int dns_pps;

isc_result_t
dns_lib_init(void);
/*%<
 * A set of initialization procedures used in the DNS library.  This function
 * is provided for an application that is not aware of the underlying ISC or
 * DNS libraries much.
 */

void
dns_lib_shutdown(void);
/*%<
 * Free temporary resources allocated in dns_lib_init().
 */

ISC_LANG_ENDDECLS

#endif /* DNS_LIB_H */
