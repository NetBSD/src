/*	$NetBSD: ip_var.h,v 1.38.8.1 1999/12/27 18:36:18 wrstuden Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)ip_var.h	8.2 (Berkeley) 1/9/95
 */

#ifndef _NETINET_IP_VAR_H_
#define _NETINET_IP_VAR_H_

#include <sys/queue.h>
#include <net/route.h>

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	u_int8_t  ih_x1[9];		/* (unused) */
	u_int8_t  ih_pr;		/* protocol */
	u_int16_t ih_len;		/* protocol length */
	struct	  in_addr ih_src;	/* source internet address */
	struct	  in_addr ih_dst;	/* destination internet address */
} __attribute__((__packed__));

/*
 * Ip (reassembly or sequence) queue structures.
 *
 * XXX -- The following explains why the ipqe_m field is here, for TCP's use:
 * We want to avoid doing m_pullup on incoming packets but that
 * means avoiding dtom on the tcp reassembly code.  That in turn means
 * keeping an mbuf pointer in the reassembly queue (since we might
 * have a cluster).  As a quick hack, the source & destination
 * port numbers (which are no longer needed once we've located the
 * tcpcb) are overlayed with an mbuf pointer.
 */
LIST_HEAD(ipqehead, ipqent);
struct ipqent {
	LIST_ENTRY(ipqent) ipqe_q;
	union {
		struct ip	*_ip;
		struct tcpiphdr *_tcp;
	} _ipqe_u1;
	struct mbuf	*ipqe_m;	/* mbuf contains packet */
	u_int8_t	ipqe_mff;	/* for IP fragmentation */
	/*
	 * The following are used in TCP reassembly
	 */
	LIST_ENTRY(ipqent) ipqe_timeq;
	u_int32_t ipqe_seq;
	u_int32_t ipqe_len;
	u_int32_t ipqe_flags;
};
#define	ipqe_ip		_ipqe_u1._ip
#define	ipqe_tcp	_ipqe_u1._tcp

/*
 * Ip reassembly queue structure.  Each fragment
 * being reassembled is attached to one of these structures.
 * They are timed out after ipq_ttl drops to 0, and may also
 * be reclaimed if memory becomes tight.
 */
struct ipq {
	LIST_ENTRY(ipq) ipq_q;		/* to other reass headers */
	u_int8_t  ipq_ttl;		/* time for reass q to live */
	u_int8_t  ipq_p;		/* protocol of this fragment */
	u_int16_t ipq_id;		/* sequence id for reassembly */
	struct	  ipqehead ipq_fragq;	/* to ip fragment queue */
	struct	  in_addr ipq_src, ipq_dst;
};

/*
 * Structure stored in mbuf in inpcb.ip_options
 * and passed to ip_output when ip options are in use.
 * The actual length of the options (including ipopt_dst)
 * is in m_len.
 */
#define	MAX_IPOPTLEN	40

struct ipoption {
	struct	in_addr ipopt_dst;	/* first-hop dst if source routed */
	int8_t	ipopt_list[MAX_IPOPTLEN];	/* options proper */
};

/*
 * Structure attached to inpcb.ip_moptions and
 * passed to ip_output when IP multicast options are in use.
 */
struct ip_moptions {
	struct	  ifnet *imo_multicast_ifp; /* ifp for outgoing multicasts */
	u_int8_t  imo_multicast_ttl;	/* TTL for outgoing multicasts */
	u_int8_t  imo_multicast_loop;	/* 1 => hear sends if a member */
	u_int16_t imo_num_memberships;	/* no. memberships this socket */
	struct	  in_multi *imo_membership[IP_MAX_MEMBERSHIPS];
};

struct	ipstat {
	u_quad_t ips_total;		/* total packets received */
	u_quad_t ips_badsum;		/* checksum bad */
	u_quad_t ips_tooshort;		/* packet too short */
	u_quad_t ips_toosmall;		/* not enough data */
	u_quad_t ips_badhlen;		/* ip header length < data size */
	u_quad_t ips_badlen;		/* ip length < ip header length */
	u_quad_t ips_fragments;		/* fragments received */
	u_quad_t ips_fragdropped;	/* frags dropped (dups, out of space) */
	u_quad_t ips_fragtimeout;	/* fragments timed out */
	u_quad_t ips_forward;		/* packets forwarded */
	u_quad_t ips_fastforward;	/* packets fast forwarded */
	u_quad_t ips_cantforward;	/* packets rcvd for unreachable dest */
	u_quad_t ips_redirectsent;	/* packets forwarded on same net */
	u_quad_t ips_noproto;		/* unknown or unsupported protocol */
	u_quad_t ips_delivered;		/* datagrams delivered to upper level*/
	u_quad_t ips_localout;		/* total ip packets generated here */
	u_quad_t ips_odropped;		/* lost packets due to nobufs, etc. */
	u_quad_t ips_reassembled;	/* total packets reassembled ok */
	u_quad_t ips_fragmented;	/* datagrams sucessfully fragmented */
	u_quad_t ips_ofragments;	/* output fragments created */
	u_quad_t ips_cantfrag;		/* don't fragment flag was set, etc. */
	u_quad_t ips_badoptions;	/* error in option processing */
	u_quad_t ips_noroute;		/* packets discarded due to no route */
	u_quad_t ips_badvers;		/* ip version != 4 */
	u_quad_t ips_rawout;		/* total raw ip packets generated */
	u_quad_t ips_badfrags;		/* malformed fragments (bad length) */
	u_quad_t ips_rcvmemdrop;	/* frags dropped for lack of memory */
	u_quad_t ips_toolong;		/* ip length > max ip packet size */
	u_quad_t ips_nogif;		/* no match gif found */
};

#define	IPFLOW_HASHBITS			6 /* should not be a multiple of 8 */
struct ipflow {
	LIST_ENTRY(ipflow) ipf_list;	/* next in active list */
	LIST_ENTRY(ipflow) ipf_hash;	/* next ipflow in bucket */
	struct in_addr ipf_dst;		/* destination address */
	struct in_addr ipf_src;		/* source address */
	u_int8_t ipf_tos;		/* type-of-service */
	struct route ipf_ro;		/* associated route entry */
	u_long ipf_uses;		/* number of uses in this period */
	u_long ipf_last_uses;		/* number of uses in last period */
	u_long ipf_dropped;		/* ENOBUFS returned by if_output */
	u_long ipf_errors;		/* other errors returned by if_output */
	u_int ipf_timer;		/* lifetime timer */
	time_t ipf_start;		/* creation time */
};

#ifdef _KERNEL
/* flags passed to ip_output as last parameter */
#define	IP_FORWARDING		0x1		/* most of ip header exists */
#define	IP_RAWOUTPUT		0x2		/* raw ip header exists */
#define	IP_RETURNMTU		0x4		/* pass back mtu on EMSGSIZE */
#define	IP_ROUTETOIF		SO_DONTROUTE	/* bypass routing tables */
#define	IP_ALLOWBROADCAST	SO_BROADCAST	/* can send broadcast packets */

extern struct ipstat ipstat;		/* ip statistics */
extern LIST_HEAD(ipqhead, ipq) ipq;	/* ip reass. queue */
extern u_int16_t ip_id;			/* ip packet ctr, for ids */
extern int   ip_defttl;			/* default IP ttl */
extern int   ipforwarding;		/* ip forwarding */
extern int   ip_gif_ttl;		/* default TTL for gif encap packet */
extern int   ip_mtudisc;		/* mtu discovery */
extern u_int ip_mtudisc_timeout;	/* seconds to timeout mtu discovery */
extern int   anonportmin;		/* minimum ephemeral port */
extern int   anonportmax;		/* maximum ephemeral port */
extern struct rttimer_queue *ip_mtudisc_timeout_q;
#ifdef GATEWAY
extern int ip_maxflows;
#endif
extern struct pool ipqent_pool;
struct	 inpcb;

int	 ip_ctloutput __P((int, struct socket *, int, int, struct mbuf **));
int	 ip_dooptions __P((struct mbuf *));
void	 ip_drain __P((void));
void	 ip_forward __P((struct mbuf *, int));
void	 ip_freef __P((struct ipq *));
void	 ip_freemoptions __P((struct ip_moptions *));
int	 ip_getmoptions __P((int, struct ip_moptions *, struct mbuf **));
void	 ip_init __P((void));
int	 ip_mforward __P((struct mbuf *, struct ifnet *));
int	 ip_optcopy __P((struct ip *, struct ip *));
u_int	 ip_optlen __P((struct inpcb *));
int	 ip_output __P((struct mbuf *, ...));
int	 ip_pcbopts __P((struct mbuf **, struct mbuf *));
struct mbuf *
	 ip_reass __P((struct ipqent *, struct ipq *));
struct in_ifaddr *
	 ip_rtaddr __P((struct in_addr));
void	 ip_savecontrol __P((struct inpcb *, struct mbuf **, struct ip *,
	   struct mbuf *));
int	 ip_setmoptions __P((int, struct ip_moptions **, struct mbuf *));
void	 ip_slowtimo __P((void));
struct mbuf *
	 ip_srcroute __P((void));
void	 ip_stripoptions __P((struct mbuf *, struct mbuf *));
int	 ip_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
void	 ipintr __P((void));
int	 rip_ctloutput __P((int, struct socket *, int, int, struct mbuf **));
void	 rip_init __P((void));
void	 rip_input __P((struct mbuf *, ...));
int	 rip_output __P((struct mbuf *, ...));
int	 rip_usrreq __P((struct socket *,
	    int, struct mbuf *, struct mbuf *, struct mbuf *, struct proc *));
void	ipflow_init __P((void));
struct	ipflow *ipflow_reap __P((int));
void	ipflow_create __P((const struct route *, struct mbuf *));
void	ipflow_slowtimo __P((void));
#endif

#endif /* _NETINET_IP_VAR_H_ */
