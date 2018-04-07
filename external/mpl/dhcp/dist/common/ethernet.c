/*	$NetBSD: ethernet.c,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/* ethernet.c

   Packet assembly code, originally contributed by Archie Cobbs. */

/*
 * Copyright (c) 2004-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-2003 by Internet Software Consortium
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ethernet.c,v 1.2 2018/04/07 22:37:29 christos Exp $");

#include "dhcpd.h"

#if defined (PACKET_ASSEMBLY) || defined (PACKET_DECODING)
#include "includes/netinet/if_ether.h"
#endif /* PACKET_ASSEMBLY || PACKET_DECODING */

#if defined (PACKET_ASSEMBLY)
/* Assemble an hardware header... */

void assemble_ethernet_header (interface, buf, bufix, to)
	struct interface_info *interface;
	unsigned char *buf;
	unsigned *bufix;
	struct hardware *to;
{
	struct isc_ether_header eh;

	if (to && to -> hlen == 7) /* XXX */
		memcpy (eh.ether_dhost, &to -> hbuf [1],
			sizeof eh.ether_dhost);
	else
		memset (eh.ether_dhost, 0xff, sizeof (eh.ether_dhost));
	if (interface -> hw_address.hlen - 1 == sizeof (eh.ether_shost))
		memcpy (eh.ether_shost, &interface -> hw_address.hbuf [1],
			sizeof (eh.ether_shost));
	else
		memset (eh.ether_shost, 0x00, sizeof (eh.ether_shost));

	eh.ether_type = htons (ETHERTYPE_IP);

	memcpy (&buf [*bufix], &eh, ETHER_HEADER_SIZE);
	*bufix += ETHER_HEADER_SIZE;
}
#endif /* PACKET_ASSEMBLY */

#ifdef PACKET_DECODING
/* Decode a hardware header... */

ssize_t decode_ethernet_header (interface, buf, bufix, from)
     struct interface_info *interface;
     unsigned char *buf;
     unsigned bufix;
     struct hardware *from;
{
  struct isc_ether_header eh;

  memcpy (&eh, buf + bufix, ETHER_HEADER_SIZE);

#ifdef USERLAND_FILTER
  if (ntohs (eh.ether_type) != ETHERTYPE_IP)
	  return -1;
#endif
  memcpy (&from -> hbuf [1], eh.ether_shost, sizeof (eh.ether_shost));
  from -> hbuf [0] = ARPHRD_ETHER;
  from -> hlen = (sizeof eh.ether_shost) + 1;

  return ETHER_HEADER_SIZE;
}
#endif /* PACKET_DECODING */
