/*	$NetBSD: fddi.c,v 1.2.2.2 2018/04/16 01:59:46 pgoyette Exp $	*/

/* fddi.c

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
__RCSID("$NetBSD: fddi.c,v 1.2.2.2 2018/04/16 01:59:46 pgoyette Exp $");

#include "dhcpd.h"

#if defined (DEC_FDDI)
#include <netinet/if_fddi.h>
#include <net/if_llc.h>

#if defined (PACKET_ASSEMBLY) || defined (PACKET_DECODING)
#include "includes/netinet/if_ether.h"
#endif /* PACKET_ASSEMBLY || PACKET_DECODING */

#if defined (PACKET_ASSEMBLY)
/* Assemble an hardware header... */

void assemble_fddi_header (interface, buf, bufix, to)
	struct interface_info *interface;
	unsigned char *buf;
	unsigned *bufix;
	struct hardware *to;
{
	struct fddi_header   fh;
	struct llc     lh;

	if (to && to -> hlen == 7)
		memcpy (fh.fddi_dhost, &to -> hbuf [1],
			sizeof (fh.fddi_dhost));
	memcpy (fh.fddi_shost,
		&interface -> hw_address.hbuf [1], sizeof (fh.fddi_shost));
	fh.fddi_fc = FDDIFC_LLC_ASYNC;
	memcpy (&buf [*bufix], &fh, sizeof fh);
	*bufix += sizeof fh;

	lh.llc_dsap = LLC_SNAP_LSAP;
	lh.llc_ssap = LLC_SNAP_LSAP;
	lh.llc_un.type_snap.control = LLC_UI;
	lh.llc_un.type_snap.ether_type = htons (ETHERTYPE_IP);
	memcpy (&buf [*bufix], &lh, LLC_SNAP_LEN);
	*bufix += LLC_SNAP_LEN;
}
#endif /* PACKET_ASSEMBLY */

#ifdef PACKET_DECODING
/* Decode a hardware header... */

ssize_t decode_fddi_header (interface, buf, bufix, from)
     struct interface_info *interface;
     unsigned char *buf;
     unsigned bufix;
     struct hardware *from;
{
	struct fddi_header   fh;
	struct llc     lh;
	
	from -> hbuf [0] = HTYPE_FDDI;
	memcpy (&from -> hbuf [1], fh.fddi_shost, sizeof fh.fddi_shost);
	return FDDI_HEADER_SIZE + LLC_SNAP_LEN;
}
#endif /* PACKET_DECODING */
#endif /* DEC_FDDI */
