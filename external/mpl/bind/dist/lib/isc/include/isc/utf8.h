/*	$NetBSD: utf8.h,v 1.1.1.1 2020/08/03 17:07:14 christos Exp $	*/

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

/*! \file isc/utf8.h */

#pragma once

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

bool
isc_utf8_bom(const unsigned char *buf, size_t len);
/*<
 * Returns 'true' if the string of bytes in 'buf' starts
 * with an UTF-8 Byte Order Mark.
 *
 * Requires:
 *\li 	'buf' != NULL
 */

bool
isc_utf8_valid(const unsigned char *buf, size_t len);
/*<
 * Returns 'true' if the string of bytes in 'buf' is a valid UTF-8
 * byte sequence otherwise 'false' is returned.
 *
 * Requires:
 *\li 	'buf' != NULL
 */

ISC_LANG_ENDDECLS
