/*	$NetBSD: entropy.c,v 1.8 2025/01/26 16:25:36 christos Exp $	*/

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
#include <isc/types.h>
#include <isc/util.h>
#include <isc/uv.h>

void
isc_entropy_get(void *buf, size_t buflen) {
	int r = uv_random(NULL, NULL, buf, buflen, 0, NULL);

	UV_RUNTIME_CHECK(uv_random, r);
}
