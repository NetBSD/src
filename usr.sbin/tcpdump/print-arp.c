/*	$NetBSD: print-arp.c,v 1.4 1997/10/03 19:54:58 christos Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char rcsid[] =
    "@(#) Header: print-arp.c,v 1.43 97/06/15 13:20:27 leres Exp  (LBL)";
#else
__RCSID("$NetBSD: print-arp.c,v 1.4 1997/10/03 19:54:58 christos Exp $");
#endif
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#include <netinet/in.h>
#ifdef __NetBSD__
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <netinet/if_inarp.h>
#else
#include <netinet/if_ether.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"
#include "extract.h"			/* must come after interface.h */

/* Compatibility */
#ifndef REVARP_REQUEST
#define REVARP_REQUEST		3
#endif
#ifndef REVARP_REPLY
#define REVARP_REPLY		4
#endif

static u_char ezero[6];

void
arp_print(register const u_char *bp, u_int length, u_int caplen)
{
	register const APTYPE *ap;
	register const struct ether_header *eh;
	register u_short pro, hrd, op;

	ap = (APTYPE *)bp;
	if ((u_char *)(ap + 1) > snapend) {
		printf("[|arp]");
		return;
	}
	if (length < sizeof(struct arphdr)) {
		(void)printf("truncated-arp");
		default_print((u_char *)ap, length);
		return;
	}

	pro = EXTRACT_16BITS(&PRO(ap));
	hrd = EXTRACT_16BITS(&HRD(ap));
	op = EXTRACT_16BITS(&OP(ap));

	if ((pro != ETHERTYPE_IP && pro != ETHERTYPE_TRAIL)
#ifndef __NetBSD__
	    || ap->arp_hln != sizeof(SHA(ap))
	    || ap->arp_pln != sizeof(SPA(ap))
#endif
	    ) {
		(void)printf("arp-#%d for proto #%d (%d) hardware #%d (%d)",
				op, pro, PLN(ap), hrd, HLN(ap));
		return;
	}
	if (pro == ETHERTYPE_TRAIL)
		(void)printf("trailer-");
	eh = (struct ether_header *)packetp;
	switch (op) {

	case ARPOP_REQUEST:
		(void)printf("arp who-has %s", ipaddr_string(TPA(ap)));
		if (memcmp((char *)ezero, (char *)THA(ap), HLN(ap)) != 0)
			(void)printf(" (%s)",
			    linkaddr_string(THA(ap), HLN(ap)));
		(void)printf(" tell %s", ipaddr_string(SPA(ap)));

#ifndef __NetBSD__
		if (memcmp((char *)ESRC(eh), (char *)SHA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(SHA(ap)));
#endif
		break;

	case ARPOP_REPLY:
		(void)printf("arp reply %s", ipaddr_string(SPA(ap)));

#ifndef __NetBSD__
		if (memcmp((char *)ESRC(eh), (char *)SHA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(SHA(ap)));
#endif

		(void)printf(" is-at %s", linkaddr_string(SHA(ap), HLN(ap)));

#ifndef __NetBSD__
		(void)printf(" is-at %s", etheraddr_string(SHA(ap)));
		if (memcmp((char *)EDST(eh), (char *)THA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(THA(ap)));
#endif
		break;

	case REVARP_REQUEST:
		(void)printf("rarp who-is %s tell %s",
			linkaddr_string(THA(ap), HLN(ap)),
			linkaddr_string(SHA(ap), HLN(ap)));
		break;

	case REVARP_REPLY:
		(void)printf("rarp reply %s at %s",
			linkaddr_string(THA(ap), HLN(ap)),
			ipaddr_string(TPA(ap)));
		break;

	default:
		(void)printf("arp-#%d", op);
		default_print((u_char *)ap, caplen);
		return;
	}
	if (hrd != ARPHRD_ETHER)
		printf(" hardware #%d", hrd);
}
