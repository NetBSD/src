/*	$NetBSD: nonce.c,v 1.3 2020/05/24 19:46:26 christos Exp $	*/

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

#include <isc/nonce.h>

#include "entropy_private.h"

void
isc_nonce_buf(void *buf, size_t buflen) {
	isc_entropy_get(buf, buflen);
}
