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
 * @(#) $Header: /cvsroot/src/sys/boot/Attic/net.c,v 1.1 1993/10/12 06:02:29 glass Exp $ (LBL)
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <errno.h>

#include "netboot.h"
#include "config.h"
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
	register struct udphdr *up;
	register u_char *ea;

	if (debug)
	    printf("sendudp: called\n");
	up = ((struct udphdr *)buf) - 1;
	ip = ((struct ip *)up) - 1;
	len += sizeof(*ip) + sizeof(*up);

	bzero(ip, sizeof(*ip) + sizeof(*up));

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_len = htons(len);
	ip->ip_p = IPPROTO_UDP;
	ip->ip_ttl = IP_TTL;
	ip->ip_src.s_addr = d->myip;
	ip->ip_dst.s_addr = d->destip;
	ip->ip_sum = in_cksum(ip, sizeof(*ip));

	up->uh_sport = htons(d->myport);
	up->uh_dport = htons(d->destport);
	up->uh_ulen = htons(len - sizeof(*ip));
	up->uh_sum = in_cksum(up, len - sizeof(*ip));

	if (ip->ip_dst.s_addr == INADDR_BROADCAST || ip->ip_src.s_addr == 0 ||
	    mask == 0 || SAMENET(ip->ip_src.s_addr, ip->ip_dst.s_addr, mask))
		ea = arpwhohas(d, ip->ip_dst.s_addr);
	else
		ea = arpwhohas(d, gateip);

	cc = sendether(d, ip, len, ea, ETHERTYPE_IP);

	if (cc < 0)
		return (cc);
	if (cc != len)
		panic("sendudp: bad write (%d != %d)", cc, len);

	return (cc - (sizeof(*ip) + sizeof(*up)));
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

	if (debug)
	    printf("sendether: called\n");
	eh = ((struct ether_header *)buf) - 1;
	len += ETHER_SIZE;

	MACPY(d->myea, eh->ether_shost);
	MACPY(dea, eh->ether_dhost);

	eh->ether_type = etype;
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
	register struct udphdr *up;

	if (debug)
	    printf("checkudp: called\n");
	eh = pkt;
	ip = (struct ip *)(eh + 1);
	up = (struct udphdr *)(ip + 1);

	/* Must be to us */
	if (bcmp(d->myea, eh->ether_dhost, 6) != 0 &&
	    bcmp(bcea, eh->ether_dhost, 6) != 0)
		return (NULL);

	/* And ip */
	if (eh->ether_type != ETHERTYPE_IP)
		return (NULL);

	/* Check ip header */
	if (ip->ip_v != IPVERSION || ip->ip_p != IPPROTO_UDP)
		return (NULL);

	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(*ip) || in_cksum(ip, hlen) != 0)
		return (NULL);
	NTOHS(ip->ip_len);
	if (*lenp - sizeof(*eh) < ip->ip_len)
		return (NULL);
	if (d->myip && ip->ip_dst.s_addr != d->myip)
		return (NULL);

	/* If there were ip options, make them go away */
	if (hlen != sizeof(*ip)) {
		bcopy(((u_char *)ip) + hlen, up,
		    *lenp - (sizeof(*eh) + hlen));
		ip->ip_len = sizeof(*ip);
		*lenp -= hlen - sizeof(*ip);
	}
	if (ntohs(up->uh_dport) != d->myport)
		return (NULL);

	if (up->uh_sum) {
		len = ntohs(up->uh_ulen);
		if (len > RECV_SIZE - (sizeof(*eh) + sizeof(*ip))) {
			printf("checkudp: huge packet, udp len %lu\n", len);
			return (NULL);
		}
		if (in_cksum(up, len) != 0)
			return (NULL);
	}
	NTOHS(up->uh_dport);
	NTOHS(up->uh_sport);
	NTOHS(up->uh_ulen);
	if (up->uh_ulen < sizeof(*up))
		return (NULL);
	*lenp -= sizeof(*eh) + sizeof(*ip) + sizeof(*up);
	return (up + 1);
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

	if (debug)
	    printf("sendrecv: called\n");
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
    val = desc->io_netif->netif_get(desc, pkt, len, timeout);
    if (debug)
	printf("ethernet_get: received %d\n", val);
    return val;
}
int ethernet_put(desc, pkt, len)
     struct iodesc *desc;
     void *pkt;
     int len;
{
    int count, val;

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
    if (!desc->io_netif)
	panic("ethernet_put: no netif_put support");
    val = desc->io_netif->netif_put(desc, pkt, len);
    return val;
}
