/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#ifdef notdef
static const char rcsid[] = "Id: inet_net_ntop.c,v 1.1.2.1 2002/08/02 02:17:21 marka Exp ";
#else
__RCSID("$NetBSD: inet_net_ntop.c,v 1.5 2026/04/08 14:12:06 christos Exp $");
#endif
#endif

#include "port_before.h"

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "port_after.h"

#ifdef __weak_alias
__weak_alias(inet_net_ntop,_inet_net_ntop)
#endif

static char *	inet_net_ntop_ipv4(const u_char *src, int bits,
					char *dst, size_t size);
static char *	inet_net_ntop_ipv6(const u_char *src, int bits,
					char *dst, size_t size);

/*
 * char *
 * inet_net_ntop(af, src, bits, dst, size)
 *	convert network number from network to presentation format.
 *	generates CIDR style result always.
 * return:
 *	pointer to dst, or NULL if an error occurred (check errno).
 * author:
 *	Paul Vixie (ISC), July 1996
 */
char *
inet_net_ntop(int af, const void *src, int bits, char *dst, size_t size)
{
	switch (af) {
	case AF_INET:
		return (inet_net_ntop_ipv4(src, bits, dst, size));
	case AF_INET6:
		return (inet_net_ntop_ipv6(src, bits, dst, size));
	default:
		errno = EAFNOSUPPORT;
		return (NULL);
	}
}

/*
 * static char *
 * inet_net_ntop_ipv4(src, bits, dst, size)
 *	convert IPv4 network number from network to presentation format.
 *	generates CIDR style result always.
 * return:
 *	pointer to dst, or NULL if an error occurred (check errno).
 * note:
 *	network byte order assumed.  this means 192.5.5.240/28 has
 *	0b11110000 in its fourth octet.
 * author:
 *	Paul Vixie (ISC), July 1996
 */
static char *
inet_net_ntop_ipv4(const u_char *src, int bits, char *dst, size_t size)
{
	size_t t;
	u_int m;
	int b, l;

	if (bits < 0 || bits > 32) {
		errno = EINVAL;
		return (NULL);
	}


	t = 0;
	if (bits == 0)
		ADDC('0');

	/* Format whole octets. */
	t = 0;
	for (b = bits / 8; b > 0; b--) {
		ADDS(snprintf(dst + t, size - t, "%u", *src++));
		if (b > 1)
			ADDC('.');
	}

	/* Format partial octet. */
	b = bits % 8;
	if (b > 0) {
		if (t)
			ADDC('.');
		m = ((1 << b) - 1) << (8 - b);
		ADDS(snprintf(dst + t, size - t, "%u", *src & m));
	}

	/* Format CIDR /width. */
	ADDS(snprintf(dst + t, size - t, "/%u", bits));
	return dst;

 emsgsize:
	errno = EMSGSIZE;
	return (NULL);
}

/*
 * static char *
 * inet_net_ntop_ipv6(src, bits, fakebits, dst, size)
 *	convert IPv6 network number from network to presentation format.
 *	generates CIDR style result always. Picks the shortest representation
 *	unless the IP is really IPv4.
 *	always prints specified number of bits (bits).
 * return:
 *	pointer to dst, or NULL if an error occurred (check errno).
 * note:
 *	network byte order assumed.  this means 192.5.5.240/28 has
 *	0x11110000 in its fourth octet.
 * author:
 *	Vadim Kogan (UCB), June 2001
 *  Original version (IPv4) by Paul Vixie (ISC), July 1996
 */

static char *
inet_net_ntop_ipv6(const u_char *src, int bits, char *dst, size_t size)
{
	u_int	m;
	int	b, t, l;
	size_t	p;
	size_t	zero_s, zero_l, tmp_zero_s, tmp_zero_l;
	size_t	i;
	int	is_ipv4 = 0;
	unsigned char inbuf[16];
	size_t	words;
	u_char	*s;

	if (bits < 0 || bits > 128) {
		errno = EINVAL;
		return (NULL);
	}

	t = 0;
	if (bits == 0) {
		ADDC(':');
		ADDC(':');
	} else {
		/* Copy src to private buffer.  Zero host part. */	
		p = (bits + 7) / 8;
		memcpy(inbuf, src, p);
		memset(inbuf + p, 0, 16 - p);
		b = bits % 8;
		if (b != 0) {
			m = ~0u << (8 - b);
			inbuf[p-1] &= m;
		}

		s = inbuf;

		/* how many words need to be displayed in output */
		words = (bits + 15) / 16;
		if (words == 1)
			words = 2;
		
		/* Find the longest substring of zero's */
		zero_s = zero_l = tmp_zero_s = tmp_zero_l = 0;
		for (i = 0; i < (words * 2); i += 2) {
			if ((s[i] | s[i+1]) == 0) {
				if (tmp_zero_l == 0)
					tmp_zero_s = i / 2;
				tmp_zero_l++;
			} else {
				if (tmp_zero_l && zero_l < tmp_zero_l) {
					zero_s = tmp_zero_s;
					zero_l = tmp_zero_l;
					tmp_zero_l = 0;
				}
			}
		}

		if (tmp_zero_l && zero_l < tmp_zero_l) {
			zero_s = tmp_zero_s;
			zero_l = tmp_zero_l;
		}

		if (zero_l != words && zero_s == 0 && ((zero_l == 6) ||
		    ((zero_l == 5 && s[10] == 0xff && s[11] == 0xff) ||
		    ((zero_l == 7 && s[14] != 0 && s[15] != 1)))))
			is_ipv4 = 1;

		/* Format whole words. */
		for (p = 0; p < words; p++) {
			if (zero_l != 0 && p >= zero_s && p < zero_s + zero_l) {
				/* Time to skip some zeros */
				if (p == zero_s)
					ADDC(':');
				if (p == words - 1)
					ADDC(':');
				s++;
				s++;
				continue;
			}

			if (is_ipv4 && p > 5 ) {
				ADDS(snprintf(dst + t, size - t, "%c%u",
				    (p == 6) ? ':' : '.', *s++));
				/* we can potentially drop the last octet */
				if (p != 7 || bits > 120) {
					ADDS(snprintf(dst + t, size - t, ".%u",
					    *s++));
				}
			} else {
				if (t)
					ADDC(':');
				ADDS(snprintf(dst + t, size - t, "%x",
				    *s * 256 + s[1]));
				s += 2;
			}
		}
	}
	/* Format CIDR /width. */
	ADDS(snprintf(dst + t, size - t, "/%u", bits));
	return (dst);

emsgsize:
	errno = EMSGSIZE;
	return (NULL);
}
