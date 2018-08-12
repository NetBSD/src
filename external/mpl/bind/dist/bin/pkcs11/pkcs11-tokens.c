/*	$NetBSD: pkcs11-tokens.c,v 1.1.1.1 2018/08/12 12:07:26 christos Exp $	*/

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


/* pkcs11-tokens [-m module] */

/*! \file */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <isc/commandline.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/types.h>

#include <pk11/pk11.h>
#include <pk11/result.h>

int
main(int argc, char *argv[]) {
	isc_result_t result;
	char *lib_name = NULL;
	int c, errflg = 0;
	isc_mem_t *mctx = NULL;
	pk11_context_t pctx;

	while ((c = isc_commandline_parse(argc, argv, ":m:v")) != -1) {
		switch (c) {
		case 'm':
			lib_name = isc_commandline_argument;
			break;
		case 'v':
			pk11_verbose_init = ISC_TRUE;
			break;
		case ':':
			fprintf(stderr, "Option -%c requires an operand\n",
				isc_commandline_option);
			errflg++;
			break;
		case '?':
		default:
			fprintf(stderr, "Unrecognised option: -%c\n",
				isc_commandline_option);
			errflg++;
		}
	}

	if (errflg) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "\tpkcs11-tokens [-v] [-m module]\n");
		exit(1);
	}

	if (isc_mem_create(0, 0, &mctx) != ISC_R_SUCCESS) {
		fprintf(stderr, "isc_mem_create() failed\n");
		exit(1);
	}

	pk11_result_register();

	/* Initialize the CRYPTOKI library */
	if (lib_name != NULL)
		pk11_set_lib_name(lib_name);

	result = pk11_get_session(&pctx, OP_ANY, ISC_TRUE, ISC_FALSE,
				  ISC_FALSE, NULL, 0);
	if (result == PK11_R_NORANDOMSERVICE ||
	    result == PK11_R_NODIGESTSERVICE ||
	    result == PK11_R_NOAESSERVICE) {
		fprintf(stderr, "Warning: %s\n", isc_result_totext(result));
		fprintf(stderr, "This HSM will not work with BIND 9 "
				"using native PKCS#11.\n\n");
	} else if ((result != ISC_R_SUCCESS) && (result != ISC_R_NOTFOUND)) {
		fprintf(stderr, "Unrecoverable error initializing "
				"PKCS#11: %s\n", isc_result_totext(result));
		exit(1);
	}

	pk11_dump_tokens();

	if (pctx.handle != NULL)
		pk11_return_session(&pctx);
	(void) pk11_finalize();

	isc_mem_destroy(&mctx);

	exit(0);
}
