/* inet.c

   Subroutines to manipulate internet addresses in a safely portable
   way... */

/*
 * Copyright (c) 2004-2005 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1995-2003 by Internet Software Consortium
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
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: inet.c,v 1.1.1.3 2005/08/11 16:54:27 drochner Exp $ Copyright (c) 2004-2005 Internet Systems Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

/* Return just the network number of an internet address... */

struct iaddr subnet_number (addr, mask)
	struct iaddr addr;
	struct iaddr mask;
{
	int i;
	struct iaddr rv;

	if (addr.len > sizeof(addr.iabuf))
		log_fatal("subnet_number():%s:%d: Invalid addr length.", MDL);
	if (addr.len != mask.len)
		log_fatal("subnet_number():%s:%d: Addr/mask length mismatch.",
			  MDL);

	rv.len = 0;

	/* Both addresses must have the same length... */
	if (addr.len != mask.len)
		return rv;

	rv.len = addr.len;
	for (i = 0; i < rv.len; i++)
		rv.iabuf [i] = addr.iabuf [i] & mask.iabuf [i];
	return rv;
}

/* Combine a network number and a integer to produce an internet address.
   This won't work for subnets with more than 32 bits of host address, but
   maybe this isn't a problem. */

struct iaddr ip_addr (subnet, mask, host_address)
	struct iaddr subnet;
	struct iaddr mask;
	u_int32_t host_address;
{
	int i, j, k;
	u_int32_t swaddr;
	struct iaddr rv;
	unsigned char habuf [sizeof swaddr];

	if (subnet.len > sizeof(subnet.iabuf))
		log_fatal("ip_addr():%s:%d: Invalid addr length.", MDL);
	if (subnet.len != mask.len)
		log_fatal("ip_addr():%s:%d: Addr/mask length mismatch.",
			  MDL);

	swaddr = htonl (host_address);
	memcpy (habuf, &swaddr, sizeof swaddr);

	/* Combine the subnet address and the host address.   If
	   the host address is bigger than can fit in the subnet,
	   return a zero-length iaddr structure. */
	rv = subnet;
	j = rv.len - sizeof habuf;
	for (i = sizeof habuf - 1; i >= 0; i--) {
		if (mask.iabuf [i + j]) {
			if (habuf [i] > (mask.iabuf [i + j] ^ 0xFF)) {
				rv.len = 0;
				return rv;
			}
			for (k = i - 1; k >= 0; k--) {
				if (habuf [k]) {
					rv.len = 0;
					return rv;
				}
			}
			rv.iabuf [i + j] |= habuf [i];
			break;
		} else
			rv.iabuf [i + j] = habuf [i];
	}
		
	return rv;
}

/* Given a subnet number and netmask, return the address on that subnet
   for which the host portion of the address is all ones (the standard
   broadcast address). */

struct iaddr broadcast_addr (subnet, mask)
	struct iaddr subnet;
	struct iaddr mask;
{
	int i;
	struct iaddr rv;

	if (subnet.len > sizeof(subnet.iabuf))
		log_fatal("broadcast_addr():%s:%d: Invalid addr length.", MDL);
	if (subnet.len != mask.len)
		log_fatal("broadcast_addr():%s:%d: Addr/mask length mismatch.",
			  MDL);

	if (subnet.len != mask.len) {
		rv.len = 0;
		return rv;
	}

	for (i = 0; i < subnet.len; i++) {
		rv.iabuf [i] = subnet.iabuf [i] | (~mask.iabuf [i] & 255);
	}
	rv.len = subnet.len;

	return rv;
}

u_int32_t host_addr (addr, mask)
	struct iaddr addr;
	struct iaddr mask;
{
	int i;
	u_int32_t swaddr;
	struct iaddr rv;

	if (addr.len > sizeof(addr.iabuf))
		log_fatal("host_addr():%s:%d: Invalid addr length.", MDL);
	if (addr.len != mask.len)
		log_fatal("host_addr():%s:%d: Addr/mask length mismatch.",
			  MDL);

	rv.len = 0;

	/* Mask out the network bits... */
	rv.len = addr.len;
	for (i = 0; i < rv.len; i++)
		rv.iabuf [i] = addr.iabuf [i] & ~mask.iabuf [i];

	/* Copy out up to 32 bits... */
	memcpy (&swaddr, &rv.iabuf [rv.len - sizeof swaddr], sizeof swaddr);

	/* Swap it and return it. */
	return ntohl (swaddr);
}

int addr_eq (addr1, addr2)
	struct iaddr addr1, addr2;
{
	if (addr1.len > sizeof(addr1.iabuf))
		log_fatal("addr_eq():%s:%d: Invalid addr length.", MDL);

	if (addr1.len != addr2.len)
		return 0;
	return memcmp (addr1.iabuf, addr2.iabuf, addr1.len) == 0;
}

char *piaddr (addr)
	struct iaddr addr;
{
	static char pbuf [4 * 16];
	char *s = pbuf;
	int i;

	if (addr.len > sizeof(addr.iabuf))
		log_fatal("piaddr():%s:%d: Address length too long.", MDL);

	if (addr.len == 0) {
		strcpy (s, "<null address>");
	}
	for (i = 0; i < addr.len; i++) {
		sprintf (s, "%s%d", i ? "." : "", addr.iabuf [i]);
		s += strlen (s);
	}
	return pbuf;
}

char *piaddrmask (struct iaddr addr, struct iaddr mask,
		  const char *file, int line)
{
	char *s, tbuf[sizeof("255.255.255.255/32")];
	int mw;
	unsigned i, oct, bit;

	if (addr.len != 4)
		log_fatal("piaddrmask():%s:%d: Address length %d invalid",
			  MDL, addr.len);
	if (addr.len != mask.len)
		log_fatal("piaddrmask():%s:%d: Address and mask size mismatch",
			  MDL);

	/* Determine netmask width in bits. */
	for (mw = 32; mw > 0; ) {
		oct = (mw - 1) / 8;
		bit = 0x80 >> ((mw - 1) % 8);
		if (!mask.iabuf [oct])
			mw -= 8;
		else if (mask.iabuf [oct] & bit)
			break;
		else
			mw--;
	}

	s = tbuf;
	for (i = 0 ; i <= oct ; i++) {
		sprintf(s, "%s%d", i ? "." : "", addr.iabuf[i]);
		s += strlen(s);
	}
	sprintf(s, "/%d", mw);

	s = dmalloc (strlen(tbuf) + 1, file, line);
	if (!s)
		return s;
	strcpy(s, tbuf);
	return s;
}

