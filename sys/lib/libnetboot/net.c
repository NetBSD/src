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
 * from @(#) Header: net.c,v 1.9 93/08/06 19:32:15 leres Exp  (LBL)
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <errno.h>

#include "salibc.h"

#include "netboot.h"
#include "netif.h"

/* Caller must leave room for ethernet, ip and udp headers in front!! */
int
sendudp(d, buf, len)
	register struct iodesc *d;
	register void *buf;
	register int len;
{
	register int cc;
	register struct ip *ip;
	register struct udpiphdr *ui;
	register struct udphdr *uh;
	register u_char *ea;
	struct ip tip;

#ifdef DEBUG
 	if (debug)
 	    printf("sendudp: called\n");
#endif
	uh = ((struct udphdr *)buf) - 1;
	ip = ((struct ip *)uh) - 1;
	len += sizeof(*ip) + sizeof(*uh);

	bzero(ip, sizeof(*ip) + sizeof(*uh));

	ip->ip_v = IPVERSION;			/* half-char */
	ip->ip_hl = sizeof(*ip) >> 2;		/* half-char */
	ip->ip_len = htons(len);
	ip->ip_p = IPPROTO_UDP;			/* char */
	ip->ip_ttl = IP_TTL;			/* char */
	ip->ip_src.s_addr = htonl(d->myip);
	ip->ip_dst.s_addr = htonl(d->destip);
	ip->ip_sum = in_cksum(ip, sizeof(*ip));	 /* short, but special */

	uh->uh_sport = htons(d->myport);
	uh->uh_dport = htons(d->destport);
	uh->uh_ulen = htons(len - sizeof(*ip));

	/* Calculate checksum (must save and restore ip header) */
	tip = *ip;
	ui = (struct udpiphdr *)ip;
	ui->ui_next = 0;
	ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_len = uh->uh_ulen;
	uh->uh_sum = in_cksum(ui, len);
	*ip = tip;

	if (ip->ip_dst.s_addr == INADDR_BROADCAST || ip->ip_src.s_addr == 0 ||
	    mask == 0 || SAMENET(ip->ip_src.s_addr, ip->ip_dst.s_addr, mask))
		ea = arpwhohas(d, ip->ip_dst.s_addr);
	else
		ea = arpwhohas(d, htonl(gateip));

	cc = sendether(d, ip, len, ea, ETHERTYPE_IP);
	if (cc < 0)
		return (cc);
	if (cc != len)
		panic("sendudp: bad write (%d != %d)", cc, len);
	return (cc - (sizeof(*ip) + sizeof(*uh)));
}

/* Caller must leave room for ethernet header in front!! */
int
sendether(d, buf, len, dea, etype)
	register struct iodesc *d;
	register void *buf;
	register int len;
	register u_char *dea;
	register int etype;
{
	register struct ether_header *eh;

#ifdef DEBUG
 	if (debug)
 	    printf("sendether: called\n");
#endif
	eh = ((struct ether_header *)buf) - 1;
	len += ETHER_SIZE;

	MACPY(d->myea, eh->ether_shost);		/* by byte */
	MACPY(dea, eh->ether_dhost);			/* by byte */
	eh->ether_type = htons(etype);
	return (ethernet_put(d, eh, len) - ETHER_SIZE);
}

/* Check that packet is a valid udp packet for us */
void *
checkudp(d, pkt, lenp)
	register struct iodesc *d;
	register void *pkt;
	register int *lenp;
{
	register int hlen, len;
	register struct ether_header *eh;
	register struct ip *ip;
	register struct udphdr *uh;
	register struct udpiphdr *ui;
	struct ip tip;

#ifdef DEBUG
	if (debug)
	    printf("checkudp: called\n");
#endif
	eh = pkt;
	ip = (struct ip *)(eh + 1);
	uh = (struct udphdr *)(ip + 1);

	/* Must be to us */
	if (bcmp(d->myea, eh->ether_dhost, 6) != 0 &&	/* by byte */
	    bcmp(bcea, eh->ether_dhost, 6) != 0)	/* by byte */
		return (NULL);

	/* And ip */
	if (ntohs(eh->ether_type) != ETHERTYPE_IP)
		return (NULL);

	/* Check ip header */
	if (ip->ip_v != IPVERSION || ip->ip_p != IPPROTO_UDP)	/* half char */
		return (NULL);

	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(*ip) || in_cksum(ip, hlen) != 0)
		return (NULL);
	NTOHS(ip->ip_len);
	if (*lenp - sizeof(*eh) < ip->ip_len)
		return (NULL);
	if (d->myip && ntohl(ip->ip_dst.s_addr) != d->myip)
		return (NULL);

	/* If there were ip options, make them go away */
	if (hlen != sizeof(*ip)) {
		bcopy(((u_char *)ip) + hlen, uh,
		    *lenp - (sizeof(*eh) + hlen));
		ip->ip_len = sizeof(*ip);
		*lenp -= hlen - sizeof(*ip);
	}
	if (ntohs(uh->uh_dport) != d->myport)
		return (NULL);

	if (uh->uh_sum) {
		len = ntohs(uh->uh_ulen);
		if (len > RECV_SIZE - (sizeof(*eh) + sizeof(*ip))) {
			printf("checkudp: huge packet, udp len %d\n", len);
			return (NULL);
		}

		/* Check checksum (must save and restore ip header) */
		tip = *ip;
		ui = (struct udpiphdr *)ip;
		ui->ui_next = 0;
		ui->ui_prev = 0;
		ui->ui_x1 = 0;
		ui->ui_len = uh->uh_ulen;
		if (in_cksum(ui, len + sizeof(*ip)) != 0) {
			*ip = tip;
			return (NULL);
		}
		*ip = tip;
	}
	NTOHS(uh->uh_dport);
	NTOHS(uh->uh_sport);
	NTOHS(uh->uh_ulen);
	if (uh->uh_ulen < sizeof(*uh))
		return (NULL);
	*lenp -= sizeof(*eh) + sizeof(*ip) + sizeof(*uh);
	return (uh + 1);
}

/*
 * Send a packet and wait for a reply, with exponential backoff.
 *
 * The send routine must return the actual number of bytes written.
 *
 * The receive routine can indicate success by returning the number of
 * bytes read; it can return 0 to indicate EOF; it can return -1 with a
 * non-zero errno to indicate failure; finally, it can return -1 with a
 * zero errno to indicate it isn't done yet.
 */
int
sendrecv(d, sproc, sbuf, ssize, rproc, rbuf, rsize)
	register struct iodesc *d;
	register int (*sproc)(struct iodesc *, void *, int);
	register void *sbuf;
	register int ssize;
	register int (*rproc)(struct iodesc *, void *, int);
	register void *rbuf;
	register int rsize;
{
	register int cc;
	register time_t t, tmo, tlast, tleft;

#ifdef DEBUG
	if (debug)
	    printf("sendrecv: called\n");
#endif
	tmo = MINTMO;
	tlast = tleft = 0;
	t = getsecs();
	for (;;) {
		if (tleft <= 0) {
			cc = (*sproc)(d, sbuf, ssize);
			if (cc < ssize)
				panic("sendrecv: short write! (%d < %d)",
				    cc, ssize);

			tleft = tmo;
			tmo <<= 1;
			if (tmo > MAXTMO)
				tmo = MAXTMO;
			tlast = t;
		}

		cc = ethernet_get(d, rbuf, rsize, tleft);
		if (cc >= 0) {
			/* Got a packet, process it */
			cc = (*rproc)(d, rbuf, cc);
			/* Return on data, EOF or real error */
			if (cc >= 0 || errno != 0)
				return (cc);
		}
		/* Timed out or didn't get the packet we're waiting for */
		t = getsecs();
		tleft -= t - tlast;
		tlast = t;
	}
}

int ethernet_get(desc, pkt, len, timeout)
     struct iodesc *desc;
     void *pkt;
     int len;
     time_t timeout;
{
    int val;
    val = netif_get(desc, pkt, len, timeout);
#ifdef DEBUG
    if (debug)
	printf("ethernet_get: received %d\n", val);
#endif
    return val;
}
int ethernet_put(desc, pkt, len)
     struct iodesc *desc;
     void *pkt;
     int len;
{
    int count, val;

#ifdef DEBUG
    if (debug) {
	struct ether_header *eh;
	
	printf("ethernet_put: called\n");
	eh = pkt;
	printf("ethernet_put: ether_dhost %s\n",
	       ether_sprintf(eh->ether_dhost));
	printf("ethernet_put: ether_shost %s\n",
	       ether_sprintf(eh->ether_shost));
	printf("ethernet_put: ether_type %x\n", eh->ether_type);
    }
#endif
    val = netif_put(desc, pkt, len);
    return val;
}
