/*	$NetBSD: ipv6.h,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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


#ifndef ISC_IPV6_H
#define ISC_IPV6_H 1

/*****
 ***** Module Info
 *****/

/*
 * IPv6 definitions for systems which do not support IPv6.
 *
 * MP:
 *	No impact.
 *
 * Reliability:
 *	No anticipated impact.
 *
 * Resources:
 *	N/A.
 *
 * Security:
 *	No anticipated impact.
 *
 * Standards:
 *	RFC2553.
 */

#if _MSC_VER < 1300
#define in6_addr in_addr6
#endif

#ifndef IN6ADDR_ANY_INIT
#define IN6ADDR_ANY_INIT 	{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}
#endif
#ifndef IN6ADDR_LOOPBACK_INIT
#define IN6ADDR_LOOPBACK_INIT 	{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }}
#endif
#ifndef IN6ADDR_V4MAPPED_INIT
#define IN6ADDR_V4MAPPED_INIT 	{{ 0,0,0,0,0,0,0,0,0,0,0xff,0xff,0,0,0,0 }}
#endif

LIBISC_EXTERNAL_DATA extern const struct in6_addr isc_in6addr_any;
LIBISC_EXTERNAL_DATA extern const struct in6_addr isc_in6addr_loopback;

/*
 * Unspecified
 */
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) (\
*((u_long *)((a)->s6_addr)    ) == 0 && \
*((u_long *)((a)->s6_addr) + 1) == 0 && \
*((u_long *)((a)->s6_addr) + 2) == 0 && \
*((u_long *)((a)->s6_addr) + 3) == 0 \
)
#endif

/*
 * Loopback
 */
#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) (\
*((u_long *)((a)->s6_addr)    ) == 0 && \
*((u_long *)((a)->s6_addr) + 1) == 0 && \
*((u_long *)((a)->s6_addr) + 2) == 0 && \
*((u_long *)((a)->s6_addr) + 3) == htonl(1) \
)
#endif

/*
 * IPv4 compatible
 */
#define IN6_IS_ADDR_V4COMPAT(a)  (\
*((u_long *)((a)->s6_addr)    ) == 0 && \
*((u_long *)((a)->s6_addr) + 1) == 0 && \
*((u_long *)((a)->s6_addr) + 2) == 0 && \
*((u_long *)((a)->s6_addr) + 3) != 0 && \
*((u_long *)((a)->s6_addr) + 3) != htonl(1) \
)

/*
 * Mapped
 */
#define IN6_IS_ADDR_V4MAPPED(a) (\
*((u_long *)((a)->s6_addr)    ) == 0 && \
*((u_long *)((a)->s6_addr) + 1) == 0 && \
*((u_long *)((a)->s6_addr) + 2) == htonl(0x0000ffff))

/*
 * Multicast
 */
#define IN6_IS_ADDR_MULTICAST(a)	\
	((a)->s6_addr[0] == 0xffU)

/*
 * Unicast link / site local.
 */
#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a)	(\
       ((a)->s6_addr[0] == 0xfe) && \
       (((a)->s6_addr[1] & 0xc0) == 0x80))
#endif

#ifndef IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_SITELOCAL(a)	(\
       ((a)->s6_addr[0] == 0xfe) && \
       (((a)->s6_addr[1] & 0xc0) == 0xc0))
#endif

#endif /* ISC_IPV6_H */
