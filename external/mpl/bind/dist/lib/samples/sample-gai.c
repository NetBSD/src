/*	$NetBSD: sample-gai.c,v 1.2 2018/08/12 13:02:41 christos Exp $	*/

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

#include <isc/net.h>
#include <isc/print.h>

#include <irs/netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void
do_gai(int family, char *hostname) {
	struct addrinfo hints, *res, *res0;
	int error;
	char namebuf[1024], addrbuf[1024], servbuf[1024];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	error = getaddrinfo(hostname, "http", &hints, &res0);
	if (error) {
		fprintf(stderr, "getaddrinfo failed for %s,family=%d: %s\n",
			hostname, family, gai_strerror(error));
		return;
	}

	for (res = res0; res; res = res->ai_next) {
		error = getnameinfo(res->ai_addr,
				    (socklen_t)res->ai_addrlen,
				    addrbuf, sizeof(addrbuf),
				    NULL, 0, NI_NUMERICHOST);
		if (error == 0)
			error = getnameinfo(res->ai_addr,
					    (socklen_t)res->ai_addrlen,
					    namebuf, sizeof(namebuf),
					    servbuf, sizeof(servbuf), 0);
		if (error != 0) {
			fprintf(stderr, "getnameinfo failed: %s\n",
				gai_strerror(error));
		} else {
			printf("%s(%s/%s)=%s:%s\n", hostname,
			       res->ai_canonname, addrbuf, namebuf, servbuf);
		}
	}

	freeaddrinfo(res0);
}

int
main(int argc, char *argv[]) {
	if (argc < 2)
		exit(1);

	do_gai(AF_INET, argv[1]);
	do_gai(AF_INET6, argv[1]);
	do_gai(AF_UNSPEC, argv[1]);

	return (0);
}
