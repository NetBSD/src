/*	$NetBSD: netstat.h,v 1.51.18.1 2022/09/12 14:23:41 martin Exp $	*/

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

extern int	Aflag;		/* show addresses of protocol control block */
extern int	aflag;		/* show all sockets (including servers) */
extern int	Bflag;		/* show Berkeley Packet Filter information */
extern int	bflag;		/* show i/f byte stats */
extern int	dflag;		/* show i/f dropped packets */
#ifndef SMALL
extern int	gflag;		/* show group (multicast) routing or stats */
#endif
extern int	hflag;		/* humanize byte counts */
extern int	iflag;		/* show interfaces */
extern int	Lflag;		/* don't show LLINFO entries */
extern int	lflag;		/* show routing table with use and ref */
extern int	mflag;		/* show memory stats */
extern int	numeric_addr;	/* show addresses numerically */
extern int	numeric_port;	/* show ports numerically */
extern int	nflag;		/* same as above, for show.c compat */
extern int	Pflag;		/* dump a PCB */
extern int	pflag;		/* show given protocol */
extern int	qflag;		/* show softintrq */
extern int	rflag;		/* show routing tables (or routing stats) */
extern int	sflag;		/* show protocol statistics */
extern int	tagflag;	/* show route tags */
extern int	tflag;		/* show i/f watchdog timers */
extern int	Vflag;	       /* show Vestigial TIME_WAIT (VTW) information */
extern int	vflag;	/* verbose route information or don't truncate names */

extern char	*interface;   /* desired i/f for stats, or NULL for all i/fs */

extern int	af;		/* address family */
extern int	use_sysctl;	/* use sysctl instead of kmem */
extern int	force_sysctl; /* force use of sysctl (or exit) - for testing */


int	kread(u_long addr, char *buf, int size);
const char *plural(int);
const char *plurales(int);
int	get_hardticks(void);

void	protopr(u_long, const char *);
void	tcp_stats(u_long, const char *);
void	tcp_dump(u_long, const char *, u_long);
void	udp_stats(u_long, const char *);
void	ip_stats(u_long, const char *);
void	icmp_stats(u_long, const char *);
void	igmp_stats(u_long, const char *);
void	pim_stats(u_long, const char *);
void	arp_stats(u_long, const char *);
void	carp_stats(u_long, const char *);
void	pfsync_stats(u_long, const char*);
#ifdef IPSEC
void	fast_ipsec_stats(u_long, const char *);
#endif

#ifdef INET6
struct sockaddr_in6;
struct in6_addr;
void	ip6protopr(u_long, const char *);
void	tcp6_stats(u_long, const char *);
void	tcp6_dump(u_long, const char *, u_long);
void	udp6_stats(u_long, const char *);
void	ip6_stats(u_long, const char *);
void	ip6_ifstats(const char *);
void	icmp6_stats(u_long, const char *);
void	icmp6_ifstats(const char *);
void	pim6_stats(u_long, const char *);
void	rip6_stats(u_long, const char *);
void	mroute6pr(u_long, u_long, u_long);
void	mrt6_stats(u_long, u_long);
#endif /*INET6*/

#ifdef IPSEC
void	pfkey_stats(u_long, const char *);
#endif

void	mbpr(u_long, u_long, u_long, u_long, u_long);

void	hostpr(u_long, u_long);
void	impstats(u_long, u_long);

void	rt_stats(u_long);
char	*ns_phost(struct sockaddr *);

const char *atalk_print(const struct sockaddr *, int);
const char *atalk_print2(const struct sockaddr *, const struct sockaddr *,
    int);
char	*ns_print(struct sockaddr *);

void	nsprotopr(u_long, const char *);
void	spp_stats(u_long, const char *);
void	idp_stats(u_long, const char *);
void	nserr_stats(u_long, const char *);

void	atalkprotopr(u_long, const char *);
void	ddp_stats(u_long, const char *);

void	intpr(int, u_long, void (*)(const char *));

void	unixpr(u_long);

void	routepr(u_long);
void	mroutepr(u_long, u_long, u_long, u_long);
void	mrt_stats(u_long, u_long);

void	bpf_stats(void);
void	bpf_dump(const char *);

kvm_t *get_kvmd(void);

char	*mpls_ntoa(const struct sockaddr *);

struct kinfo_pcb *getpcblist_sysctl(const char *, size_t *);

#define PLEN    (LONG_BIT / 4 + 2)
