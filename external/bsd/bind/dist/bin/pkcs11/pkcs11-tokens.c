/*	$NetBSD: pkcs11-tokens.c,v 1.1.1.1 2014/02/28 17:40:07 christos Exp $	*/

/*
 * Copyright (C) 2014  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

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
#include <isc/result.h>
#include <isc/types.h>

#include <pk11/pk11.h>

extern void dst__pkcs11_init(isc_mem_t *mctx, const char *engine);

int
main(int argc, char *argv[]) {
	char *lib_name = NULL;
	int c, errflg = 0;
	isc_mem_t *mctx = NULL;

	while ((c = isc_commandline_parse(argc, argv, ":m:")) != -1) {
		switch (c) {
		case 'm':
			lib_name = isc_commandline_argument;
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
		fprintf(stderr, "\tpkcs11-tokens [-m module]\n");
		exit(1);
	}

	if (isc_mem_create(0, 0, &mctx) != ISC_R_SUCCESS) {
		fprintf(stderr, "isc_mem_create() failed\n");
		exit(1);
	}

	dst__pkcs11_init(mctx, lib_name);

	pk11_dump_tokens();

	pk11_shutdown();

	isc_mem_destroy(&mctx);

	exit(0);
}
