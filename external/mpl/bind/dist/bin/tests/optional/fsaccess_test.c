/*	$NetBSD: fsaccess_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>		/* Non-portable. */
#include <sys/stat.h>		/* Non-portable. */

#include <isc/fsaccess.h>
#include <isc/print.h>
#include <isc/result.h>

#define PATH "/tmp/fsaccess"

int
main(void) {
	isc_fsaccess_t access;
	isc_result_t result;
	FILE *fp;
	int n;

	n = remove(PATH);
	if (n != 0 && errno != ENOENT) {
		fprintf(stderr, "unable to remove(%s)\n", PATH);
		exit(1);
	}
	fp = fopen(PATH, "w");
	if (fp == NULL) {
		fprintf(stderr, "unable to fopen(%s)\n", PATH);
		exit(1);
	}
	n = chmod(PATH, 0);
	if (n != 0) {
		fprintf(stderr, "unable chmod(%s, 0)\n", PATH);
		exit(1);
	}

	access = 0;

	isc_fsaccess_add(ISC_FSACCESS_OWNER | ISC_FSACCESS_GROUP,
			 ISC_FSACCESS_READ | ISC_FSACCESS_WRITE,
			 &access);

	printf("fsaccess=%u\n", access);

	isc_fsaccess_add(ISC_FSACCESS_OTHER, ISC_FSACCESS_READ, &access);

	printf("fsaccess=%u\n", access);

	result = isc_fsaccess_set(PATH, access);
	if (result != ISC_R_SUCCESS)
		fprintf(stderr, "result = %s\n", isc_result_totext(result));
	(void)fclose(fp);

	return (0);
}
