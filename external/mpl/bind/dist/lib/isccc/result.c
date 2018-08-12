/*	$NetBSD: result.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <isc/once.h>
#include <isc/util.h>

#include <isccc/result.h>
#include <isccc/lib.h>

static const char *text[ISCCC_R_NRESULTS] = {
	"unknown version",			/* 1 */
	"syntax error",				/* 2 */
	"bad auth",				/* 3 */
	"expired",				/* 4 */
	"clock skew",				/* 5 */
	"duplicate"				/* 6 */
};

static const char *ids[ISCCC_R_NRESULTS] = {
	"ISCCC_R_UNKNOWNVERSION",
	"ISCCC_R_SYNTAX",
	"ISCCC_R_BADAUTH",
	"ISCCC_R_EXPIRED",
	"ISCCC_R_CLOCKSKEW",
	"ISCCC_R_DUPLICATE",
};

#define ISCCC_RESULT_RESULTSET			2

static isc_once_t		once = ISC_ONCE_INIT;

static void
initialize_action(void) {
	isc_result_t result;

	result = isc_result_register(ISC_RESULTCLASS_ISCCC, ISCCC_R_NRESULTS,
				     text, isccc_msgcat,
				     ISCCC_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_register() failed: %u", result);

	result = isc_result_registerids(ISC_RESULTCLASS_ISCCC, ISCCC_R_NRESULTS,
					ids, isccc_msgcat,
					ISCCC_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_registerids() failed: %u", result);
}

static void
initialize(void) {
	isccc_lib_initmsgcat();
	RUNTIME_CHECK(isc_once_do(&once, initialize_action) == ISC_R_SUCCESS);
}

const char *
isccc_result_totext(isc_result_t result) {
	initialize();

	return (isc_result_totext(result));
}

void
isccc_result_register(void) {
	initialize();
}
