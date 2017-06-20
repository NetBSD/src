/*	$NetBSD: feature-test.c,v 1.1.1.1.4.2 2017/06/20 17:02:03 snj Exp $	*/

/*
 * Copyright (C) 2016  Internet Systems Consortium, Inc. ("ISC")
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

#include <config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isc/print.h>
#include <isc/util.h>
#include <isc/net.h>

#ifdef WIN32
#include <Winsock2.h>
#endif

#ifndef MAXHOSTNAMELEN
#ifdef HOST_NAME_MAX
#define MAXHOSTNAMELEN HOST_NAME_MAX
#else
#define MAXHOSTNAMELEN 256
#endif
#endif

static void
usage() {
	fprintf(stderr, "usage: feature-test <arg>\n");
	fprintf(stderr, "args:\n");
	fprintf(stderr, "	--enable-fetchlimit\n");
	fprintf(stderr, "	--enable-filter-aaaa\n");
	fprintf(stderr, "	--gethostname\n");
	fprintf(stderr, "	--gssapi\n");
	fprintf(stderr, "	--have-dlopen\n");
	fprintf(stderr, "	--have-geoip\n");
	fprintf(stderr, "	--have-libxml2\n");
	fprintf(stderr, "	--ipv6only=no\n");
	fprintf(stderr, "	--rpz-nsdname\n");
	fprintf(stderr, "	--rpz-nsip\n");
	fprintf(stderr, "	--with-idn\n");
}

int
main(int argc, char **argv) {
	if (argc != 2) {
		usage();
		return (1);
	}

	if (strcmp(argv[1], "--enable-fetchlimit") == 0) {
#ifdef ENABLE_FETCHLIMIT
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--enable-filter-aaaa") == 0) {
#ifdef ALLOW_FILTER_AAAA
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--gethostname") == 0) {
		char hostname[MAXHOSTNAMELEN];
		int n;
#ifdef WIN32
		/* From lwres InitSocket() */
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 0);
		err = WSAStartup( wVersionRequested, &wsaData );
		if (err != 0) {
			fprintf(stderr, "WSAStartup() failed: %d\n", err);
			exit(1);
		}
#endif

		n = gethostname(hostname, sizeof(hostname));
		if (n == -1) {
			perror("gethostname");
			return(1);
		}
		fprintf(stdout, "%s\n", hostname);
#ifdef WIN32
		WSACleanup();
#endif
		return (0);
	}

	if (strcmp(argv[1], "--gssapi") == 0) {
#if defined(GSSAPI)
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--have-dlopen") == 0) {
#if defined(HAVE_DLOPEN) && defined(ISC_DLZ_DLOPEN)
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--have-geoip") == 0) {
#ifdef HAVE_GEOIP
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--have-libxml2") == 0) {
#ifdef HAVE_LIBXML2
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--rpz-nsip") == 0) {
#ifdef ENABLE_RPZ_NSIP
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--rpz-nsdname") == 0) {
#ifdef ENABLE_RPZ_NSDNAME
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--with-idn") == 0) {
#ifdef WITH_IDN
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--ipv6only=no") == 0) {
#ifdef WIN32
		return (0);
#elif defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
		int s;
		int n = -1;
		int v6only = -1;
		ISC_SOCKADDR_LEN_T len = sizeof(v6only);

		s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (s >= 0) {
			n = getsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
				       (void *)&v6only, &len);
			close(s);
		}
		return ((n == 0 && v6only == 0) ? 0 : 1);
#else
		return (1);
#endif
	}

	fprintf(stderr, "unknown arg: %s\n", argv[1]);
	usage();
	return (1);
}
