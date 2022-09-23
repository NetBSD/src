/*	$NetBSD: arpaname.c,v 1.6 2022/09/23 12:15:26 christos Exp $	*/

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

#include <stdio.h>

#include <isc/net.h>
#include <isc/print.h>

#define UNUSED(x) (void)(x)

int
main(int argc, char *argv[]) {
	unsigned char buf[16];
	int i;

	UNUSED(argc);

	while (argv[1]) {
		if (inet_pton(AF_INET6, argv[1], buf) == 1) {
			for (i = 15; i >= 0; i--) {
				fprintf(stdout, "%X.%X.", buf[i] & 0xf,
					(buf[i] >> 4) & 0xf);
			}
			fprintf(stdout, "IP6.ARPA\n");
			argv++;
			continue;
		}
		if (inet_pton(AF_INET, argv[1], buf) == 1) {
			fprintf(stdout, "%u.%u.%u.%u.IN-ADDR.ARPA\n", buf[3],
				buf[2], buf[1], buf[0]);
			argv++;
			continue;
		}
		return (1);
	}
	fflush(stdout);
	return (ferror(stdout));
}
