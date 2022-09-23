/*	$NetBSD: fuzz.h,v 1.4 2022/09/23 12:15:33 christos Exp $	*/

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

#ifndef ISC_FUZZ_H
#define ISC_FUZZ_H

typedef enum {
	isc_fuzz_none,
	isc_fuzz_client,
	isc_fuzz_tcpclient,
	isc_fuzz_resolver,
	isc_fuzz_http,
	isc_fuzz_rndc
} isc_fuzztype_t;

#endif /* ISC_FUZZ_H */
