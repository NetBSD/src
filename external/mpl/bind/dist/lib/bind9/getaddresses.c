/*	$NetBSD: getaddresses.c,v 1.7 2023/01/25 21:43:29 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/netdb.h>
#include <isc/netscope.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

#include <bind9/getaddresses.h>

isc_result_t
bind9_getaddresses(const char *hostname, in_port_t port, isc_sockaddr_t *addrs,
		   int addrsize, int *addrcount) {
	struct in_addr in4;
	struct in6_addr in6;
	bool have_ipv4, have_ipv6;
	int i;

	struct addrinfo *ai = NULL, *tmpai, hints;
	int result;

	REQUIRE(hostname != NULL);
	REQUIRE(addrs != NULL);
	REQUIRE(addrcount != NULL);
	REQUIRE(addrsize > 0);

	have_ipv4 = (isc_net_probeipv4() == ISC_R_SUCCESS);
	have_ipv6 = (isc_net_probeipv6() == ISC_R_SUCCESS);

	/*
	 * Try IPv4, then IPv6.  In order to handle the extended format
	 * for IPv6 scoped addresses (address%scope_ID), we'll use a local
	 * working buffer of 128 bytes.  The length is an ad-hoc value, but
	 * should be enough for this purpose; the buffer can contain a string
	 * of at least 80 bytes for scope_ID in addition to any IPv6 numeric
	 * addresses (up to 46 bytes), the delimiter character and the
	 * terminating NULL character.
	 */
	if (inet_pton(AF_INET, hostname, &in4) == 1) {
		if (have_ipv4) {
			isc_sockaddr_fromin(&addrs[0], &in4, port);
		} else {
			isc_sockaddr_v6fromin(&addrs[0], &in4, port);
		}
		*addrcount = 1;
		return (ISC_R_SUCCESS);
	} else if (strlen(hostname) <= 127U) {
		char tmpbuf[128], *d;
		uint32_t zone = 0;

		strlcpy(tmpbuf, hostname, sizeof(tmpbuf));
		d = strchr(tmpbuf, '%');
		if (d != NULL) {
			*d = '\0';
		}

		if (inet_pton(AF_INET6, tmpbuf, &in6) == 1) {
			isc_netaddr_t na;

			if (!have_ipv6) {
				return (ISC_R_FAMILYNOSUPPORT);
			}

			if (d != NULL) {
				isc_result_t iresult;

				iresult = isc_netscope_pton(AF_INET6, d + 1,
							    &in6, &zone);

				if (iresult != ISC_R_SUCCESS) {
					return (iresult);
				}
			}

			isc_netaddr_fromin6(&na, &in6);
			isc_netaddr_setzone(&na, zone);
			isc_sockaddr_fromnetaddr(
				&addrs[0], (const isc_netaddr_t *)&na, port);

			*addrcount = 1;
			return (ISC_R_SUCCESS);
		}
	}
	memset(&hints, 0, sizeof(hints));
	if (!have_ipv6) {
		hints.ai_family = PF_INET;
	} else if (!have_ipv4) {
		hints.ai_family = PF_INET6;
	} else {
		hints.ai_family = PF_UNSPEC;
#ifdef AI_ADDRCONFIG
		hints.ai_flags = AI_ADDRCONFIG;
#endif /* ifdef AI_ADDRCONFIG */
	}
	hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
again:
#endif /* ifdef AI_ADDRCONFIG */
	result = getaddrinfo(hostname, NULL, &hints, &ai);
	switch (result) {
	case 0:
		break;
	case EAI_NONAME:
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
	case EAI_NODATA:
#endif /* if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME) */
		return (ISC_R_NOTFOUND);
#ifdef AI_ADDRCONFIG
	case EAI_BADFLAGS:
		if ((hints.ai_flags & AI_ADDRCONFIG) != 0) {
			hints.ai_flags &= ~AI_ADDRCONFIG;
			goto again;
		}
#endif /* ifdef AI_ADDRCONFIG */
		FALLTHROUGH;
	default:
		return (ISC_R_FAILURE);
	}
	for (tmpai = ai, i = 0; tmpai != NULL && i < addrsize;
	     tmpai = tmpai->ai_next)
	{
		if (tmpai->ai_family != AF_INET && tmpai->ai_family != AF_INET6)
		{
			continue;
		}
		if (tmpai->ai_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)tmpai->ai_addr;
			isc_sockaddr_fromin(&addrs[i], &sin->sin_addr, port);
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)tmpai->ai_addr;
			isc_sockaddr_fromin6(&addrs[i], &sin6->sin6_addr, port);
		}
		i++;
	}
	freeaddrinfo(ai);
	*addrcount = i;
	if (*addrcount == 0) {
		return (ISC_R_NOTFOUND);
	} else {
		return (ISC_R_SUCCESS);
	}
}
