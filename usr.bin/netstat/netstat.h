/*	$NetBSD: netstat.h,v 1.43.2.1 2012/04/17 00:09:37 yamt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	Regents of the University of California.  All rights reserved.
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
 *
 *	from: @(#)netstat.h	8.2 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
#include <kvm.h>

int	Aflag;		/* show addresses of protocol control block */
int	aflag;		/* show all sockets (including servers) */
int	Bflag;		/* show Berkeley Packet Filter information */
int	bflag;		/* show i/f byte stats */
int	dflag;		/* show i/f dropped packets */
#ifndef SMALL
int	gflag;		/* show group (multicast) routing or stats */
#endif
int	hflag;		/* humanize byte counts */
int	iflag;		/* show interfaces */
int	Lflag;		/* don't show LLINFO entries */
int	lflag;		/* show routing table with use and ref */
int	mflag;		/* show memory stats */
int	numeric_addr;	/* show addresses numerically */
int	numeric_port;	/* show ports numerically */
int	nflag;		/* same as above, for show.c compat */
int	Pflag;		/* dump a PCB */
int	pflag;		/* show given protocol */
int	qflag;		/* show softintrq */
int	rflag;		/* show routing tables (or routing stats) */
int	sflag;		/* show protocol statistics */
int	tagflag;	/* show route tags */
int	tflag;		/* show i/f watchdog timers */
int	Vflag;		/* show Vestigial TIME_WAIT (VTW) information */
int	vflag;		/* verbose route information or don't truncate names */

char	*interface;	/* desired i/f for stats, or NULL for all i/fs */

int	af;		/* address family */
int	use_sysctl;	/* use sysctl instead of kmem */
int	force_sysctl;	/* force use of sysctl (or exit) - for testing */


int	kread __P((u_long addr, char *buf, int size));
const char *plural __P((int));
const char *plurales __P((int));
int	get_hardticks __P((void));

void	protopr __P((u_long, const char *));
void	tcp_stats __P((u_long, const char *));
void	tcp_dump __P((u_long));
void	udp_stats __P((u_long, const char *));
void	ip_stats __P((u_long, const char *));
void	icmp_stats __P((u_long, const char *));
void	igmp_stats __P((u_long, const char *));
void	pim_stats __P((u_long, const char *));
void	arp_stats __P((u_long, const char *));
void	carp_stats __P((u_long, const char *));
void	pfsync_stats __P((u_long, const char*));
#ifdef IPSEC
void	fast_ipsec_stats __P((u_long, const char *));
#endif

#ifdef INET6
struct sockaddr_in6;
struct in6_addr;
void	ip6protopr __P((u_long, const char *));
void	tcp6_stats __P((u_long, const char *));
void	tcp6_dump __P((u_long));
void	udp6_stats __P((u_long, const char *));
void	ip6_stats __P((u_long, const char *));
void	ip6_ifstats __P((const char *));
void	icmp6_stats __P((u_long, const char *));
void	icmp6_ifstats __P((const char *));
void	pim6_stats __P((u_long, const char *));
void	rip6_stats __P((u_long, const char *));
void	mroute6pr __P((u_long, u_long, u_long));
void	mrt6_stats __P((u_long, u_long));
char	*routename6 __P((struct sockaddr_in6 *));
#endif /*INET6*/

#ifdef IPSEC
void	pfkey_stats __P((u_long, const char *));
#endif

void	mbpr(u_long, u_long, u_long, u_long, u_long);

void	hostpr __P((u_long, u_long));
void	impstats __P((u_long, u_long));

void	pr_rthdr __P((int, int));
void	pr_family __P((int));
void	rt_stats __P((u_long));
char	*ns_phost __P((struct sockaddr *));
void	upHex __P((char *));

void	p_rttables(int);
void	p_flags(int, const char *);
void	p_addr(struct sockaddr *, struct sockaddr *, int);
void	p_gwaddr(struct sockaddr *, int);
void	p_sockaddr(struct sockaddr *, struct sockaddr *, int, int);
char	*routename(struct sockaddr *);
char	*routename4(in_addr_t);
char	*netname(struct sockaddr *, struct sockaddr *);
char	*netname4(in_addr_t, in_addr_t);

/* char	*routename __P((u_int32_t)); */
/* char	*netname __P((u_int32_t, u_int32_t)); */
#ifdef INET6
char	*netname6 __P((struct sockaddr_in6 *, struct sockaddr_in6 *));
#endif 
const char *atalk_print __P((const struct sockaddr *, int));
const char *atalk_print2 __P((const struct sockaddr *, const struct sockaddr *,
    int));
char	*ns_print __P((struct sockaddr *));
void	routepr __P((u_long));

void	nsprotopr __P((u_long, const char *));
void	spp_stats __P((u_long, const char *));
void	idp_stats __P((u_long, const char *));
void	nserr_stats __P((u_long, const char *));

void	atalkprotopr __P((u_long, const char *));
void	ddp_stats __P((u_long, const char *));

void	intpr __P((int, u_long, void (*) __P((const char *))));

void	unixpr __P((u_long));

void	esis_stats __P((u_long, const char *));
void	clnp_stats __P((u_long, const char *));
void	cltp_stats __P((u_long, const char *));
void	iso_protopr __P((u_long, const char *));
void	iso_protopr1 __P((u_long, int));
void	tp_protopr __P((u_long, const char *));
void	tp_inproto __P((u_long));
void	tp_stats __P((u_long, const char *));

void	mroutepr __P((u_long, u_long, u_long, u_long));
void	mrt_stats __P((u_long, u_long));

void	bpf_stats(void);
void	bpf_dump(const char *);

kvm_t *get_kvmd(void);

char	*mpls_ntoa(const struct sockaddr *);

#define PLEN    (LONG_BIT / 4 + 2)
