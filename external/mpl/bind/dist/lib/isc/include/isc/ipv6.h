/*	$NetBSD: ipv6.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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

/*! \file isc/ipv6.h
 * \brief IPv6 definitions for systems which do not support IPv6.
 *
 * \li MP:
 *	No impact.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	N/A.
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	RFC2553.
 */

/***
 *** Imports.
 ***/

#include <isc/int.h>
#include <isc/platform.h>

/***
 *** Types.
 ***/

struct in6_addr {
	union {
		isc_uint8_t	_S6_u8[16];
		isc_uint16_t	_S6_u16[8];
		isc_uint32_t	_S6_u32[4];
	} _S6_un;
};
#define s6_addr		_S6_un._S6_u8
#define s6_addr8	_S6_un._S6_u8
#define s6_addr16	_S6_un._S6_u16
#define s6_addr32	_S6_un._S6_u32

#define IN6ADDR_ANY_INIT 	{{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}}
#define IN6ADDR_LOOPBACK_INIT 	{{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }}}

LIBISC_EXTERNAL_DATA extern const struct in6_addr in6addr_any;
LIBISC_EXTERNAL_DATA extern const struct in6_addr in6addr_loopback;

struct sockaddr_in6 {
#ifdef ISC_PLATFORM_HAVESALEN
	isc_uint8_t		sin6_len;
	isc_uint8_t		sin6_family;
#else
	isc_uint16_t		sin6_family;
#endif
	isc_uint16_t		sin6_port;
	isc_uint32_t		sin6_flowinfo;
	struct in6_addr		sin6_addr;
	isc_uint32_t		sin6_scope_id;
};

#ifdef ISC_PLATFORM_HAVESALEN
#define SIN6_LEN 1
#endif

/*%
 * Unspecified
 */
#define IN6_IS_ADDR_UNSPECIFIED(a)      \
	(((a)->s6_addr32[0] == 0) &&    \
	 ((a)->s6_addr32[1] == 0) &&    \
	 ((a)->s6_addr32[2] == 0) &&    \
	 ((a)->s6_addr32[3] == 0))

/*%
 * Loopback
 */
#define IN6_IS_ADDR_LOOPBACK(a)         \
	(((a)->s6_addr32[0] == 0) &&    \
	 ((a)->s6_addr32[1] == 0) &&    \
	 ((a)->s6_addr32[2] == 0) &&    \
	 ((a)->s6_addr32[3] == htonl(1)))

/*%
 * IPv4 compatible
 */
#define IN6_IS_ADDR_V4COMPAT(a)         \
	(((a)->s6_addr32[0] == 0) &&    \
	 ((a)->s6_addr32[1] == 0) &&    \
	 ((a)->s6_addr32[2] == 0) &&    \
	 ((a)->s6_addr32[3] != 0) &&    \
	 ((a)->s6_addr32[3] != htonl(1)))

/*%
 * Mapped
 */
#define IN6_IS_ADDR_V4MAPPED(a)               \
	(((a)->s6_addr32[0] == 0) &&          \
	 ((a)->s6_addr32[1] == 0) &&          \
	 ((a)->s6_addr32[2] == htonl(0x0000ffff)))

/*%
 * Multicast
 */
#define IN6_IS_ADDR_MULTICAST(a)	\
	((a)->s6_addr8[0] == 0xffU)

/*%
 * Unicast link / site local.
 */
#define IN6_IS_ADDR_LINKLOCAL(a)	\
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
#define IN6_IS_ADDR_SITELOCAL(a)	\
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))

#endif /* ISC_IPV6_H */
