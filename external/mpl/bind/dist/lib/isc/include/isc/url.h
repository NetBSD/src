/*	$NetBSD: url.h,v 1.3.2.1 2024/02/25 15:47:23 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 and MIT
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <isc/result.h>

/*
 * Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run
 * faster
 */
#ifndef HTTP_PARSER_STRICT
#define HTTP_PARSER_STRICT 1
#endif

typedef enum {
	ISC_UF_SCHEMA = 0,
	ISC_UF_HOST = 1,
	ISC_UF_PORT = 2,
	ISC_UF_PATH = 3,
	ISC_UF_QUERY = 4,
	ISC_UF_FRAGMENT = 5,
	ISC_UF_USERINFO = 6,
	ISC_UF_MAX = 7
} isc_url_field_t;

/* Result structure for isc_url_parse().
 *
 * Callers should index into field_data[] with UF_* values iff field_set
 * has the relevant (1 << UF_*) bit set. As a courtesy to clients (and
 * because we probably have padding left over), we convert any port to
 * a uint16_t.
 */
typedef struct {
	uint16_t field_set; /* Bitmask of (1 << UF_*) values */
	uint16_t port;	    /* Converted UF_PORT string */

	struct {
		uint16_t off; /* Offset into buffer in which field starts */
		uint16_t len; /* Length of run in buffer */
	} field_data[ISC_UF_MAX];
} isc_url_parser_t;

isc_result_t
isc_url_parse(const char *buf, size_t buflen, bool is_connect,
	      isc_url_parser_t *up);
/*%<
 * Parse a URL; return nonzero on failure
 */
