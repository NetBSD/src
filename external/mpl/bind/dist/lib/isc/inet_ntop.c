/*	$NetBSD: inet_ntop.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <isc/net.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#define NS_INT16SZ	 2
#define NS_IN6ADDRSZ	16

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static const char *inet_ntop4(const unsigned char *src, char *dst,
			      size_t size);

#ifdef AF_INET6
static const char *inet_ntop6(const unsigned char *src, char *dst,
			      size_t size);
#endif

/*! char *
 * isc_net_ntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * \return
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * \author
 *	Paul Vixie, 1996.
 */
const char *
isc_net_ntop(int af, const void *src, char *dst, size_t size)
{
	switch (af) {
	case AF_INET:
		return (inet_ntop4(src, dst, size));
#ifdef AF_INET6
	case AF_INET6:
		return (inet_ntop6(src, dst, size));
#endif
	default:
		errno = EAFNOSUPPORT;
		return (NULL);
	}
	/* NOTREACHED */
}

/*! const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address
 * \return
 *	`dst' (as a const)
 * \note
 *	(1) uses no statics
 * \note
 *	(2) takes a unsigned char* not an in_addr as input
 * \author
 *	Paul Vixie, 1996.
 */
static const char *
inet_ntop4(const unsigned char *src, char *dst, size_t size)
{
	static const char *fmt = "%u.%u.%u.%u";
	char tmp[sizeof("255.255.255.255")];
	int n;


	n = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
	if (n < 0 || (size_t)n >= size) {
		errno = ENOSPC;
		return (NULL);
	}
	strlcpy(dst, tmp, size);

	return (dst);
}

/*! const char *
 * isc_inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * \author
 *	Paul Vixie, 1996.
 */
#ifdef AF_INET6
static const char *
inet_ntop6(const unsigned char *src, char *dst, size_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")], *tp;
	struct { int base, len; } best, cur;
	unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof(words));
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	best.len = 0;	/* silence compiler */
	cur.base = -1;
	cur.len = 0;	/* silence compiler */
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			*tp++ = ':';
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 && (best.len == 6 ||
		    (best.len == 7 && words[7] != 0x0001) ||
		    (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src+12, tp,
					sizeof(tmp) - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		INSIST((size_t)(tp - tmp) < sizeof(tmp));
		tp += snprintf(tp, sizeof(tmp) - (tp - tmp), "%x", words[i]);
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) ==
	    (NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	/*
	 * Check for overflow, copy, and we're done.
	 */
	if ((size_t)(tp - tmp) > size) {
		errno = ENOSPC;
		return (NULL);
	}
	strlcpy(dst, tmp, size);
	return (dst);
}
#endif /* AF_INET6 */
