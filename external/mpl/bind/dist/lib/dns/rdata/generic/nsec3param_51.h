/*	$NetBSD: nsec3param_51.h,v 1.1.1.5 2024/02/21 21:54:53 christos Exp $	*/

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

/*!
 * \brief Per RFC 5155 */

#include <isc/iterated_hash.h>

typedef struct dns_rdata_nsec3param {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_hash_t hash;
	unsigned char flags; /* DNS_NSEC3FLAG_* */
	dns_iterations_t iterations;
	unsigned char salt_length;
	unsigned char *salt;
} dns_rdata_nsec3param_t;
