/*	$NetBSD: bit.h,v 1.1.1.6 2024/02/21 21:54:53 christos Exp $	*/

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

#pragma once

/*! \file dns/bit.h */

#include <inttypes.h>

typedef uint64_t dns_bitset_t;

#define DNS_BIT_SET(bit, bitset)   (*(bitset) |= ((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CLEAR(bit, bitset) (*(bitset) &= ~((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CHECK(bit, bitset) \
	((*(bitset) & ((dns_bitset_t)1 << (bit))) == ((dns_bitset_t)1 << (bit)))
