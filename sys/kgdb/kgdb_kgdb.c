/*
 * Copyright (C) 1993-1996 Wolfgang Solfrank.
 * Copyright (C) 1993-1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>

#include <machine/stdarg.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>

#include <kgdb/kgdb.h>
#include "debuggers.h"
#include <machine/kgdb.h>

int kgdbpanic = 0;

#ifdef	KGDBUSER
char kgdbuser = 0;	/* allows debugging of user processes by KGDB when set */
#endif

static struct kgdb_if kgdb_if;
#ifdef	KGDBTEST
static struct kgdb_if new_if;
static int kgdb_test = 0;
#endif

static u_char *kgdbaddr __P((u_char *, int *, void **));
static void init __P((struct kgdb_if *));
static void peekmem __P((struct kgdb_if *, char *, char *, long));
static void pokemem __P((struct kgdb_if *, char *, char *, long));
static void getpkt __P((struct kgdb_if *, char *, int *));
static void putpkt __P((struct kgdb_if *, char *, int));
static int check_kgdb __P((struct kgdb_if *, struct in_addr *, u_short, u_short, char *, int));
static int connectkgdb __P((struct kgdb_if *, char *));

void
kgdb_init()
{
	kgdb_if.connect = KGDB_DEF;
	kgdbinit();
	if (kgdbifinit(&kgdb_if, 0) < 0
	    || !(kgdb_if.flags&KGDB_MYHW)) {
		/* Interface not found, drop KGDB */
		printf("KGDB: No interface found!\n");
		kgdb_if.connect = KGDB_NOIF;
		boothowto &= ~RB_KDB;
	}
}

void
kgdb_connect(when)
{
	boothowto |= RB_KDB;
	if (when == 0)
		printf("waiting for remote GDB\n");
	kgdb_trap();
#ifdef	KGDBTEST
	new_if.connect = KGDB_ALL;
	if (kgdbifinit(&new_if, 1) < 0
	    || !(new_if.flags&KGDB_MYHW)) {
		/* Interface not found, no test */
		return;
	}
	init(&new_if);
	putpkt(&new_if, "s", 1);
	for (kgdb_test = 1; kgdb_test;) {
		static char buf[512];
		int plen;
		
		getpkt(&new_if, buf, &plen);
		if (!plen)
			continue;
		putpkt(&new_if, "eunknown command", 16);
	}
#endif
}

void
kgdb_panic()
{
	if (kgdb_if.connect == KGDB_NOIF)
		return;
	kgdbpanic = 1;
	kgdb_trap();
}

int
kgdbcmds()
{
	static char buf[512];
	char *cp;
	int plen;

	if (!(kgdb_if.flags&KGDB_MYHW))	/* no interface */
		return 0;
	init(&kgdb_if);
	if (kgdbpanic > 1) {
		kgdb_if.leave(&kgdb_if);
		return 0;
	}
	putpkt(&kgdb_if, "s", 1);
	while (1) {
		getpkt(&kgdb_if, buf, &plen);
		if (!plen) {
			if (kgdbpanic && kgdb_poll()) {
				kgdb_if.leave(&kgdb_if);
				return 0;
			} else
				continue;
		} else
			kgdbpanic = 0;
		switch (*buf) {
		default:
			putpkt(&kgdb_if, "eunknown command", 16);
			break;
		case 'O':
			/* This is an allowed reconnect, ack it */
			putpkt(&kgdb_if, "s", 1);
			break;
		case 'R':
			peekmem(&kgdb_if, buf, (char *)kgdbregs, sizeof kgdbregs);
			break;
		case 'W':
			if (plen != sizeof kgdbregs + 1) {
				putpkt(&kgdb_if, "einvalid register size", 22);
				break;
			}
			pokemem(&kgdb_if, buf + 1, (char *)kgdbregs, sizeof kgdbregs);
			break;
		case 'M':
			{
				char *addr;
				long len;
				
				plen--;
				if (!(cp = kgdbaddr(buf + 1, &plen, (void **)&addr))
				    || !kgdbaddr(cp, &plen, (void **)&len)) {
					putpkt(&kgdb_if, "einvalid peek format", 20);
					break;
				}
				peekmem(&kgdb_if, buf, addr, len);
				break;
			}
		case 'N':
			{
				char *addr;
				int len;
				
				plen--;
				if (!(cp = kgdbaddr(buf + 1, &plen, (void **)&addr))
				    || !(cp = kgdbaddr(cp, &plen, (void **)&len))
				    || plen < len) {
					putpkt(&kgdb_if, "einvalid poke format", 20);
					break;
				}
				pokemem(&kgdb_if, cp, addr, len);
				break;
			}
		case 'S':
			kgdb_if.leave(&kgdb_if);
			return 1;
		case 'X':
			putpkt(&kgdb_if, "ok",2);
#ifdef	KGDBUSER
			kgdbuser = 0;
#endif
			kgdb_if.connect = KGDB_DEF; /* ??? */
			kgdb_if.leave(&kgdb_if);
			return 2;
		case 'C':
			kgdb_if.leave(&kgdb_if);
			return 0;
		}
	}
}

static u_char *
kgdbaddr(cp, pl, dp)
	u_char *cp;
	int *pl;
	void **dp;
{
	/* Assume that sizeof(void*) <= sizeof(long) */
	long l;
	int i;
	
	if ((*pl -= sizeof(void *)) < 0)
		return 0;
	for (i = sizeof(void *), l = 0; --i >= 0;) {
		l <<= 8;
		l |= *cp++;
	}
	*dp = (void *)l;
	return cp;
}

static void
peekmem(ifp, buf, addr, len)
	struct kgdb_if *ifp;
	char *buf, *addr;
	long len;
{
	char *cp;
	int l;
	
	cp = buf;
	*cp++ = 'p';
	for (l = len; --l >= 0;)
		*cp++ = kgdbfbyte(addr++);
	putpkt(ifp, buf, len + 1);
}

static void
pokemem(ifp, cp, addr, len)
	struct kgdb_if *ifp;
	char *cp, *addr;
	long len;
{
	int c;

	while (--len >= 0)
		kgdbsbyte(addr++, *cp++);
	putpkt(ifp, "ok", 2);
}

__inline static u_long
getnl(s)
	u_char *s;
{
	return  (*s << 24)
	       |(s[1] << 16)
	       |(s[2] << 8)
	       |s[3];
}
__inline static u_int
getns(s)
	u_char *s;
{
	return  (*s << 8)
	       |s[1];
}

__inline static void
setnl(s, l)
	u_char *s;
	long l;
{
	*s++ = l >> 24;
	*s++ = l >> 16;
	*s++ = l >> 8;
	*s = l;
}

__inline static void
setns(s, l)
	u_char *s;
{
	*s++ = l >> 8;
	*s = l;
}

static u_short
cksum(s, cp, l)
	u_long s;
	u_char *cp;
	int l;
{
	for (; (l -= 2) >= 0; cp += 2)
		s += (*cp << 8) + cp[1];
	if (l == -1)
		s += *cp << 8;
	while (s&0xffff0000)
		s = (s&0xffff) + (s >> 16);
	return s == 0xffff ? 0 : s;
}

static int
assemble(ifp, buf)
	struct kgdb_if *ifp;
	u_char *buf;
{
	struct ip *ip, iph;
	int off, len, i;
	u_char *cp, *ecp;
	
	ip = (struct ip *)buf;
	kgdbcopy(ip, &iph, sizeof iph);
	iph.ip_hl = 5;
	iph.ip_tos = 0;
	iph.ip_len = 0;
	iph.ip_off = 0;
	iph.ip_ttl = 0;
	iph.ip_sum = 0;
	if (ifp->asslen) {
		if (kgdbcmp(&iph, ifp->ass, sizeof iph)) {
			/*
			 * different packet
			 * decide whether to keep the old
			 * or start a new one
			 */
			i = getns(&ip->ip_id)^getns(&((struct ip *)ifp->ass)->ip_id);
			i ^= (i >> 2)^(i >> 4)^(i >> 8)^(i >> 12);
			if (i&1)
				/* keep the old */
				return 0;
			ifp->asslen = 0;
		}
	}
	if (!ifp->asslen) {
		kgdbzero(ifp->assbit, sizeof ifp->assbit);
		kgdbcopy(&iph, ifp->ass, sizeof iph);
	}
	off = getns(&ip->ip_off);
	len = ((off&IP_OFFMASK) << 3) + getns(&ip->ip_len) - ip->ip_hl * 4;
	if (ifp->asslen < len)
		ifp->asslen = len;
	if (ifp->asslen + sizeof *ip > sizeof ifp->ass) {
		/* packet too long */
		ifp->asslen = 0;
		return 0;
	}
	if (!(off&IP_MF)) {
		off &= IP_OFFMASK;
		cp = ifp->assbit + (off >> 3);
		for (i = off & 7; i < 8; *cp |= 1 << i++);
		for (; cp < ifp->assbit + sizeof ifp->assbit; *cp++ = -1);
	} else {
		off &= IP_OFFMASK;
		cp = ifp->assbit + (off >> 3);
		ecp = ifp->assbit + (len >> 6);
		if (cp == ecp)
			for (i = off & 7; i <= (len >> 3)&7; *cp |= 1 << i++);
		else {
			for (i = off & 7; i < 8; *cp |= 1 << i++);
			for (; ++cp < ecp; *cp = -1);
			for (i = 0; i < ((len >> 3)&7); *cp |= 1 << i++);
		}
	}
	kgdbcopy(buf + ip->ip_hl * 4,
		 ifp->ass + sizeof *ip + (off << 3),
		 len - (off << 3));
	for (cp = ifp->assbit; cp < ifp->assbit + sizeof ifp->assbit;)
		if (*cp++ != (u_char)-1)
			/* not complete */
			return 0;
	ip = (struct ip *)ifp->ass;
	setns(&ip->ip_len, sizeof *ip + ifp->asslen);
	/* complete */
	return 1;
}

static char *
inpkt(ifp, ibuf, poll)
	struct kgdb_if *ifp;
	char *ibuf;
	int poll;
{
	int cnt = 1000000;
	int l, ul;
	struct ether_header *eh;
	struct ether_arp *ah;
	struct ip *ip;
	struct udphdr *udp;
	struct ipovly ipo;
	
	while (1) {
		l = ifp->receive(ifp, ibuf, poll != 0);
		if (!l) {
			if (poll == 1 || (poll == 2 && --cnt <= 0))
				break;
			else
				continue;
		}
		eh = (struct ether_header *)ibuf;
		switch (getns(&eh->ether_type)) {
		case ETHERTYPE_ARP:
			ah = (struct ether_arp *)(ibuf + 14);
			if (   getns(&ah->arp_hrd) != ARPHRD_ETHER
			    || getns(&ah->arp_pro) != ETHERTYPE_IP
			    || ah->arp_hln != 6
			    || ah->arp_pln != 4)
				/* unsupported arp packet */
				break;
			switch (getns(&ah->arp_op)) {
			case ARPOP_REQUEST:
				if (   (ifp->flags&KGDB_MYIP)
				    && !kgdbcmp(ah->arp_tpa,
						ifp->myinetaddr,
						sizeof ifp->myinetaddr)) {
					/* someone requested my address */
					kgdbcopy(eh->ether_shost,
						 eh->ether_dhost,
						 sizeof eh->ether_dhost);
					kgdbcopy(ifp->myenetaddr,
						 eh->ether_shost,
						 sizeof eh->ether_shost);
					setns(&ah->arp_op, ARPOP_REPLY);
					kgdbcopy(ah->arp_sha,
						 ah->arp_tha,
						 sizeof ah->arp_tha);
					kgdbcopy(ah->arp_spa,
						 ah->arp_tpa,
						 sizeof ah->arp_tpa);
					kgdbcopy(ifp->myenetaddr,
						 ah->arp_sha,
						 sizeof ah->arp_sha);
					kgdbcopy(ifp->myinetaddr,
						 ah->arp_spa,
						 sizeof ah->arp_spa);
					ifp->send(ifp, ibuf, 74);
					continue;
				}
				break;
			default:
				break;
			}
			break;
		case ETHERTYPE_IP:
			if (kgdbcmp(eh->ether_dhost,
				    ifp->myenetaddr,
				    sizeof ifp->myenetaddr))
				/* not only for us */
				break;
			ip = (struct ip *)(ibuf + 14);
			if (   ip->ip_v != IPVERSION
			    || ip->ip_hl < 5
			    || getns(&ip->ip_len) + 14 > l)
				/* invalid packet */
				break;
			if (cksum(0, ip, ip->ip_hl * 4))
				/* wrong checksum */
				break;
			if (ip->ip_p != IPPROTO_UDP)
				break;
			if (getns(&ip->ip_off)&~IP_DF) {
				if (!assemble(ifp, ip))
					break;
				ip = (struct ip *)ifp->ass;
				ifp->asslen = 0;
			}
			udp = (struct udphdr *)((char *)ip + ip->ip_hl * 4);
			ul = getns(&ip->ip_len) - ip->ip_hl * 4;
			if (getns(&udp->uh_ulen) != ul)
				/* invalid UDP packet length */
				break;
			kgdbcopy(ip, &ipo, sizeof ipo);
			kgdbzero(ipo.ih_x1, sizeof ipo.ih_x1);
			ipo.ih_len = udp->uh_ulen;
			if (   udp->uh_sum
			    && cksum(cksum(0, &ipo, sizeof ipo), udp, ul))
				/* wrong checksum */
				break;
			if (!(ifp->flags&KGDB_MYIP)) {
				if (   getns(&udp->uh_sport) == 67
				    && getns(&udp->uh_dport) == 68
				    && *(char *)(udp + 1) == 2) {
					/* this is a BOOTP reply to our ethernet address */
					/* should check a bit more?		XXX */
					kgdbcopy(&ip->ip_dst,
						 ifp->myinetaddr,
						 sizeof ifp->myinetaddr);
					ifp->flags |= KGDB_MYIP;
				}
				/* give caller a chance to resend his request */
				return 0;
			}
			if (   kgdbcmp(&ip->ip_dst, ifp->myinetaddr, sizeof ifp->myinetaddr)
			    || getns(&udp->uh_sport) != KGDBPORT
			    || getns(&udp->uh_dport) != KGDBPORT)
				break;
			/* so now it's a UDP packet for the debugger */
			{
				/* Check for reconnect packet */
				u_char *p;
				
				p = (u_char *)(udp + 1);
				if (!getnl(p) && p[6] == 'O') {
					l = getns(p + 4);
					if (l <= ul - sizeof *udp - 6
					    && check_kgdb(ifp, &ip->ip_src, udp->uh_sport,
							  udp->uh_dport, p + 6, l)) {
						kgdbcopy(&ip->ip_src,
							 ifp->hisinetaddr,
							 sizeof ifp->hisinetaddr);
						kgdbcopy(eh->ether_shost,
							 ifp->hisenetaddr,
							 sizeof ifp->hisenetaddr);
						ifp->flags |= KGDB_HISHW|KGDB_HISIP;
						return p;
					}
				}
			}
			if ((ifp->flags&KGDB_HISIP)
			    && kgdbcmp(&ip->ip_src,
				       ifp->hisinetaddr, sizeof ifp->hisinetaddr))
				/* It's a packet from someone else */
				break;
			if (!(ifp->flags&KGDB_HISIP)) {
				ifp->flags |= KGDB_HISIP;
				kgdbcopy(&ip->ip_src,
					 ifp->hisinetaddr, sizeof ifp->hisinetaddr);
			}
			if (!(ifp->flags&KGDB_HISHW)) {
				ifp->flags |= KGDB_HISHW;
				kgdbcopy(eh->ether_shost,
					 ifp->hisenetaddr, sizeof ifp->hisenetaddr);
			}
			return (char *)(udp + 1);
		default:
			/* unknown type */
			break;
		}
		if (l)
			kgdbgotpkt(ifp, ibuf, l);
	}
	return 0;
}

static short kgdb_id = 0;

static void
outpkt(ifp, in, l, srcport, dstport)
	struct kgdb_if *ifp;
	char *in;
	int l;
	int srcport, dstport;
{
	u_char *sp;
	struct ether_header *eh;
	struct ip *ip;
	struct udphdr *udp;
	u_char *cp;
	char obuf[ETHERMTU+14];
	struct ipovly ipo;
	int i, off;
	
	kgdbzero(obuf, sizeof obuf);
	eh = (struct ether_header *)obuf;
	if (!(ifp->flags&KGDB_HISHW))
		for (cp = eh->ether_dhost; cp < eh->ether_dhost + sizeof eh->ether_dhost;)
			*cp++ = -1;
	else
		kgdbcopy(ifp->hisenetaddr, eh->ether_dhost, sizeof eh->ether_dhost);
	kgdbcopy(ifp->myenetaddr, eh->ether_shost, sizeof eh->ether_shost);
	setns(&eh->ether_type, ETHERTYPE_IP);
	ip = (struct ip *)(obuf + 14);
	ip->ip_v = IPVERSION;
	ip->ip_hl = 5;
	setns(&ip->ip_id, kgdb_id++);
	ip->ip_ttl = 255;
	ip->ip_p = IPPROTO_UDP;
	kgdbcopy(ifp->myinetaddr, &ip->ip_src, sizeof ip->ip_src);
	kgdbcopy(ifp->hisinetaddr, &ip->ip_dst, sizeof ip->ip_dst);
	udp = (struct udphdr *)(ip + 1);
	setns(&udp->uh_sport, srcport);
	setns(&udp->uh_dport, dstport);
	setns(&udp->uh_ulen, l + sizeof *udp);
	kgdbcopy(ip, &ipo, sizeof ipo);
	kgdbzero(ipo.ih_x1, sizeof ipo.ih_x1);
	ipo.ih_len = udp->uh_ulen;
	setns(&udp->uh_sum,
	      ~cksum(cksum(cksum(0, &ipo, sizeof ipo),
			   udp, sizeof *udp),
		     in, l));
	for (cp = (u_char *)(udp + 1), l += sizeof *udp, off = 0;
	     l > 0;
	     l -= i, in += i, off += i, cp = (u_char *)udp) {
		i = l > ifp->mtu - sizeof *ip ? ((ifp->mtu - sizeof *ip)&~7) : l;
		kgdbcopy(in, cp, i);
		setns(&ip->ip_len, i + sizeof *ip);
		setns(&ip->ip_off, (l > i ? IP_MF : 0)|(off >> 3));
		ip->ip_sum = 0;
		setns(&ip->ip_sum, ~cksum(0, ip, sizeof *ip));
		if (i + sizeof *ip < ETHERMIN)
			i = ETHERMIN - sizeof *ip;
		ifp->send(ifp, obuf, i + sizeof *ip + 14);
	}
}

static void
init(ifp)
	struct kgdb_if *ifp;
{
	u_char *cp;
	struct ether_header *eh;
	struct ip *ip;
	struct udphdr *udp;
	u_char buf[ETHERMTU+14];
	struct ipovly ipo;
	int secs = 0;
	
	ifp->start(ifp);
#ifdef	__notyet__
	if (!(ifp->flags&KGDB_MYIP))
		kgdbinet(ifp);
#endif
	if (ifp->flags&KGDB_MYIP)
		return;
	
	while (!(ifp->flags&KGDB_MYIP)) {
		kgdbzero(buf, sizeof buf);
		cp = buf;
		*cp++ = 1;		/* BOOTP_REQUEST */
		*cp++ = 1;		/* Ethernet hardware */
		*cp++ = 6;		/* length of address */
		setnl(++cp, 0x12345678); /* some random number? */
		setns(cp + 4, secs++);
		kgdbcopy(ifp->myenetaddr, cp + 24, sizeof ifp->myenetaddr);
		outpkt(ifp, buf, 300, 68, 67);
		inpkt(ifp, buf, 2);
		if (kgdbpanic && kgdb_poll()) {
			kgdbpanic++;
			return;
		}
	}
	cp = ifp->myinetaddr;
	printf("My IP address is %d.%d.%d.%d\n", cp[0], cp[1], cp[2], cp[3]);
}

static int
chksum(p, l)
	char *p;
{
	char csum;
	
	for (csum = 0; --l >= 0; csum += *p++);
	return csum;
}

static void
getpkt(ifp, buf, lp)
	struct kgdb_if *ifp;
	char *buf;
	int *lp;
{
	char *got;
	int l;
	char ibuf[ETHERMTU+14];
	
	*lp = 0;
	while (1) {
		if (!(got = inpkt(ifp, ibuf, kgdbpanic != 0))) {
			*lp = 0;
			return;
		}
		if (   ifp->seq == getnl(got)
		    && got[6] >= 'A'
		    && got[6] <= 'Z'
		    && (l = getns(got + 4))
		    && (got[6] == 'O' || chksum(got + 6,l) == got[l + 6])) {
			kgdbcopy(got + 6, buf, *lp = l);
			return;
		}
		if (ifp->pktlen
		    && ((ifp->flags&(KGDB_MYIP|KGDB_HISIP|KGDB_CONNECTED))
			== (KGDB_MYIP|KGDB_HISIP|KGDB_CONNECTED)))
			outpkt(ifp, ifp->pkt, ifp->pktlen, KGDBPORT, KGDBPORT);
	}
}

static void
putpkt(ifp, buf, l)
	struct kgdb_if *ifp;
	char *buf;
	int l;
{
	setnl(ifp->pkt, ifp->seq++);
	setns(ifp->pkt + 4, l);
	kgdbcopy(buf, ifp->pkt + 6, l);
	ifp->pkt[l + 6] = chksum(ifp->pkt + 6, l);
	ifp->pktlen = l + 7;
	if ((ifp->flags&(KGDB_MYIP|KGDB_HISIP|KGDB_CONNECTED))
	    != (KGDB_MYIP|KGDB_HISIP|KGDB_CONNECTED))
		return;
	outpkt(ifp, ifp->pkt, ifp->pktlen, KGDBPORT, KGDBPORT);
}

static __inline int
maskcmp(in, mask, match)
	u_char *in, *mask, *match;
{
	int i;
	
	for (i = 4; --i >= 0;)
		if ((*in++&*mask++) != *match++)
			return 0;
	return 1;
}

static int
check_kgdb(ifp, shost, sport, dport, p, l)
	struct kgdb_if *ifp;
	struct in_addr *shost;
	u_short sport, dport;
	char *p;
	int l;
{
	u_char hisenet[6];
	u_char hisinet[4];
	char save;
	struct kgdb_allow *kap;
	
	if (chksum(p, l) != p[l])
		return 0;
	p[l] = 0;
	switch (ifp->connect) {
	default:
		return 0;
	case KGDB_SAME:
		if (kgdbcmp(shost, ifp->hisinetaddr, sizeof ifp->hisinetaddr))
			return 0;
		if (getns(&sport) != KGDBPORT || getns(&dport) != KGDBPORT)
			return 0;
		bzero(&hisinet, sizeof hisinet);
		break;
	case KGDB_ALL:
		for (kap = kgdballow; kap < kgdballow + kgdbcount; kap++) {
			if (maskcmp(shost, kap->mask, kap->match))
				break;
		}
		if (kap >= kgdballow + kgdbcount)
			return 0;
		if (getns(&sport) != KGDBPORT || getns(&dport) != KGDBPORT)
			return 0;
		kgdbcopy(ifp->hisenetaddr, hisenet, sizeof hisenet);
		kgdbcopy(ifp->hisinetaddr, hisinet, sizeof hisinet);
		save = ifp->flags;
		kgdbcopy(shost, ifp->hisinetaddr, sizeof ifp->hisinetaddr);
		ifp->flags &= ~KGDB_HISHW;
		ifp->flags |= KGDB_HISIP;
		break;
	}
	if (connectkgdb(ifp, p) < 0) {
		if (ifp->connect == KGDB_ALL) {
			kgdbcopy(hisenet, ifp->hisenetaddr, sizeof ifp->hisenetaddr);
			kgdbcopy(hisinet, ifp->hisinetaddr, sizeof ifp->hisinetaddr);
			kgdb_if.flags = save;
		}
		return 0;
	}
	return 1;
}

/*
 * Should check whether packet came across the correct interface
 */
int
checkkgdb(shost, sport, dport, m, off, len)
	struct in_addr *shost;
	u_short sport, dport;
	struct mbuf *m;
{
	char *p;
	int l;
	char ibuf[ETHERMTU+50];
	
	m_copydata(m, off, len, ibuf);
	p = ibuf;
	if (getnl(p) || p[6] != 'O')
		return 0;
	l = getns(p + 4);
	if (l > len - 6 || !check_kgdb(&kgdb_if, shost, sport, dport, p + 6, l))
		return 0;
	kgdb_connect(1);
	return 1;
}

static int
connectkgdb(ifp, buf)
	struct kgdb_if *ifp;
	char *buf;
{
	char *cp;
	u_char *ip;
	
	if (*buf != 'O')
		return -1;
	for (cp = buf; *cp  && *cp != ':'; cp++);
	if (!*cp)
		return -1;
	*cp++ = 0;
	ip = ifp->hisinetaddr;
	printf("debugged by %s@%s (%d.%d.%d.%d)\n",buf + 1,cp,
	    ip[0], ip[1], ip[2], ip[3]);
	ifp->connect = KGDB_SAME; /* if someone once connected, he may do so again */
	ifp->flags |= KGDB_CONNECTED;
	ifp->seq = 0;
	ifp->pktlen = 0;
	return 0;
}
