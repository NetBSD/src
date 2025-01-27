/* $NetBSD: getnameinfo.c,v 1.2 2025/01/27 19:12:11 wiz Exp $ */

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Attaullah Ansari.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: getnameinfo.c,v 1.2 2025/01/27 19:12:11 wiz Exp $");
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * getnameinfo: Resolve IP addresses and ports to hostnames and service names,
 * similar to the getnameinfo function in the standard library.
 *
 * usage:
 *   getnameinfo [-46fHNnrSu] [-p port] <IP-address>
 *
 *   -4: Restrict lookup to IPv4 addresses only
 *   -6: Restrict lookup to IPv6 addresses only
 *   -f: Suppress the fully-qualified domain name (FQDN)
 *   -H: Display only the hostname, omitting the service name
 *   -N: Display the numeric service name instead of the service name
 *   -n: Display the numeric host address instead of the hostname
 *   -p: Specify the port number to be used in the lookup
 *   -r: Ensure that the name is returned (error if no name is found)
 *   -S: Display only the service name, omitting the hostname
 *   -u: Use UDP instead of the default TCP
 */


static void	usage(void) __dead;
static void	print_result(int, int, char *, char *);
static in_port_t	get_port(const char *);
static uint8_t	get_family(const char *);

int
main(int argc, char **argv)
{
	int hostlen = NI_MAXHOST, servlen = NI_MAXSERV;
	char hostname[hostlen], service[servlen];
	bool hostname_only = false, service_only = false;
	int family = AF_UNSPEC;
	int flags = 0;
	char *address = NULL; 
	in_port_t port = 0;
	struct sockaddr_storage addr_st;
	struct sockaddr_in *addr_in;
	struct sockaddr_in6 *addr_in6;
	int addr_stlen;
	int ch;
	int error;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "46rufnNHSp:")) != -1) {
		switch (ch) {
		case '4':
			if (family != AF_UNSPEC)
				goto opt46;
			family = AF_INET;
			break;
		case '6':
			if (family != AF_UNSPEC)
				goto opt46;
			family = AF_INET6;
			break;
		case 'r':
			flags |= NI_NAMEREQD;
			break;
		case 'u':
			flags |= NI_DGRAM;
			break;
		case 'f':
			flags |= NI_NOFQDN;
			break;
		case 'n':
			flags |= NI_NUMERICHOST;
			break;
		case 'N':
			flags |= NI_NUMERICSERV;
			break;
		case 'H':
			if (service_only)
				goto optHS;
			hostname_only = true;
			break;
		case 'S':
			if (hostname_only)
				goto optHS;
			service_only = true;
			break;
		case 'p':
			port = get_port(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 1) {
		warnx("Too many addresses");
		usage();
	}

	if (argc == 0 && (hostname_only || 
	    (hostname_only == 0 && service_only == 0))) {
		warnx("No IP address provided");
		usage();
	}

	if (port == 0 && (service_only ||
	    (hostname_only == 0 && service_only == 0))) {
		warnx("No port number provided");
		usage();
	}

	if (argc == 1) {
		address = argv[0];
		if (family == AF_UNSPEC)
			family = get_family(address);
	}

	switch (family) {
	case AF_INET:
		addr_in = (struct sockaddr_in *)&addr_st;
		addr_in->sin_family = family;
		addr_in->sin_port = htons(port);
		if (inet_pton(family, address, &addr_in->sin_addr) == 0) {
			warnx("Invalid IPv4 address: %s", address);
			return EXIT_FAILURE;
		} 
		addr_stlen = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		addr_in6 = (struct sockaddr_in6 *)&addr_st;
		addr_in6->sin6_family = family;
		addr_in6->sin6_port = htons(port);
		if (inet_pton(family, address, &addr_in6->sin6_addr) == 0) {
			warnx("Invalid IPv6 address: %s", address);
			return EXIT_FAILURE;
		}
		addr_stlen = sizeof(struct sockaddr_in6);
		break;
	default:
		warnx("Unsupported family %d", family);
		return EXIT_FAILURE;
	}

	if (hostname_only)
		servlen = 0;
	else if (service_only)
		hostlen = 0;

	error = getnameinfo((struct sockaddr *)&addr_st, addr_stlen,
	    hostname, hostlen, service, servlen, flags);
	if (error) 
		errx(EXIT_FAILURE, "%s", gai_strerror(error));

	print_result(hostname_only, service_only, hostname, service);

	return EXIT_SUCCESS;
opt46:
	warnx("Options -4 and -6 cannot be used together");
	usage();
optHS:
	warnx("Options -H and -S cannot be used together");
	usage();
}

static uint8_t
get_family(const char* address)
{
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (inet_pton(AF_INET, address, &ipv4_addr) == 1)
		return AF_INET;

	if (inet_pton(AF_INET6, address, &ipv6_addr) == 1)
		return AF_INET6;

	errx(EXIT_FAILURE, "Invalid addrsss %s", address);
}

static in_port_t
get_port(const char *port_str)
{
	int r;
	intmax_t port = strtoi(port_str, NULL, 0, 0, 65535, &r);
	if (r)
		errc(EXIT_FAILURE, r, "Invalid port number %s", port_str);

	return (in_port_t)port;
}

static void
print_result(int hostname_only, int service_only, char *hostname, char *service)
{
	int n;
	if (hostname_only)
		n = printf("%s\n", hostname);
	else if (service_only)
		n = printf("%s\n", service);
	else
		n = printf("%s %s\n", hostname, service);
	if (n < 0)
		err(EXIT_FAILURE, "printf");
}


static void __dead
usage(void)
{
	(void)fprintf(stderr, "Usage: %s", getprogname());
	(void)fprintf(stderr, " [-46fHNnrSu] [-p port] <IP-address>\n");
	exit(EXIT_FAILURE);
}
