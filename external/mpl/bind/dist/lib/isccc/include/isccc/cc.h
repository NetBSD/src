/*	$NetBSD: cc.h,v 1.6.2.1 2024/02/25 15:47:31 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 2001 Nominum, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NOMINUM DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

/*! \file isccc/cc.h */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/lang.h>

#include <dst/dst.h>
#include <isccc/types.h>

ISC_LANG_BEGINDECLS

/*%
 * The HMAC algorithms supported by isccc_cc_fromwire and
 * isccc_cc_towire as implemented in DST.
 */
#define ISCCC_ALG_UNKNOWN    0
#define ISCCC_ALG_HMACMD5    DST_ALG_HMACMD5
#define ISCCC_ALG_HMACSHA1   DST_ALG_HMACSHA1
#define ISCCC_ALG_HMACSHA224 DST_ALG_HMACSHA224
#define ISCCC_ALG_HMACSHA256 DST_ALG_HMACSHA256
#define ISCCC_ALG_HMACSHA384 DST_ALG_HMACSHA384
#define ISCCC_ALG_HMACSHA512 DST_ALG_HMACSHA512

/*% Maximum Datagram Package */
#define ISCCC_CC_MAXDGRAMPACKET 4096

/*% Message Type String */
#define ISCCC_CCMSGTYPE_STRING 0x00
/*% Message Type Binary Data */
#define ISCCC_CCMSGTYPE_BINARYDATA 0x01
/*% Message Type Table */
#define ISCCC_CCMSGTYPE_TABLE 0x02
/*% Message Type List */
#define ISCCC_CCMSGTYPE_LIST 0x03

/*% Send to Wire */
isc_result_t
isccc_cc_towire(isccc_sexpr_t *alist, isc_buffer_t **buffer, uint32_t algorithm,
		isccc_region_t *secret);

/*% Get From Wire */
isc_result_t
isccc_cc_fromwire(isccc_region_t *source, isccc_sexpr_t **alistp,
		  uint32_t algorithm, isccc_region_t *secret);

/*% Create Message */
isc_result_t
isccc_cc_createmessage(uint32_t version, const char *from, const char *to,
		       uint32_t serial, isccc_time_t now, isccc_time_t expires,
		       isccc_sexpr_t **alistp);

/*% Create Acknowledgment */
isc_result_t
isccc_cc_createack(isccc_sexpr_t *message, bool ok, isccc_sexpr_t **ackp);

/*% Is Ack? */
bool
isccc_cc_isack(isccc_sexpr_t *message);

/*% Is Reply? */
bool
isccc_cc_isreply(isccc_sexpr_t *message);

/*% Create Response */
isc_result_t
isccc_cc_createresponse(isccc_sexpr_t *message, isccc_time_t now,
			isccc_time_t expires, isccc_sexpr_t **alistp);

/*% Define String */
isccc_sexpr_t *
isccc_cc_definestring(isccc_sexpr_t *alist, const char *key, const char *str);

/*% Define uint 32 */
isccc_sexpr_t *
isccc_cc_defineuint32(isccc_sexpr_t *alist, const char *key, uint32_t i);

/*% Lookup String */
isc_result_t
isccc_cc_lookupstring(isccc_sexpr_t *alist, const char *key, char **strp);

/*% Lookup uint 32 */
isc_result_t
isccc_cc_lookupuint32(isccc_sexpr_t *alist, const char *key, uint32_t *uintp);

/*% Create Symbol Table */
isc_result_t
isccc_cc_createsymtab(isccc_symtab_t **symtabp);

/*% Clean up Symbol Table */
void
isccc_cc_cleansymtab(isccc_symtab_t *symtab, isccc_time_t now);

/*% Check for Duplicates */
isc_result_t
isccc_cc_checkdup(isccc_symtab_t *symtab, isccc_sexpr_t *message,
		  isccc_time_t now);

ISC_LANG_ENDDECLS
