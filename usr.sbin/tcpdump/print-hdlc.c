/*	$NetBSD: print-hdlc.c,v 1.3 1999/09/04 03:36:41 itojun Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1993, 1994
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

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: print-hdlc.c,v 1.3 1999/09/04 03:36:41 itojun Exp $");
#endif

#ifdef HAVE_HDLC
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <pcap.h>
#include <signal.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"

/* XXX This goes somewhere else. */
#define HDLC_HDRLEN 4

void
hdlc_if_print(u_char *user, const struct pcap_pkthdr *h,
	      register const u_char *p)
{
	register int length = h->len;
	register int caplen = h->caplen;
	const struct ip *ip;

	ts_print(&h->ts);

	if (caplen < HDLC_HDRLEN) {
		printf("[|hdlc]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	if (eflag)
		printf("%02x %02x %04x: ", p[0], p[1],
		       ntohs(*(u_short *)&p[2]));

	length -= HDLC_HDRLEN;
	ip = (struct ip *)(p + HDLC_HDRLEN);
	ip_print((const u_char *)ip, length);

	if (xflag)
		default_print((const u_char *)ip, caplen - HDLC_HDRLEN);
out:
	putchar('\n');
}
#else
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include "interface.h"
void
hdlc_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	error("not configured for hdlc");
	/* NOTREACHED */
}
#endif
