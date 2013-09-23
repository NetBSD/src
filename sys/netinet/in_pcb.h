/*	$NetBSD: in_pcb.h,v 1.51.2.2 2013/09/23 00:57:53 rmind Exp $	*/

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

#ifndef _NETINET_IN_PCB_H_
#define _NETINET_IN_PCB_H_

struct inpcb;
struct inpcb_hdr;
struct inpcbtable;
struct vestigial_inpcb;

LIST_HEAD(inpcbhead, inpcb_hdr);
typedef struct inpcbhead inpcbhead_t;
typedef struct inpcbtable inpcbtable_t;
typedef struct inpcb inpcb_t;

#include <sys/types.h>
#include <sys/queue.h>

struct inpcbpolicy;
struct vestigial_hooks;

struct inpcb_hdr {
	LIST_ENTRY(inpcb_hdr) inph_hash;
	LIST_ENTRY(inpcb_hdr) inph_lhash;
	CIRCLEQ_ENTRY(inpcb_hdr) inph_queue;
	int		inph_af;	/* address family - AF_INET */
	void *		inph_ppcb;	/* pointer to per-protocol pcb */
	int		inph_state;	/* bind/connect state */
	int		inph_portalgo;
	struct socket *	inph_socket;	/* back pointer to socket */
	inpcbtable_t *	inph_table;
	struct inpcbpolicy *inph_sp;	/* security policy */
};

#if defined(__INPCB_PRIVATE)

struct inpcbtable {
	kmutex_t	inpt_lock;
	int		inpt_flags;
	CIRCLEQ_HEAD(, inpcb_hdr) inpt_queue;
	inpcbhead_t *	inpt_porthashtbl;
	inpcbhead_t *	inpt_bindhashtbl;
	inpcbhead_t *	inpt_connecthashtbl;
	u_long		inpt_porthash;
	u_long		inpt_bindhash;
	u_long		inpt_connecthash;
	in_port_t	inpt_lastport;
	in_port_t	inpt_lastlow;
	struct vestigial_hooks *inpt_vestige;
};

#define	inpt_lasthi	inpt_lastport

#define	INPT_MPSAFE	0x01

/*
 * Common structure PCB for internet protocol implementation.
 * It stores pointers to the local and foreign host table entries,
 * local and foreign socket numbers, and pointers up (to a socket
 * structure) and down (to a protocol-specific) control block.
 */
struct inpcb {
	struct inpcb_hdr inp_head;
#define inp_hash	inp_head.inph_hash
#define inp_queue	inp_head.inph_queue
#define inp_af		inp_head.inph_af
#define inp_ppcb	inp_head.inph_ppcb
#define inp_state	inp_head.inph_state
#define inp_portalgo	inp_head.inph_portalgo
#define inp_socket	inp_head.inph_socket
#define inp_table	inp_head.inph_table
#define inp_sp		inp_head.inph_sp
	struct route	inp_route;	/* placeholder for routing entry */
	in_port_t	inp_fport;	/* foreign port */
	in_port_t	inp_lport;	/* local port */
	int		inp_flags;	/* generic IP/datagram flags */
	struct ip	inp_ip;		/* header prototype; should have more */
	struct mbuf *	inp_options;	/* IP options */
	struct ip_moptions *inp_moptions; /* IP multicast options */
	int		inp_errormtu;	/* MTU of last xmit status = EMSGSIZE */
	uint8_t		inp_ip_minttl;
	bool		inp_bindportonsend;
};

#define	inp_faddr	inp_ip.ip_dst
#define	inp_laddr	inp_ip.ip_src

#endif /* !defined(__INPCB_PRIVATE) */

/* States in inpcb_t::inp_state */
#define	INP_ATTACHED		0
#define	INP_BOUND		1
#define	INP_CONNECTED		2

/* Flags in inpcb_t::inp_flags */
#define	INP_RECVOPTS		0x0001	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x0002	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x0004	/* receive IP dst address */
#define	INP_HDRINCL		0x0008	/* user supplies entire IP header */
#define	INP_HIGHPORT		0x0010	/* (unused; FreeBSD compat) */
#define	INP_LOWPORT		0x0020	/* user wants "low" port binding */
#define	INP_ANONPORT		0x0040	/* port chosen for user */
#define	INP_RECVIF		0x0080	/* receive incoming interface */
/* XXX should move to an UDP control block */
#define INP_ESPINUDP		0x0100	/* ESP over UDP for NAT-T */
#define INP_ESPINUDP_NON_IKE	0x0200	/* ESP over UDP for NAT-T */
#define INP_ESPINUDP_ALL	(INP_ESPINUDP|INP_ESPINUDP_NON_IKE)
#define INP_NOHEADER		0x0400	/* Kernel removes IP header
					 * before feeding a packet
					 * to the raw socket user.
					 * The socket user will
					 * not supply an IP header.
					 * Cancels INP_HDRINCL.
					 */
#define	INP_RECVTTL		0x0800	/* receive incoming IP TTL */
#define	INP_PKTINFO		0x1000	/* receive dst packet info */
#define	INP_RECVPKTINFO		0x2000	/* receive dst packet info */
#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|\
				INP_RECVIF|INP_RECVTTL|INP_RECVPKTINFO|\
				INP_PKTINFO)

#define	sotoinpcb_hdr(so)	((struct inpcb_hdr *)(so)->so_pcb)
#define	sotoinpcb(so)		((struct inpcb *)(so)->so_pcb)

#ifdef _KERNEL

struct route;
struct ip_moptions;

typedef int (*inpcb_func_t)(inpcb_t *, void *);

inpcbtable_t *inpcb_init(size_t, size_t, int);

int	inpcb_create(struct socket *, inpcbtable_t *);
int	inpcb_bind(inpcb_t *, struct mbuf *, struct lwp *);
int	inpcb_connect(inpcb_t *, struct mbuf *, struct lwp *);
void	inpcb_destroy(inpcb_t *);
void	inpcb_disconnect(inpcb_t *);
inpcb_t *inpcb_lookup_local(inpcbtable_t *, struct in_addr, in_port_t, int,
    struct vestigial_inpcb *);
inpcb_t *inpcb_lookup_bound(inpcbtable_t *, struct in_addr, in_port_t);
inpcb_t *inpcb_lookup(inpcbtable_t *, struct in_addr, in_port_t,
    struct in_addr, in_port_t, struct vestigial_inpcb *);
int	inpcb_notify(inpcbtable_t *, struct in_addr, u_int,
	    struct in_addr, u_int, int, void (*)(inpcb_t *, int));
void	inpcb_notifyall(inpcbtable_t *, struct in_addr, int,
	    void (*)(inpcb_t *, int));
void	inpcb_purgeif0(inpcbtable_t *, struct ifnet *);
void	inpcb_purgeif(inpcbtable_t *, struct ifnet *);
int	inpcb_foreach(inpcbtable_t *, int, inpcb_func_t, void *);
void	inpcb_fetch_peeraddr(inpcb_t *, struct mbuf *);
void	inpcb_fetch_sockaddr(inpcb_t *, struct mbuf *);

void	inpcb_set_state(inpcb_t *, int);

struct rtentry *inpcb_rtentry(inpcb_t *);
void	inpcb_rtchange(inpcb_t *, int);
void	inpcb_losing(inpcb_t *);

struct socket *inpcb_get_socket(inpcb_t *);
struct route *inpcb_get_route(inpcb_t *);
void *	inpcb_get_protopcb(inpcb_t *);
void	inpcb_set_protopcb(inpcb_t *, void *);

int	inpcb_get_flags(inpcb_t *);
void	inpcb_set_flags(inpcb_t *, int);
void	inpcb_get_addrs(inpcb_t *, struct in_addr *, struct in_addr *);
void	inpcb_set_addrs(inpcb_t *, struct in_addr *, struct in_addr *);
void	inpcb_get_ports(inpcb_t *, in_port_t *, in_port_t *);
void	inpcb_set_ports(inpcb_t *, in_port_t, in_port_t);

struct mbuf *inpcb_get_options(inpcb_t *);
void	inpcb_set_options(inpcb_t *, struct mbuf *);
struct ip *in_getiphdr(inpcb_t *);
struct ip_moptions *inpcb_get_moptions(inpcb_t *);
void	inpcb_set_moptions(inpcb_t *, struct ip_moptions *);
int	inpcb_get_portalgo(inpcb_t *);
int	inpcb_get_errormtu(inpcb_t *);
void	inpcb_set_errormtu(inpcb_t *, int);
uint8_t	inpcb_get_minttl(inpcb_t *);
void	inpcb_set_minttl(inpcb_t *, uint8_t);

struct inpcbpolicy *inpcb_get_sp(inpcb_t *);

void	inpcb_set_vestige(inpcbtable_t *, void *);
void *	inpcb_get_vestige(inpcbtable_t *);

#endif

#endif /* !_NETINET_IN_PCB_H_ */
