/*	$NetBSD: dns_name_fromwire.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/ascii.h>
#include <isc/buffer.h>
#include <isc/random.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>

#include "old.h"

static uint32_t
old_bench(const uint8_t *data, size_t size) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name = dns_fixedname_initname(&fixed);
	dns_decompress_t dctx = DNS_DECOMPRESS_PERMITTED;
	isc_buffer_t buf;
	uint32_t count = 0;

	isc_buffer_constinit(&buf, data, size);
	isc_buffer_add(&buf, size);
	isc_buffer_setactive(&buf, size);

	while (isc_buffer_consumedlength(&buf) < size) {
		result = old_name_fromwire(name, &buf, dctx, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			isc_buffer_forward(&buf, 1);
		}
		count++;
	}
	return count;
}

static uint32_t
new_bench(const uint8_t *data, size_t size) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name = dns_fixedname_initname(&fixed);
	dns_decompress_t dctx = DNS_DECOMPRESS_PERMITTED;
	isc_buffer_t buf;
	uint32_t count = 0;

	isc_buffer_constinit(&buf, data, size);
	isc_buffer_add(&buf, size);
	isc_buffer_setactive(&buf, size);

	while (isc_buffer_consumedlength(&buf) < size) {
		result = dns_name_fromwire(name, &buf, dctx, NULL);
		if (result != ISC_R_SUCCESS) {
			isc_buffer_forward(&buf, 1);
		}
		count++;
	}
	return count;
}

static void
oldnew_bench(const uint8_t *data, size_t size) {
	isc_time_t t0;
	t0 = isc_time_now_hires();
	uint32_t n1 = old_bench(data, size);
	isc_time_t t1;
	t1 = isc_time_now_hires();
	uint32_t n2 = new_bench(data, size);
	isc_time_t t2;
	t2 = isc_time_now_hires();

	double t01 = (double)isc_time_microdiff(&t1, &t0);
	double t12 = (double)isc_time_microdiff(&t2, &t1);
	printf("  old %u / %f ms; %f / us\n", n1, t01 / 1000.0, n1 / t01);
	printf("  new %u / %f ms; %f / us\n", n2, t12 / 1000.0, n2 / t12);
	printf("  old/new %f or %f\n", t01 / t12, t12 / t01);
}

#define NAMES 1000
static uint8_t buf[1024 * NAMES];

int
main(void) {
	unsigned int p;

	printf("random buffer\n");
	isc_random_buf(buf, sizeof(buf));
	oldnew_bench(buf, sizeof(buf));

	p = 0;
	for (unsigned int name = 0; name < NAMES; name++) {
		unsigned int start = p;
		unsigned int prev = p;
		buf[p++] = 0;
		for (unsigned int label = 0; label < 127; label++) {
			unsigned int ptr = prev - start;
			prev = p;
			buf[p++] = 1;
			buf[p++] = 'a';
			buf[p++] = 0xC0 | (ptr >> 8);
			buf[p++] = 0xFF & ptr;
		}
	}
	printf("127 compression pointers\n");
	oldnew_bench(buf, p);

	p = 0;
	for (unsigned int name = 0; name < NAMES; name++) {
		for (unsigned int label = 0; label < 127; label++) {
			buf[p++] = 1;
			buf[p++] = 'a';
		}
		buf[p++] = 0;
	}
	printf("127 sequential labels\n");
	oldnew_bench(buf, p);

	p = 0;
	for (unsigned int name = 0; name < NAMES; name++) {
		for (unsigned int label = 0; label < 4; label++) {
			buf[p++] = 62;
			for (unsigned int c = 0; c < 62; c++) {
				buf[p++] = 'a';
			}
		}
		buf[p++] = 0;
	}
	printf("4 long sequential labels\n");
	oldnew_bench(buf, p);
}
