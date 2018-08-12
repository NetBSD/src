/*	$NetBSD: net.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2000, 2001  Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (C) 2004 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isc/result.h>
#include <isc/sockaddr.h>

#include <bind9/getaddresses.h>

#include "log.h"
#include "net.h"
#include "opt.h"

int
perf_net_parsefamily(const char *family)
{
	if (family == NULL || strcmp(family, "any") == 0)
		return AF_UNSPEC;
	else if (strcmp(family, "inet") == 0)
		return AF_INET;
#ifdef AF_INET6
	else if (strcmp(family, "inet6") == 0)
		return AF_INET6;
#endif
	else {
		fprintf(stderr, "invalid family %s\n", family);
		perf_opt_usage();
		exit(1);
	}
}

void
perf_net_parseserver(int family, const char *name, unsigned int port,
		     isc_sockaddr_t *addr)
{
	isc_sockaddr_t addrs[8];
	int count, i;
	isc_result_t result;

	if (port == 0) {
		fprintf(stderr, "server port cannot be 0\n");
		perf_opt_usage();
		exit(1);
	}

	count = 0;
	result = bind9_getaddresses(name, port, addrs, 8, &count);
	if (result == ISC_R_SUCCESS) {
		for (i = 0; i < count; i++) {
			if (isc_sockaddr_pf(&addrs[i]) == family ||
			    family == AF_UNSPEC)
			{
				*addr = addrs[i];
				return;
			}
		}
	}

	fprintf(stderr, "invalid server address %s\n", name);
	perf_opt_usage();
	exit(1);
}

void
perf_net_parselocal(int family, const char *name, unsigned int port,
		    isc_sockaddr_t *addr)
{
	struct in_addr in4a;
	struct in6_addr in6a;

	if (name == NULL) {
		isc_sockaddr_anyofpf(addr, family);
		isc_sockaddr_setport(addr, port);
	} else if (inet_pton(AF_INET, name, &in4a) == 1) {
		isc_sockaddr_fromin(addr, &in4a, port);
	} else if (inet_pton(AF_INET6, name, &in6a) == 1) {
		isc_sockaddr_fromin6(addr, &in6a, port);
	} else {
		fprintf(stderr, "invalid local address %s\n", name);
		perf_opt_usage();
		exit(1);
	}
}

int
perf_net_opensocket(const isc_sockaddr_t *server, const isc_sockaddr_t *local,
		    unsigned int offset, int bufsize)
{
	int family;
	int sock;
	isc_sockaddr_t tmp;
	int port;
	int ret;
	int flags;

	family = isc_sockaddr_pf(server);

	if (isc_sockaddr_pf(local) != family)
		perf_log_fatal("server and local addresses have "
			       "different families");

	sock = socket(family, SOCK_DGRAM, 0);
	if (sock == -1)
		perf_log_fatal("Error: socket: %s\n", strerror(errno));

#if defined(AF_INET6) && defined(IPV6_V6ONLY)
	if (family == AF_INET6) {
		int on = 1;

		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
			       &on, sizeof(on)) == -1) {
			perf_log_warning("setsockopt(IPV6_V6ONLY) failed");
		}
	}
#endif

	tmp = *local;
	port = isc_sockaddr_getport(&tmp);
	if (port != 0 && offset != 0) {
		port += offset;
		if (port >= 0xFFFF)
			perf_log_fatal("port %d out of range", port);
		isc_sockaddr_setport(&tmp, port);
	}

	if (bind(sock, &tmp.type.sa, tmp.length) == -1)
		perf_log_fatal("bind: %s\n", strerror(errno));

	if (bufsize > 0) {
		bufsize *= 1024;

		ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
				 &bufsize, sizeof(bufsize));
		if (ret < 0)
			perf_log_warning("setsockbuf(SO_RCVBUF) failed");

		ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
				 &bufsize, sizeof(bufsize));
		if (ret < 0)
			perf_log_warning("setsockbuf(SO_SNDBUF) failed");
	}

	flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0)
		perf_log_fatal("fcntl(F_GETFL)");
	ret = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if (ret < 0)
		perf_log_fatal("fcntl(F_SETFL)");

	return sock;
}
