/*	$NetBSD: netaddr.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_NETADDR_H
#define ISC_NETADDR_H 1

/*! \file isc/netaddr.h */

#include <isc/lang.h>
#include <isc/net.h>
#include <isc/types.h>

#ifdef ISC_PLATFORM_HAVESYSUNH
#include <sys/types.h>
#include <sys/un.h>
#endif

ISC_LANG_BEGINDECLS

struct isc_netaddr {
	unsigned int family;
	union {
		struct in_addr in;
		struct in6_addr in6;
#ifdef ISC_PLATFORM_HAVESYSUNH
		char un[sizeof(((struct sockaddr_un *)0)->sun_path)];
#endif
	} type;
	isc_uint32_t zone;
};

isc_boolean_t
isc_netaddr_equal(const isc_netaddr_t *a, const isc_netaddr_t *b);

/*%<
 * Compare network addresses 'a' and 'b'.  Return #ISC_TRUE if
 * they are equal, #ISC_FALSE if not.
 */

isc_boolean_t
isc_netaddr_eqprefix(const isc_netaddr_t *a, const isc_netaddr_t *b,
		     unsigned int prefixlen);
/*%<
 * Compare the 'prefixlen' most significant bits of the network
 * addresses 'a' and 'b'.  If 'b''s scope is zero then 'a''s scope is
 * ignored.  Return #ISC_TRUE if they are equal, #ISC_FALSE if not.
 */

isc_result_t
isc_netaddr_masktoprefixlen(const isc_netaddr_t *s, unsigned int *lenp);
/*%<
 * Convert a netmask in 's' into a prefix length in '*lenp'.
 * The mask should consist of zero or more '1' bits in the
 * most significant part of the address, followed by '0' bits.
 * If this is not the case, #ISC_R_MASKNONCONTIG is returned.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_MASKNONCONTIG
 */

isc_result_t
isc_netaddr_totext(const isc_netaddr_t *netaddr, isc_buffer_t *target);
/*%<
 * Append a text representation of 'sockaddr' to the buffer 'target'.
 * The text is NOT null terminated.  Handles IPv4 and IPv6 addresses.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOSPACE	The text or the null termination did not fit.
 *\li	#ISC_R_FAILURE	Unspecified failure
 */

void
isc_netaddr_format(const isc_netaddr_t *na, char *array, unsigned int size);
/*%<
 * Format a human-readable representation of the network address '*na'
 * into the character array 'array', which is of size 'size'.
 * The resulting string is guaranteed to be null-terminated.
 */

#define ISC_NETADDR_FORMATSIZE \
	sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:XXX.XXX.XXX.XXX%SSSSSSSSSS")
/*%<
 * Minimum size of array to pass to isc_netaddr_format().
 */

void
isc_netaddr_fromsockaddr(isc_netaddr_t *netaddr, const isc_sockaddr_t *source);

void
isc_netaddr_fromin(isc_netaddr_t *netaddr, const struct in_addr *ina);

void
isc_netaddr_fromin6(isc_netaddr_t *netaddr, const struct in6_addr *ina6);

isc_result_t
isc_netaddr_frompath(isc_netaddr_t *netaddr, const char *path);

void
isc_netaddr_setzone(isc_netaddr_t *netaddr, isc_uint32_t zone);

isc_uint32_t
isc_netaddr_getzone(const isc_netaddr_t *netaddr);

void
isc_netaddr_any(isc_netaddr_t *netaddr);
/*%<
 * Return the IPv4 wildcard address.
 */

void
isc_netaddr_any6(isc_netaddr_t *netaddr);
/*%<
 * Return the IPv6 wildcard address.
 */

void
isc_netaddr_unspec(isc_netaddr_t *netaddr);
/*%<
 * Initialize as AF_UNSPEC address.
 */

isc_boolean_t
isc_netaddr_ismulticast(const isc_netaddr_t *na);
/*%<
 * Returns ISC_TRUE if the address is a multicast address.
 */

isc_boolean_t
isc_netaddr_isexperimental(const isc_netaddr_t *na);
/*%<
 * Returns ISC_TRUE if the address is a experimental (CLASS E) address.
 */

isc_boolean_t
isc_netaddr_islinklocal(const isc_netaddr_t *na);
/*%<
 * Returns #ISC_TRUE if the address is a link local address.
 */

isc_boolean_t
isc_netaddr_issitelocal(const isc_netaddr_t *na);
/*%<
 * Returns #ISC_TRUE if the address is a site local address.
 */

isc_boolean_t
isc_netaddr_isnetzero(const isc_netaddr_t *na);
/*%<
 * Returns #ISC_TRUE if the address is in net zero.
 */

void
isc_netaddr_fromv4mapped(isc_netaddr_t *t, const isc_netaddr_t *s);
/*%<
 * Convert an IPv6 v4mapped address into an IPv4 address.
 */

isc_result_t
isc_netaddr_prefixok(const isc_netaddr_t *na, unsigned int prefixlen);
/*
 * Test whether the netaddr 'na' and 'prefixlen' are consistant.
 * e.g. prefixlen within range.
 *      na does not have bits set which are not covered by the prefixlen.
 *
 * Returns:
 *	ISC_R_SUCCESS
 *	ISC_R_RANGE		prefixlen out of range
 *	ISC_R_NOTIMPLEMENTED	unsupported family
 *	ISC_R_FAILURE		extra bits.
 */

isc_boolean_t
isc_netaddr_isloopback(const isc_netaddr_t *na);
/*
 * Test whether the netaddr 'na' is a loopback IPv4 or IPv6 address (in
 * 127.0.0.0/8 or ::1).
 */
ISC_LANG_ENDDECLS

#endif /* ISC_NETADDR_H */
