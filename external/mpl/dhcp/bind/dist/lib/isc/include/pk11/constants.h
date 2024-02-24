/*	$NetBSD: constants.h,v 1.1.2.2 2024/02/24 13:07:28 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

#include <inttypes.h>

/*! \file pk11/constants.h */

/*%
 * Static arrays of data used for key template initialization
 */
#define PK11_ECC_PRIME256V1                                                \
	(uint8_t[]) {                                                      \
		0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 \
	}
#define PK11_ECC_SECP384R1 \
	(uint8_t[]) { 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22 }
#define PK11_ECX_ED25519                                                     \
	(uint8_t[]) {                                                        \
		0x13, 0xc, 'e', 'd', 'w', 'a', 'r', 'd', 's', '2', '5', '5', \
			'1', '9'                                             \
	}
#define PK11_ECX_ED448                                                      \
	(uint8_t[]) {                                                       \
		0x13, 0xa, 'e', 'd', 'w', 'a', 'r', 'd', 's', '4', '4', '8' \
	}
