/*	$NetBSD: pk11_result.c,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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
#include <isc/util.h>

#include <pk11/result.h>

static const char *text[PK11_R_NRESULTS] = {
	"PKCS#11 initialization failed",		/*%< 0 */
	"no PKCS#11 provider",				/*%< 1 */
	"PKCS#11 no random service",			/*%< 2 */
	"PKCS#11 no digist service",			/*%< 3 */
	"PKCS#11 no AES service",			/*%< 4 */
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
initialize_action(void) {
	isc_result_t result;

	result = isc_result_register(ISC_RESULTCLASS_PK11, PK11_R_NRESULTS,
				     text, PK11_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_register() failed: %u", result);
	}

	result = isc_result_registerids(ISC_RESULTCLASS_PK11, PK11_R_NRESULTS,
					ids, PK11_RESULT_RESULTSET);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_registerids() failed: %u", result);
	}
}

static void
initialize(void) {
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
