/*	$NetBSD: pk11_result.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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

#include <config.h>
#include <stddef.h>

#include <isc/once.h>
#include <isc/msgcat.h>
#include <isc/util.h>

#include <pk11/result.h>

LIBISC_EXTERNAL_DATA isc_msgcat_t *		pk11_msgcat = NULL;

static isc_once_t		msgcat_once = ISC_ONCE_INIT;

static const char *text[PK11_R_NRESULTS] = {
	"PKCS#11 initialization failed",		/*%< 0 */
	"no PKCS#11 provider",				/*%< 1 */
	"PKCS#11 provider has no random service",	/*%< 2 */
	"PKCS#11 provider has no digest service",	/*%< 3 */
	"PKCS#11 provider has no AES service",		/*%< 4 */
};

static const char *ids[PK11_R_NRESULTS] = {
	"PK11_R_INITFAILED",
	"PK11_R_NOPROVIDER",
	"PK11_R_NORANDOMSERVICE",
	"PK11_R_NODIGESTSERVICE",
	"PK11_R_NOAESSERVICE",
};

#define PK11_RESULT_RESULTSET			2

static isc_once_t		once = ISC_ONCE_INIT;

static void
open_msgcat(void) {
	isc_msgcat_open("libpk11.cat", &pk11_msgcat);
}

void
pk11_initmsgcat(void) {

	/*
	 * Initialize the PKCS#11 support's message catalog,
	 * pk11_msgcat, if it has not already been initialized.
	 */

	RUNTIME_CHECK(isc_once_do(&msgcat_once, open_msgcat) == ISC_R_SUCCESS);
}

static void
initialize_action(void) {
	isc_result_t result;

	result = isc_result_register(ISC_RESULTCLASS_PK11, PK11_R_NRESULTS,
				     text, pk11_msgcat, PK11_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_register() failed: %u", result);

	result = isc_result_registerids(ISC_RESULTCLASS_PK11, PK11_R_NRESULTS,
					ids, pk11_msgcat,
					PK11_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_registerids() failed: %u", result);
}

static void
initialize(void) {
	pk11_initmsgcat();
	RUNTIME_CHECK(isc_once_do(&once, initialize_action) == ISC_R_SUCCESS);
}

const char *
pk11_result_totext(isc_result_t result) {
	initialize();

	return (isc_result_totext(result));
}

void
pk11_result_register(void) {
	initialize();
}
