/*	$NetBSD: print-ppp.c,v 1.7 1999/07/02 11:31:35 itojun Exp $	*/

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
__RCSID("$NetBSD: print-ppp.c,v 1.7 1999/07/02 11:31:35 itojun Exp $");
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
#define PPP_HDRLEN 4

/* Standard PPP printer */
void
ppp_if_print(u_char *user, const struct pcap_pkthdr *h,
	     register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	const struct ip *ip;
	u_int proto;

	ts_print(&h->ts);

	if (caplen < PPP_HDRLEN) {
		printf("[|ppp]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	proto = ntohs(*(u_short *)&p[2]);
	packetp = p;
	snapend = p + caplen;

	if (eflag)
		printf("%c %4d %02x %04x: ", p[0] ? 'O' : 'I', length,
		       p[1], ntohs(*(u_short *)&p[2]));

	length -= PPP_HDRLEN;
	ip = (struct ip *)(p + PPP_HDRLEN);
	switch (proto) {
	case ETHERTYPE_IP:
	case PPP_IP:
		ip_print((const u_char *)ip, length);
		break;
#ifdef INET6
	case ETHERTYPE_IPV6:
		ip6_print((const u_char *)ip, length);
		break;
#endif
	}
	if (xflag)
		default_print((const u_char *)ip, caplen - PPP_HDRLEN);
out:
	putchar('\n');
}

/* proto type to string mapping */
static struct tok ptype2str[] = {
	{ PPP_IP,	"IP" },
	{ PPP_OSI,	"OSI" },
	{ PPP_NS,	"NS" },
	{ PPP_DECNET,	"DEC" },
	{ PPP_APPLE,	"AT" },
	{ PPP_IPX,	"IPX" },
	{ PPP_VJC,	"VJC" },
	{ PPP_VJNC,	"VJNC" },
	{ PPP_BRPDU,	"BRPDU" },
	{ PPP_STII,	"STII" },
	{ PPP_VINES,	"VINES" },
	{ PPP_IPV6,	"IPv6" },

	{ PPP_HELLO,	"HELLO" },
	{ PPP_LUXCOM,	"LUXCOM" },
	{ PPP_SNS,	"SNS" },

	{ PPP_IPCP,	"IPCP" },
	{ PPP_OSICP,	"OSICP" },
	{ PPP_NSCP,	"NSCP" },
	{ PPP_DECNETCP,	"DECCP" },
	{ PPP_APPLECP,	"ATCP" },
	{ PPP_IPXCP,	"IPXCP" },
	{ PPP_STIICP,	"STIICP" },
	{ PPP_VINESCP,	"VINESCP" },

	{ PPP_LCP,	"LCP" },
	{ PPP_PAP,	"PAP" },
	{ PPP_LQM,	"LQM" },
	{ PPP_CBCP,	"CBCP" },
	{ PPP_CHAP,	"CHAP" },

	{ 0,		NULL }
};

#define PPP_BSDI_HDRLEN 24

/* BSD/OS specific PPP printer */
void
ppp_bsdos_if_print(u_char *user, const struct pcap_pkthdr *h,
	     register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	register int hdrlength;
	u_short ptype;

	ts_print(&h->ts);

	if (caplen < PPP_BSDI_HDRLEN) {
		printf("[|ppp]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;
	hdrlength = 0;

	if (p[0] == PPP_ADDRESS && p[1] == PPP_CONTROL) {
		if (eflag) 
			printf("%02x %02x ", p[0], p[1]);
		p += 2;
		hdrlength = 2;
	}

	if (eflag) 
		printf("%d ", length);
	/* Retrieve the protocol type */
	if (*p & 01) {
		/* Compressed protocol field */
		ptype = *p;
		if (eflag) 
			printf("%02x ", ptype);
		p++;
		hdrlength += 1;
	} else {
		/* Un-compressed protocol field */
		ptype = ntohs(*(u_short *)p);
		if (eflag) 
			printf("%04x ", ptype);
		p += 2;
		hdrlength += 2;
	}
  
	length -= hdrlength;

	if (ptype == PPP_IP)
		ip_print(p, length);
	else
		printf("%s ", tok2str(ptype2str, "proto-#%d", ptype));

	if (xflag)
		default_print((const u_char *)p, caplen - hdrlength);
out:
	putchar('\n');
}

/*
 * NetBSD-specific PPP printers.  Handles multiple PPP encaps, and
 * Cisco frames.
 */

void	ppp_netbsd_common_print(const u_char *, u_int, u_int, u_short);

#define	PPP_NETBSD_SERIAL_HDRLEN	4

void
ppp_netbsd_serial_if_print(u_char *user, const struct pcap_pkthdr *h,
		register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	u_short ptype;
	u_char addr, ctrl;

	ts_print(&h->ts);

	if (caplen < PPP_NETBSD_SERIAL_HDRLEN) {
		printf("[|ppp]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	addr = p[0];
	ctrl = p[1];

	switch (addr) {
	case PPP_ADDR_ALLSTATIONS:
		/*
		 * Regular serial PPP packet.
		 */
		if (eflag)
			printf("%02x %02x %d ", p[0], p[1], length);

		ptype = (p[2] << 8) | p[3];

		p += PPP_NETBSD_SERIAL_HDRLEN;
		length -= PPP_NETBSD_SERIAL_HDRLEN;
		caplen -= PPP_NETBSD_SERIAL_HDRLEN;

		ppp_netbsd_common_print(p, length, caplen, ptype);
		break;

	case PPP_ADDR_CISCO_MULTICAST:
	case PPP_ADDR_CISCO_UNICAST:
		if (eflag) {
			/* Control field not valid here. */
			printf("CISCO %02x %d ", p[0], length);
		}

		ptype = (p[2] << 8) | p[3];

		p += PPP_NETBSD_SERIAL_HDRLEN;
		length -= PPP_NETBSD_SERIAL_HDRLEN;
		caplen -= PPP_NETBSD_SERIAL_HDRLEN;

		switch (ptype) {
		case PPP_CISCO_KEEPALIVE:
		    {
			u_int type, par1, par2;
			u_short rel, time0, time1;

			if (caplen < CISCO_KEEP_LEN) {
				printf("[|cisco keepalive]");
				goto out;
			}

#define	GET4(p, o) \
	(p)[(o) + 0] << 24 | \
	(p)[(o) + 1] << 16 | \
	(p)[(o) + 2] << 8  | \
	(p)[(o) + 3]

			type = GET4(p, CISCO_KEEP_TYPE_OFF);
			par1 = GET4(p, CISCO_KEEP_PAR1_OFF);
			par2 = GET4(p, CISCO_KEEP_PAR2_OFF);

#undef GET4

#define	GET2(p, o) \
	(p)[(o) + 0] << 8 | \
	(p)[(o) + 1]

			rel = GET2(p, CISCO_KEEP_REL_OFF);
			time0 = GET2(p, CISCO_KEEP_TIME0_OFF);
			time1 = GET2(p, CISCO_KEEP_TIME1_OFF);

#undef GET2

			switch(type) {
			case CISCO_KEEP_TYPE_ADDR_REPLY:
				printf("CISCO ADDR REPLY ");
				break;

			case CISCO_KEEP_TYPE_KEEP_REQ:
				printf("CISCO KEEP REQ ");
				break;

			case CISCO_KEEP_TYPE_ADDR_REQ:
				printf("CISCO ADDR REQ ");
				break;

			default:
				printf("CISCO type %04x ", type);
			}

			printf("%08x %08x %08x %04x-%04x ",
			    par1, par2, rel, time0, time1);

			p += CISCO_KEEP_LEN;
			length -= CISCO_KEEP_LEN;
			caplen -= CISCO_KEEP_LEN;
			break;
		    }

		default:
			if (ether_encap_print(ptype, p, length, caplen) == 0)
				printf("proto-#%d ", ptype);
		}

		if (xflag)
			default_print((const u_char *)p, caplen);
		break;

	default:
		/* Address not known, print raw packet. */
		if (!qflag)
			default_print((const u_char *)p, caplen);
	}

 out:
	putchar('\n');
}

void
ppp_netbsd_common_print(const u_char *p, u_int length, u_int caplen,
		u_short ptype)
{

	switch (ptype) {
	case PPP_IP:
		ip_print(p, length);
		break;

	case PPP_IPX:
		ipx_print(p, length);
		break;

	default:
		printf("%s ", tok2str(ptype2str, "proto-#%d", ptype));
	}

	if (xflag)
		default_print((const u_char *)p, caplen);
}
