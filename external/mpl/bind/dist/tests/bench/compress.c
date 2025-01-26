/*	$NetBSD: compress.c,v 1.1.1.1 2025/01/26 16:12:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>

static void
CHECKRESULT(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		printf("%s: %s\n", msg, isc_result_totext(result));
		exit(EXIT_FAILURE);
	}
}

int
main(void) {
	isc_result_t result;
	isc_buffer_t buf;

	isc_mem_t *mctx = NULL;
	isc_mem_create(&mctx);

	static dns_fixedname_t fixedname[65536];
	unsigned int count = 0;

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, stdin)) > 0) {
		if (line[linelen - 1] == '\n') {
			line[--linelen] = '\0';
		}
		isc_buffer_init(&buf, line, linelen);
		isc_buffer_add(&buf, linelen);

		if (count == ARRAY_SIZE(fixedname)) {
			errx(1, "too many names");
		}
		dns_name_t *name = dns_fixedname_initname(&fixedname[count++]);
		result = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
		CHECKRESULT(result, line);
	}

	unsigned int repeat = 100;

	isc_time_t start;
	start = isc_time_now_hires();

	for (unsigned int n = 0; n < repeat; n++) {
		static uint8_t wire[4 * 1024];
		dns_compress_t cctx;

		isc_buffer_init(&buf, wire, sizeof(wire));
		dns_compress_init(&cctx, mctx, 0);

		for (unsigned int i = 0; i < count; i++) {
			dns_name_t *name = dns_fixedname_name(&fixedname[i]);
			result = dns_name_towire(name, &cctx, &buf, NULL);
			if (result == ISC_R_NOSPACE) {
				dns_compress_invalidate(&cctx);
				dns_compress_init(&cctx, mctx, 0);
				isc_buffer_init(&buf, wire, sizeof(wire));
			} else {
				CHECKRESULT(result, "dns_name_towire");
			}
		}
		dns_compress_invalidate(&cctx);
	}

	isc_time_t finish;
	finish = isc_time_now_hires();

	uint64_t microseconds = isc_time_microdiff(&finish, &start);
	printf("time %f / %u\n", (double)microseconds / 1000000.0, repeat);

	printf("names %u\n", count);

	isc_mem_destroy(&mctx);

	return 0;
}
