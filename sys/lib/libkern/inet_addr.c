/*	$NetBSD: inet_addr.c,v 1.4.2.2 2001/04/21 17:46:33 bouyer Exp $	*/

/* Copyright (c) 1996 by Internet Software Consortium.
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

/*
 * Derived from:
 * src/lib/libc/net/inet_pton.c
 */

#include <sys/types.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#else
#include <lib/libkern/libkern.h>
#endif

/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 */
#if !defined(_KERNEL) && !defined(_STANDALONE)
u_long /* XXX to comply with the prototype in <arpa/inet.h> */
#else
u_int32_t
#endif
inet_addr(src)
	const char *src;
{
	u_int32_t val;
	int base, n;
	unsigned char c;
	u_int parts[4];
	u_int *pp = parts;

	c = *src;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit(c))
			return (0);
		val = 0; base = 10;
		if (c == '0') {
			c = *++src;
			if (toupper(c) == 'X')
				base = 16, c = *++src;
			else
				base = 8;
		}
		for (;;) {
			if (isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++src;
			} else if (base == 16 && isxdigit(toupper(c))) {
				val = (val << 4) |
					(toupper(c) + 10 - 'A');
				c = *++src;
			} else
			break;
		}
		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++src;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && !isspace(c))
		return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		return (0);		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}

	val = htonl(val);
	return (val);
}
