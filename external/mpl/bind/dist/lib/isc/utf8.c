/*	$NetBSD: utf8.c,v 1.4 2023/01/25 21:43:31 christos Exp $	*/

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

#include <string.h>

#include <isc/utf8.h>
#include <isc/util.h>

/*
 * UTF-8 is defined in "The Unicode Standard -- Version 4.0"
 * Also see RFC 3629.
 *
 * Char. number range  |        UTF-8 octet sequence
 *    (hexadecimal)    |              (binary)
 *  --------------------+---------------------------------------------
 * 0000 0000-0000 007F | 0xxxxxxx
 * 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 * 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 * 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
bool
isc_utf8_valid(const unsigned char *buf, size_t len) {
	REQUIRE(buf != NULL);

	for (size_t i = 0; i < len; i++) {
		if (buf[i] <= 0x7f) {
			continue;
		}
		if ((i + 1) < len && (buf[i] & 0xe0) == 0xc0 &&
		    (buf[i + 1] & 0xc0) == 0x80)
		{
			unsigned int w;
			w = (buf[i] & 0x1f) << 6;
			w |= (buf[++i] & 0x3f);
			if (w < 0x80) {
				return (false);
			}
			continue;
		}
		if ((i + 2) < len && (buf[i] & 0xf0) == 0xe0 &&
		    (buf[i + 1] & 0xc0) == 0x80 && (buf[i + 2] & 0xc0) == 0x80)
		{
			unsigned int w;
			w = (buf[i] & 0x0f) << 12;
			w |= (buf[++i] & 0x3f) << 6;
			w |= (buf[++i] & 0x3f);
			if (w < 0x0800) {
				return (false);
			}
			continue;
		}
		if ((i + 3) < len && (buf[i] & 0xf8) == 0xf0 &&
		    (buf[i + 1] & 0xc0) == 0x80 &&
		    (buf[i + 2] & 0xc0) == 0x80 && (buf[i + 3] & 0xc0) == 0x80)
		{
			unsigned int w;
			w = (buf[i] & 0x07) << 18;
			w |= (buf[++i] & 0x3f) << 12;
			w |= (buf[++i] & 0x3f) << 6;
			w |= (buf[++i] & 0x3f);
			if (w < 0x10000 || w > 0x10FFFF) {
				return (false);
			}
			continue;
		}
		return (false);
	}
	return (true);
}

bool
isc_utf8_bom(const unsigned char *buf, size_t len) {
	REQUIRE(buf != NULL);

	if (len >= 3U && !memcmp(buf, "\xef\xbb\xbf", 3)) {
		return (true);
	}
	return (false);
}
