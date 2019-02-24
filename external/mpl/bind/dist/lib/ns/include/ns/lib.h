/*	$NetBSD: lib.h,v 1.3 2019/02/24 20:01:33 christos Exp $	*/

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

#pragma once

/*! \file include/ns/lib.h */

#include <isc/types.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

LIBNS_EXTERNAL_DATA extern unsigned int ns_pps;

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
