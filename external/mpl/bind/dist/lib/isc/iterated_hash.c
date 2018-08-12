/*	$NetBSD: iterated_hash.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


#include "config.h"

#include <stdio.h>

#include <isc/sha1.h>
#include <isc/iterated_hash.h>

int
isc_iterated_hash(unsigned char out[ISC_SHA1_DIGESTLENGTH],
		  unsigned int hashalg, int iterations,
		  const unsigned char *salt, int saltlength,
		  const unsigned char *in, int inlength)
{
	isc_sha1_t ctx;
	int n = 0;

	if (hashalg != 1)
		return (0);

	do {
		isc_sha1_init(&ctx);
		isc_sha1_update(&ctx, in, inlength);
		isc_sha1_update(&ctx, salt, saltlength);
		isc_sha1_final(&ctx, out);
		in = out;
		inlength = ISC_SHA1_DIGESTLENGTH;
	} while (n++ < iterations);

	return (ISC_SHA1_DIGESTLENGTH);
}
