/*	$NetBSD: lib.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stddef.h>

#include <isc/once.h>
#include <isc/msgcat.h>
#include <isc/util.h>

#include <isccc/lib.h>

/***
 *** Globals
 ***/

LIBISCCC_EXTERNAL_DATA isc_msgcat_t *		isccc_msgcat = NULL;


/***
 *** Private
 ***/

static isc_once_t		msgcat_once = ISC_ONCE_INIT;


/***
 *** Functions
 ***/

static void
open_msgcat(void) {
	isc_msgcat_open("libisccc.cat", &isccc_msgcat);
}

void
isccc_lib_initmsgcat(void) {

	/*
	 * Initialize the DNS library's message catalog, isccc_msgcat, if it
	 * has not already been initialized.
	 */

	RUNTIME_CHECK(isc_once_do(&msgcat_once, open_msgcat) == ISC_R_SUCCESS);
}
