/*	$NetBSD: random.h,v 1.1.2.2 2024/02/24 13:07:26 martin Exp $	*/

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
 * \brief Will return a single 32-bit value, uniformly distributed but
 *        less than upper_bound.  This is recommended over
 *        constructions like ``isc_random() % upper_bound'' as it
 *        avoids "modulo bias" when the upper bound is not a power of
 *        two.  In the worst case, this function may require multiple
 *        iterations to ensure uniformity.
 */

ISC_LANG_ENDDECLS
