/*	$NetBSD: inet_cidr_ntop.c,v 1.9 2026/04/08 14:12:06 christos Exp $	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1998,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static const char rcsid[] = "Id: inet_cidr_ntop.c,v 1.7 2006/10/11 02:18:18 marka Exp";
#else
__RCSID("$NetBSD: inet_cidr_ntop.c,v 1.9 2026/04/08 14:12:06 christos Exp $");
#endif
#endif

#include "port_before.h"

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "port_after.h"

#ifdef __weak_alias
__weak_alias(inet_cidr_ntop,_inet_cidr_ntop)
#endif

static char *
inet_cidr_ntop_ipv4(const u_char *src, int bits, char *dst, size_t size);
static char *
inet_cidr_ntop_ipv6(const u_char *src, int bits, char *dst, size_t size);

/*%
 * char *
 * inet_cidr_ntop(af, src, bits, dst, size)
 *	convert network address from network to presentation format.
 *	"src"'s size is determined from its "af".
 * return:
 *	pointer to dst, or NULL if an error occurred (check errno).
 * note:
 *	192.5.5.1/28 has a nonzero host part, which means it isn't a network
 *	as called for by inet_net_ntop() but it can be a host address with
 *	an included netmask.
 * author:
 *	Paul Vixie (ISC), October 1998
 */
char *
inet_cidr_ntop(int af, const void *src, int bits, char *dst, size_t size) {
	switch (af) {
	case AF_INET:
		return (inet_cidr_ntop_ipv4(src, bits, dst, size));
	case AF_INET6:
		return (inet_cidr_ntop_ipv6(src, bits, dst, size));
	default:
		errno = EAFNOSUPPORT;
		return (NULL);
	}
}

static int
decoct(const u_char *src, size_t bytes, char *dst, size_t size) {
	size_t b;
	int l, t = 0;

	for (b = 1; b <= bytes; b++) {
		ADDS(snprintf(dst + t, size - t, "%u", *src++));
		if (b != bytes)
			ADDC('.');
	}
	return t;
emsgsize:
	return l < 0 ? l : l + t;
}

/*%
 * static char *
 * inet_cidr_ntop_ipv4(src, bits, dst, size)
 *	convert IPv4 network address from network to presentation format.
 *	"src"'s size is determined from its "af".
 * return:
 *	pointer to dst, or NULL if an error occurred (check errno).
 * note:
 *	network byte order assumed.  this means 192.5.5.240/28 has
 *	0b11110000 in its fourth octet.
 * author:
 *	Paul Vixie (ISC), October 1998
 */
static char *
inet_cidr_ntop_ipv4(const u_char *src, int bits, char *dst, size_t size) {
	size_t len = 4, b, bytes, t = 0;
	int l;

	if ((bits < -1) || (bits > 32)) {
		errno = EINVAL;
		return (NULL);
	}

	/* Find number of significant bytes in address. */
	if (bits == -1)
		len = 4;
	else
		for (len = 1, b = 1 ; b < 4U; b++)
			if (*(src + b))
				len = b + 1;

	/* Format whole octets plus nonzero trailing octets. */
	bytes = (((bits <= 0) ? 1 : bits) + 7) / 8;
	if (len > bytes)
		bytes = len;
	ADDS(decoct(src, bytes, dst + t, size - t));

	if (bits != -1) {
		/* Format CIDR /width. */
		ADDS(snprintf(dst + t, size - t, "/%u", bits));
	}

	return (dst);

 emsgsize:
	errno = EMSGSIZE;
	return (NULL);
}
 
static char *
inet_cidr_ntop_ipv6(const u_char *src, int bits, char *dst, size_t size) {
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	struct { int base, len; } best, cur;
	u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i, l;
	size_t t;

	if ((bits < -1) || (bits > 128)) {
		errno = EINVAL;
		return (NULL);
	}

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	best.len = 0;
	cur.base = -1;
	cur.len = 0;
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
	t = 0;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base)
				ADDC(':');
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			ADDC(':');
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 && (best.len == 6 ||
		    (best.len == 7 && words[7] != 0x0001) ||
		    (best.len == 5 && words[5] == 0xffff))) {
			size_t n;

			if (src[15] || bits == -1 || bits > 120)
				n = 4;
			else if (src[14] || bits > 112)
				n = 3;
			else
				n = 2;
			ADDS(decoct(src+12, n, dst + t, size - t));
			break;
		}
		ADDS(snprintf(dst + t, size - t, "%x", words[i]));
	}

	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) == 
	    (NS_IN6ADDRSZ / NS_INT16SZ)) {
		ADDC(':');
	}

	if (bits != -1)
		ADDS(snprintf(dst + t, size - t, "/%u", bits));

	return dst;
emsgsize:
	errno = EMSGSIZE;
	return (NULL);
}

/*! \file */
