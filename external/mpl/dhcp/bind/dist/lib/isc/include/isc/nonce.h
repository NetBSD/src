/*	$NetBSD: nonce.h,v 1.1.2.2 2024/02/24 13:07:26 martin Exp $	*/

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

#include <stdlib.h>

#include <isc/lang.h>

/*! \file isc/nonce.h
 * \brief Provides a function for generating an arbitrarily long nonce.
 */

ISC_LANG_BEGINDECLS

void
isc_nonce_buf(void *buf, size_t buflen);
/*!<
 * Fill 'buf', up to 'buflen' bytes, with random data from the
 * crypto provider's random function.
 */

ISC_LANG_ENDDECLS
