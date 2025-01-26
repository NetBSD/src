/*	$NetBSD: old.h,v 1.1.1.1 2025/01/26 16:12:29 christos Exp $	*/

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

/*%
 * For verifying no functional change in the rewrite of dns_name_fromwire()
 */
isc_result_t
old_name_fromwire(dns_name_t *name, isc_buffer_t *source, dns_decompress_t dctx,
		  unsigned int options, isc_buffer_t *target);
