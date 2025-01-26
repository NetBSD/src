/*	$NetBSD: hashmap.h,v 1.2 2025/01/26 16:25:41 christos Exp $	*/

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

/* ! \file */

#pragma once

#include <inttypes.h>
#include <string.h>

#include <isc/result.h>
#include <isc/types.h>

typedef struct isc_hashmap	isc_hashmap_t;
typedef struct isc_hashmap_iter isc_hashmap_iter_t;

typedef bool (*isc_hashmap_match_fn)(void *node, const void *key);

/*%
 * Create hashmap at *hashmapp, using memory context and size of (1<<bits)
 *
 * Requires:
 * \li	'hashmapp' is not NULL and '*hashmapp' is NULL.
 * \li	'mctx' is a valid memory context.
 * \li	'bits' >=1 and 'bits' <=32
 *
 */
void
isc_hashmap_create(isc_mem_t *mctx, uint8_t bits, isc_hashmap_t **hashmapp);

/*%
 * Destroy hashmap, freeing everything
 *
 * Requires:
 * \li	'*hashmapp' is valid hashmap
 */
void
isc_hashmap_destroy(isc_hashmap_t **hashmapp);

/*%
 * Add a node to hashmap, pointed by binary key 'key' of size 'keysize';
 * set its value to 'value'
 *
 * Requires:
 * \li	'hashmap' is a valid hashmap
 * \li	'hashval' is a precomputed hash value of 'key'
 * \li	'key' is non-null key of size 'keysize'
 *
 * Returns:
 * \li	#ISC_R_EXISTS		-- node of the same key already exists
 * \li	#ISC_R_SUCCESS		-- all is well.
 */
isc_result_t
isc_hashmap_add(isc_hashmap_t *hashmap, const uint32_t hashval,
		isc_hashmap_match_fn match, const void *key, void *value,
		void **foundp);

/*%
 * Find a node matching 'key'/'keysize' in hashmap 'hashmap';
 * if found, set '*valuep' to its value. (If 'valuep' is NULL,
 * then simply return SUCCESS or NOTFOUND to indicate whether the
 * key exists in the hashmap.)
 *
 * Requires:
 * \li	'hashmap' is a valid hashmap
 * \li	'hashval' is a precomputed hash value of 'key'
 * \li	'key' is non-null key of size 'keysize'
 *
 * Returns:
 * \li	#ISC_R_SUCCESS		-- success
 * \li	#ISC_R_NOTFOUND		-- key not found
 */
isc_result_t
isc_hashmap_find(const isc_hashmap_t *hashmap, const uint32_t hashval,
		 isc_hashmap_match_fn match, const void *key, void **valuep);

/*%
 * Delete node from hashmap
 *
 * Requires:
 * \li	'hashmap' is a valid hashmap
 * \li	'hashval' is a precomputed hash value of 'key'
 * \li	'key' is non-null key
 *
 * Returns:
 * \li	#ISC_R_NOTFOUND		-- key not found
 * \li	#ISC_R_SUCCESS		-- all is well
 */
isc_result_t
isc_hashmap_delete(isc_hashmap_t *hashmap, const uint32_t hashval,
		   isc_hashmap_match_fn match, const void *key);

/*%
 * Create an iterator for the hashmap; point '*itp' to it.
 *
 * Requires:
 * \li	'hashmap' is a valid hashmap
 * \li	'itp' is non NULL and '*itp' is NULL.
 */
void
isc_hashmap_iter_create(isc_hashmap_t *hashmap, isc_hashmap_iter_t **itp);

/*%
 * Destroy the iterator '*itp', set it to NULL
 *
 * Requires:
 * \li	'itp' is non NULL and '*itp' is non NULL.
 */
void
isc_hashmap_iter_destroy(isc_hashmap_iter_t **itp);

/*%
 * Set an iterator to the first entry.
 *
 * Requires:
 * \li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- no data in the hashmap
 */
isc_result_t
isc_hashmap_iter_first(isc_hashmap_iter_t *it);

/*%
 * Set an iterator to the next entry.
 *
 * Requires:
 * \li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- end of hashmap reached
 */
isc_result_t
isc_hashmap_iter_next(isc_hashmap_iter_t *it);

/*%
 * Delete current entry and set an iterator to the next entry.
 *
 * Requires:
 * \li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- end of hashmap reached
 */
isc_result_t
isc_hashmap_iter_delcurrent_next(isc_hashmap_iter_t *it);

/*%
 * Set 'value' to the current value under the iterator
 *
 * Requires:
 * \li	'it' is non NULL.
 * \li   'valuep' is non NULL and '*valuep' is NULL.
 */
void
isc_hashmap_iter_current(isc_hashmap_iter_t *it, void **valuep);

/*%
 * Set 'key' to the current key for the value under the iterator
 *
 * Requires:
 * \li	'it' is non NULL.
 * \li   'key' is non NULL and '*key' is NULL.
 * \li	'keysize' is non NULL.
 */
void
isc_hashmap_iter_currentkey(isc_hashmap_iter_t *it, const unsigned char **key);

/*%
 * Returns the number of items in the hashmap.
 *
 * Requires:
 * \li	'hashmap' is a valid hashmap
 */
unsigned int
isc_hashmap_count(isc_hashmap_t *hashmap);
