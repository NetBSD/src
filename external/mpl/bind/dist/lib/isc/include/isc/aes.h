/*	$NetBSD: aes.h,v 1.6.2.1 2024/02/25 15:47:19 martin Exp $	*/

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

/*! \file isc/aes.h */

#pragma once

#include <isc/lang.h>
#include <isc/types.h>

#define ISC_AES128_KEYLENGTH 16U
#define ISC_AES192_KEYLENGTH 24U
#define ISC_AES256_KEYLENGTH 32U
#define ISC_AES_BLOCK_LENGTH 16U

ISC_LANG_BEGINDECLS

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out);

void
isc_aes192_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out);

void
isc_aes256_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out);

ISC_LANG_ENDDECLS
