/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from @(#) Header: arp.c,v 1.5 93/07/15 05:52:26 leres Exp  (LBL)
 *     $Id: rarp.c,v 1.3 1994/08/04 19:39:37 brezak Exp $
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include <string.h>

#include "stand.h"
#include "net.h"
#include "netif.h"

static int rarpsend(struct iodesc *, void *, int);
static int rarprecv(struct iodesc *, void *, int);

/*
 * Ethernet (Reverse) Address Resolution Protocol (see RFC 903, and 826).
 */
n_long
rarp_getipaddress(sock)
	int sock;
{
	struct iodesc *d;
	register struct ether_arp *ap;
	register void *pkt;
	struct {
		u_char header[HEADER_SIZE];
		struct ether_arp wrarp;
	} wbuf;
	union {
		u_char buffer[RECV_SIZE];
		struct {
			u_char header[HEADER_SIZE];
			struct ether_arp xrrarp;
		}xrbuf;
#define rrarp  xrbuf.xrrarp
	} rbuf;

#ifdef RARP_DEBUG
 	if (debug)
		printf("rarp: socket=%d\n", sock);
#endif
	if (!(d = socktodesc(sock))) {
		printf("rarp: bad socket. %d\n", sock);
		return(INADDR_ANY);
	}
#ifdef RARP_DEBUG
 	if (debug)
		printf("rarp: d=%x\n", (u_int)d);
#endif
	ap = &wbuf.wrarp;
	pkt = &rbuf.rrarp;
	pkt -= HEADER_SIZE;

	bzero(ap, sizeof(*ap));

	ap->arp_hrd = htons(ARPHRD_ETHER);
	ap->arp_pro = htons(ETHERTYPE_IP);
	ap->arp_hln = sizeof(ap->arp_sha); /* hardware address length */
	ap->arp_pln = sizeof(ap->arp_spa); /* protocol address length */
	ap->arp_op = htons(ARPOP_REQUEST);
	bcopy(d->myea, ap->arp_sha, 6);
	bcopy(d->myea, ap->arp_tha, 6);

	if (sendrecv(d,
		     rarpsend, ap, sizeof(*ap),
		     rarprecv, pkt, RECV_SIZE) < 0) {
		printf("No response for RARP request\n");
		return(INADDR_ANY);
	}

	return(myip);
}

/*
 * Broadcast a RARP request (i.e. who knows who I am)
 */
static int
rarpsend(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
#ifdef RARP_DEBUG
 	if (debug)
 	    printf("rarpsend: called\n");
#endif
	return (sendether(d, pkt, len, bcea, ETHERTYPE_REVARP));
}

/*
 * Called when packet containing RARP is received
 */
static int
rarprecv(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
	register struct ether_header *ep;
	register struct ether_arp *ap;

#ifdef RARP_DEBUG
 	if (debug)
 	    printf("rarprecv: called\n");
#endif
	if (len < sizeof(struct ether_header) + sizeof(struct ether_arp)) {
		errno = 0;
		return (-1);
	}

	ep = (struct ether_header *)pkt;
	if (ntohs(ep->ether_type) != ETHERTYPE_REVARP) {
		errno = 0;
		return (-1);
	}

	ap = (struct ether_arp *)(ep + 1);
	if (ntohs(ap->arp_op) != ARPOP_REPLY ||
	    ntohs(ap->arp_pro) != ETHERTYPE_IP)  {
		errno = 0;
		return (-1);
	}

	if (bcmp(ap->arp_tha, d->myea, 6)) {
		errno = 0;
		return (-1);
	}

	bcopy(ap->arp_tpa, (char *)&myip, sizeof(myip));
	bcopy(ap->arp_spa, (char *)&rootip, sizeof(rootip));

	return(0);
}
