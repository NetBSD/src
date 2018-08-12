/*	$NetBSD: types.h,v 1.1.1.1 2018/08/12 12:08:29 christos Exp $	*/

/*
 * Portions Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 *
 * Portions Copyright (C) 2001 Nominum, Inc.
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


#ifndef ISCCC_TYPES_H
#define ISCCC_TYPES_H 1

/*! \file isccc/types.h */

#include <isc/boolean.h>
#include <isc/int.h>
#include <isc/result.h>

/*% isccc_time_t typedef */
typedef isc_uint32_t isccc_time_t;

/*% isccc_sexpr_t typedef */
typedef struct isccc_sexpr isccc_sexpr_t;
/*% isccc_dottedpair_t typedef */
typedef struct isccc_dottedpair isccc_dottedpair_t;
/*% isccc_symtab_t typedef */
typedef struct isccc_symtab isccc_symtab_t;

/*% iscc region structure */
typedef struct isccc_region {
	unsigned char *		rstart;
	unsigned char *		rend;
} isccc_region_t;

#endif /* ISCCC_TYPES_H */
