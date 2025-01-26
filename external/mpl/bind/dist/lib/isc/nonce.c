/*	$NetBSD: nonce.c,v 1.1.1.5 2025/01/26 16:12:30 christos Exp $	*/

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

#include <isc/entropy.h>
#include <isc/nonce.h>

void
isc_nonce_buf(void *buf, size_t buflen) {
	isc_entropy_get(buf, buflen);
}
