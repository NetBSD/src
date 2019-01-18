/*	$NetBSD: entropy_private.h,v 1.2.2.2 2019/01/18 08:49:57 pgoyette Exp $	*/

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

#pragma once

#include <stdlib.h>

#include <isc/lang.h>

/*! \file isc/entropy_private.h
 * \brief Implements wrapper around CSPRNG cryptographic library calls
 * for getting cryptographically secure pseudo-random numbers.
 *
 * - If OpenSSL is used, it uses RAND_bytes()
 * - If PKCS#11 is used, it uses pkcs_C_GenerateRandom()
 *
 */

ISC_LANG_BEGINDECLS

void
isc_entropy_get(void *buf, size_t buflen);
/*!<
 * \brief Get cryptographically-secure pseudo-random data.
 */

ISC_LANG_ENDDECLS
