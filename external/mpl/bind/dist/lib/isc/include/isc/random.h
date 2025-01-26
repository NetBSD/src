/*	$NetBSD: random.h,v 1.6 2025/01/26 16:25:42 christos Exp $	*/

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

#include <inttypes.h>
#include <stdlib.h>

#include <isc/lang.h>
#include <isc/types.h>

/*! \file isc/random.h
 * \brief Implements wrapper around a non-cryptographically secure
 * pseudo-random number generator.
 *
 */

ISC_LANG_BEGINDECLS

uint8_t
isc_random8(void);
/*!<
 * \brief Returns a single 8-bit random value.
 */

uint16_t
isc_random16(void);
/*!<
 * \brief Returns a single 16-bit random value.
 */

uint32_t
isc_random32(void);
/*!<
 * \brief Returns a single 32-bit random value.
 */

void
isc_random_buf(void *buf, size_t buflen);
/*!<
 * \brief Fills the region buf of length buflen with random data.
 */

uint32_t
isc_random_uniform(uint32_t upper_bound);
/*!<
 * \brief Returns a single 32-bit uniformly distributed random value
 *        less than upper_bound.
 *
 * This is better than ``isc_random() % upper_bound'' as it avoids
 * "modulo bias" when the upper bound is not a power of two. This
 * function is also faster, because it usually avoids doing any
 * divisions (which are typically very slow).
 *
 * It uses rejection sampling to ensure uniformity, so it may require
 * multiple iterations to get a result; the probability of needing to
 * resample is very small when the upper_bound is small, rising to 0.5
 * when upper_bound is UINT32_MAX/2.
 */

ISC_LANG_ENDDECLS
