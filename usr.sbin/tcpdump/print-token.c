/*	$NetBSD: print-token.c,v 1.3 1999/09/04 03:36:42 itojun Exp $	*/

/*
 * Copyright (c) 1991, 1992, 1993, 1994, 1995, 1996
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
#if 0
static  char rcsid[] =
	"@(#)$Header: /cvsroot/src/usr.sbin/tcpdump/Attic/print-token.c,v 1.3 1999/09/04 03:36:42 itojun Exp $ (LBL)";
#else
#include <sys/cdefs.h>
__RCSID("$NetBSD: print-token.c,v 1.3 1999/09/04 03:36:42 itojun Exp $");
#endif
#endif

#ifdef HAVE_TOKEN
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
#include <net/if_token.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <ctype.h>
#include <netdb.h>
#include <pcap.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"

#define TOKEN_HDRLEN (sizeof(struct token_header))

/* Extract src, dst addresses */
static inline void
extract_token_addrs(const struct token_header *trp, char *fsrc, char *fdst)
{
	memcpy(fdst, (char *)trp->token_dhost, 6);
	memcpy(fsrc, (char *)trp->token_shost, 6);
}

/*
 * Print the TR MAC header
 */
static inline void
token_print(register const struct token_header *trp, register u_int length,
	   register const u_char *fsrc, register const u_char *fdst)
{
	char *srcname, *dstname;

	srcname = etheraddr_string(fsrc);
	dstname = etheraddr_string(fdst);

	if (vflag)
		(void) printf("%02x %02x %s %s %d: ",
		       trp->token_ac,
		       trp->token_fc,
		       srcname, dstname,
		       length);
	else
		printf("%s %s %d: ", srcname, dstname, length);
}

/*
 * This is the top level routine of the printer.  'sp' is the points
 * to the TR header of the packet, 'tvp' is the timestamp,
 * 'length' is the length of the packet off the wire, and 'caplen'
 * is the number of bytes actually captured.
 */
void
token_if_print(u_char *pcap, const struct pcap_pkthdr *h,
	      register const u_char *p)
{
	u_int caplen = h->caplen;
	u_int length = h->len;
	const struct token_header *trp = (struct token_header *)p;
	extern u_short extracted_ethertype;
	struct ether_header ehdr;

	ts_print(&h->ts);

	if (caplen < TOKEN_HDRLEN) {
		printf("[|tr]");
		goto out;
	}
	/*
	 * Get the TR addresses into a canonical form
	 */
	extract_token_addrs(trp, (char*)ESRC(&ehdr), (char*)EDST(&ehdr));
	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	snapend = p + caplen;
	/*
	 * Actually, the only printer that uses packetp is print-bootp.c,
	 * and it assumes that packetp points to an Ethernet header.  The
	 * right thing to do is to fix print-bootp.c to know which link
	 * type is in use when it excavates. XXX
	 */
	packetp = (u_char *)&ehdr;

	if (eflag)
		token_print(trp, length, ESRC(&ehdr), EDST(&ehdr));

	/* Skip over TR MAC header */

	length -= TOKEN_HDRLEN;
	p += TOKEN_HDRLEN;
	caplen -= TOKEN_HDRLEN;

	if (trp->token_shost[0] & TOKEN_RI_PRESENT) {
		u_int riflen;
		struct token_rif	*rif;

		rif = TOKEN_RIF(trp);
		riflen = (ntohs(rif->tr_rcf) & TOKEN_RCF_LEN_MASK) >> 8;
/*
 * XXX if (vflag && eflag) print RIF ???
 */
		length -= riflen;
		p += riflen;
		caplen -= riflen;
	}

	/* Frame Control field determines interpretation of packet */
	extracted_ethertype = 0;
	/* Try to print the LLC-layer header & higher layers */
	if (llc_print(p, length, caplen, ESRC(&ehdr), EDST(&ehdr)) == 0) {
		/*
		 * Some kinds of LLC packet we cannot
		 * handle intelligently
		 */
		if (!eflag)
			token_print(trp, length, ESRC(&ehdr), EDST(&ehdr));
		if (extracted_ethertype) {
			printf("(LLC %s) ",
			etherproto_string(htons(extracted_ethertype)));
		}
		if (!xflag && !qflag)
			default_print(p, caplen);
	}
out:
	putchar('\n');
}
#else
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include "interface.h"
void
token_if_print(u_char *pcap, const struct pcap_pkthdr *h,
	      register const u_char *p)
{

	error("not configured for token-ring");
	/* NOTREACHED */
}
#endif
