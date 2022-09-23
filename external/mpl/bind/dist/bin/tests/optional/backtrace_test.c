/*	$NetBSD: backtrace_test.c,v 1.1.1.4 2022/09/23 12:09:09 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdio.h>
#include <string.h>

#include <isc/backtrace.h>
#include <isc/print.h>
#include <isc/result.h>

const char *expected_symbols[] = { "func3", "func2", "func1", "main" };

static int
func3() {
	void *tracebuf[16];
	int i, nframes;
	int error = 0;
	const char *fname;
	isc_result_t result;
	unsigned long offset;

	result = isc_backtrace_gettrace(tracebuf, 16, &nframes);
	if (result != ISC_R_SUCCESS) {
		printf("isc_backtrace_gettrace failed: %s\n",
		       isc_result_totext(result));
		return (1);
	}

	if (nframes < 4) {
		error++;
	}

	for (i = 0; i < 4 && i < nframes; i++) {
		fname = NULL;
		result = isc_backtrace_getsymbol(tracebuf[i], &fname, &offset);
		if (result != ISC_R_SUCCESS) {
			error++;
			continue;
		}
		if (strcmp(fname, expected_symbols[i]) != 0) {
			error++;
		}
	}

	if (error) {
		printf("Unexpected result:\n");
		printf("  # of frames: %d (expected: at least 4)\n", nframes);
		printf("  symbols:\n");
		for (i = 0; i < nframes; i++) {
			fname = NULL;
			result = isc_backtrace_getsymbol(tracebuf[i], &fname,
							 &offset);
			if (result == ISC_R_SUCCESS) {
				printf("  [%d] %s\n", i, fname);
			} else {
				printf("  [%d] %p getsymbol failed: %s\n", i,
				       tracebuf[i], isc_result_totext(result));
			}
		}
	}

	return (error);
}

static int
func2() {
	return (func3());
}

static int
func1() {
	return (func2());
}

int
main() {
	return (func1());
}
