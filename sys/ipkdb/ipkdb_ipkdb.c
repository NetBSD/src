/*	$NetBSD: ipkdb_ipkdb.c,v 1.27 2009/10/02 22:05:52 elad Exp $	*/

/*
 * Copyright (C) 1993-2000 Wolfgang Solfrank.
 * Copyright (C) 1993-2000 TooLs GmbH.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipkdb_ipkdb.c,v 1.27 2009/10/02 22:05:52 elad Exp $");

#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kauth.h>
#include <sys/cpu.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_inarp.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>

#include <machine/reg.h>

#include <ipkdb/ipkdb.h>
#include <machine/ipkdb.h>

int ipkdbpanic = 0;

#ifndef IPKDBKEY
#error You must specify the IPKDBKEY option to use IPKDB.
#else
static char ipkdbkey[] = IPKDBKEY;
#endif

static struct ipkdb_if ipkdb_if;

static kauth_listener_t ipkdb_listener;

static u_char *ipkdbaddr(u_char *, int *, void **);
static void peekmem(struct ipkdb_if *, u_char *, void *, long);
static void pokemem(struct ipkdb_if *, u_char *, void *, long);
static u_int32_t getnl(void *);
static u_int getns(void *);
static void setnl(void *, u_int32_t);
static void setns(void *, int);
static u_short cksum(u_short, void *, int);
static int assemble(struct ipkdb_if *, void *);
static char *inpkt(struct ipkdb_if *, char *, int);
static void outpkt(struct ipkdb_if *, char *, int, int, int);
static void init(struct ipkdb_if *);
static void *chksum(void *, int);
static void getpkt(struct ipkdb_if *, char *, int *);
static void putpkt(struct ipkdb_if *, const char *, int);
static int check_ipkdb(struct ipkdb_if *, struct in_addr *, char *, int);
static int connectipkdb(struct ipkdb_if *, char *, int);
static int hmac_init(void);

static int
ipkdb_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	enum kauth_system_req req;
	int result;

	req = (enum kauth_system_req)arg0;
	result = KAUTH_RESULT_DEFER;

	if ((action != KAUTH_SYSTEM_DEBUG) ||
	    (req != KAUTH_REQ_SYSTEM_DEBUG_IPKDB))
		return result;

	result = KAUTH_RESULT_ALLOW;

	return result;
}

void
ipkdb_init(void)
{
	ipkdbinit();
	if (   ipkdbifinit(&ipkdb_if) < 0
	    || !(ipkdb_if.flags&IPKDB_MYHW)
	    || !hmac_init()) {
		/* Interface not found, drop IPKDB */
		printf("IPKDB: No interface found!\n");
		boothowto &= ~RB_KDB;
	}

	ipkdb_listener = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    ipkdb_listener_cb, NULL);
}

void
ipkdb_connect(int when)
{
	boothowto |= RB_KDB;
	if (when == 0)
		printf("waiting for remote debugger\n");
	ipkdb_trap();
}

void
ipkdb_panic(void)
{
	ipkdbpanic = 1;
	ipkdb_trap();
}

/*
 * Doesn't handle overlapping regions!
 */
void
ipkdbcopy(const void *s, void *d, int n)
{
	const char *sp = s;
	char *dp = d;

	while (--n >= 0)
		*dp++ = *sp++;
}

void
ipkdbzero(void *d, int n)
{
	char *dp = d;

	while (--n >= 0)
		*dp++ = 0;
}

int
ipkdbcmp(void *s, void *d, int n)
{
	char *sp = s, *dp = d;

	while (--n >= 0)
		if (*sp++ != *dp++)
			return *--dp - *--sp;
	return 0;
}

int
ipkdbcmds(void)
{
	static char buf[512];
	char *cp;
	int plen;

	if (!(ipkdb_if.flags&IPKDB_MYHW))	/* no interface */
		return IPKDB_CMD_EXIT;
	init(&ipkdb_if);
	if (ipkdbpanic > 1) {
		ipkdb_if.leave(&ipkdb_if);
		return IPKDB_CMD_RUN;
	}
	putpkt(&ipkdb_if, "s", 1);
	while (1) {
		getpkt(&ipkdb_if, buf, &plen);
		if (!plen) {
			if (ipkdbpanic && ipkdb_poll()) {
				ipkdb_if.leave(&ipkdb_if);
				return IPKDB_CMD_RUN;
			} else
				continue;
		} else
			ipkdbpanic = 0;
		switch (*buf) {
		default:
			putpkt(&ipkdb_if, "eunknown command", 16);
			break;
		case 'O':
			/* This is an allowed reconnect, ack it */
			putpkt(&ipkdb_if, "s", 1);
			break;
		case 'R':
			peekmem(&ipkdb_if, buf, ipkdbregs, sizeof ipkdbregs);
			break;
		case 'W':
			if (plen != sizeof ipkdbregs + 1) {
				putpkt(&ipkdb_if, "einvalid register size", 22);
				break;
			}
			pokemem(&ipkdb_if, buf + 1, ipkdbregs, sizeof ipkdbregs);
			break;
		case 'M':
			{
				void *addr, *len;

				plen--;
				if (   !(cp = ipkdbaddr(buf + 1, &plen, &addr))
				    || !ipkdbaddr(cp, &plen, &len)) {
					putpkt(&ipkdb_if, "einvalid peek format", 20);
					break;
				}
				peekmem(&ipkdb_if, buf, addr, (long)len);
				break;
			}
		case 'N':
			{
				void *addr, *len;

				plen--;
				if (   !(cp = ipkdbaddr(buf + 1, &plen, &addr))
				    || !(cp = ipkdbaddr(cp, &plen, &len))
				    || plen < (long)len) {
					putpkt(&ipkdb_if, "einvalid poke format", 20);
					break;
				}
				pokemem(&ipkdb_if, cp, addr, (long)len);
				break;
			}
		case 'S':
			ipkdb_if.leave(&ipkdb_if);
			return IPKDB_CMD_STEP;
		case 'X':
			putpkt(&ipkdb_if, "ok",2);
			ipkdb_if.leave(&ipkdb_if);
			return IPKDB_CMD_EXIT;
		case 'C':
			ipkdb_if.leave(&ipkdb_if);
			return IPKDB_CMD_RUN;
		}
	}
}

static u_char *
ipkdbaddr(u_char *cp, int *pl, void **dp)
{
	/* Assume that sizeof(void *) <= sizeof(u_long) */
	u_long l;
	int i;

	if ((*pl -= sizeof *dp) < 0)
		return 0;
	for (i = sizeof *dp, l = 0; --i >= 0;) {
		l <<= 8;
		l |= *cp++;
	}
	*dp = (void *)l;
	return cp;
}

static void
peekmem(struct ipkdb_if *ifp, u_char *buf, void *addr, long len)
{
	u_char *cp, *p = addr;
	int l;

	cp = buf;
	*cp++ = 'p';
	for (l = len; --l >= 0;)
		*cp++ = ipkdbfbyte(p++);
	putpkt(ifp, buf, len + 1);
}

static void
pokemem(struct ipkdb_if *ifp, u_char *cp, void *addr, long len)
{
	u_char *p = addr;

	while (--len >= 0)
		ipkdbsbyte(p++, *cp++);
	putpkt(ifp, "ok", 2);
}

inline static u_int32_t
getnl(void *vs)
{
	u_char *s = vs;

	return (*s << 24)|(s[1] << 16)|(s[2] << 8)|s[3];
}

inline static u_int
getns(void *vs)
{
	u_char *s = vs;

	return (*s << 8)|s[1];
}

inline static void
setnl(void *vs, u_int32_t l)
{
	u_char *s = vs;

	*s++ = l >> 24;
	*s++ = l >> 16;
	*s++ = l >> 8;
	*s = l;
}

inline static void
setns(void *vs, int l)
{
	u_char *s = vs;

	*s++ = l >> 8;
	*s = l;
}

static u_short
cksum(u_short st, void *vcp, int l)
{
	u_char *cp = vcp;
	u_long s;

	for (s = st; (l -= 2) >= 0; cp += 2)
		s += (*cp << 8) + cp[1];
	if (l == -1)
		s += *cp << 8;
	while (s&0xffff0000)
		s = (s&0xffff) + (s >> 16);
	return s == 0xffff ? 0 : s;
}

static int
assemble(struct ipkdb_if *ifp, void *buf)
{
	struct ip *ip, iph;
	int off, len, i;
	u_char *cp, *ecp;

	ip = (struct ip *)buf;
	ipkdbcopy(ip, &iph, sizeof iph);
	iph.ip_hl = 5;
	iph.ip_tos = 0;
	iph.ip_len = 0;
	iph.ip_off = 0;
	iph.ip_ttl = 0;
	iph.ip_sum = 0;
	if (ifp->asslen) {
		if (ipkdbcmp(&iph, ifp->ass, sizeof iph)) {
			/*
			 * different packet
			 * decide whether to keep the old
			 * or start a new one
			 */
			i = (getns(&ip->ip_id)
			     ^ getns(&((struct ip *)ifp->ass)->ip_id));
			i ^= ((i >> 2) ^ (i >> 4) ^ (i >> 8) ^ (i >> 12));
			if (i & 1)
				/* keep the old */
				return 0;
			ifp->asslen = 0;
		}
	}
	if (!ifp->asslen) {
		ipkdbzero(ifp->assbit, sizeof ifp->assbit);
		ipkdbcopy(&iph, ifp->ass, sizeof iph);
	}
	off = getns(&ip->ip_off);
	len = ((off & IP_OFFMASK) << 3) + getns(&ip->ip_len) - ip->ip_hl * 4;
	if (ifp->asslen < len)
		ifp->asslen = len;
	if (ifp->asslen + sizeof *ip > sizeof ifp->ass) {
		/* packet too long */
		ifp->asslen = 0;
		return 0;
	}
	if (!(off & IP_MF)) {
		off &= IP_OFFMASK;
		cp = ifp->assbit + (off >> 3);
		for (i = (off & 7); i < 8; *cp |= 1 << i++);
		for (; cp < ifp->assbit + sizeof ifp->assbit; *cp++ = -1);
	} else {
		off &= IP_OFFMASK;
		cp = ifp->assbit + (off >> 3);
		ecp = ifp->assbit + (len >> 6);
		if (cp == ecp)
			for (i = (off & 7); i <= ((len >> 3) & 7);
			     *cp |= 1 << i++);
		else {
			for (i = (off & 7); i < 8; *cp |= 1 << i++);
			for (; ++cp < ecp; *cp = -1);
			for (i = 0; i < ((len >> 3) & 7); *cp |= 1 << i++);
		}
	}
	ipkdbcopy((char *)buf + ip->ip_hl * 4,
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
inpkt(struct ipkdb_if *ifp, char *ibuf, int poll)
{
	int cnt = 1000000;
	int l, ul;
	struct ether_header *eh;
	struct arphdr *ah;
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
			ah = (struct arphdr *)(ibuf + 14);
			if (   getns(&ah->ar_hrd) != ARPHRD_ETHER
			    || getns(&ah->ar_pro) != ETHERTYPE_IP
			    || ah->ar_hln != 6
			    || ah->ar_pln != 4)
				/* unsupported arp packet */
				break;
			switch (getns(&ah->ar_op)) {
			case ARPOP_REQUEST:
				if (   (ifp->flags&IPKDB_MYIP)
				    && !ipkdbcmp(ar_tpa(ah),
						 ifp->myinetaddr,
						 sizeof ifp->myinetaddr)) {
					/* someone requested my address */
					ipkdbcopy(eh->ether_shost,
						  eh->ether_dhost,
						  sizeof eh->ether_dhost);
					ipkdbcopy(ifp->myenetaddr,
						  eh->ether_shost,
						  sizeof eh->ether_shost);
					setns(&ah->ar_op, ARPOP_REPLY);
					ipkdbcopy(ar_sha(ah),
						  ar_tha(ah),
						  ah->ar_hln);
					ipkdbcopy(ar_spa(ah),
						  ar_tpa(ah),
						  ah->ar_pln);
					ipkdbcopy(ifp->myenetaddr,
						  ar_sha(ah),
						  ah->ar_hln);
					ipkdbcopy(ifp->myinetaddr,
						  ar_spa(ah),
						  ah->ar_pln);
					ifp->send(ifp, ibuf, 74);
					continue;
				}
				break;
			default:
				break;
			}
			break;
		case ETHERTYPE_IP:
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
			if (getns(&ip->ip_off) & ~IP_DF) {
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
			ipkdbcopy(ip, &ipo, sizeof ipo);
			ipkdbzero(ipo.ih_x1, sizeof ipo.ih_x1);
			ipo.ih_len = udp->uh_ulen;
			if (   udp->uh_sum
			    && cksum(cksum(0, &ipo, sizeof ipo), udp, ul))
				/* wrong checksum */
				break;
			if (!(ifp->flags & IPKDB_MYIP)) {
				if (   getns(&udp->uh_sport) == 67
				    && getns(&udp->uh_dport) == 68
				    && *(char *)(udp + 1) == 2) {
					/* this is a BOOTP reply to our ethernet address */
					/* should check a bit more?		XXX */
					char *bootp = (char *)(udp + 1);
					ipkdbcopy(bootp + 16,
						  ifp->myinetaddr,
						  sizeof ifp->myinetaddr);
					ifp->flags |= IPKDB_MYIP;
				}
				/* give caller a chance to resend his request */
				return 0;
			}
			if (   ipkdbcmp(&ip->ip_dst, ifp->myinetaddr, sizeof ifp->myinetaddr)
			    || getns(&udp->uh_dport) != IPKDBPORT)
				break;
			/* so now it's a UDP packet for the debugger */
			{
				/* Check for reconnect packet */
				u_char *p;

				p = (u_char *)(udp + 1);
				if (!getnl(p) && p[6] == 'O') {
					l = getns(p + 4);
					if (   l <= ul - sizeof *udp - 6
					    && check_ipkdb(ifp, &ip->ip_src,
							   p, l + 6)) {
						ipkdbcopy(&ip->ip_src,
							  ifp->hisinetaddr,
							  sizeof ifp->hisinetaddr);
						ipkdbcopy(eh->ether_shost,
							  ifp->hisenetaddr,
							  sizeof ifp->hisenetaddr);
						ifp->hisport = getns(&udp->uh_sport);
						ifp->flags |= IPKDB_HISHW|IPKDB_HISIP;
						return p;
					}
				}
			}
			if (   (ifp->flags&IPKDB_HISIP)
			    && ipkdbcmp(&ip->ip_src,
					ifp->hisinetaddr, sizeof ifp->hisinetaddr))
				/* It's a packet from someone else */
				break;
			if (!(ifp->flags&IPKDB_HISIP))
				break;
			return (char *)(udp + 1);
		default:
			/* unknown type */
			break;
		}
	}
	return 0;
}

static short ipkdb_ipid = 0;

static void
outpkt(struct ipkdb_if *ifp, char *in, int l, int srcport, int dstport)
{
	struct ether_header *eh;
	struct ip *ip;
	struct udphdr *udp;
	u_char *cp;
	char _obuf[ETHERMTU + 16];
#define	obuf	(_obuf + 2)		/* align ip data in packet */
	struct ipovly ipo;
	int i, off;

	ipkdbzero(_obuf, sizeof _obuf);
	eh = (struct ether_header *)obuf;
	/*
	 * If we don't have his ethernet address, or this is a bootp request,
	 * broadcast the packet.
	 */
	if (!(ifp->flags & IPKDB_HISHW)
	    || dstport == 67)
		for (cp = eh->ether_dhost;
		     cp < eh->ether_dhost + sizeof eh->ether_dhost;
		     *cp++ = -1);
	else
		ipkdbcopy(ifp->hisenetaddr, eh->ether_dhost, sizeof eh->ether_dhost);
	ipkdbcopy(ifp->myenetaddr, eh->ether_shost, sizeof eh->ether_shost);
	setns(&eh->ether_type, ETHERTYPE_IP);
	ip = (struct ip *)(obuf + 14);
	ip->ip_v = IPVERSION;
	ip->ip_hl = 5;
	setns(&ip->ip_id, ipkdb_ipid++);
	ip->ip_ttl = 255;
	ip->ip_p = IPPROTO_UDP;
	ipkdbcopy(ifp->myinetaddr, &ip->ip_src, sizeof ip->ip_src);
	/*
	 * If this is a bootp request, broadcast it.
	 */
	if (dstport == 67)
		for (cp = (u_char *)&ip->ip_dst;
		     cp < (u_char *)&ip->ip_dst + sizeof ip->ip_dst;
		     *cp++ = -1);
	else
		ipkdbcopy(ifp->hisinetaddr, &ip->ip_dst, sizeof ip->ip_dst);
	udp = (struct udphdr *)(ip + 1);
	setns(&udp->uh_sport, srcport);
	setns(&udp->uh_dport, dstport);
	setns(&udp->uh_ulen, l + sizeof *udp);
	ipkdbcopy(ip, &ipo, sizeof ipo);
	ipkdbzero(ipo.ih_x1, sizeof ipo.ih_x1);
	ipo.ih_len = udp->uh_ulen;
	setns(&udp->uh_sum,
	      ~cksum(cksum(cksum(0, &ipo, sizeof ipo),
			   udp, sizeof *udp),
		     in, l));
	for (cp = (u_char *)(udp + 1), l += sizeof *udp, off = 0;
	     l > 0;
	     l -= i, in += i, off += i, cp = (u_char *)udp) {
		i = l > ifp->mtu - sizeof *ip ? ((ifp->mtu - sizeof *ip) & ~7) : l;
		ipkdbcopy(in, cp, i);
		setns(&ip->ip_len, i + sizeof *ip);
		setns(&ip->ip_off, (l > i ? IP_MF : 0) | (off >> 3));
		ip->ip_sum = 0;
		setns(&ip->ip_sum, ~cksum(0, ip, sizeof *ip));
		if (i + sizeof *ip < ETHERMIN)
			i = ETHERMIN - sizeof *ip;
		ifp->send(ifp, obuf, i + sizeof *ip + 14);
	}
#undef	obuf
}

static void
init(struct ipkdb_if *ifp)
{
	u_char *cp;
	u_char _ibuf[ETHERMTU + 16];
#define	ibuf	(_ibuf + 2)		/* align ip data in packet */
	int secs = 0;

	ifp->start(ifp);
	if (ifp->flags & IPKDB_MYIP)
		return;

	while (!(ifp->flags & IPKDB_MYIP)) {
		ipkdbzero(_ibuf, sizeof _ibuf);
		cp = _ibuf;
		*cp++ = 1;		/* BOOTP_REQUEST */
		*cp++ = 1;		/* Ethernet hardware */
		*cp++ = 6;		/* length of address */
		setnl(++cp, 0x12345678); /* some random number? */
		setns(cp + 4, secs++);
		ipkdbcopy(ifp->myenetaddr, cp + 24, sizeof ifp->myenetaddr);
		outpkt(ifp, _ibuf, 300, 68, 67);
		inpkt(ifp, ibuf, 2);
		if (ipkdbpanic && ipkdb_poll()) {
			ipkdbpanic++;
			return;
		}
	}
	cp = ifp->myinetaddr;
	printf("My IP address is %d.%d.%d.%d\n",
	       cp[0], cp[1], cp[2], cp[3]);
#undef	ibuf
}

/* HMAC Checksumming routines, see draft-ietf-ipsec-hmac-md5-00.txt */
#define	LENCHK	16	/* Length of checksum in bytes */

/*
 * This code is based on the MD5 implementation as found in ssh.
 * It's quite a bit hacked by myself, but the original has
 * the following non-copyright comments on it:
 */
/* This code has been heavily hacked by Tatu Ylonen <ylo@cs.hut.fi> to
   make it compile on machines like Cray that don't have a 32 bit integer
   type. */
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */
static struct ipkdb_MD5Context {
	u_int buf[4];
	u_int bits[2];
	u_char in[64];
} icontext, ocontext;

static u_int32_t getNl(void *);
static void setNl(void *, u_int32_t);
static void ipkdb_MD5Transform(struct ipkdb_MD5Context *);
static void ipkdb_MD5Init(struct ipkdb_MD5Context *);
static void ipkdb_MD5Update(struct ipkdb_MD5Context *, u_char *, u_int);
static u_char *ipkdb_MD5Final(struct ipkdb_MD5Context *);

inline static u_int32_t
getNl(void *vs)
{
	u_char *s = vs;

	return *s | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
}

inline static void
setNl(void *vs, u_int32_t l)
{
	u_char *s = vs;

	*s++ = l;
	*s++ = l >> 8;
	*s++ = l >> 16;
	*s = l >> 24;
}

/* The four core functions - F1 is optimized somewhat */
/* #define F1(x, y, z)	(((x) & (y)) | (~(x) & (z))) */
#define	F1(x, y, z)	((z) ^ ((x) & ((y) ^ (z))))
#define	F2(x, y, z)	F1(z, x, y)
#define	F3(x, y, z)	((x) ^ (y) ^ (z))
#define	F4(x, y, z)	((y) ^ ((x) | ~(z)))

/* This is the central step in the MD5 algorithm. */
#define	ipkdb_MD5STEP(f, w, x, y, z, data, s) \
	((w) += f(x, y, z) + (data), \
	 (w) = ((w) << (s)) | (((w) >> (32 - s)) & 0xffffffff), \
	 (w) += (x))

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data for this routine.
 */
static void
ipkdb_MD5Transform(struct ipkdb_MD5Context *ctx)
{
	u_int a, b, c, d, i;
	u_int in[16];

	for (i = 0; i < 16; i++)
		in[i] = getNl(ctx->in + 4 * i);

	a = ctx->buf[0];
	b = ctx->buf[1];
	c = ctx->buf[2];
	d = ctx->buf[3];

	ipkdb_MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	ipkdb_MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	ipkdb_MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	ipkdb_MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	ipkdb_MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	ipkdb_MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	ipkdb_MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	ipkdb_MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	ipkdb_MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	ipkdb_MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	ipkdb_MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	ipkdb_MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	ipkdb_MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	ipkdb_MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	ipkdb_MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	ipkdb_MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	ipkdb_MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	ipkdb_MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	ipkdb_MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	ipkdb_MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	ipkdb_MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	ipkdb_MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	ipkdb_MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	ipkdb_MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	ipkdb_MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	ipkdb_MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	ipkdb_MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	ipkdb_MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	ipkdb_MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	ipkdb_MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	ipkdb_MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	ipkdb_MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	ipkdb_MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	ipkdb_MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	ipkdb_MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	ipkdb_MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	ipkdb_MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	ipkdb_MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	ipkdb_MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	ipkdb_MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	ipkdb_MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	ipkdb_MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	ipkdb_MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	ipkdb_MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	ipkdb_MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	ipkdb_MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	ipkdb_MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	ipkdb_MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	ipkdb_MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	ipkdb_MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	ipkdb_MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	ipkdb_MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	ipkdb_MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	ipkdb_MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	ipkdb_MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	ipkdb_MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	ipkdb_MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	ipkdb_MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	ipkdb_MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	ipkdb_MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	ipkdb_MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	ipkdb_MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	ipkdb_MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	ipkdb_MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	ctx->buf[0] += a;
	ctx->buf[1] += b;
	ctx->buf[2] += c;
	ctx->buf[3] += d;
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
static void
ipkdb_MD5Init(struct ipkdb_MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
static void
ipkdb_MD5Update(struct ipkdb_MD5Context *ctx, u_char *buf, unsigned len)
{
	u_int t;

	/* Update bitcount */
	t = ctx->bits[0];
	if ((ctx->bits[0] = (t + (len << 3)) & 0xffffffff) < t)
		ctx->bits[1]++;	/* Carry from low to high */
	ctx->bits[1] += (len >> 29) & 0xffffffff;

	t = (t >> 3) & 0x3f;	/* Bytes already in ctx->in */

	/* Handle any leading odd-sized chunks */
	if (t) {
		u_char *p = ctx->in + t;

		t = 64 - t;
		if (len < t) {
			ipkdbcopy(buf, p, len);
			return;
		}
		ipkdbcopy(buf, p, t);
		ipkdb_MD5Transform(ctx);
		buf += t;
		len -= t;
	}

	/* Process data in 64-byte chunks */
	while (len >= 64) {
		ipkdbcopy(buf, ctx->in, 64);
		ipkdb_MD5Transform(ctx);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */
	ipkdbcopy(buf, ctx->in, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, LSB-first)
 */
static u_char *
ipkdb_MD5Final(struct ipkdb_MD5Context *ctx)
{
	static u_char digest[16];
	unsigned count;
	u_char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3f;

	/* Set the first char of padding to 0x80.  This is safe since there is
	   always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		ipkdbzero(p, count);
		ipkdb_MD5Transform(ctx);

		/* Now fill the next block with 56 bytes */
		ipkdbzero(ctx->in, 56);
	} else
		/* Pad block to 56 bytes */
		ipkdbzero(p, count - 8);

	/* Append length in bits and transform */
	setNl(ctx->in + 56, ctx->bits[0]);
	setNl(ctx->in + 60, ctx->bits[1]);

	ipkdb_MD5Transform(ctx);
	setNl(digest, ctx->buf[0]);
	setNl(digest + 4, ctx->buf[1]);
	setNl(digest + 8, ctx->buf[2]);
	setNl(digest + 12, ctx->buf[3]);

	return digest;
}

/*
 * The following code is more or less stolen from the hmac_md5
 * function in the Appendix of the HMAC IETF draft, but is
 * optimized as suggested in this same paper.
 */
static int
hmac_init(void)
{
	char pad[64];
	char tk[16];
	u_char *key = ipkdbkey;
	int key_len = strlen(key);
	int i;

	/* Require key to be at least 16 bytes long */
	if (key_len < 16) {
		printf("IPKDBKEY must be at least 16 bytes long!\n");
		ipkdbzero(key, key_len);				/* XXX */
		return 0;
	}

	/* if key is longer than 64 bytes reset it to key=MD5(key) */
	if (key_len > 64) {
		ipkdb_MD5Init(&icontext);
		ipkdb_MD5Update(&icontext, key, key_len);
		ipkdbcopy(ipkdb_MD5Final(&icontext), tk, 16);
		ipkdbzero(key, key_len);				/* XXX */
		key = tk;
		key_len = 16;
	}

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is and n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */
	/*
	 * We do the initial part of MD5(K XOR ipad)
	 * and MD5(K XOR opad) here, in order to
	 * speed up the computation later on.
	 */
	ipkdbzero(pad, sizeof pad);
	ipkdbcopy(key, pad, key_len);
	for (i = 0; i < 64; i++)
		pad[i] ^= 0x36;
	ipkdb_MD5Init(&icontext);
	ipkdb_MD5Update(&icontext, pad, 64);

	ipkdbzero(pad, sizeof pad);
	ipkdbcopy(key, pad, key_len);
	for (i = 0; i < 64; i++)
		pad[i] ^= 0x5c;
	ipkdb_MD5Init(&ocontext);
	ipkdb_MD5Update(&ocontext, pad, 64);

	/* Zero out the key						XXX */
	ipkdbzero(key, key_len);

	return 1;
}

/*
 * This is more or less hmac_md5 from the HMAC IETF draft, Appendix.
 */
static void *
chksum(void *buf, int len)
{
	u_char *digest;
	struct ipkdb_MD5Context context;

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */
	/*
	 * Since we've already done the precomputation,
	 * we can now stuff the data into the relevant
	 * preinitialized contexts to get the result.
	 */
	/*
	 * perform inner MD5
	 */
	ipkdbcopy(&icontext, &context, sizeof context);
	ipkdb_MD5Update(&context, buf, len);
	digest = ipkdb_MD5Final(&context);
	/*
	 * perform outer MD5
	 */
	ipkdbcopy(&ocontext, &context, sizeof context);
	ipkdb_MD5Update(&context, digest, 16);
	return ipkdb_MD5Final(&context);
}

static void
getpkt(struct ipkdb_if *ifp, char *buf, int *lp)
{
	char *got;
	int l;
	char _ibuf[ETHERMTU + 16];
#define	ibuf	(_ibuf + 2)		/* align ip data in packet */

	*lp = 0;
	while (1) {
		if (!(got = inpkt(ifp, ibuf, ipkdbpanic != 0))) {
			*lp = 0;
			return;
		}
		if (   ifp->seq == getnl(got)
		    && got[6] >= 'A'
		    && got[6] <= 'Z'
		    && (l = getns(got + 4))
		    && !ipkdbcmp(chksum(got, l + 6), got + l + 6, LENCHK)) {
			ipkdbcopy(got + 6, buf, *lp = l);
			return;
		}
		if (   ifp->pktlen
		    && ((ifp->flags & (IPKDB_MYIP | IPKDB_HISIP | IPKDB_CONNECTED))
			== (IPKDB_MYIP | IPKDB_HISIP | IPKDB_CONNECTED)))
			outpkt(ifp, ifp->pkt, ifp->pktlen, IPKDBPORT, ifp->hisport);
	}
#undef	ibuf
}

static void
putpkt(struct ipkdb_if *ifp, const char *buf, int l)
{
	setnl(ifp->pkt, ifp->seq++);
	setns(ifp->pkt + 4, l);
	ipkdbcopy(buf, ifp->pkt + 6, l);
	ipkdbcopy(chksum(ifp->pkt, l + 6), ifp->pkt + 6 + l, LENCHK);
	ifp->pktlen = l + 6 + LENCHK;
	if (   (ifp->flags & (IPKDB_MYIP | IPKDB_HISIP | IPKDB_CONNECTED))
	    != (IPKDB_MYIP | IPKDB_HISIP | IPKDB_CONNECTED))
		return;
	outpkt(ifp, ifp->pkt, ifp->pktlen, IPKDBPORT, ifp->hisport);
}

static int
check_ipkdb(struct ipkdb_if *ifp, struct in_addr *shost, char *p, int l)
{
	u_char hisenet[6];
	u_char hisinet[4];
	u_int16_t hisport;
	char save;

#ifndef	IPKDBSECURE
	if (kauth_authorize_system(curlwp->l_cred, KAUTH_SYSTEM_DEBUG,
	    KAUTH_REQ_SYSTEM_DEBUG_IPKDB, NULL, NULL, NULL))
		return 0;
#endif
	if (ipkdbcmp(chksum(p, l), p + l, LENCHK))
		return 0;
	ipkdbcopy(ifp->hisenetaddr, hisenet, sizeof hisenet);
	ipkdbcopy(ifp->hisinetaddr, hisinet, sizeof hisinet);
	hisport = ifp->hisport;
	save = ifp->flags;
	ipkdbcopy(shost, ifp->hisinetaddr, sizeof ifp->hisinetaddr);
	ifp->flags &= ~IPKDB_HISHW;
	ifp->flags |= IPKDB_HISIP;
	if (connectipkdb(ifp, p + 6, l - 6) < 0) {
		ipkdbcopy(hisenet, ifp->hisenetaddr, sizeof ifp->hisenetaddr);
		ipkdbcopy(hisinet, ifp->hisinetaddr, sizeof ifp->hisinetaddr);
		ifp->hisport = hisport;
		ifp->flags = save;
		return 0;
	}
	return 1;
}

/*
 * Should check whether packet came across the correct interface.	XXX
 */
int
checkipkdb(struct in_addr *shost, u_short sport, u_short dport, struct mbuf *m, int off, int len)
{
	char *p;
	int l;
	char ibuf[ETHERMTU+50];

	if (dport != IPKDBPORT)
		return 0;
	if (len > sizeof ibuf)
		return 0;
	m_copydata(m, off, len, ibuf);
	p = ibuf;
	if (getnl(p) || p[6] != 'O')
		return 0;
	l = getns(p + 4);
	if (l > len - 6 || !check_ipkdb(&ipkdb_if, shost, p, l + 6))
		return 0;
	ipkdb_if.hisport = sport;
	ipkdb_connect(1);
	return 1;
}

static int
connectipkdb(struct ipkdb_if *ifp, char *buf, int l)
{
	char *cp;
	u_char *ip;

	if (*buf != 'O')
		return -1;
	if (getnl(buf + 1) == ifp->id)
		/* It's a retry of a connect packet, ignore it */
		return -1;
	ip = ifp->hisinetaddr;
	printf("debugged by ");
	l -= 1 + sizeof(u_int32_t);
	for (cp = buf + 1 + sizeof(u_int32_t); --l >= 0; printf("%c", *cp++));
	printf(" (%d.%d.%d.%d)\n", ip[0], ip[1], ip[2], ip[3]);
	ifp->flags |= IPKDB_CONNECTED;
	ifp->seq = 0;
	ifp->pktlen = 0;
	ifp->id = getnl(buf + 1);
	return 0;
}
