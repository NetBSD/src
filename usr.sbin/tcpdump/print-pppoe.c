/*	$NetBSD: print-pppoe.c,v 1.1 2001/04/14 12:31:34 martin Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1993, 1994, 1995, 1996, 1997
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
    "@(#) Header: print-ppp.c,v 1.26 97/06/12 14:21:29 leres Exp  (LBL)";
#else
__RCSID("$NetBSD: print-pppoe.c,v 1.1 2001/04/14 12:31:34 martin Exp $");
#endif
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#ifdef __NetBSD__
#include <net/if_ether.h>
#else
#include <netinet/if_ether.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <ctype.h>
#include <netdb.h>
#include <pcap.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"
#include "ppp.h"

/* XXX This goes somewhere else. */
#define PPPOE_HDRLEN		6
#define PPP_SHORT_HDRLEN	2

/* PPPoE printer */
void
pppoe_if_print(u_char *user, const struct pcap_pkthdr *h,
	     register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	const struct ip *ip;
	u_int proto;

	ts_print(&h->ts);

	if (caplen < PPPOE_HDRLEN+PPP_SHORT_HDRLEN) {
		printf("[|pppoe]");
		goto out;
	}

	if (eflag)
		printf("pppoe session 0x%x length %d\n",
			ntohs(*(u_short *)&p[2]),
			ntohs(*(u_short *)&p[4]));

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	proto = ntohs(*(u_short *)&p[PPPOE_HDRLEN]);
	packetp = p;
	snapend = p + caplen;

	length -= PPPOE_HDRLEN+PPP_SHORT_HDRLEN;
	ip = (struct ip *)(p + PPPOE_HDRLEN+PPP_SHORT_HDRLEN);
	switch (proto) {
	case ETHERTYPE_IP:
	case PPP_IP:
		ip_print((const u_char *)ip, length);
		break;
#ifdef INET6
	case ETHERTYPE_IPV6:
	case PPP_IPV6:
		ip6_print((const u_char *)ip, length);
		break;
#endif
	}
	if (xflag)
		default_print((const u_char *)ip, caplen - PPPOE_HDRLEN-PPP_SHORT_HDRLEN);
out:
	putchar('\n');
}
