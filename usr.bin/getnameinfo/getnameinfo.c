/*	$NetBSD: getnameinfo.c,v 1.8 2025/02/06 20:59:00 christos Exp $	*/

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
__RCSID("$NetBSD: getnameinfo.c,v 1.8 2025/02/06 20:59:00 christos Exp $");
#endif

#include <sys/types.h>

#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netatalk/at.h>
#include <sys/un.h>
#include <net/if_dl.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "support.h"

/*
 * getnameinfo: Resolve IP addresses and ports to hostnames and service names,
 * similar to the getnameinfo function in the standard library.
 *
 * usage:
 *   getnameinfo [-46FHNnrSu] [-f family] [-p port] <IP-address>
 *
 *   -4: Restrict lookup to IPv4 addresses only
 *   -6: Restrict lookup to IPv6 addresses only
 *   -F: Suppress the fully-qualified domain name (FQDN)
 *   -f: Specify address family to look up
 *   -H: Display only the hostname, omitting the service name
 *   -N: Display the numeric service name instead of the service name
 *   -n: Display the numeric host address instead of the hostname
 *   -p: Specify the port number to be used in the lookup
 *   -r: Ensure that the name is returned (error if no name is found)
 *   -S: Display only the service name, omitting the hostname
 *   -u: Use UDP instead of the default TCP
 */

static void		usage(void) __dead;
static void		print_result(bool, bool, char *, char *);
static in_port_t	get_port(const char *);
static uint8_t		get_family_from_address(const char *);
static uint8_t		get_family(const char *);
static void		parse_atalk(const char *, struct sockaddr_at *);

int
main(int argc, char **argv)
{
	socklen_t hostlen = NI_MAXHOST, servlen = NI_MAXSERV, addrlen;
	char hostname[NI_MAXHOST], service[NI_MAXSERV];
	bool hostname_only = false, service_only = false;
	uint8_t family = AF_UNSPEC;
	int flags = 0;
	char *address = NULL;
	in_port_t port = 0;
	struct sockaddr_storage addr_st;
	struct sockaddr_in *addr_in;
	struct sockaddr_in6 *addr_in6;
	struct sockaddr_un *addr_un;
	struct sockaddr_dl *addr_dl;
	int ch;
	int error;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "46Ff:HNnp:rSu")) != -1) {
		switch (ch) {
		case '4':
			if (family != AF_UNSPEC)
				goto opt46f;
			family = AF_INET;
			break;
		case '6':
			if (family != AF_UNSPEC)
				goto opt46f;
			family = AF_INET6;
			break;
		case 'r':
			flags |= NI_NAMEREQD;
			break;
		case 'u':
			flags |= NI_DGRAM;
			break;
		case 'f':
			if (family != AF_UNSPEC)
				goto opt46f;
			family = get_family(optarg);
			break;
		case 'F':
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
	    (!hostname_only && !service_only))) {
		warnx("No IP address provided");
		usage();
	}

	if (port == 0 && (service_only ||
	    (!hostname_only && !service_only))) {
		warnx("No port number provided");
		usage();
	}

	if (argc == 1) {
		address = argv[0];
		if (family == AF_UNSPEC)
			family = get_family_from_address(address);
	}

	memset(&addr_st, 0, sizeof(addr_st));

	switch (family) {
	case AF_APPLETALK:
		parse_atalk(address, (struct sockaddr_at *)&addr_st);
		addrlen = sizeof(struct sockaddr_at *);
		break;
	case AF_INET:
		addr_in = (struct sockaddr_in *)&addr_st;
		addr_in->sin_family = family;
		addr_in->sin_port = htons(port);
		if (address == NULL) {
			addr_in->sin_addr.s_addr = INADDR_ANY;
		} else if (inet_pton(family, address, &addr_in->sin_addr)
		    == 0)
			errx(EXIT_FAILURE, "Invalid IPv4 address: %s", address);
		addrlen = sizeof(*addr_in);
		break;
	case AF_INET6:
		addr_in6 = (struct sockaddr_in6 *)&addr_st;
		addr_in6->sin6_family = family;
		addr_in6->sin6_port = htons(port);
		if (address == NULL) {
			addr_in6->sin6_addr =
			    (struct in6_addr)IN6ADDR_ANY_INIT;
		} else if (inet_pton(family, address, &addr_in6->sin6_addr)
		    == 0)
			errx(EXIT_FAILURE, "Invalid IPv6 address: %s", address);
		addrlen = sizeof(*addr_in6);
		break;
	case AF_LINK:
		addr_dl = (struct sockaddr_dl *)&addr_st;
		addr_dl->sdl_len = sizeof(addr_st);
		link_addr(address, addr_dl);
		addrlen = addr_dl->sdl_len;
		break;
	case AF_LOCAL:
		addr_un = (struct sockaddr_un *)&addr_st;
		addr_un->sun_family = family;
		if (strlen(address) >= sizeof(addr_un->sun_path))
			errx(EXIT_FAILURE, "Invalid AF_LOCAL address: %s",
			    address);
		(void)strncpy(addr_un->sun_path, address,
		    sizeof(addr_un->sun_path));
		addrlen = sizeof(*addr_un);
		break;
	default:
		errx(EXIT_FAILURE, "Unsupported family %d", family);
	}

	if (hostname_only)
		servlen = 0;
	else if (service_only)
		hostlen = 0;

	error = getnameinfo((struct sockaddr *)&addr_st, addrlen,
	    hostname, hostlen, service, servlen, flags);
	if (error)
		errx(EXIT_FAILURE, "%s", gai_strerror(error));

	print_result(hostname_only, service_only, hostname, service);

	fflush(stdout);
	return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
opt46f:
	warnx("Options -4, -6, -f cannot be used together");
	usage();
optHS:
	warnx("Options -H and -S cannot be used together");
	usage();
}

static void
parse_atalk(const char *address, struct sockaddr_at *addr_at)
{
	int net, node, port;

	if (sscanf(address, "%d:%d:%d", &net, &node, &port) != 3)
badat:		errx(EXIT_FAILURE, "Invalid appletalk address: %s", address);

	if (net < 0 || net > 0xFFFF || node < 0 || node > 0xFF ||
	     port < 0 || port > 0xFFFF)
		goto badat;
	addr_at->sat_family = AF_APPLETALK;
	addr_at->sat_addr.s_net = htons((u_short)net);
	addr_at->sat_addr.s_node = (u_char)node;
	addr_at->sat_port = htons((u_short)port);
}

static uint8_t
get_family_from_address(const char *address)
{
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (inet_pton(AF_INET, address, &ipv4_addr) == 1)
		return AF_INET;

	if (inet_pton(AF_INET6, address, &ipv6_addr) == 1)
		return AF_INET6;

	errx(EXIT_FAILURE, "Invalid addrsss %s", address);
}

static uint8_t
get_family(const char* str)
{
	int fam;
	if (!parse_af(str, &fam) || fam <= 0 || fam > 255)
		errx(EXIT_FAILURE, "Invalid family %s", str);
	return (int8_t)fam;
}

static in_port_t
get_port(const char *port_str)
{
	int r;

	const intmax_t port = strtoi(port_str, NULL, 0, 0, 65535, &r);
	if (r)
		errc(EXIT_FAILURE, r, "Invalid port number %s", port_str);

	return (in_port_t)port;
}

static void
print_result(bool hostname_only, bool service_only,
    char *hostname, char *service)
{

	if (hostname_only)
		printf("%s\n", hostname);
	else if (service_only)
		printf("%s\n", service);
	else
		printf("%s %s\n", hostname, service);
}

static void __dead
usage(void)
{

	(void)fprintf(stderr, "Usage: %s", getprogname());
	(void)fprintf(stderr, " [-46fHNnrSu] [-p port] [<IP-address>]\n");
	exit(EXIT_FAILURE);
}
