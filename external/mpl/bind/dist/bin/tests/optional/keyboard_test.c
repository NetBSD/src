/*	$NetBSD: keyboard_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

#include <isc/keyboard.h>
#include <isc/print.h>
#include <isc/util.h>

static void
CHECK(const char *msg, isc_result_t result) {
	if (result != ISC_R_SUCCESS) {
		printf("FAILURE:  %s:  %s\n", msg, isc_result_totext(result));
		exit(1);
	}
}

int
main(int argc, char **argv) {
	isc_keyboard_t kbd;
	unsigned char c;
	isc_result_t res;
	unsigned int count;

	UNUSED(argc);
	UNUSED(argv);

	printf("Type Q to exit.\n");

	res = isc_keyboard_open(&kbd);
	CHECK("isc_keyboard_open()", res);

	c = 'x';
	count = 0;
	while (res == ISC_R_SUCCESS && c != 'Q') {
		res = isc_keyboard_getchar(&kbd, &c);
		printf(".");
		fflush(stdout);
		count++;
		if (count % 64 == 0)
			printf("\r\n");
	}
	printf("\r\n");
	if (res != ISC_R_SUCCESS) {
		printf("FAILURE:  keyboard getchar failed:  %s\r\n",
		       isc_result_totext(res));
		goto errout;
	}

 errout:
	res = isc_keyboard_close(&kbd, 3);
	CHECK("isc_keyboard_close()", res);

	return (0);
}
