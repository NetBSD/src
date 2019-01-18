/*	$NetBSD: nonce.h,v 1.2.2.2 2019/01/18 08:49:58 pgoyette Exp $	*/

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
