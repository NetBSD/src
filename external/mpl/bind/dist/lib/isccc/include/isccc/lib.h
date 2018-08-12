/*	$NetBSD: lib.h,v 1.1.1.1 2018/08/12 12:08:29 christos Exp $	*/

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


#ifndef ISCCC_LIB_H
#define ISCCC_LIB_H 1

/*! \file isccc/lib.h */

#include <isc/types.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

LIBISCCC_EXTERNAL_DATA extern isc_msgcat_t *isccc_msgcat;

void
isccc_lib_initmsgcat(void);
/*%
 * Initialize the ISCCC library's message catalog, isccc_msgcat, if it
 * has not already been initialized.
 */

ISC_LANG_ENDDECLS

#endif /* ISCCC_LIB_H */
