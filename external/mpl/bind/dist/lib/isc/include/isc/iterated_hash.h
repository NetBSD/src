/*	$NetBSD: iterated_hash.h,v 1.8 2026/04/08 00:16:16 christos Exp $	*/

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

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength);

/*
 * Private
 */

void
isc__iterated_hash_initialize(void);
void
isc__iterated_hash_shutdown(void);

ISC_LANG_ENDDECLS
