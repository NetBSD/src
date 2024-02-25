/*	$NetBSD: siphash.h,v 1.6.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

/*! \file isc/siphash.h */

#pragma once

#include <isc/lang.h>
#include <isc/types.h>

#define ISC_SIPHASH24_KEY_LENGTH 128 / 8
#define ISC_SIPHASH24_TAG_LENGTH 64 / 8

#define ISC_HALFSIPHASH24_KEY_LENGTH 64 / 8
#define ISC_HALFSIPHASH24_TAG_LENGTH 32 / 8

ISC_LANG_BEGINDECLS

void
isc_siphash24(const uint8_t *key, const uint8_t *in, const size_t inlen,
	      uint8_t *out);
void
isc_halfsiphash24(const uint8_t *key, const uint8_t *in, const size_t inlen,
		  uint8_t *out);

ISC_LANG_ENDDECLS
