/*	$NetBSD: getaddresses.c,v 1.1.1.1 2004/05/17 23:44:48 christos Exp $	*/

/*
 * Copyright (C) 2004  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001, 2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/* Id: getaddresses.c,v 1.13.126.4 2004/03/08 09:04:27 marka Exp */

#include <config.h>
#include <string.h>

#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/netdb.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <bind9/getaddresses.h>

#ifdef HAVE_ADDRINFO
#ifdef HAVE_GETADDRINFO
#ifdef HAVE_GAISTRERROR
#define USE_GETADDRINFO
#endif
#endif
#endif

#ifndef USE_GETADDRINFO
#ifndef ISC_PLATFORM_NONSTDHERRNO
extern int h_errno;
#endif
#endif

isc_result_t
bind9_getaddresses(const char *hostname, in_port_t port,
		   isc_sockaddr_t *addrs, int addrsize, int *addrcount)
{
	struct in_addr in4;
	struct in6_addr in6;
	isc_boolean_t have_ipv4, have_ipv6;
	int i;

#ifdef USE_GETADDRINFO
	struct addrinfo *ai = NULL, *tmpai, hints;
	int result;
#else
	struct hostent *he;
#endif

	REQUIRE(hostname != NULL);
	REQUIRE(addrs != NULL);
	REQUIRE(addrcount != NULL);
	REQUIRE(addrsize > 0);

	have_ipv4 = (isc_net_probeipv4() == ISC_R_SUCCESS);
	have_ipv6 = (isc_net_probeipv6() == ISC_R_SUCCESS);

	if (inet_pton(AF_INET6, hostname, &in6) == 1) {
		if (!have_ipv6)
			return (ISC_R_FAMILYNOSUPPORT);
		isc_sockaddr_fromin6(&addrs[0], &in6, port);
		*addrcount = 1;
		return (ISC_R_SUCCESS);
	} else if (inet_pton(AF_INET, hostname, &in4) == 1) {
		if (have_ipv4)
			isc_sockaddr_fromin(&addrs[0], &in4, port);
		else
			isc_sockaddr_v6fromin(&addrs[0], &in4, port);
		*addrcount = 1;
		return (ISC_R_SUCCESS);
	}
#ifdef USE_GETADDRINFO
	memset(&hints, 0, sizeof(hints));
	if (!have_ipv6)
		hints.ai_family = PF_INET;
	else if (!have_ipv4)
		hints.ai_family = PF_INET6;
	else {
		hints.ai_family = PF_UNSPEC;
#ifdef AI_ADDRCONFIG
		hints.ai_flags = AI_ADDRCONFIG;
#endif
	}
	hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
 again:
#endif
	result = getaddrinfo(hostname, NULL, &hints, &ai);
	switch (result) {
	case 0:
		break;
	case EAI_NONAME:
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
	case EAI_NODATA:
#endif
		return (ISC_R_NOTFOUND);
#ifdef AI_ADDRCONFIG
	case EAI_BADFLAGS:
		if ((hints.ai_flags & AI_ADDRCONFIG) != 0) {
			hints.ai_flags &= ~AI_ADDRCONFIG;
			goto again;
		}
#endif
	default:
		return (ISC_R_FAILURE);
	}
	for (tmpai = ai, i = 0;
	     tmpai != NULL && i < addrsize;
	     tmpai = tmpai->ai_next)
	{
		if (tmpai->ai_family != AF_INET &&
		    tmpai->ai_family != AF_INET6)
			continue;
		if (tmpai->ai_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)tmpai->ai_addr;
			isc_sockaddr_fromin(&addrs[i], &sin->sin_addr, port);
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)tmpai->ai_addr;
			isc_sockaddr_fromin6(&addrs[i], &sin6->sin6_addr,
					     port);
		}
		i++;

	}
	freeaddrinfo(ai);
	*addrcount = i;
#else
	he = gethostbyname(hostname);
	if (he == NULL) {
		switch (h_errno) {
		case HOST_NOT_FOUND:
#ifdef NO_DATA
		case NO_DATA:
#endif
#if defined(NO_ADDRESS) && (!defined(NO_DATA) || (NO_DATA != NO_ADDRESS))
		case NO_ADDRESS:
#endif
			return (ISC_R_NOTFOUND);
		default:
			return (ISC_R_FAILURE);
		}
	}
	if (he->h_addrtype != AF_INET && he->h_addrtype != AF_INET6)
		return (ISC_R_NOTFOUND);
	for (i = 0; i < addrsize; i++) {
		if (he->h_addrtype == AF_INET) {
			struct in_addr *inp;
			inp = (struct in_addr *)(he->h_addr_list[i]);
			if (inp == NULL)
				break;
			isc_sockaddr_fromin(&addrs[i], inp, port);
		} else {
			struct in6_addr *in6p;
			in6p = (struct in6_addr *)(he->h_addr_list[i]);
			if (in6p == NULL)
				break;
			isc_sockaddr_fromin6(&addrs[i], in6p, port);
		}
	}
	*addrcount = i;
#endif
	if (*addrcount == 0)
		return (ISC_R_NOTFOUND);
	else
		return (ISC_R_SUCCESS);
}
