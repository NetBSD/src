/*	$NetBSD: entropy.c,v 1.6.2.1 2024/02/25 15:47:15 martin Exp $	*/

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

#include <openssl/err.h>
#include <openssl/rand.h>

#include <isc/types.h>
#include <isc/util.h>

#include "entropy_private.h"

void
isc_entropy_get(void *buf, size_t buflen) {
	if (RAND_bytes(buf, buflen) < 1) {
		FATAL_ERROR("RAND_bytes(): %s",
			    ERR_error_string(ERR_get_error(), NULL));
	}
}
