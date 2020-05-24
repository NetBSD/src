/*	$NetBSD: feature-test.c,v 1.7 2020/05/24 19:46:14 christos Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/net.h>
#include <isc/print.h>
#include <isc/util.h>

#include <dns/edns.h>

#ifdef WIN32
#include <Winsock2.h>
#endif /* ifdef WIN32 */

#ifndef MAXHOSTNAMELEN
#ifdef HOST_NAME_MAX
#define MAXHOSTNAMELEN HOST_NAME_MAX
#else /* ifdef HOST_NAME_MAX */
#define MAXHOSTNAMELEN 256
#endif /* ifdef HOST_NAME_MAX */
#endif /* ifndef MAXHOSTNAMELEN */

static void
usage() {
	fprintf(stderr, "usage: feature-test <arg>\n");
	fprintf(stderr, "args:\n");
	fprintf(stderr, "	--edns-version\n");
	fprintf(stderr, "	--enable-dnsrps\n");
	fprintf(stderr, "	--gethostname\n");
	fprintf(stderr, "	--gssapi\n");
	fprintf(stderr, "	--have-dlopen\n");
	fprintf(stderr, "	--have-geoip2\n");
	fprintf(stderr, "	--have-libxml2\n");
	fprintf(stderr, "	--ipv6only=no\n");
	fprintf(stderr, "	--with-idn\n");
	fprintf(stderr, "	--with-lmdb\n");
	fprintf(stderr, "	--with-dlz-filesystem\n");
}

int
main(int argc, char **argv) {
	if (argc != 2) {
		usage();
		return (1);
	}

	if (strcmp(argv[1], "--enable-dnsrps") == 0) {
#ifdef USE_DNSRPS
		return (0);
#else  /* ifdef USE_DNSRPS */
		return (1);
#endif /* ifdef USE_DNSRPS */
	}

	if (strcmp(argv[1], "--edns-version") == 0) {
#ifdef DNS_EDNS_VERSION
		printf("%d\n", DNS_EDNS_VERSION);
#else  /* ifdef DNS_EDNS_VERSION */
		printf("0\n");
#endif /* ifdef DNS_EDNS_VERSION */
		return (0);
	}

	if (strcmp(argv[1], "--gethostname") == 0) {
		char hostname[MAXHOSTNAMELEN];
		int n;
#ifdef WIN32
		/* From InitSocket() */
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 0);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) {
			fprintf(stderr, "WSAStartup() failed: %d\n", err);
			exit(1);
		}
#endif /* ifdef WIN32 */

		n = gethostname(hostname, sizeof(hostname));
		if (n == -1) {
			perror("gethostname");
			return (1);
		}
		fprintf(stdout, "%s\n", hostname);
#ifdef WIN32
		WSACleanup();
#endif /* ifdef WIN32 */
		return (0);
	}

	if (strcmp(argv[1], "--gssapi") == 0) {
#if defined(GSSAPI)
		return (0);
#else  /* if defined(GSSAPI) */
		return (1);
#endif /* if defined(GSSAPI) */
	}

	if (strcmp(argv[1], "--have-dlopen") == 0) {
#if defined(HAVE_DLOPEN) && defined(ISC_DLZ_DLOPEN)
		return (0);
#else  /* if defined(HAVE_DLOPEN) && defined(ISC_DLZ_DLOPEN) */
		return (1);
#endif /* if defined(HAVE_DLOPEN) && defined(ISC_DLZ_DLOPEN) */
	}

	if (strcmp(argv[1], "--have-geoip2") == 0) {
#ifdef HAVE_GEOIP2
		return (0);
#else  /* ifdef HAVE_GEOIP2 */
		return (1);
#endif /* ifdef HAVE_GEOIP2 */
	}

	if (strcmp(argv[1], "--have-libxml2") == 0) {
#ifdef HAVE_LIBXML2
		return (0);
#else  /* ifdef HAVE_LIBXML2 */
		return (1);
#endif /* ifdef HAVE_LIBXML2 */
	}

	if (strcmp(argv[1], "--with-idn") == 0) {
#ifdef HAVE_LIBIDN2
		return (0);
#else  /* ifdef HAVE_LIBIDN2 */
		return (1);
#endif /* ifdef HAVE_LIBIDN2 */
	}

	if (strcmp(argv[1], "--with-lmdb") == 0) {
#ifdef HAVE_LMDB
		return (0);
#else  /* ifdef HAVE_LMDB */
		return (1);
#endif /* ifdef HAVE_LMDB */
	}

	if (strcmp(argv[1], "--with-dlz-filesystem") == 0) {
#ifdef DLZ_FILESYSTEM
		return (0);
#else  /* ifdef DLZ_FILESYSTEM */
		return (1);
#endif /* ifdef DLZ_FILESYSTEM */
	}

	if (strcmp(argv[1], "--ipv6only=no") == 0) {
#ifdef WIN32
		return (0);
#elif defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
		int s;
		int n = -1;
		int v6only = -1;
		socklen_t len = sizeof(v6only);

		s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (s >= 0) {
			n = getsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
				       (void *)&v6only, &len);
			close(s);
		}
		return ((n == 0 && v6only == 0) ? 0 : 1);
#else  /* ifdef WIN32 */
		return (1);
#endif /* ifdef WIN32 */
	}

	fprintf(stderr, "unknown arg: %s\n", argv[1]);
	usage();
	return (1);
}
