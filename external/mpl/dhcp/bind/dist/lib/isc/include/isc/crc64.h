/*	$NetBSD: crc64.h,v 1.1.2.2 2024/02/24 13:07:24 martin Exp $	*/

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

#ifndef ISC_CRC64_H
#define ISC_CRC64_H 1

/*! \file isc/crc64.h
 * \brief CRC64 in C
 */

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

void
isc_crc64_init(uint64_t *crc);
/*%
 * Initialize a new CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 */

void
isc_crc64_update(uint64_t *crc, const void *data, size_t len);
/*%
 * Add data to the CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 * * 'data' is not NULL.
 */

void
isc_crc64_final(uint64_t *crc);
/*%
 * Finalize the CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_CRC64_H */
