/*	$NetBSD: route.c,v 1.66.2.1 2006/06/19 04:17:07 chap Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)route.c	8.3 (Berkeley) 3/9/94";
#else
__RCSID("$NetBSD: route.c,v 1.66.2.1 2006/06/19 04:17:07 chap Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/un.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#define _KERNEL
#include <net/route.h>
#undef _KERNEL
#include <netinet/in.h>
#include <netatalk/at.h>
#include <netiso/iso.h>

#include <netns/ns.h>

#include <sys/sysctl.h>

#include <arpa/inet.h>

#include <err.h>
#include <kvm.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

#define kget(p, d) (kread((u_long)(p), (char *)&(d), sizeof (d)))

/* alignment constraint for routing socket */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

/*
 * XXX we put all of the sockaddr types in here to force the alignment
 * to be correct.
 */
static union sockaddr_union {
	struct	sockaddr u_sa;
	struct	sockaddr_in u_in;
	struct	sockaddr_un u_un;
	struct	sockaddr_iso u_iso;
	struct	sockaddr_at u_at;
	struct	sockaddr_dl u_dl;
	struct	sockaddr_ns u_ns;
	u_short	u_data[128];
	int u_dummy;		/* force word-alignment */
} pt_u;

int	do_rtent = 0;
struct	rtentry rtentry;
struct	radix_node rnode;
struct	radix_mask rmask;

static struct sockaddr *kgetsa(struct sockaddr *);
static void p_tree(struct radix_node *);
static void p_rtnode(void);
static void p_krtentry(struct rtentry *);

/*
 * Print routing tables.
 */
void
routepr(rtree)
	u_long rtree;
{
	struct radix_node_head *rnh, head;
	struct radix_node_head *rt_tables[AF_MAX+1];
	int i;

	printf("Routing tables\n");

	if (rtree == 0) {
		printf("rt_tables: symbol not in namelist\n");
		return;
	}

	kget(rtree, rt_tables);
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) == 0)
			continue;
		kget(rnh, head);
		if (i == AF_UNSPEC) {
			if (Aflag && (af == 0 || af == 0xff)) {
				printf("Netmasks:\n");
				p_tree(head.rnh_treetop);
			}
		} else if (af == AF_UNSPEC || af == i) {
			pr_family(i);
			do_rtent = 1;
			pr_rthdr(i, Aflag);
			p_tree(head.rnh_treetop);
		}
	}
}

static struct sockaddr *
kgetsa(dst)
	struct sockaddr *dst;
{

	kget(dst, pt_u.u_sa);
	if (pt_u.u_sa.sa_len > sizeof (pt_u.u_sa))
		kread((u_long)dst, (char *)pt_u.u_data, pt_u.u_sa.sa_len);
	return (&pt_u.u_sa);
}

static void
p_tree(rn)
	struct radix_node *rn;
{

again:
	kget(rn, rnode);
	if (rnode.rn_b < 0) {
		if (Aflag)
			printf("%-8.8lx ", (u_long) rn);
		if (rnode.rn_flags & RNF_ROOT) {
			if (Aflag)
				printf("(root node)%s",
				    rnode.rn_dupedkey ? " =>\n" : "\n");
		} else if (do_rtent) {
			kget(rn, rtentry);
			p_krtentry(&rtentry);
			if (Aflag)
				p_rtnode();
		} else {
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_key),
			    NULL, 0, 44);
			putchar('\n');
		}
		if ((rn = rnode.rn_dupedkey) != NULL)
			goto again;
	} else {
		if (Aflag && do_rtent) {
			printf("%-8.8lx ", (u_long) rn);
			p_rtnode();
		}
		rn = rnode.rn_r;
		p_tree(rnode.rn_l);
		p_tree(rn);
	}
}

static void
p_rtnode()
{
	struct radix_mask *rm = rnode.rn_mklist;
	char	nbuf[20];

	if (rnode.rn_b < 0) {
		if (rnode.rn_mask) {
			printf("\t  mask ");
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_mask),
				    NULL, 0, -1);
		} else if (rm == 0)
			return;
	} else {
		(void)snprintf(nbuf, sizeof nbuf, "(%d)", rnode.rn_b);
		printf("%6.6s %8.8lx : %8.8lx", nbuf, (u_long) rnode.rn_l,
		    (u_long) rnode.rn_r);
	}
	while (rm) {
		kget(rm, rmask);
		(void)snprintf(nbuf, sizeof nbuf, " %d refs, ", rmask.rm_refs);
		printf(" mk = %8.8lx {(%d),%s", (u_long) rm,
		    -1 - rmask.rm_b, rmask.rm_refs ? nbuf : " ");
		if (rmask.rm_flags & RNF_NORMAL) {
			struct radix_node rnode_aux;
			printf(" <normal>, ");
			kget(rmask.rm_leaf, rnode_aux);
			p_sockaddr(kgetsa((struct sockaddr *)rnode_aux.rn_mask),
				    NULL, 0, -1);
		} else
			p_sockaddr(kgetsa((struct sockaddr *)rmask.rm_mask),
			    NULL, 0, -1);
		putchar('}');
		if ((rm = rmask.rm_mklist) != NULL)
			printf(" ->");
	}
	putchar('\n');
}

static struct sockaddr *sockcopy __P((struct sockaddr *,
    union sockaddr_union *));

/*
 * copy a sockaddr into an allocated region, allocate at least sockaddr
 * bytes and zero unused
 */
static struct sockaddr *
sockcopy(sp, dp)
	struct sockaddr *sp;
	union sockaddr_union *dp;
{
	int len;

	if (sp == 0 || sp->sa_len == 0)
		(void)memset(dp, 0, sizeof (*sp));
	else {
		len = (sp->sa_len >= sizeof (*sp)) ? sp->sa_len : sizeof (*sp);
		(void)memcpy(dp, sp, len);
	}
	return ((struct sockaddr *)dp);
}

static void
p_krtentry(rt)
	struct rtentry *rt;
{
	static struct ifnet ifnet, *lastif;
	union sockaddr_union addr_un, mask_un;
	struct sockaddr *addr, *mask;
	int af;

	if (Lflag && (rt->rt_flags & RTF_LLINFO))
		return;

	memset(&addr_un, 0, sizeof(addr_un));
	memset(&mask_un, 0, sizeof(mask_un));
	addr = sockcopy(kgetsa(rt_key(rt)), &addr_un);
	af = addr->sa_family;
	if (rt_mask(rt))
		mask = sockcopy(kgetsa(rt_mask(rt)), &mask_un);
	else
		mask = sockcopy(NULL, &mask_un);
	p_addr(addr, mask, rt->rt_flags);
	p_gwaddr(kgetsa(rt->rt_gateway), kgetsa(rt->rt_gateway)->sa_family);
	p_flags(rt->rt_flags, "%-6.6s ");
	printf("%6d %8lu ", rt->rt_refcnt, rt->rt_use);
	if (rt->rt_rmx.rmx_mtu)
		printf("%6lu", rt->rt_rmx.rmx_mtu); 
	else
		printf("%6s", "-");
	putchar((rt->rt_rmx.rmx_locks & RTV_MTU) ? 'L' : ' ');
	if (rt->rt_ifp) {
		if (rt->rt_ifp != lastif) {
			kget(rt->rt_ifp, ifnet);
			lastif = rt->rt_ifp;
		}
		printf(" %.16s%s", ifnet.if_xname,
			rt->rt_nodes[0].rn_dupedkey ? " =>" : "");
	}
	putchar('\n');
 	if (vflag) {
 		printf("\texpire   %10lu%c  recvpipe %10ld%c  "
		       "sendpipe %10ld%c\n",
 			rt->rt_rmx.rmx_expire, 
 			(rt->rt_rmx.rmx_locks & RTV_EXPIRE) ? 'L' : ' ',
 			rt->rt_rmx.rmx_recvpipe,
 			(rt->rt_rmx.rmx_locks & RTV_RPIPE) ? 'L' : ' ',
 			rt->rt_rmx.rmx_sendpipe,
 			(rt->rt_rmx.rmx_locks & RTV_SPIPE) ? 'L' : ' ');
 		printf("\tssthresh %10lu%c  rtt      %10ld%c  "
		       "rttvar   %10ld%c\n",
 			rt->rt_rmx.rmx_ssthresh, 
 			(rt->rt_rmx.rmx_locks & RTV_SSTHRESH) ? 'L' : ' ',
 			rt->rt_rmx.rmx_rtt, 
 			(rt->rt_rmx.rmx_locks & RTV_RTT) ? 'L' : ' ',
 			rt->rt_rmx.rmx_rttvar, 
			(rt->rt_rmx.rmx_locks & RTV_RTTVAR) ? 'L' : ' ');
 		printf("\thopcount %10lu%c\n",
 			rt->rt_rmx.rmx_hopcount, 
			(rt->rt_rmx.rmx_locks & RTV_HOPCOUNT) ? 'L' : ' ');
 	}
}

/*
 * Print routing statistics
 */
void
rt_stats(off)
	u_long off;
{
	struct rtstat rtstat;

	if (use_sysctl) {
		size_t rtsize = sizeof(rtstat);

		if (sysctlbyname("net.route.stats", &rtstat, &rtsize,
		    NULL, 0) == -1)
			err(1, "rt_stats: sysctl");
	} else 	if (off == 0) {
		printf("rtstat: symbol not in namelist\n");
		return;
	} else
		kread(off, (char *)&rtstat, sizeof (rtstat));

	printf("routing:\n");
	printf("\t%llu bad routing redirect%s\n",
		(unsigned long long)rtstat.rts_badredirect,
		plural(rtstat.rts_badredirect));
	printf("\t%llu dynamically created route%s\n",
		(unsigned long long)rtstat.rts_dynamic,
		plural(rtstat.rts_dynamic));
	printf("\t%llu new gateway%s due to redirects\n",
		(unsigned long long)rtstat.rts_newgateway,
		plural(rtstat.rts_newgateway));
	printf("\t%llu destination%s found unreachable\n",
		(unsigned long long)rtstat.rts_unreach,
		plural(rtstat.rts_unreach));
	printf("\t%llu use%s of a wildcard route\n",
		(unsigned long long)rtstat.rts_wildcard,
		plural(rtstat.rts_wildcard));
}

short ns_nullh[] = {0,0,0};
short ns_bh[] = {-1,-1,-1};

char *
ns_print(sa)
	struct sockaddr *sa;
{
	struct sockaddr_ns *sns = (struct sockaddr_ns*)sa;
	struct ns_addr work;
	union {
		union	ns_net net_e;
		u_long	long_e;
	} net;
	u_short port;
	static char mybuf[50], cport[10], chost[25];
	char *host = "";
	char *p;
	u_char *q;

	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.long_e == 0) {
		if (port ) {
			(void)snprintf(mybuf, sizeof mybuf, "*.%xH", port);
			upHex(mybuf);
		} else
			(void)snprintf(mybuf, sizeof mybuf, "*.*");
		return (mybuf);
	}

	if (memcmp(ns_bh, work.x_host.c_host, 6) == 0) {
		host = "any";
	} else if (memcmp(ns_nullh, work.x_host.c_host, 6) == 0) {
		host = "*";
	} else {
		q = work.x_host.c_host;
		(void)snprintf(chost, sizeof chost, "%02x%02x%02x%02x%02x%02xH",
			q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++)
			continue;
		host = p;
	}
	if (port)
		(void)snprintf(cport, sizeof cport, ".%xH", htons(port));
	else
		*cport = 0;

	(void)snprintf(mybuf, sizeof mybuf, "%xH.%s%s", (int)ntohl(net.long_e),
	    host, cport);
	upHex(mybuf);
	return (mybuf);
}

char *
ns_phost(sa)
	struct sockaddr *sa;
{
	struct sockaddr_ns *sns = (struct sockaddr_ns *)sa;
	struct sockaddr_ns work;
	static union ns_net ns_zeronet;
	char *p;

	work = *sns;
	work.sns_addr.x_port = 0;
	work.sns_addr.x_net = ns_zeronet;

	p = ns_print((struct sockaddr *)&work);
	if (strncmp("0H.", p, 3) == 0)
		p += 3;
	return (p);
}

void
upHex(p0)
	char *p0;
{
	char *p = p0;

	for (; *p; p++)
		switch (*p) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			*p += ('A' - 'a');
		}
}


