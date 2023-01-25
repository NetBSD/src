/*	$NetBSD: feature-test.c,v 1.10 2023/01/25 21:43:24 christos Exp $	*/

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

#include <limits.h>
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

#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255
#endif

static void
usage() {
	fprintf(stderr, "usage: feature-test <arg>\n");
	fprintf(stderr, "args:\n");
	fprintf(stderr, "\t--edns-version\n");
	fprintf(stderr, "\t--enable-dnsrps\n");
	fprintf(stderr, "\t--enable-dnstap\n");
	fprintf(stderr, "\t--gethostname\n");
	fprintf(stderr, "\t--gssapi\n");
	fprintf(stderr, "\t--have-dlopen\n");
	fprintf(stderr, "\t--have-geoip2\n");
	fprintf(stderr, "\t--have-json-c\n");
	fprintf(stderr, "\t--have-libxml2\n");
	fprintf(stderr, "\t--ipv6only=no\n");
	fprintf(stderr, "\t--tsan\n");
	fprintf(stderr, "\t--with-dlz-filesystem\n");
	fprintf(stderr, "\t--with-idn\n");
	fprintf(stderr, "\t--with-lmdb\n");
	fprintf(stderr, "\t--with-zlib\n");
}

int
main(int argc, char **argv) {
	if (argc != 2) {
		usage();
		return (1);
	}

	if (strcmp(argv[1], "--edns-version") == 0) {
#ifdef DNS_EDNS_VERSION
		printf("%d\n", DNS_EDNS_VERSION);
#else  /* ifdef DNS_EDNS_VERSION */
		printf("0\n");
#endif /* ifdef DNS_EDNS_VERSION */
		return (0);
	}

	if (strcmp(argv[1], "--enable-dnsrps") == 0) {
#ifdef USE_DNSRPS
		return (0);
#else  /* ifdef USE_DNSRPS */
		return (1);
#endif /* ifdef USE_DNSRPS */
	}

	if (strcmp(argv[1], "--enable-dnstap") == 0) {
#ifdef HAVE_DNSTAP
		return (0);
#else  /* ifdef HAVE_DNSTAP */
		return (1);
#endif /* ifdef HAVE_DNSTAP */
	}

	if (strcmp(argv[1], "--gethostname") == 0) {
		char hostname[_POSIX_HOST_NAME_MAX + 1];
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

	if (strcmp(argv[1], "--have-json-c") == 0) {
#ifdef HAVE_JSON_C
		return (0);
#else  /* ifdef HAVE_JSON_C */
		return (1);
#endif /* ifdef HAVE_JSON_C */
	}

	if (strcmp(argv[1], "--have-libxml2") == 0) {
#ifdef HAVE_LIBXML2
		return (0);
#else  /* ifdef HAVE_LIBXML2 */
		return (1);
#endif /* ifdef HAVE_LIBXML2 */
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

	if (strcmp(argv[1], "--tsan") == 0) {
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
		return (0);
#endif
#endif
#if __SANITIZE_THREAD__
		return (0);
#else
		return (1);
#endif
	}

	if (strcmp(argv[1], "--with-dlz-filesystem") == 0) {
#ifdef DLZ_FILESYSTEM
		return (0);
#else  /* ifdef DLZ_FILESYSTEM */
		return (1);
#endif /* ifdef DLZ_FILESYSTEM */
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

	if (strcmp(argv[1], "--with-zlib") == 0) {
#ifdef HAVE_ZLIB
		return (0);
#else  /* ifdef HAVE_ZLIB */
		return (1);
#endif /* ifdef HAVE_ZLIB */
	}

	fprintf(stderr, "unknown arg: %s\n", argv[1]);
	usage();
	return (1);
}
