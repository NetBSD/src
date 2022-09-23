/*	$NetBSD: serial_test.c,v 1.6 2022/09/23 12:15:23 christos Exp $	*/

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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/print.h>
#include <isc/serial.h>

int
main() {
	uint32_t a, b;
	char buf[1024];
	char *s, *e;

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		buf[sizeof(buf) - 1] = '\0';
		s = buf;
		a = strtoul(s, &e, 0);
		if (s == e) {
			continue;
		}
		s = e;
		b = strtoul(s, &e, 0);
		if (s == e) {
			continue;
		}
		fprintf(stdout, "%u %u gt:%d lt:%d ge:%d le:%d eq:%d ne:%d\n",
			a, b, isc_serial_gt(a, b), isc_serial_lt(a, b),
			isc_serial_ge(a, b), isc_serial_le(a, b),
			isc_serial_eq(a, b), isc_serial_ne(a, b));
	}
	return (0);
}
