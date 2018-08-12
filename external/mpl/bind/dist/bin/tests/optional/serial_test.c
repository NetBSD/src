/*	$NetBSD: serial_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

#include <stdio.h>

#include <isc/print.h>
#include <isc/serial.h>
#include <isc/stdlib.h>

int
main() {
	isc_uint32_t a, b;
	char buf[1024];
	char *s, *e;

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		buf[sizeof(buf) - 1] = '\0';
		s = buf;
		a = strtoul(s, &e, 0);
		if (s == e)
			continue;
		s = e;
		b = strtoul(s, &e, 0);
		if (s == e)
			continue;
		fprintf(stdout, "%u %u gt:%d lt:%d ge:%d le:%d eq:%d ne:%d\n",
			a, b,
			isc_serial_gt(a,b), isc_serial_lt(a,b),
			isc_serial_ge(a,b), isc_serial_le(a,b),
			isc_serial_eq(a,b), isc_serial_ne(a,b));
	}
	return (0);
}
