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

#include <sys/param.h>
#include <sys/socket.h>

#include <string.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include "stand.h"
#include "net.h"
#include "netif.h"

n_long	myip;

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

#ifdef NET_DEBUG
 	if (debug) {
		printf("sendudp: d=%x called.\n", (u_int)d);
		if (d) {
			printf("saddr: %s:%d",
				intoa(d->myip), d->myport);
			printf(" daddr: %s:%d\n",
				intoa(d->destip), d->destport);
		}
	}
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

#ifdef NET_DEBUG
	if (debug)
	    printf("checkudp: called\n");
#endif
	eh = pkt;
	ip = (struct ip *)(eh + 1);
	uh = (struct udphdr *)(ip + 1);

	/* Must be to us */
	if (bcmp(d->myea, eh->ether_dhost, 6) != 0 &&	/* by byte */
	    bcmp(bcea, eh->ether_dhost, 6) != 0) {	/* by byte */
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: not ours. myea=%s bcea=%s\n",
				ether_sprintf(d->myea), ether_sprintf(bcea));
#endif
		return (NULL);
	    }

	/* And ip */
	if (ntohs(eh->ether_type) != ETHERTYPE_IP) {
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: not IP. ether_type=%x\n", eh->ether_type);
#endif
		return (NULL);
	}

	/* Check ip header */
	if (ip->ip_v != IPVERSION || ip->ip_p != IPPROTO_UDP) {	/* half char */
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: IP version or not UDP. ip_v=%d ip_p=%d\n", ip->ip_v, ip->ip_p);
#endif
		return (NULL);
	}

	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(*ip) || in_cksum(ip, hlen) != 0) {
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: short hdr or bad cksum.\n");
#endif
		return (NULL);
	}
	NTOHS(ip->ip_len);
	if (*lenp - sizeof(*eh) < ip->ip_len) {
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: bad length %d < %d.\n",
				*lenp - sizeof(*eh), ip->ip_len);
#endif
		return (NULL);
	}
	if (d->myip && ntohl(ip->ip_dst.s_addr) != d->myip) {
#ifdef NET_DEBUG
		if (debug) {
			printf("checkudp: bad saddr %s != ",
				intoa(d->myip));
			printf("%s\n",
				intoa(ntohl(ip->ip_dst.s_addr)));
		}
#endif
		return (NULL);
	}

	/* If there were ip options, make them go away */
	if (hlen != sizeof(*ip)) {
		bcopy(((u_char *)ip) + hlen, uh,
		    *lenp - (sizeof(*eh) + hlen));
		ip->ip_len = sizeof(*ip);
		*lenp -= hlen - sizeof(*ip);
	}
	if (ntohs(uh->uh_dport) != d->myport) {
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: bad dport %d != %d\n",
				d->myport, ntohs(uh->uh_dport));
#endif
		return (NULL);
	}

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
#ifdef NET_DEBUG
			if (debug)
				printf("checkudp: bad cksum\n");
#endif
			*ip = tip;
			return (NULL);
		}
		*ip = tip;
	}
	NTOHS(uh->uh_dport);
	NTOHS(uh->uh_sport);
	NTOHS(uh->uh_ulen);
	if (uh->uh_ulen < sizeof(*uh)) {
#ifdef NET_DEBUG
		if (debug)
			printf("checkudp: bad udp len %d < %d\n",
				uh->uh_ulen, sizeof(*uh));
#endif
		return (NULL);
	}
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

#ifdef NET_DEBUG
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

		cc = netif_get(d, rbuf, rsize, tleft);
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

/* Similar to inet_ntoa() */
char *
intoa(addr)
	n_long addr;
{
	register char *cp;
	register u_int byte;
	register int n;
	static char buf[17];	/* strlen(".255.255.255.255") + 1 */

	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return (cp+1);
}

static char *
number(s, n)
	char *s;
	int *n;
{
	for (*n = 0; isdigit(*s); s++)
		*n = (*n * 10) + *s - '0';
	return s;
}

n_long
ip_convertaddr(p)
	char *p;
{
#define IP_ANYADDR	0
	n_long addr = 0, n;

	if (p == (char *)0 || *p == '\0')
		return IP_ANYADDR;
	p = number(p, &n);
	addr |= (n << 24) & 0xff000000;
	if (*p == '\0' || *p++ != '.')
		return IP_ANYADDR;
	p = number(p, &n);
	addr |= (n << 16) & 0xff0000;
	if (*p == '\0' || *p++ != '.')
		return IP_ANYADDR;
	p = number(p, &n);
	addr |= (n << 8) & 0xff00;
	if (*p == '\0' || *p++ != '.')
		return IP_ANYADDR;
	p = number(p, &n);
	addr |= n & 0xff;
	if (*p != '\0')
		return IP_ANYADDR;

	return ntohl(addr);
}
