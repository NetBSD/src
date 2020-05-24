/*	$NetBSD: bit.h,v 1.4 2020/05/24 19:46:23 christos Exp $	*/

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

#ifndef DNS_BIT_H
#define DNS_BIT_H 1

/*! \file dns/bit.h */

#include <inttypes.h>

typedef uint64_t dns_bitset_t;

#define DNS_BIT_SET(bit, bitset)   (*(bitset) |= ((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CLEAR(bit, bitset) (*(bitset) &= ~((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CHECK(bit, bitset) \
	((*(bitset) & ((dns_bitset_t)1 << (bit))) == ((dns_bitset_t)1 << (bit)))

#endif /* DNS_BIT_H */
