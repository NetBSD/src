/*	$NetBSD: util.h,v 1.4.2.1 2024/02/25 15:44:08 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) Red Hat
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND AUTHORS DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Memory allocation and error handling utilities.
 */

#pragma once

#include <isc/mem.h>

#include <dns/types.h>

#include "log.h"

#define CLEANUP_WITH(result_code)       \
	do {                            \
		result = (result_code); \
		goto cleanup;           \
	} while (0)

#define CHECK(op)                            \
	do {                                 \
		result = (op);               \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

#define CHECKED_MEM_GET(m, target_ptr, s)                      \
	do {                                                   \
		(target_ptr) = isc_mem_get((m), (s));          \
		if ((target_ptr) == NULL) {                    \
			result = ISC_R_NOMEMORY;               \
			log_error("Memory allocation failed"); \
			goto cleanup;                          \
		}                                              \
	} while (0)

#define CHECKED_MEM_GET_PTR(m, target_ptr) \
	CHECKED_MEM_GET(m, target_ptr, sizeof(*(target_ptr)))

#define CHECKED_MEM_STRDUP(m, source, target)                  \
	do {                                                   \
		(target) = isc_mem_strdup((m), (source));      \
		if ((target) == NULL) {                        \
			result = ISC_R_NOMEMORY;               \
			log_error("Memory allocation failed"); \
			goto cleanup;                          \
		}                                              \
	} while (0)

#define ZERO_PTR(ptr) memset((ptr), 0, sizeof(*(ptr)))

#define MEM_PUT_AND_DETACH(target_ptr)                        \
	isc_mem_putanddetach(&(target_ptr)->mctx, target_ptr, \
			     sizeof(*(target_ptr)))
