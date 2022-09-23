/*	$NetBSD: lib.h,v 1.6 2022/09/23 12:15:36 christos Exp $	*/

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

/*! \file include/ns/lib.h */

#include <isc/lang.h>
#include <isc/types.h>

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
