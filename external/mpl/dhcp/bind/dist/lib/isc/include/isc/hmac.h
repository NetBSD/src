/*	$NetBSD: hmac.h,v 1.1.2.2 2024/02/24 13:07:25 martin Exp $	*/

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

/*!
 * \file isc/hmac.h
 * \brief This is the header for for message authentication code.
 */

#pragma once

#include <isc/lang.h>
#include <isc/md.h>
#include <isc/platform.h>
#include <isc/result.h>
#include <isc/types.h>

typedef void isc_hmac_t;

/**
 * isc_hmac:
 * @type: the digest type
 * @key: the key
 * @keylen: the length of the key
 * @buf: data to hash
 * @len: length of the data to hash
 * @digest: the output buffer
 * @digestlen: the length of the data written to @digest
 *
 * This function computes the message authentication code using a digest type
 * @type with key @key which is @keylen bytes long from data in @buf which is
 * @len bytes long, and places the output into @digest, which must have space
 * for the hash function output (use ISC_MAX_MD_SIZE if unsure).  If the
 * @digestlen parameter is not NULL then the number of bytes of data written
 * (i.e. the length of the digest) will be written to the @digestlen.
 */
isc_result_t
isc_hmac(const isc_md_type_t *type, const void *key, const int keylen,
	 const unsigned char *buf, const size_t len, unsigned char *digest,
	 unsigned int *digestlen);

/**
 * isc_hmac_new:
 *
 * This function allocates, initializes and returns HMAC context.
 */
isc_hmac_t *
isc_hmac_new(void);

/**
 * isc_hmac_free:
 * @md: HMAC context
 *
 * This function cleans up HMAC context and frees up the space allocated to it.
 */
void
isc_hmac_free(isc_hmac_t *hmac);

/**
 * isc_hmac_init:
 * @md: HMAC context
 * @key: HMAC key
 * @keylen: HMAC key length
 * @type: digest type
 *
 * This function sets up HMAC context to use a hash function of @type and key
 * @key which is @keylen bytes long.
 */

isc_result_t
isc_hmac_init(isc_hmac_t *hmac, const void *key, size_t keylen,
	      const isc_md_type_t *type);

/**
 * isc_hmac_reset:
 * @hmac: HMAC context
 *
 * This function resets the HMAC context.  This can be used to reuse an already
 * existing context.
 */
isc_result_t
isc_hmac_reset(isc_hmac_t *hmac);

/**
 * isc_hmac_update:
 * @hmac: HMAC context
 * @buf: data to hash
 * @len: length of the data to hash
 *
 * This function can be called repeatedly with chunks of the message @buf to be
 * authenticated which is @len bytes long.
 */
isc_result_t
isc_hmac_update(isc_hmac_t *hmac, const unsigned char *buf, const size_t len);

/**
 * isc_hmac_final:
 * @hmac: HMAC context
 * @digest: the output buffer
 * @digestlen: the length of the data written to @digest
 *
 * This function retrieves the message authentication code from @hmac and places
 * it in @digest, which must have space for the hash function output.  If the
 * @digestlen parameter is not NULL then the number of bytes of data written
 * (i.e. the length of the digest) will be written to the @digestlen.  After
 * calling this function no additional calls to isc_hmac_update() can be made.
 */
isc_result_t
isc_hmac_final(isc_hmac_t *hmac, unsigned char *digest,
	       unsigned int *digestlen);

/**
 * isc_hmac_md_type:
 * @hmac: HMAC context
 *
 * This function return the isc_md_type_t previously set for the supplied
 * HMAC context or NULL if no isc_md_type_t has been set.
 */
const isc_md_type_t *
isc_hmac_get_md_type(isc_hmac_t *hmac);

/**
 * isc_hmac_get_size:
 *
 * This function return the size of the message digest when passed an isc_hmac_t
 * structure, i.e. the size of the hash.
 */
size_t
isc_hmac_get_size(isc_hmac_t *hmac);

/**
 * isc_hmac_get_block_size:
 *
 * This function return the block size of the message digest when passed an
 * isc_hmac_t structure.
 */
int
isc_hmac_get_block_size(isc_hmac_t *hmac);
