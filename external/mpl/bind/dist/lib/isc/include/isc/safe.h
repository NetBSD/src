/*	$NetBSD: safe.h,v 1.6.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

/*! \file isc/safe.h */

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

int
isc_safe_memequal(const void *, const void *, size_t);

/*%<
 * Returns true iff. two blocks of memory are equal, otherwise
 * false.
 *
 */

void
isc_safe_memwipe(void *, size_t);

/*%<
 * Clear the memory of length `len` pointed to by `ptr`.
 *
 * Some crypto code calls memset() on stack allocated buffers just
 * before return so that they are wiped. Such memset() calls can be
 * optimized away by the compiler. We provide this external non-inline C
 * function to perform the memset operation so that the compiler cannot
 * infer about what the function does and optimize the call away.
 */

ISC_LANG_ENDDECLS
