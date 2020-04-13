/*	$NetBSD: hash.h,v 1.3.2.3 2020/04/13 08:02:58 martin Exp $	*/

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

#ifndef ISC_HASH_H
#define ISC_HASH_H 1

#include <inttypes.h>
#include <stdbool.h>

#include "isc/lang.h"
#include "isc/types.h"

/***
 *** Functions
 ***/
ISC_LANG_BEGINDECLS

const void *
isc_hash_get_initializer(void);

void
isc_hash_set_initializer(const void *initializer);

uint64_t
isc_hash_function(const void *data, const size_t length,
		  const bool case_sensitive);
/*!<
 * \brief Calculate a hash over data.
 *
 * This hash function is useful for hashtables. The hash function is
 * opaque and not important to the caller. The returned hash values are
 * non-deterministic and will have different mapping every time a
 * process using this library is run, but will have uniform
 * distribution.
 *
 * isc_hash_function() calculates the hash from start to end over the
 * input data.
 *
 * 'data' is the data to be hashed.
 *
 * 'length' is the size of the data to be hashed.
 *
 * 'case_sensitive' specifies whether the hash key should be treated as
 * case_sensitive values.  It should typically be false if the hash key
 * is a DNS name.
 *
 * WARNING: In case of case insensitive input, the input buffer cannot
 * be longer than 1024, which should be fine, as it is only used for
 * DNS names.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_HASH_H */
