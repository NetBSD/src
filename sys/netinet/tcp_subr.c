/*	$NetBSD: tcp_subr.c,v 1.81.2.4 2001/01/05 17:36:55 bouyer Exp $	*/

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

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Kevin M. Lahey of the Numerical Aerospace Simulation
 * Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
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
 *	@(#)tcp_subr.c	8.2 (Berkeley) 5/24/95
 */

#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_tcp_compat_42.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6protosw.h>
#include <netinet/icmp6.h>
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /*IPSEC*/

#ifdef INET6
struct in6pcb tcb6;
#endif

/* patchable/settable parameters for tcp */
int 	tcp_mssdflt = TCP_MSS;
int 	tcp_rttdflt = TCPTV_SRTTDFLT / PR_SLOWHZ;
int	tcp_do_rfc1323 = 1;	/* window scaling / timestamps (obsolete) */
int	tcp_do_sack = 1;	/* selective acknowledgement */
int	tcp_do_win_scale = 1;	/* RFC1323 window scaling */
int	tcp_do_timestamps = 1;	/* RFC1323 timestamps */
int	tcp_do_newreno = 0;	/* Use the New Reno algorithms */
int	tcp_ack_on_push = 0;	/* set to enable immediate ACK-on-PUSH */
int	tcp_init_win = 1;
int	tcp_mss_ifmtu = 0;
#ifdef TCP_COMPAT_42
int	tcp_compat_42 = 1;
#else
int	tcp_compat_42 = 0;
#endif
int	tcp_rst_ppslim = 100;	/* 100pps */

/* tcb hash */
#ifndef TCBHASHSIZE
#define	TCBHASHSIZE	128
#endif
int	tcbhashsize = TCBHASHSIZE;

/* syn hash parameters */
#define	TCP_SYN_HASH_SIZE	293
#define	TCP_SYN_BUCKET_SIZE	35
int	tcp_syn_cache_size = TCP_SYN_HASH_SIZE;
int	tcp_syn_cache_limit = TCP_SYN_HASH_SIZE*TCP_SYN_BUCKET_SIZE;
int	tcp_syn_bucket_limit = 3*TCP_SYN_BUCKET_SIZE;
struct	syn_cache_head tcp_syn_cache[TCP_SYN_HASH_SIZE];
int	tcp_syn_cache_interval = 1;	/* runs timer twice a second */

int	tcp_freeq __P((struct tcpcb *));

#ifdef INET
void	tcp_mtudisc_callback __P((struct in_addr));
#endif
#ifdef INET6
void	tcp6_mtudisc_callback __P((struct in6_addr *));
#endif

void	tcp_mtudisc __P((struct inpcb *, int));
#ifdef INET6
void	tcp6_mtudisc __P((struct in6pcb *, int));
#endif

struct pool tcpcb_pool;

/*
 * Tcp initialization
 */
void
tcp_init()
{
	int hlen;

	pool_init(&tcpcb_pool, sizeof(struct tcpcb), 0, 0, 0, "tcpcbpl",
	    0, NULL, NULL, M_PCB);
	in_pcbinit(&tcbtable, tcbhashsize, tcbhashsize);
#ifdef INET6
	tcb6.in6p_next = tcb6.in6p_prev = &tcb6;
#endif
	LIST_INIT(&tcp_delacks);

	hlen = sizeof(struct ip) + sizeof(struct tcphdr);
#ifdef INET6
	if (sizeof(struct ip) < sizeof(struct ip6_hdr))
		hlen = sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
#endif
	if (max_protohdr < hlen)
		max_protohdr = hlen;
	if (max_linkhdr + hlen > MHLEN)
		panic("tcp_init");

#ifdef INET
	icmp_mtudisc_callback_register(tcp_mtudisc_callback);
#endif
#ifdef INET6
	icmp6_mtudisc_callback_register(tcp6_mtudisc_callback);
#endif

	/* Initialize the compressed state engine. */
	syn_cache_init();
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct mbuf *
tcp_template(tp)
	struct tcpcb *tp;
{
	struct inpcb *inp = tp->t_inpcb;
#ifdef INET6
	struct in6pcb *in6p = tp->t_in6pcb;
#endif
	struct tcphdr *n;
	struct mbuf *m;
	int hlen;

	switch (tp->t_family) {
	case AF_INET:
		hlen = sizeof(struct ip);
		if (inp)
			break;
#ifdef INET6
		if (in6p) {
			/* mapped addr case */
			if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)
			 && IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr))
				break;
		}
#endif
		return NULL;	/*EINVAL*/
#ifdef INET6
	case AF_INET6:
		hlen = sizeof(struct ip6_hdr);
		if (in6p) {
			/* more sainty check? */
			break;
		}
		return NULL;	/*EINVAL*/
#endif
	default:
		hlen = 0;	/*pacify gcc*/
		return NULL;	/*EAFNOSUPPORT*/
	}
#ifdef DIAGNOSTIC
	if (hlen + sizeof(struct tcphdr) > MCLBYTES)
		panic("mclbytes too small for t_template");
#endif
	m = tp->t_template;
	if (m && m->m_len == hlen + sizeof(struct tcphdr))
		;
	else {
		if (m)
			m_freem(m);
		m = tp->t_template = NULL;
		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m && hlen + sizeof(struct tcphdr) > MHLEN) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_free(m);
				m = NULL;
			}
		}
		if (m == NULL)
			return NULL;
		m->m_pkthdr.len = m->m_len = hlen + sizeof(struct tcphdr);
	}
	bzero(mtod(m, caddr_t), m->m_len);
	switch (tp->t_family) {
	case AF_INET:
	    {
		struct ipovly *ipov;
		mtod(m, struct ip *)->ip_v = 4;
		ipov = mtod(m, struct ipovly *);
		ipov->ih_pr = IPPROTO_TCP;
		ipov->ih_len = htons(sizeof(struct tcphdr));
		if (inp) {
			ipov->ih_src = inp->inp_laddr;
			ipov->ih_dst = inp->inp_faddr;
		}
#ifdef INET6
		else if (in6p) {
			/* mapped addr case */
			bcopy(&in6p->in6p_laddr.s6_addr32[3], &ipov->ih_src,
				sizeof(ipov->ih_src));
			bcopy(&in6p->in6p_faddr.s6_addr32[3], &ipov->ih_dst,
				sizeof(ipov->ih_dst));
		}
#endif
		break;
	    }
#ifdef INET6
	case AF_INET6:
	    {
		struct ip6_hdr *ip6;
		mtod(m, struct ip *)->ip_v = 6;
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_nxt = IPPROTO_TCP;
		ip6->ip6_plen = htons(sizeof(struct tcphdr));
		ip6->ip6_src = in6p->in6p_laddr;
		ip6->ip6_dst = in6p->in6p_faddr;
		ip6->ip6_flow = in6p->in6p_flowinfo & IPV6_FLOWINFO_MASK;
		if (ip6_auto_flowlabel) {
			ip6->ip6_flow &= ~IPV6_FLOWLABEL_MASK;
			ip6->ip6_flow |= 
				(htonl(ip6_flow_seq++) & IPV6_FLOWLABEL_MASK);
		}
		ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6->ip6_vfc |= IPV6_VERSION;
		break;
	    }
#endif
	}
	n = (struct tcphdr *)(mtod(m, caddr_t) + hlen);
	if (inp) {
		n->th_sport = inp->inp_lport;
		n->th_dport = inp->inp_fport;
	}
#ifdef INET6
	else if (in6p) {
		n->th_sport = in6p->in6p_lport;
		n->th_dport = in6p->in6p_fport;
	}
#endif
	n->th_seq = 0;
	n->th_ack = 0;
	n->th_x2 = 0;
	n->th_off = 5;
	n->th_flags = 0;
	n->th_win = 0;
	n->th_sum = 0;
	n->th_urp = 0;
	return (m);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If m == 0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
int
tcp_respond(tp, template, m, th0, ack, seq, flags)
	struct tcpcb *tp;
	struct mbuf *template;
	struct mbuf *m;
	struct tcphdr *th0;
	tcp_seq ack, seq;
	int flags;
{
	struct route *ro;
	int error, tlen, win = 0;
	int hlen;
	struct ip *ip;
#ifdef INET6
	struct ip6_hdr *ip6;
#endif
	int family;	/* family on packet, not inpcb/in6pcb! */
	struct tcphdr *th;

	if (tp != NULL && (flags & TH_RST) == 0) {
#ifdef DIAGNOSTIC
		if (tp->t_inpcb && tp->t_in6pcb)
			panic("tcp_respond: both t_inpcb and t_in6pcb are set");
#endif
#ifdef INET
		if (tp->t_inpcb)
			win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
#endif
#ifdef INET6
		if (tp->t_in6pcb)
			win = sbspace(&tp->t_in6pcb->in6p_socket->so_rcv);
#endif
	}

	ip = NULL;
#ifdef INET6
	ip6 = NULL;
#endif
	if (m == 0) {
		if (!template)
			return EINVAL;

		/* get family information from template */
		switch (mtod(template, struct ip *)->ip_v) {
		case 4:
			family = AF_INET;
			hlen = sizeof(struct ip);
			break;
#ifdef INET6
		case 6:
			family = AF_INET6;
			hlen = sizeof(struct ip6_hdr);
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}

		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_free(m);
				m = NULL;
			}
		}
		if (m == NULL)
			return (ENOBUFS);

		if (tcp_compat_42)
			tlen = 1;
		else
			tlen = 0;

		m->m_data += max_linkhdr;
		bcopy(mtod(template, caddr_t), mtod(m, caddr_t),
			template->m_len);
		switch (family) {
		case AF_INET:
			ip = mtod(m, struct ip *);
			th = (struct tcphdr *)(ip + 1);
			break;
#ifdef INET6
		case AF_INET6:
			ip6 = mtod(m, struct ip6_hdr *);
			th = (struct tcphdr *)(ip6 + 1);
			break;
#endif
#if 0
		default:
			/* noone will visit here */
			m_freem(m);
			return EAFNOSUPPORT;
#endif
		}
		flags = TH_ACK;
	} else {

		if ((m->m_flags & M_PKTHDR) == 0) {
#if 0
			printf("non PKTHDR to tcp_respond\n");
#endif
			m_freem(m);
			return EINVAL;
		}
#ifdef DIAGNOSTIC
		if (!th0)
			panic("th0 == NULL in tcp_respond");
#endif

		/* get family information from m */
		switch (mtod(m, struct ip *)->ip_v) {
		case 4:
			family = AF_INET;
			hlen = sizeof(struct ip);
			ip = mtod(m, struct ip *);
			break;
#ifdef INET6
		case 6:
			family = AF_INET6;
			hlen = sizeof(struct ip6_hdr);
			ip6 = mtod(m, struct ip6_hdr *);
			break;
#endif
		default:
			m_freem(m);
			return EAFNOSUPPORT;
		}
		if ((flags & TH_SYN) == 0 || sizeof(*th0) > (th0->th_off << 2))
			tlen = sizeof(*th0);
		else
			tlen = th0->th_off << 2;

		if (m->m_len > hlen + tlen && (m->m_flags & M_EXT) == 0 &&
		    mtod(m, caddr_t) + hlen == (caddr_t)th0) {
			m->m_len = hlen + tlen;
			m_freem(m->m_next);
			m->m_next = NULL;
		} else {
			struct mbuf *n;

#ifdef DIAGNOSTIC
			if (max_linkhdr + hlen + tlen > MCLBYTES) {
				m_freem(m);
				return EMSGSIZE;
			}
#endif
			MGETHDR(n, M_DONTWAIT, MT_HEADER);
			if (n && max_linkhdr + hlen + tlen > MHLEN) {
				MCLGET(n, M_DONTWAIT);
				if ((n->m_flags & M_EXT) == 0) {
					m_freem(n);
					n = NULL;
				}
			}
			if (!n) {
				m_freem(m);
				return ENOBUFS;
			}

			n->m_data += max_linkhdr;
			n->m_len = hlen + tlen;
			m_copyback(n, 0, hlen, mtod(m, caddr_t));
			m_copyback(n, hlen, tlen, (caddr_t)th0);

			m_freem(m);
			m = n;
			n = NULL;
		}

#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		switch (family) {
		case AF_INET:
			ip = mtod(m, struct ip *);
			th = (struct tcphdr *)(ip + 1);
			ip->ip_p = IPPROTO_TCP;
			xchg(ip->ip_dst, ip->ip_src, struct in_addr);
			ip->ip_p = IPPROTO_TCP;
			break;
#ifdef INET6
		case AF_INET6:
			ip6 = mtod(m, struct ip6_hdr *);
			th = (struct tcphdr *)(ip6 + 1);
			ip6->ip6_nxt = IPPROTO_TCP;
			xchg(ip6->ip6_dst, ip6->ip6_src, struct in6_addr);
			ip6->ip6_nxt = IPPROTO_TCP;
			break;
#endif
#if 0
		default:
			/* noone will visit here */
			m_freem(m);
			return EAFNOSUPPORT;
#endif
		}
		xchg(th->th_dport, th->th_sport, u_int16_t);
#undef xchg
		tlen = 0;	/*be friendly with the following code*/
	}
	th->th_seq = htonl(seq);
	th->th_ack = htonl(ack);
	th->th_x2 = 0;
	if ((flags & TH_SYN) == 0) {
		if (tp)
			win >>= tp->rcv_scale;
		if (win > TCP_MAXWIN)
			win = TCP_MAXWIN;
		th->th_win = htons((u_int16_t)win);
		th->th_off = sizeof (struct tcphdr) >> 2;
		tlen += sizeof(*th);
	} else
		tlen += th->th_off << 2;
	m->m_len = hlen + tlen;
	m->m_pkthdr.len = hlen + tlen;
	m->m_pkthdr.rcvif = (struct ifnet *) 0;
	th->th_flags = flags;
	th->th_urp = 0;

	switch (family) {
#ifdef INET
	case AF_INET:
	    {
		struct ipovly *ipov = (struct ipovly *)ip;
		bzero(ipov->ih_x1, sizeof ipov->ih_x1);
		ipov->ih_len = htons((u_int16_t)tlen);

		th->th_sum = 0;
		th->th_sum = in_cksum(m, hlen + tlen);
		ip->ip_len = hlen + tlen;	/*will be flipped on output*/
		ip->ip_ttl = ip_defttl;
		break;
	    }
#endif
#ifdef INET6
	case AF_INET6:
	    {
		th->th_sum = 0;
		th->th_sum = in6_cksum(m, IPPROTO_TCP, sizeof(struct ip6_hdr),
				tlen);
		ip6->ip6_plen = ntohs(tlen);
		if (tp && tp->t_in6pcb) {
			struct ifnet *oifp;
			ro = (struct route *)&tp->t_in6pcb->in6p_route;
			oifp = ro->ro_rt ? ro->ro_rt->rt_ifp : NULL;
			ip6->ip6_hlim = in6_selecthlim(tp->t_in6pcb, oifp);
		} else
			ip6->ip6_hlim = ip6_defhlim;
		ip6->ip6_flow &= ~IPV6_FLOWINFO_MASK;
		if (ip6_auto_flowlabel) {
			ip6->ip6_flow |= 
				(htonl(ip6_flow_seq++) & IPV6_FLOWLABEL_MASK);
		}
		break;
	    }
#endif
	}

#ifdef IPSEC
	ipsec_setsocket(m, NULL);
#endif /*IPSEC*/

	if (tp != NULL && tp->t_inpcb != NULL) {
		ro = &tp->t_inpcb->inp_route;
#ifdef IPSEC
		ipsec_setsocket(m, tp->t_inpcb->inp_socket);
#endif
#ifdef DIAGNOSTIC
		if (family != AF_INET)
			panic("tcp_respond: address family mismatch");
		if (!in_hosteq(ip->ip_dst, tp->t_inpcb->inp_faddr)) {
			panic("tcp_respond: ip_dst %x != inp_faddr %x",
			    ntohl(ip->ip_dst.s_addr),
			    ntohl(tp->t_inpcb->inp_faddr.s_addr));
		}
#endif
	}
#ifdef INET6
	else if (tp != NULL && tp->t_in6pcb != NULL) {
		ro = (struct route *)&tp->t_in6pcb->in6p_route;
#ifdef IPSEC
		ipsec_setsocket(m, tp->t_in6pcb->in6p_socket);
#endif
#ifdef DIAGNOSTIC
		if (family == AF_INET) {
			if (!IN6_IS_ADDR_V4MAPPED(&tp->t_in6pcb->in6p_faddr))
				panic("tcp_respond: not mapped addr");
			if (bcmp(&ip->ip_dst,
					&tp->t_in6pcb->in6p_faddr.s6_addr32[3],
					sizeof(ip->ip_dst)) != 0) {
				panic("tcp_respond: ip_dst != in6p_faddr");
			}
		} else if (family == AF_INET6) {
			if (!IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &tp->t_in6pcb->in6p_faddr))
				panic("tcp_respond: ip6_dst != in6p_faddr");
		} else
			panic("tcp_respond: address family mismatch");
#endif
	}
#endif
	else
		ro = NULL;

	switch (family) {
#ifdef INET
	case AF_INET:
		error = ip_output(m, NULL, ro,
		    (ip_mtudisc ? IP_MTUDISC : 0),
		    NULL);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = ip6_output(m, NULL, (struct route_in6 *)ro, 0, NULL,
			NULL);
		break;
#endif
	default:
		error = EAFNOSUPPORT;
		break;
	}

	return (error);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
struct tcpcb *
tcp_newtcpcb(family, aux)
	int family;	/* selects inpcb, or in6pcb */
	void *aux;
{
	struct tcpcb *tp;

	switch (family) {
	case PF_INET:
		break;
#ifdef INET6
	case PF_INET6:
		break;
#endif
	default:
		return NULL;
	}

	tp = pool_get(&tcpcb_pool, PR_NOWAIT);
	if (tp == NULL)
		return (NULL);
	bzero((caddr_t)tp, sizeof(struct tcpcb));
	LIST_INIT(&tp->segq);
	LIST_INIT(&tp->timeq);
	tp->t_family = family;		/* may be overridden later on */
	tp->t_peermss = tcp_mssdflt;
	tp->t_ourmss = tcp_mssdflt;
	tp->t_segsz = tcp_mssdflt;
	LIST_INIT(&tp->t_sc);

	tp->t_flags = 0;
	if (tcp_do_rfc1323 && tcp_do_win_scale)
		tp->t_flags |= TF_REQ_SCALE;
	if (tcp_do_rfc1323 && tcp_do_timestamps)
		tp->t_flags |= TF_REQ_TSTMP;
	if (tcp_do_sack == 2)
		tp->t_flags |= TF_WILL_SACK;
	else if (tcp_do_sack == 1)
		tp->t_flags |= TF_WILL_SACK|TF_IGNR_RXSACK;
	tp->t_flags |= TF_CANT_TXSACK;
	switch (family) {
	case PF_INET:
		tp->t_inpcb = (struct inpcb *)aux;
		break;
#ifdef INET6
	case PF_INET6:
		tp->t_in6pcb = (struct in6pcb *)aux;
		break;
#endif
	}
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = tcp_rttdflt * PR_SLOWHZ << (TCP_RTTVAR_SHIFT + 2 - 1);
	tp->t_rttmin = TCPTV_MIN;
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	if (family == AF_INET) {
		struct inpcb *inp = (struct inpcb *)aux;
		inp->inp_ip.ip_ttl = ip_defttl;
		inp->inp_ppcb = (caddr_t)tp;
	}
#ifdef INET6
	else if (family == AF_INET6) {
		struct in6pcb *in6p = (struct in6pcb *)aux;
		in6p->in6p_ip6.ip6_hlim = in6_selecthlim(in6p,
			in6p->in6p_route.ro_rt ? in6p->in6p_route.ro_rt->rt_ifp
					       : NULL);
		in6p->in6p_ppcb = (caddr_t)tp;
	}
#endif
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, errno)
	struct tcpcb *tp;
	int errno;
{
	struct socket *so = NULL;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_drop: both t_inpcb and t_in6pcb are set");
#endif
#ifdef INET
	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif
	if (!so)
		return NULL;

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		tcpstat.tcps_drops++;
	} else
		tcpstat.tcps_conndrops++;
	if (errno == ETIMEDOUT && tp->t_softerror)
		errno = tp->t_softerror;
	so->so_error = errno;
	return (tcp_close(tp));
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct tcpcb *
tcp_close(tp)
	struct tcpcb *tp;
{
	struct inpcb *inp;
#ifdef INET6
	struct in6pcb *in6p;
#endif
	struct socket *so;
#ifdef RTV_RTT
	struct rtentry *rt;
#endif
	struct route *ro;

	inp = tp->t_inpcb;
#ifdef INET6
	in6p = tp->t_in6pcb;
#endif
	so = NULL;
	ro = NULL;
	if (inp) {
		so = inp->inp_socket;
		ro = &inp->inp_route;
	}
#ifdef INET6
	else if (in6p) {
		so = in6p->in6p_socket;
		ro = (struct route *)&in6p->in6p_route;
	}
#endif

#ifdef RTV_RTT
	/*
	 * If we sent enough data to get some meaningful characteristics,
	 * save them in the routing entry.  'Enough' is arbitrarily 
	 * defined as the sendpipesize (default 4K) * 16.  This would
	 * give us 16 rtt samples assuming we only get one sample per
	 * window (the usual case on a long haul net).  16 samples is
	 * enough for the srtt filter to converge to within 5% of the correct
	 * value; fewer samples and we could save a very bogus rtt.
	 *
	 * Don't update the default route's characteristics and don't
	 * update anything that the user "locked".
	 */
	if (SEQ_LT(tp->iss + so->so_snd.sb_hiwat * 16, tp->snd_max) &&
	    ro && (rt = ro->ro_rt) &&
	    !in_nullhost(satosin(rt_key(rt))->sin_addr)) {
		u_long i = 0;

		if ((rt->rt_rmx.rmx_locks & RTV_RTT) == 0) {
			i = tp->t_srtt *
			    ((RTM_RTTUNIT / PR_SLOWHZ) >> (TCP_RTT_SHIFT + 2));
			if (rt->rt_rmx.rmx_rtt && i)
				/*
				 * filter this update to half the old & half
				 * the new values, converting scale.
				 * See route.h and tcp_var.h for a
				 * description of the scaling constants.
				 */
				rt->rt_rmx.rmx_rtt =
				    (rt->rt_rmx.rmx_rtt + i) / 2;
			else
				rt->rt_rmx.rmx_rtt = i;
		}
		if ((rt->rt_rmx.rmx_locks & RTV_RTTVAR) == 0) {
			i = tp->t_rttvar *
			    ((RTM_RTTUNIT / PR_SLOWHZ) >> (TCP_RTTVAR_SHIFT + 2));
			if (rt->rt_rmx.rmx_rttvar && i)
				rt->rt_rmx.rmx_rttvar =
				    (rt->rt_rmx.rmx_rttvar + i) / 2;
			else
				rt->rt_rmx.rmx_rttvar = i;
		}
		/*
		 * update the pipelimit (ssthresh) if it has been updated
		 * already or if a pipesize was specified & the threshhold
		 * got below half the pipesize.  I.e., wait for bad news
		 * before we start updating, then update on both good
		 * and bad news.
		 */
		if (((rt->rt_rmx.rmx_locks & RTV_SSTHRESH) == 0 &&
		    (i = tp->snd_ssthresh) && rt->rt_rmx.rmx_ssthresh) ||
		    i < (rt->rt_rmx.rmx_sendpipe / 2)) {
			/*
			 * convert the limit from user data bytes to
			 * packets then to packet data bytes.
			 */
			i = (i + tp->t_segsz / 2) / tp->t_segsz;
			if (i < 2)
				i = 2;
			i *= (u_long)(tp->t_segsz + sizeof (struct tcpiphdr));
			if (rt->rt_rmx.rmx_ssthresh)
				rt->rt_rmx.rmx_ssthresh =
				    (rt->rt_rmx.rmx_ssthresh + i) / 2;
			else
				rt->rt_rmx.rmx_ssthresh = i;
		}
	}
#endif /* RTV_RTT */
	/* free the reassembly queue, if any */
	TCP_REASS_LOCK(tp);
	(void) tcp_freeq(tp);
	TCP_REASS_UNLOCK(tp);

	TCP_CLEAR_DELACK(tp);
	syn_cache_cleanup(tp);

	if (tp->t_template) {
		m_free(tp->t_template);
		tp->t_template = NULL;
	}
	pool_put(&tcpcb_pool, tp);
	if (inp) {
		inp->inp_ppcb = 0;
		soisdisconnected(so);
		in_pcbdetach(inp);
	}
#ifdef INET6
	else if (in6p) {
		in6p->in6p_ppcb = 0;
		soisdisconnected(so);
		in6_pcbdetach(in6p);
	}
#endif
	tcpstat.tcps_closed++;
	return ((struct tcpcb *)0);
}

int
tcp_freeq(tp)
	struct tcpcb *tp;
{
	struct ipqent *qe;
	int rv = 0;
#ifdef TCPREASS_DEBUG
	int i = 0;
#endif

	TCP_REASS_LOCK_CHECK(tp);

	while ((qe = tp->segq.lh_first) != NULL) {
#ifdef TCPREASS_DEBUG
		printf("tcp_freeq[%p,%d]: %u:%u(%u) 0x%02x\n",
			tp, i++, qe->ipqe_seq, qe->ipqe_seq + qe->ipqe_len,
			qe->ipqe_len, qe->ipqe_flags & (TH_SYN|TH_FIN|TH_RST));
#endif
		LIST_REMOVE(qe, ipqe_q);
		LIST_REMOVE(qe, ipqe_timeq);
		m_freem(qe->ipqe_m);
		pool_put(&ipqent_pool, qe);
		rv = 1;
	}
	return (rv);
}

/*
 * Protocol drain routine.  Called when memory is in short supply.
 */
void
tcp_drain()
{
	struct inpcb *inp;
	struct tcpcb *tp;

	/*
	 * Free the sequence queue of all TCP connections.
	 */
	inp = tcbtable.inpt_queue.cqh_first;
	if (inp)						/* XXX */
	for (; inp != (struct inpcb *)&tcbtable.inpt_queue;
	    inp = inp->inp_queue.cqe_next) {
		if ((tp = intotcpcb(inp)) != NULL) {
			/*
			 * We may be called from a device's interrupt
			 * context.  If the tcpcb is already busy,
			 * just bail out now.
			 */
			if (tcp_reass_lock_try(tp) == 0)
				continue;
			if (tcp_freeq(tp))
				tcpstat.tcps_connsdrained++;
			TCP_REASS_UNLOCK(tp);
		}
	}
}

/*
 * Notify a tcp user of an asynchronous error;
 * store error as soft error, but wake up user
 * (for now, won't do anything until can select for soft error).
 */
void
tcp_notify(inp, error)
	struct inpcb *inp;
	int error;
{
	struct tcpcb *tp = (struct tcpcb *)inp->inp_ppcb;
	struct socket *so = inp->inp_socket;

	/*
	 * Ignore some errors if we are hooked up.
	 * If connection hasn't completed, has retransmitted several times,
	 * and receives a second error, give up now.  This is better
	 * than waiting a long time to establish a connection that
	 * can never complete.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	     (error == EHOSTUNREACH || error == ENETUNREACH ||
	      error == EHOSTDOWN)) {
		return;
	} else if (TCPS_HAVEESTABLISHED(tp->t_state) == 0 &&
	    tp->t_rxtshift > 3 && tp->t_softerror)
		so->so_error = error;
	else 
		tp->t_softerror = error;
	wakeup((caddr_t) &so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}

#ifdef INET6
void
tcp6_notify(in6p, error)
	struct in6pcb *in6p;
	int error;
{
	struct tcpcb *tp = (struct tcpcb *)in6p->in6p_ppcb;
	struct socket *so = in6p->in6p_socket;

	/*
	 * Ignore some errors if we are hooked up.
	 * If connection hasn't completed, has retransmitted several times,
	 * and receives a second error, give up now.  This is better
	 * than waiting a long time to establish a connection that
	 * can never complete.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	     (error == EHOSTUNREACH || error == ENETUNREACH ||
	      error == EHOSTDOWN)) {
		return;
	} else if (TCPS_HAVEESTABLISHED(tp->t_state) == 0 &&
	    tp->t_rxtshift > 3 && tp->t_softerror)
		so->so_error = error;
	else 
		tp->t_softerror = error;
	wakeup((caddr_t) &so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}
#endif

#ifdef INET6
void
tcp6_ctlinput(cmd, sa, d)
	int cmd;
	struct sockaddr *sa;
	void *d;
{
	struct tcphdr *thp;
	struct tcphdr th;
	void (*notify) __P((struct in6pcb *, int)) = tcp6_notify;
	int nmatch;
	struct sockaddr_in6 sa6;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
	struct in6_addr finaldst;
	struct in6_addr s;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;
	if ((unsigned)cmd >= PRC_NCMDS)
		return;
	else if (cmd == PRC_QUENCH) {
		/* XXX there's no PRC_QUENCH in IPv6 */
		notify = tcp6_quench;
	} else if (PRC_IS_REDIRECT(cmd))
		notify = in6_rtchange, d = NULL;
	else if (cmd == PRC_MSGSIZE)
		; /* special code is present, see below */
	else if (cmd == PRC_HOSTDEAD)
		d = NULL;
	else if (inet6ctlerrmap[cmd] == 0)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		struct ip6ctlparam *ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;

		/* translate addresses into internal form */
		bcopy(ip6cp->ip6c_finaldst, &finaldst, sizeof(finaldst));
		if (IN6_IS_ADDR_LINKLOCAL(&finaldst)) {
			finaldst.s6_addr16[1] =
			    htons(m->m_pkthdr.rcvif->if_index);
		}
		bcopy(&ip6->ip6_src, &s, sizeof(s));
		if (IN6_IS_ADDR_LINKLOCAL(&s))
			s.s6_addr16[1] = htons(m->m_pkthdr.rcvif->if_index);
	} else {
		m = NULL;
		ip6 = NULL;
	}

	/* translate addresses into internal form */
	sa6 = *(struct sockaddr_in6 *)sa;
	if (IN6_IS_ADDR_LINKLOCAL(&sa6.sin6_addr) && m && m->m_pkthdr.rcvif)
		sa6.sin6_addr.s6_addr16[1] = htons(m->m_pkthdr.rcvif->if_index);

	if (ip6) {
		/*
		 * XXX: We assume that when ip6 is non NULL,
		 * M and OFF are valid.
		 */

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(th))
			return;

		if (m->m_len < off + sizeof(th)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(th), (caddr_t)&th);
			thp = &th;
		} else
			thp = (struct tcphdr *)(mtod(m, caddr_t) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid TCP connection
			 * corresponding to the address in the ICMPv6 message
			 * payload.
			 */
			if (in6_pcblookup_connect(&tcb6, &finaldst,
			    thp->th_dport, &s, thp->th_sport, 0))
				valid++;

			/*
			 * Now that we've validated that we are actually
			 * communicating with the host indicated in the ICMPv6
			 * message, recalculate the new MTU, and create the
			 * corresponding routing entry.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);

			return;
		}

		nmatch = in6_pcbnotify(&tcb6, (struct sockaddr *)&sa6,
		    thp->th_dport, &s, thp->th_sport, cmd, notify);
		if (nmatch == 0 && syn_cache_count &&
		    (inet6ctlerrmap[cmd] == EHOSTUNREACH ||
		     inet6ctlerrmap[cmd] == ENETUNREACH ||
		     inet6ctlerrmap[cmd] == EHOSTDOWN)) {
			struct sockaddr_in6 sin6;
			bzero(&sin6, sizeof(sin6));
			sin6.sin6_len = sizeof(sin6);
			sin6.sin6_family = AF_INET6;
			sin6.sin6_port = thp->th_sport;
			sin6.sin6_addr = s;
			syn_cache_unreach((struct sockaddr *)&sin6, sa, thp);
		}
	} else {
		(void) in6_pcbnotify(&tcb6, (struct sockaddr *)&sa6, 0,
				     &zeroin6_addr, 0, cmd, notify);
	}
}
#endif

#ifdef INET
/* assumes that ip header and tcp header are contiguous on mbuf */
void *
tcp_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	struct tcphdr *th;
	struct icmp *icp;
	extern int inetctlerrmap[];
	void (*notify) __P((struct inpcb *, int)) = tcp_notify;
	int errno;
	int nmatch;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	errno = inetctlerrmap[cmd];
	if (cmd == PRC_QUENCH)
		notify = tcp_quench;
	else if (PRC_IS_REDIRECT(cmd))
		notify = in_rtchange, ip = 0;
	else if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid TCP connection
		 * corresponding to the address in the ICMP message
		 * payload.
		 */
		th = (struct tcphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		if (in_pcblookup_connect(&tcbtable,
					 ip->ip_dst, th->th_dport,
					 ip->ip_src, th->th_sport) == NULL)
			return NULL;

		/*
		 * Now that we've validated that we are actually communicating
		 * with the host indicated in the ICMP message, locate the
		 * ICMP header, recalculate the new MTU, and create the
		 * corresponding routing entry.
		 */
		icp = (struct icmp *)((caddr_t)ip -
		    offsetof(struct icmp, icmp_ip));
		icmp_mtudisc(icp, ip->ip_dst);

		return NULL;
	} else if (cmd == PRC_HOSTDEAD)
		ip = 0;
	else if (errno == 0)
		return NULL;
	if (ip && ip->ip_v == 4 && sa->sa_family == AF_INET) {
		th = (struct tcphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		nmatch = in_pcbnotify(&tcbtable, satosin(sa)->sin_addr,
		    th->th_dport, ip->ip_src, th->th_sport, errno, notify);
		if (nmatch == 0 && syn_cache_count &&
		    (inetctlerrmap[cmd] == EHOSTUNREACH ||
		    inetctlerrmap[cmd] == ENETUNREACH ||
		    inetctlerrmap[cmd] == EHOSTDOWN)) {
			struct sockaddr_in sin;
			bzero(&sin, sizeof(sin));
			sin.sin_len = sizeof(sin);
			sin.sin_family = AF_INET;
			sin.sin_port = th->th_sport;
			sin.sin_addr = ip->ip_src;
			syn_cache_unreach((struct sockaddr *)&sin, sa, th);
		}

		/* XXX mapped address case */
	} else
		in_pcbnotifyall(&tcbtable, satosin(sa)->sin_addr, errno,
		    notify);
	return NULL;
}

/*
 * When a source quence is received, we are being notifed of congestion.
 * Close the congestion window down to the Loss Window (one segment).
 * We will gradually open it again as we proceed.
 */
void
tcp_quench(inp, errno)
	struct inpcb *inp;
	int errno;
{
	struct tcpcb *tp = intotcpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_segsz;
}
#endif

#ifdef INET6
void
tcp6_quench(in6p, errno)
	struct in6pcb *in6p;
	int errno;
{
	struct tcpcb *tp = in6totcpcb(in6p);

	if (tp)
		tp->snd_cwnd = tp->t_segsz;
}
#endif

#ifdef INET
/*
 * Path MTU Discovery handlers.
 */
void
tcp_mtudisc_callback(faddr)
	struct in_addr faddr;
{

	in_pcbnotifyall(&tcbtable, faddr, EMSGSIZE, tcp_mtudisc);
}

/*
 * On receipt of path MTU corrections, flush old route and replace it
 * with the new one.  Retransmit all unacknowledged packets, to ensure
 * that all packets will be received.
 */
void
tcp_mtudisc(inp, errno)
	struct inpcb *inp;
	int errno;
{
	struct tcpcb *tp = intotcpcb(inp);
	struct rtentry *rt = in_pcbrtentry(inp);

	if (tp != 0) {
		if (rt != 0) {
			/*
			 * If this was not a host route, remove and realloc.
			 */
			if ((rt->rt_flags & RTF_HOST) == 0) {
				in_rtchange(inp, errno);
				if ((rt = in_pcbrtentry(inp)) == 0)
					return;
			}

			/*
			 * Slow start out of the error condition.  We
			 * use the MTU because we know it's smaller
			 * than the previously transmitted segment.
			 *
			 * Note: This is more conservative than the
			 * suggestion in draft-floyd-incr-init-win-03.
			 */
			if (rt->rt_rmx.rmx_mtu != 0)
				tp->snd_cwnd =
				    TCP_INITIAL_WINDOW(tcp_init_win,
				    rt->rt_rmx.rmx_mtu);
		}
	    
		/*
		 * Resend unacknowledged packets.
		 */
		tp->snd_nxt = tp->snd_una;
		tcp_output(tp);
	}
}
#endif

#ifdef INET6
/*
 * Path MTU Discovery handlers.
 */
void
tcp6_mtudisc_callback(faddr)
	struct in6_addr *faddr;
{
	struct sockaddr_in6 sin6;

	bzero(&sin6, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_addr = *faddr;
	(void) in6_pcbnotify(&tcb6, (struct sockaddr *)&sin6, 0,
	    &zeroin6_addr, 0, PRC_MSGSIZE, tcp6_mtudisc);
}

void
tcp6_mtudisc(in6p, errno)
	struct in6pcb *in6p;
	int errno;
{
	struct tcpcb *tp = in6totcpcb(in6p);
	struct rtentry *rt = in6_pcbrtentry(in6p);

	if (tp != 0) {
		if (rt != 0) {
			/*
			 * If this was not a host route, remove and realloc.
			 */
			if ((rt->rt_flags & RTF_HOST) == 0) {
				in6_rtchange(in6p, errno);
				if ((rt = in6_pcbrtentry(in6p)) == 0)
					return;
			}

			/*
			 * Slow start out of the error condition.  We
			 * use the MTU because we know it's smaller
			 * than the previously transmitted segment.
			 *
			 * Note: This is more conservative than the
			 * suggestion in draft-floyd-incr-init-win-03.
			 */
			if (rt->rt_rmx.rmx_mtu != 0)
				tp->snd_cwnd =
				    TCP_INITIAL_WINDOW(tcp_init_win,
				    rt->rt_rmx.rmx_mtu);
		}

		/*
		 * Resend unacknowledged packets.
		 */
		tp->snd_nxt = tp->snd_una;
		tcp_output(tp);
	}
}
#endif /* INET6 */

/*
 * Compute the MSS to advertise to the peer.  Called only during
 * the 3-way handshake.  If we are the server (peer initiated
 * connection), we are called with a pointer to the interface
 * on which the SYN packet arrived.  If we are the client (we 
 * initiated connection), we are called with a pointer to the
 * interface out which this connection should go.
 *
 * NOTE: Do not subtract IP option/extension header size nor IPsec
 * header size from MSS advertisement.  MSS option must hold the maximum
 * segment size we can accept, so it must always be:
 *	 max(if mtu) - ip header - tcp header
 */
u_long
tcp_mss_to_advertise(ifp, af)
	const struct ifnet *ifp;
	int af;
{
	extern u_long in_maxmtu;
	u_long mss = 0;
	u_long hdrsiz;

	/*
	 * In order to avoid defeating path MTU discovery on the peer,
	 * we advertise the max MTU of all attached networks as our MSS,
	 * per RFC 1191, section 3.1.
	 *
	 * We provide the option to advertise just the MTU of
	 * the interface on which we hope this connection will
	 * be receiving.  If we are responding to a SYN, we
	 * will have a pretty good idea about this, but when
	 * initiating a connection there is a bit more doubt.
	 *
	 * We also need to ensure that loopback has a large enough
	 * MSS, as the loopback MTU is never included in in_maxmtu.
	 */

	if (ifp != NULL)
		mss = ifp->if_mtu;

	if (tcp_mss_ifmtu == 0)
		mss = max(in_maxmtu, mss);

	switch (af) {
	case AF_INET:
		hdrsiz = sizeof(struct ip);
		break;
#ifdef INET6
	case AF_INET6:
		hdrsiz = sizeof(struct ip6_hdr);
		break;
#endif
	default:
		hdrsiz = 0;
		break;
	}
	hdrsiz += sizeof(struct tcphdr);
	if (mss > hdrsiz)
		mss -= hdrsiz;

	mss = max(tcp_mssdflt, mss);
	return (mss);
}

/*
 * Set connection variables based on the peer's advertised MSS.
 * We are passed the TCPCB for the actual connection.  If we
 * are the server, we are called by the compressed state engine
 * when the 3-way handshake is complete.  If we are the client,
 * we are called when we recieve the SYN,ACK from the server.
 *
 * NOTE: Our advertised MSS value must be initialized in the TCPCB
 * before this routine is called!
 */
void
tcp_mss_from_peer(tp, offer)
	struct tcpcb *tp;
	int offer;
{
	struct socket *so;
#if defined(RTV_SPIPE) || defined(RTV_SSTHRESH)
	struct rtentry *rt;
#endif
	u_long bufsize;
	int mss;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_mss_from_peer: both t_inpcb and t_in6pcb are set");
#endif
	so = NULL;
	rt = NULL;
#ifdef INET
	if (tp->t_inpcb) {
		so = tp->t_inpcb->inp_socket;
#if defined(RTV_SPIPE) || defined(RTV_SSTHRESH)
		rt = in_pcbrtentry(tp->t_inpcb);
#endif
	}
#endif
#ifdef INET6
	if (tp->t_in6pcb) {
		so = tp->t_in6pcb->in6p_socket;
#if defined(RTV_SPIPE) || defined(RTV_SSTHRESH)
		rt = in6_pcbrtentry(tp->t_in6pcb);
#endif
	}
#endif

	/*
	 * As per RFC1122, use the default MSS value, unless they 
	 * sent us an offer.  Do not accept offers less than 32 bytes.
	 */
	mss = tcp_mssdflt;
	if (offer)
		mss = offer;
	mss = max(mss, 32);		/* sanity */
	tp->t_peermss = mss;
	mss -= tcp_optlen(tp);
#ifdef INET
	if (tp->t_inpcb)
		mss -= ip_optlen(tp->t_inpcb);
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		mss -= ip6_optlen(tp->t_in6pcb);
#endif

	/*
	 * If there's a pipesize, change the socket buffer to that size.
	 * Make the socket buffer an integral number of MSS units.  If
	 * the MSS is larger than the socket buffer, artificially decrease
	 * the MSS.
	 */
#ifdef RTV_SPIPE
	if (rt != NULL && rt->rt_rmx.rmx_sendpipe != 0)
		bufsize = rt->rt_rmx.rmx_sendpipe;
	else
#endif
		bufsize = so->so_snd.sb_hiwat;
	if (bufsize < mss)
		mss = bufsize;
	else {
		bufsize = roundup(bufsize, mss);
		if (bufsize > sb_max)
			bufsize = sb_max;
		(void) sbreserve(&so->so_snd, bufsize);
	}
	tp->t_segsz = mss;

#ifdef RTV_SSTHRESH
	if (rt != NULL && rt->rt_rmx.rmx_ssthresh) {
		/*
		 * There's some sort of gateway or interface buffer
		 * limit on the path.  Use this to set the slow
		 * start threshold, but set the threshold to no less
		 * than 2 * MSS.
		 */
		tp->snd_ssthresh = max(2 * mss, rt->rt_rmx.rmx_ssthresh);
	}
#endif
}

/*
 * Processing necessary when a TCP connection is established.
 */
void
tcp_established(tp)
	struct tcpcb *tp;
{
	struct socket *so;
#ifdef RTV_RPIPE
	struct rtentry *rt;
#endif
	u_long bufsize;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_established: both t_inpcb and t_in6pcb are set");
#endif
	so = NULL;
	rt = NULL;
#ifdef INET
	if (tp->t_inpcb) {
		so = tp->t_inpcb->inp_socket;
#if defined(RTV_RPIPE)
		rt = in_pcbrtentry(tp->t_inpcb);
#endif
	}
#endif
#ifdef INET6
	if (tp->t_in6pcb) {
		so = tp->t_in6pcb->in6p_socket;
#if defined(RTV_RPIPE)
		rt = in6_pcbrtentry(tp->t_in6pcb);
#endif
	}
#endif

	tp->t_state = TCPS_ESTABLISHED;
	TCP_TIMER_ARM(tp, TCPT_KEEP, tcp_keepidle);

#ifdef RTV_RPIPE
	if (rt != NULL && rt->rt_rmx.rmx_recvpipe != 0)
		bufsize = rt->rt_rmx.rmx_recvpipe;
	else
#endif
		bufsize = so->so_rcv.sb_hiwat;
	if (bufsize > tp->t_ourmss) {
		bufsize = roundup(bufsize, tp->t_ourmss);
		if (bufsize > sb_max)
			bufsize = sb_max;
		(void) sbreserve(&so->so_rcv, bufsize);
	}
}

/*
 * Check if there's an initial rtt or rttvar.  Convert from the
 * route-table units to scaled multiples of the slow timeout timer.
 * Called only during the 3-way handshake.
 */
void
tcp_rmx_rtt(tp)
	struct tcpcb *tp;
{
#ifdef RTV_RTT
	struct rtentry *rt = NULL;
	int rtt;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_rmx_rtt: both t_inpcb and t_in6pcb are set");
#endif
#ifdef INET
	if (tp->t_inpcb)
		rt = in_pcbrtentry(tp->t_inpcb);
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		rt = in6_pcbrtentry(tp->t_in6pcb);
#endif
	if (rt == NULL)
		return;

	if (tp->t_srtt == 0 && (rtt = rt->rt_rmx.rmx_rtt)) {
		/*
		 * XXX The lock bit for MTU indicates that the value
		 * is also a minimum value; this is subject to time.
		 */
		if (rt->rt_rmx.rmx_locks & RTV_RTT)
			TCPT_RANGESET(tp->t_rttmin,
			    rtt / (RTM_RTTUNIT / PR_SLOWHZ),
			    TCPTV_MIN, TCPTV_REXMTMAX);
		tp->t_srtt = rtt /
		    ((RTM_RTTUNIT / PR_SLOWHZ) >> (TCP_RTT_SHIFT + 2));
		if (rt->rt_rmx.rmx_rttvar) {
			tp->t_rttvar = rt->rt_rmx.rmx_rttvar /
			    ((RTM_RTTUNIT / PR_SLOWHZ) >>
				(TCP_RTTVAR_SHIFT + 2));
		} else {
			/* Default variation is +- 1 rtt */
			tp->t_rttvar =
			    tp->t_srtt >> (TCP_RTT_SHIFT - TCP_RTTVAR_SHIFT);
		}
		TCPT_RANGESET(tp->t_rxtcur,
		    ((tp->t_srtt >> 2) + tp->t_rttvar) >> (1 + 2),
		    tp->t_rttmin, TCPTV_REXMTMAX);
	}
#endif
}

tcp_seq	 tcp_iss_seq = 0;	/* tcp initial seq # */

/*
 * Get a new sequence value given a tcp control block
 */
tcp_seq
tcp_new_iss(tp, len, addin)
	void            *tp;
	u_long           len;
	tcp_seq		 addin;
{
	tcp_seq          tcp_iss;

	/*
	 * Randomize.
	 */
#if NRND > 0
	rnd_extract_data(&tcp_iss, sizeof(tcp_iss), RND_EXTRACT_ANY);
#else
	tcp_iss = random();
#endif

	/*
	 * If we were asked to add some amount to a known value,
	 * we will take a random value obtained above, mask off the upper
	 * bits, and add in the known value.  We also add in a constant to
	 * ensure that we are at least a certain distance from the original
	 * value.
	 *
	 * This is used when an old connection is in timed wait
	 * and we have a new one coming in, for instance.
	 */
	if (addin != 0) {
#ifdef TCPISS_DEBUG
		printf("Random %08x, ", tcp_iss);
#endif
		tcp_iss &= TCP_ISS_RANDOM_MASK;
		tcp_iss += addin + TCP_ISSINCR;
#ifdef TCPISS_DEBUG
		printf("Old ISS %08x, ISS %08x\n", addin, tcp_iss);
#endif
	} else {
		tcp_iss &= TCP_ISS_RANDOM_MASK;
		tcp_iss += tcp_iss_seq;
		tcp_iss_seq += TCP_ISSINCR;
#ifdef TCPISS_DEBUG
		printf("ISS %08x\n", tcp_iss);
#endif
	}

	if (tcp_compat_42) {
		/*
		 * Limit it to the positive range for really old TCP
		 * implementations.
		 */
		if (tcp_iss >= 0x80000000)
			tcp_iss &= 0x7fffffff;		/* XXX */
	}

	return tcp_iss;
}

#ifdef IPSEC
/* compute ESP/AH header size for TCP, including outer IP header. */
size_t
ipsec4_hdrsiz_tcp(tp)
	struct tcpcb *tp;
{
	struct inpcb *inp;
	size_t hdrsiz;

	/* XXX mapped addr case (tp->t_in6pcb) */
	if (!tp || !tp->t_template || !(inp = tp->t_inpcb))
		return 0;
	switch (tp->t_family) {
	case AF_INET:
		/* XXX: should use currect direction. */
		hdrsiz = ipsec4_hdrsiz(tp->t_template, IPSEC_DIR_OUTBOUND, inp);
		break;
	default:
		hdrsiz = 0;
		break;
	}

	return hdrsiz;
}

#ifdef INET6
size_t
ipsec6_hdrsiz_tcp(tp)
	struct tcpcb *tp;
{
	struct in6pcb *in6p;
	size_t hdrsiz;

	if (!tp || !tp->t_template || !(in6p = tp->t_in6pcb))
		return 0;
	switch (tp->t_family) {
	case AF_INET6:
		/* XXX: should use currect direction. */
		hdrsiz = ipsec6_hdrsiz(tp->t_template, IPSEC_DIR_OUTBOUND, in6p);
		break;
	case AF_INET:
		/* mapped address case - tricky */
	default:
		hdrsiz = 0;
		break;
	}

	return hdrsiz;
}
#endif
#endif /*IPSEC*/

/*
 * Determine the length of the TCP options for this connection.
 * 
 * XXX:  What do we do for SACK, when we add that?  Just reserve
 *       all of the space?  Otherwise we can't exactly be incrementing
 *       cwnd by an amount that varies depending on the amount we last
 *       had to SACK!
 */

u_int
tcp_optlen(tp)
	struct tcpcb *tp;
{
	if ((tp->t_flags & (TF_REQ_TSTMP|TF_RCVD_TSTMP|TF_NOOPT)) == 
	    (TF_REQ_TSTMP | TF_RCVD_TSTMP))
		return TCPOLEN_TSTAMP_APPA;
	else
		return 0;
}
