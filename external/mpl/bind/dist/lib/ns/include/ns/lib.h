/*	$NetBSD: lib.h,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#ifndef NS_LIB_H
#define NS_LIB_H 1

/*! \file include/ns/lib.h */

#include <isc/types.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

LIBNS_EXTERNAL_DATA extern unsigned int ns_pps;
LIBNS_EXTERNAL_DATA extern isc_msgcat_t *ns_msgcat;

isc_result_t
ns_lib_init(void);
/*%<
 * A set of initialization procedures used in the NS library.
 */

void
ns_lib_shutdown(void);
/*%<
 * Free temporary resources allocated in ns_lib_init().
 */

ISC_LANG_ENDDECLS

#endif /* NS_LIB_H */
