/*	$NetBSD: crc64.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#ifndef ISC_CRC64_H
#define ISC_CRC64_H 1

/*! \file isc/crc64.h
 * \brief CRC64 in C
 */

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

void
isc_crc64_init(isc_uint64_t *crc);
/*%
 * Initialize a new CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 */

void
isc_crc64_update(isc_uint64_t *crc, const void *data, size_t len);
/*%
 * Add data to the CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 * * 'data' is not NULL.
 */

void
isc_crc64_final(isc_uint64_t *crc);
/*%
 * Finalize the CRC.
 *
 * Requires:
 * * 'crc' is not NULL.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_CRC64_H */
