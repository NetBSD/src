/*	$NetBSD: in_pcb.h,v 1.41.4.3 2006/02/04 03:26:27 rpaulo Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, 2003 WIDE Project.
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
 *
 */

/*
 * Merged from:
 *
 * NetBSD: in_pcb_hdr.h,v 1.4 2005/12/10 23:36:23 elad Exp
 * NetBSD: in6_pcb.h,v 1.28 2006/01/26 18:59:18 rpaulo Exp
 * + KAME: in6_pcb.h,v 1.45 2001/02/09 05:59:46 itojun Exp
 */


#ifndef _NETINET_IN_PCB_H_
#define _NETINET_IN_PCB_H_

#include <sys/queue.h>
#include <net/route.h>
#include <netinet/ip6.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define	in6pcb		inpcb	/* for KAME src sync over BSD*'s */
#define	in6p_sp		inp_sp	/* for KAME src sync over BSD*'s */

struct inpcbpolicy;
struct icmp6_filter;

struct inpcbtable {
	CIRCLEQ_HEAD(, inpcb) inpt_queue;
	struct	  inpcbhead *inpt_porthashtbl;
	struct	  inpcbhead *inpt_bindhashtbl;
	struct	  inpcbhead *inpt_connecthashtbl;
	u_long	  inpt_porthash;
	u_long	  inpt_bindhash;
	u_long	  inpt_connecthash;
	uint16_t  inpt_lastport;
	uint16_t  inpt_lastlow;
};
#define inpt_lasthi inpt_lastport

/* states in inp_state: */
#define	INP_ATTACHED		0
#define	INP_BOUND		1
#define	INP_CONNECTED		2

/* for KAME src sync over BSD*'s */
#define	IN6P_ATTACHED		INP_ATTACHED
#define	IN6P_BOUND		INP_BOUND
#define	IN6P_CONNECTED		INP_CONNECTED

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
LIST_HEAD(inpcbhead, inpcb);
LIST_HEAD(inpcbporthead, inpcbport);
typedef	u_quad_t		inp_gen_t;

/*
 * PCB with AF_INET6 null bind'ed laddr can receive AF_INET input packet.
 * So, AF_INET6 null laddr is also used as AF_INET null laddr,
 * by utilize following structure. (At last, same as INRIA)
 */
struct in_addr_4in6 {
	uint32_t	ia46_pad32[3];
	struct	in_addr	ia46_addr4;
};

/*
 * NOTE: ipv6 addrs should be 64-bit aligned, per RFC 2553.
 * in_conninfo has some extra padding to accomplish this.
 */
struct in_endpoints {
	uint16_t	ie_fport;		/* foreign port */
	uint16_t	ie_lport;		/* local port */
	/* protocol dependent part, local and foreign addr */
	union {
		/* foreign host table entry */
		struct	in_addr_4in6 ie46_foreign;
		struct	in6_addr ie6_foreign;
	} ie_dependfaddr;
	union {
		/* local host table entry */
		struct	in_addr_4in6 ie46_local;
		struct	in6_addr ie6_local;
	} ie_dependladdr;
#define	ie_faddr	ie_dependfaddr.ie46_foreign.ia46_addr4
#define	ie_laddr	ie_dependladdr.ie46_local.ia46_addr4
#define	ie6_faddr	ie_dependfaddr.ie6_foreign
#define	ie6_laddr	ie_dependladdr.ie6_local
};

struct in_conninfo {
	uint8_t		inc_isipv6;
	uint8_t		inc_len;
	uint16_t	inc_pad;	/* XXX alignment for in_endpoints */
	/* protocol dependent part */
	struct	in_endpoints inc_ie;
};

struct inpcb {
	LIST_ENTRY(inpcb) inp_hash;
	LIST_ENTRY(inpcb) inp_lhash;
	CIRCLEQ_ENTRY(inpcb) inp_queue;

	/* local and foreign ports, local and foreign addr */
	struct	in_conninfo inp_inc;

	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	int	inp_state;		/* bind/connect state */
	int	inp_af;			/* address family - AF_INET */
	struct  socket *inp_socket;	/* back pointer to socket */
	struct	inpcbtable *inp_table;
	struct	mbuf *inp_options;	/* IP options */

	struct	inpcbpolicy *inp_sp;	/* security policy */
	u_char	inp_vflag;		/* IP version flag (v4/v6) */
#define	INP_IPV4	0x1
#define	INP_IPV6	0x2
#define INP_IPV6PROTO	0x4		/* opened under IPv6 protocol */
#define INP_TIMEWAIT	0x8		/* .. probably doesn't go here */
#define	INP_ONESBCAST	0x10		/* send all-ones broadcast */
	u_char	inp_ip_ttl;		/* time to live proto */
	u_char	inp_ip_p;		/* protocol proto */
	u_char	inp_ip_minttl;		/* minimum TTL or drop */

	int	inp_flags;		/* generic IP/datagram flags */
	int	inp_errormtu;		/* MTU of last xmit status = EMSGSIZE */

	/* protocol dependent part; IPv4 */
	struct {
		struct	ip_moptions *in4p_moptions; /* IP multicast options */
		struct  route in4p_route;    /* placeholder for routing entry */
		struct	ip in4p_ip;	     /* header prototype */
	} in4p_depend;
#define inp_fport	inp_inc.inc_ie.ie_fport
#define inp_lport	inp_inc.inc_ie.ie_lport
#define	inp_faddr	inp_inc.inc_ie.ie_faddr
#define	inp_laddr	inp_inc.inc_ie.ie_laddr
#define	inp_ip		in4p_depend.in4p_ip
#define	inp_options	in4p_depend.in4p_options
#define	inp_moptions	in4p_depend.in4p_moptions
#define inp_route	in4p_depend.in4p_route
#define inp_options	in4p_depend.in4p_options

	/* protocol dependent part; IPv6 */
	struct {
		struct	 route_in6 in6p_route;   /* placeholder for 
						   routing entry */		
		struct	 ip6_pktopts *in6p_outputopts; /* IP6 options for 
							 outgoing packets */
		struct	 ip6_moptions *in6p_moptions; /* IP multicast options */
		struct	 icmp6_filter *in6p_icmp6filt; /* ICMPv6 code 
							 type filter */
		int	 in6p_cksum; 		/* IPV6_CHECKSUM setsockopt */
		int	 in6p_hops;		/* default hop limit */
		struct	 ip6_hdr in6p_ip6;	/* header prototype */
		uint32_t in6p_flowinfo;		/* priority and flowlabel */
	} in6p_depend;
#define in6p_fport	inp_fport
#define in6p_lport	inp_lport
#define	in6p_faddr	inp_inc.inc_ie.ie6_faddr
#define	in6p_laddr	inp_inc.inc_ie.ie6_laddr
#define in6p_route	in6p_depend.in6p_route
#define	in6p_outputopts	in6p_depend.in6p_outputopts
#define	in6p_moptions	in6p_depend.in6p_moptions
#define	in6p_icmp6filt	in6p_depend.in6p_icmp6filt
#define	in6p_cksum	in6p_depend.in6p_cksum
#define	in6p_hops	in6p_depend.in6p_hops
#define	in6p_ip6	in6p_depend.in6p_ip6
#define in6p_options	in6p_depend.in6p_options
#define in6p_flowinfo	in6p_depend.in6p_flowinfo
#define	in6p_socket	inp_socket
#define	in6p_ppcb	inp_ppcb
#define in6p_flags	inp_flags
};

/* flags in inp_flags: */
#define	INP_RECVOPTS		0x01	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x02	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x04	/* receive IP dst address */
#define	INP_HDRINCL		0x08	/* user supplies entire IP header */
#define	INP_HIGHPORT		0x10	/* (unused; FreeBSD compat) */
#define	INP_LOWPORT		0x20	/* user wants "low" port binding */
#define	INP_ANONPORT		0x40	/* port chosen for user */
#define	INP_RECVIF		0x80	/* receive incoming interface */
/* XXX should move to an UDP control block */
#define INP_ESPINUDP		0x100	/* ESP over UDP for NAT-T */
#define INP_ESPINUDP_NON_IKE	0x200	/* ESP over UDP for NAT-T */
#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|\
				INP_RECVIF)
#define INP_ESPINUDP_ALL	(INP_ESPINUDP|INP_ESPINUDP_NON_IKE)

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
#define	IN6P_BINDV6ONLY		0x10000000 /* do not grab IPv4 traffic */
#endif
#define IN6P_MINMTU		0x20000000 /* use minimum MTU */

#define IN6P_CONTROLOPTS	(IN6P_PKTINFO|IN6P_HOPLIMIT|IN6P_HOPOPTS|\
				 IN6P_DSTOPTS|IN6P_RTHDR|IN6P_RTHDRDSTOPTS|\
				 IN6P_MINMTU)

/* compute hash value for foreign and local in6_addr and port */
#define IN6_HASH(faddr, fport, laddr, lport) 			\
	(((faddr)->s6_addr32[0] ^ (faddr)->s6_addr32[1] ^	\
	  (faddr)->s6_addr32[2] ^ (faddr)->s6_addr32[3] ^	\
	  (laddr)->s6_addr32[0] ^ (laddr)->s6_addr32[1] ^	\
	  (laddr)->s6_addr32[2] ^ (laddr)->s6_addr32[3])	\
	 + (fport) + (lport))


#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)
#define	sotoin6pcb	sotoinpcb	/* for KAME src sync over BSD*'s */

#ifdef _KERNEL
void	in_losing(struct inpcb *);
int	in_pcballoc(struct socket *, void *);
int	in_pcbbind(void *, struct mbuf *, struct proc *);
int	in_pcbconnect(void *, struct mbuf *, struct proc *);
void	in_pcbdetach(void *);
void	in_pcbdisconnect(void *);
void	in_pcbinit(struct inpcbtable *, int, int);
struct inpcb *
	in_pcblookup_port(struct inpcbtable *,
	    struct in_addr, u_int, int);
struct inpcb *
	in_pcblookup_bind(struct inpcbtable *,
	    struct in_addr, u_int);
struct inpcb *
	in_pcblookup_connect(struct inpcbtable *,
	    struct in_addr, u_int, struct in_addr, u_int);
int	in_pcbnotify(struct inpcbtable *, struct in_addr, u_int,
	    struct in_addr, u_int, int, void (*)(struct inpcb *, int));
void	in_pcbnotifyall(struct inpcbtable *, struct in_addr, int,
	    void (*)(struct inpcb *, int));
void	in_pcbpurgeif0(struct inpcbtable *, struct ifnet *);
void	in_pcbpurgeif(struct inpcbtable *, struct ifnet *);
void	in_pcbstate(struct inpcb *, int);
void	in_rtchange(struct inpcb *, int);
void	in_setpeeraddr(struct inpcb *, struct mbuf *);
void	in_setsockaddr(struct inpcb *, struct mbuf *);
struct rtentry *
	in_pcbrtentry(struct inpcb *);
extern struct sockaddr_in *in_selectsrc(struct sockaddr_in *,
	struct route *, int, struct ip_moptions *, int *);

#ifdef INET6
void	in6_losing(struct inpcb *);
void	in6_pcbinit(struct inpcbtable *, int, int);
int	in6_pcballoc(struct socket *, void *);
int	in6_pcbbind(void *, struct mbuf *, struct proc *);
int	in6_pcbconnect(void *, struct mbuf *, struct proc *);
void	in6_pcbdetach(struct inpcb *);
void	in6_pcbdisconnect(struct inpcb *);
struct	inpcb *in6_pcblookup_port(struct inpcbtable *, struct in6_addr *,
	u_int, int);
int	in6_pcbnotify(struct inpcbtable *, struct sockaddr *,
	u_int, const struct sockaddr *, u_int, int, void *,
	void (*)(struct inpcb *, int));
void	in6_pcbpurgeif0(struct inpcbtable *, struct ifnet *);
void	in6_pcbpurgeif(struct inpcbtable *, struct ifnet *);
void	in6_pcbstate(struct inpcb *, int);
void	in6_rtchange(struct inpcb *, int);
void	in6_setpeeraddr(struct inpcb *, struct mbuf *);
void	in6_setsockaddr(struct inpcb *, struct mbuf *);

/* in in6_src.c */
int	in6_selecthlim(struct inpcb *, struct ifnet *);
int	in6_pcbsetport(struct in6_addr *, struct inpcb *, struct proc *);

extern struct rtentry *
	in6_pcbrtentry(struct inpcb *);
extern struct inpcb *in6_pcblookup_connect(struct inpcbtable *,
	struct in6_addr *, u_int, const struct in6_addr *, u_int, int);
extern struct inpcb *in6_pcblookup_bind(struct inpcbtable *,
	struct in6_addr *, u_int, int);
#endif /* INET6 */
#endif /* _KERNEL */

#endif /* !_NETINET_IN_PCB_H_ */
