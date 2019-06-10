/*	$NetBSD: iterated_hash.h,v 1.3.2.2 2019/06/10 22:04:44 christos Exp $	*/

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

#include <isc/lang.h>

/*
 * The maximal hash length that can be encoded in a name
 * using base32hex.  floor(255/8)*5
 */
#define NSEC3_MAX_HASH_LENGTH 155

/*
 * The maximum has that can be encoded in a single label using
 * base32hex.  floor(63/8)*5
 */
#define NSEC3_MAX_LABEL_HASH 35

ISC_LANG_BEGINDECLS

int
isc_iterated_hash(unsigned char *out,
		  const unsigned int hashalg, const int iterations,
		  const unsigned char *salt, const int saltlength,
		  const unsigned char *in, const int inlength);

ISC_LANG_ENDDECLS
