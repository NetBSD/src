/*	$NetBSD: hash.h,v 1.3 2019/01/09 16:55:15 christos Exp $	*/

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

uint32_t
isc_hash_function(const void *data, size_t length,
		  bool case_sensitive,
		  const uint32_t *previous_hashp);
uint32_t
isc_hash_function_reverse(const void *data, size_t length,
			  bool case_sensitive,
			  const uint32_t *previous_hashp);
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
 * input data. isc_hash_function_reverse() calculates the hash from the
 * end to the start over the input data. The difference in order is
 * useful in incremental hashing; for example, a previously hashed
 * value for 'com' can be used as input when hashing 'example.com'.
 *
 * 'data' is the data to be hashed.
 *
 * 'length' is the size of the data to be hashed.
 *
 * 'case_sensitive' specifies whether the hash key should be treated as
 * case_sensitive values.  It should typically be false if the hash key
 * is a DNS name.
 *
 * 'previous_hashp' is a pointer to a previous hash value returned by
 * this function. It can be used to perform incremental hashing. NULL
 * must be passed during first calls.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_HASH_H */
