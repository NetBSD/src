/*	$NetBSD: in6_pcb.h,v 1.23 2003/08/25 00:14:30 itojun Exp $	*/
/*	$KAME: in6_pcb.h,v 1.45 2001/02/09 05:59:46 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET6_IN6_PCB_H_
#define _NETINET6_IN6_PCB_H_

#include <sys/queue.h>

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct icmp6_filter;
struct inpcbpolicy;

struct	in6pcb {
	struct	in6pcb *in6p_next, *in6p_prev;
					/* pointers to other pcb's */
	struct	in6pcb *in6p_head;	/* pointer back to chain of
					   in6pcb's for this protocol */
	u_int16_t in6p_fport;		/* foreign port */
	u_int16_t in6p_lport;		/* local port */
	u_int32_t in6p_flowinfo;	/* priority and flowlabel */
	struct	socket *in6p_socket;	/* back pointer to socket */
	caddr_t	in6p_ppcb;		/* pointer to per-protocol pcb */
	struct	route_in6 in6p_route;	/* placeholder for routing entry */
	int	in6p_flags;		/* generic IP6/datagram flags */
	int	in6p_hops;		/* default hop limit */
	struct	ip6_hdr in6p_ip6;	/* header prototype */
	struct	mbuf *in6p_options;   /* IP6 options */
	struct	ip6_pktopts *in6p_outputopts; /* IP6 options for outgoing packets */
	struct	ip6_moptions *in6p_moptions; /* IP6 multicast options */
#if 1 /* IPSEC */
	struct inpcbpolicy *in6p_sp;	/* security policy. */
#endif
	struct icmp6_filter *in6p_icmp6filt;
	int	in6p_cksum;		/* IPV6_CHECKSUM setsockopt */
};

#define in6p_faddr	in6p_ip6.ip6_dst
#define in6p_laddr	in6p_ip6.ip6_src

/*
 * Flags in in6p_flags
 * We define KAME's original flags in higher 16 bits as much as possible
 * for compatibility with *bsd*s.
 */
#define IN6P_RECVOPTS		0x001000 /* receive incoming IP6 options */
#define IN6P_RECVRETOPTS	0x002000 /* receive IP6 options for reply */
#define IN6P_RECVDSTADDR	0x004000 /* receive IP6 dst address */
#define IN6P_IPV6_V6ONLY	0x008000 /* restrict AF_INET6 socket for v6 */
#define IN6P_PKTINFO		0x010000 /* receive IP6 dst and I/F */
#define IN6P_HOPLIMIT		0x020000 /* receive hoplimit */
#define IN6P_HOPOPTS		0x040000 /* receive hop-by-hop options */
#define IN6P_DSTOPTS		0x080000 /* receive dst options after rthdr */
#define IN6P_RTHDR		0x100000 /* receive routing header */
#define IN6P_RTHDRDSTOPTS	0x200000 /* receive dstoptions before rthdr */

#define IN6P_HIGHPORT		0x1000000 /* user wants "high" port binding */
#define IN6P_LOWPORT		0x2000000 /* user wants "low" port binding */
#define IN6P_ANONPORT		0x4000000 /* port chosen for user */
#define IN6P_FAITH		0x8000000 /* accept FAITH'ed connections */
#if 0 /* obsoleted */
#define IN6P_BINDV6ONLY		0x10000000 /* do not grab IPv4 traffic */
#endif

#define IN6P_CONTROLOPTS	(IN6P_PKTINFO|IN6P_HOPLIMIT|IN6P_HOPOPTS|\
				 IN6P_DSTOPTS|IN6P_RTHDR|IN6P_RTHDRDSTOPTS)


#define IN6PLOOKUP_WILDCARD	1
#define IN6PLOOKUP_SETLOCAL	2

/* compute hash value for foreign and local in6_addr and port */
#define IN6_HASH(faddr, fport, laddr, lport) 			\
	(((faddr)->s6_addr32[0] ^ (faddr)->s6_addr32[1] ^	\
	  (faddr)->s6_addr32[2] ^ (faddr)->s6_addr32[3] ^	\
	  (laddr)->s6_addr32[0] ^ (laddr)->s6_addr32[1] ^	\
	  (laddr)->s6_addr32[2] ^ (laddr)->s6_addr32[3])	\
	 + (fport) + (lport))

#define sotoin6pcb(so)	((struct in6pcb *)(so)->so_pcb)

#ifdef _KERNEL
void	in6_losing __P((struct in6pcb *));
int	in6_pcballoc __P((struct socket *, struct in6pcb *));
int	in6_pcbbind __P((struct in6pcb *, struct mbuf *, struct proc *));
int	in6_pcbconnect __P((struct in6pcb *, struct mbuf *));
void	in6_pcbdetach __P((struct in6pcb *));
void	in6_pcbdisconnect __P((struct in6pcb *));
struct	in6pcb *
	in6_pcblookup __P((struct in6pcb *,
			   struct in6_addr *, u_int, struct in6_addr *,
			   u_int, int));
int	in6_pcbnotify __P((struct in6pcb *, struct sockaddr *,
			   u_int, struct sockaddr *, u_int, int, void *,
			   void (*)(struct in6pcb *, int)));
void	in6_pcbpurgeif0 __P((struct in6pcb *, struct ifnet *));
void	in6_pcbpurgeif __P((struct in6pcb *, struct ifnet *));
void	in6_rtchange __P((struct in6pcb *, int));
void	in6_setpeeraddr __P((struct in6pcb *, struct mbuf *));
void	in6_setsockaddr __P((struct in6pcb *, struct mbuf *));

/* in in6_src.c */
int	in6_selecthlim __P((struct in6pcb *, struct ifnet *));
int	in6_pcbsetport __P((struct in6_addr *, struct in6pcb *, struct proc *));

extern struct rtentry *
	in6_pcbrtentry __P((struct in6pcb *));
extern struct in6pcb *in6_pcblookup_connect __P((struct in6pcb *,
	struct in6_addr *, u_int, struct in6_addr *, u_int, int));
extern struct in6pcb *in6_pcblookup_bind __P((struct in6pcb *,
	struct in6_addr *, u_int, int));
#endif /* _KERNEL */

#endif /* !_NETINET6_IN6_PCB_H_ */
