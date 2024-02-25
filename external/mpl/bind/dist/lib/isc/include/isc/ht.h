/*	$NetBSD: ht.h,v 1.6.2.1 2024/02/25 15:47:21 martin Exp $	*/

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

typedef struct isc_ht	   isc_ht_t;
typedef struct isc_ht_iter isc_ht_iter_t;

enum { ISC_HT_CASE_SENSITIVE = 0x00, ISC_HT_CASE_INSENSITIVE = 0x01 };

/*%
 * Initialize hashtable at *htp, using memory context and size of (1<<bits)
 *
 * If 'options' contains ISC_HT_CASE_INSENSITIVE, then upper- and lower-case
 * letters in key values will generate the same hash values; this can be used
 * when the key for a hash table is a DNS name.
 *
 * Requires:
 *\li	'htp' is not NULL and '*htp' is NULL.
 *\li	'mctx' is a valid memory context.
 *\li	'bits' >=1 and 'bits' <=32
 *
 */
void
isc_ht_init(isc_ht_t **htp, isc_mem_t *mctx, uint8_t bits,
	    unsigned int options);

/*%
 * Destroy hashtable, freeing everything
 *
 * Requires:
 * \li	'*htp' is valid hashtable
 */
void
isc_ht_destroy(isc_ht_t **htp);

/*%
 * Add a node to hashtable, pointed by binary key 'key' of size 'keysize';
 * set its value to 'value'
 *
 * Requires:
 *\li	'ht' is a valid hashtable
 *\li   write-lock
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY		-- not enough memory to create pool
 *\li	#ISC_R_EXISTS		-- node of the same key already exists
 *\li	#ISC_R_SUCCESS		-- all is well.
 */
isc_result_t
isc_ht_add(isc_ht_t *ht, const unsigned char *key, const uint32_t keysize,
	   void *value);

/*%
 * Find a node matching 'key'/'keysize' in hashtable 'ht';
 * if found, set '*valuep' to its value. (If 'valuep' is NULL,
 * then simply return SUCCESS or NOTFOUND to indicate whether the
 * key exists in the hashtable.)
 *
 * Requires:
 * \li	'ht' is a valid hashtable
 * \li  read-lock
 *
 * Returns:
 * \li	#ISC_R_SUCCESS		-- success
 * \li	#ISC_R_NOTFOUND		-- key not found
 */
isc_result_t
isc_ht_find(const isc_ht_t *ht, const unsigned char *key,
	    const uint32_t keysize, void **valuep);

/*%
 * Delete node from hashtable
 *
 * Requires:
 *\li	ht is a valid hashtable
 *\li   write-lock
 *
 * Returns:
 *\li	#ISC_R_NOTFOUND		-- key not found
 *\li	#ISC_R_SUCCESS		-- all is well
 */
isc_result_t
isc_ht_delete(isc_ht_t *ht, const unsigned char *key, const uint32_t keysize);

/*%
 * Create an iterator for the hashtable; point '*itp' to it.
 *
 * Requires:
 *\li	'ht' is a valid hashtable
 *\li	'itp' is non NULL and '*itp' is NULL.
 */
void
isc_ht_iter_create(isc_ht_t *ht, isc_ht_iter_t **itp);

/*%
 * Destroy the iterator '*itp', set it to NULL
 *
 * Requires:
 *\li	'itp' is non NULL and '*itp' is non NULL.
 */
void
isc_ht_iter_destroy(isc_ht_iter_t **itp);

/*%
 * Set an iterator to the first entry.
 *
 * Requires:
 *\li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- no data in the hashtable
 */
isc_result_t
isc_ht_iter_first(isc_ht_iter_t *it);

/*%
 * Set an iterator to the next entry.
 *
 * Requires:
 *\li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- end of hashtable reached
 */
isc_result_t
isc_ht_iter_next(isc_ht_iter_t *it);

/*%
 * Delete current entry and set an iterator to the next entry.
 *
 * Requires:
 *\li	'it' is non NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	-- success
 * \li	#ISC_R_NOMORE	-- end of hashtable reached
 */
isc_result_t
isc_ht_iter_delcurrent_next(isc_ht_iter_t *it);

/*%
 * Set 'value' to the current value under the iterator
 *
 * Requires:
 *\li	'it' is non NULL.
 *\li   'valuep' is non NULL and '*valuep' is NULL.
 */
void
isc_ht_iter_current(isc_ht_iter_t *it, void **valuep);

/*%
 * Set 'key' and 'keysize to the current key and keysize for the value
 * under the iterator
 *
 * Requires:
 *\li	'it' is non NULL.
 *\li   'key' is non NULL and '*key' is NULL.
 *\li	'keysize' is non NULL.
 */
void
isc_ht_iter_currentkey(isc_ht_iter_t *it, unsigned char **key, size_t *keysize);

/*%
 * Returns the number of items in the hashtable.
 *
 * Requires:
 *\li	'ht' is a valid hashtable
 */
size_t
isc_ht_count(const isc_ht_t *ht);
