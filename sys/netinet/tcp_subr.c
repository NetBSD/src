/*	$NetBSD: tcp_subr.c,v 1.64 1999/01/20 03:39:54 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

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

#ifndef TCBHASHSIZE
#define	TCBHASHSIZE	128
#endif
int	tcbhashsize = TCBHASHSIZE;

int	tcp_freeq __P((struct tcpcb *));

struct pool tcpcb_pool;
struct pool tcp_template_pool;

/*
 * Tcp initialization
 */
void
tcp_init()
{

	pool_init(&tcpcb_pool, sizeof(struct tcpcb), 0, 0, 0, "tcpcbpl",
	    0, NULL, NULL, M_PCB);
	pool_init(&tcp_template_pool, sizeof(struct tcpiphdr), 0, 0, 0,
	    "tcptmpl", 0, NULL, NULL, M_MBUF);
	in_pcbinit(&tcbtable, tcbhashsize, tcbhashsize);
	LIST_INIT(&tcp_delacks);
	if (max_protohdr < sizeof(struct tcpiphdr))
		max_protohdr = sizeof(struct tcpiphdr);
	if (max_linkhdr + sizeof(struct tcpiphdr) > MHLEN)
		panic("tcp_init");
	
	/* Initialize the compressed state engine. */
	syn_cache_init();
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct tcpiphdr *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct tcpiphdr *n;

	if ((n = tp->t_template) == 0) {
		n = pool_get(&tcp_template_pool, PR_NOWAIT);
		if (n == NULL)
			return (NULL);
	}
	bzero(n->ti_x1, sizeof n->ti_x1);
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (n);
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
tcp_respond(tp, ti, m, ack, seq, flags)
	struct tcpcb *tp;
	register struct tcpiphdr *ti;
	register struct mbuf *m;
	tcp_seq ack, seq;
	int flags;
{
	struct route iproute, *ro;
	struct rtentry *rt;
	struct sockaddr_in *dst;
	int error, tlen, win = 0;

	if (tp != NULL) {
		if ((flags & TH_RST) == 0)
			win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		ro = &tp->t_inpcb->inp_route;
#ifdef DIAGNOSTIC
		if (!in_hosteq(ti->ti_dst, tp->t_inpcb->inp_faddr))
			panic("tcp_respond: ti_dst != inp_faddr");
#endif
	} else {
		ro = &iproute;
		bzero(ro, sizeof(*ro));
	}
	if (m == 0) {
		m = m_gethdr(M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			return (ENOBUFS);

		if (tcp_compat_42)
			tlen = 1;
		else
			tlen = 0;

		m->m_data += max_linkhdr;
		*mtod(m, struct tcpiphdr *) = *ti;
		ti = mtod(m, struct tcpiphdr *);
		flags = TH_ACK;
	} else {
		m_freem(m->m_next);
		m->m_next = 0;
		m->m_data = (caddr_t)ti;
		m->m_len = sizeof (struct tcpiphdr);
		tlen = 0;
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_int32_t);
		xchg(ti->ti_dport, ti->ti_sport, u_int16_t);
#undef xchg
	}
	bzero(ti->ti_x1, sizeof ti->ti_x1);
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	if ((flags & TH_SYN) == 0) {
		if (tp)
			ti->ti_win = htons((u_int16_t) (win >> tp->rcv_scale));
		else
			ti->ti_win = htons((u_int16_t)win);
		ti->ti_off = sizeof (struct tcphdr) >> 2;
		tlen += sizeof (struct tcphdr);
	} else
		tlen += ti->ti_off << 2;
	ti->ti_len = htons((u_int16_t)tlen);
	tlen += sizeof (struct ip);
	m->m_len = tlen;
	m->m_pkthdr.len = tlen;
	m->m_pkthdr.rcvif = (struct ifnet *) 0;
	ti->ti_flags = flags;
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(m, tlen);
	((struct ip *)ti)->ip_len = tlen;
	((struct ip *)ti)->ip_ttl = ip_defttl;

	/*
	 * If we're doing Path MTU discovery, we need to set DF unless
	 * the route's MTU is locked.  If we lack a route, we need to
	 * look it up now.
	 *
	 * ip_output() could do this for us, but it's convenient to just
	 * do it here unconditionally.
	 */
	if ((rt = ro->ro_rt) == NULL || (rt->rt_flags & RTF_UP) == 0) {
		if (ro->ro_rt != NULL) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = NULL;
		}
		dst = satosin(&ro->ro_dst);
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
		dst->sin_addr = ti->ti_dst;
		rtalloc(ro);
		if ((rt = ro->ro_rt) == NULL) {
			m_freem(m);
			ipstat.ips_noroute++;
			return (EHOSTUNREACH);
		}
	}
	if (ip_mtudisc != 0 && (rt->rt_rmx.rmx_locks & RTV_MTU) == 0)
		((struct ip *)ti)->ip_off |= IP_DF;

	error = ip_output(m, NULL, ro, 0, NULL);

	if (ro == &iproute) {
		RTFREE(ro->ro_rt);
		ro->ro_rt = NULL;
	}

	return (error);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
struct tcpcb *
tcp_newtcpcb(inp)
	struct inpcb *inp;
{
	register struct tcpcb *tp;

	tp = pool_get(&tcpcb_pool, PR_NOWAIT);
	if (tp == NULL)
		return (NULL);
	bzero((caddr_t)tp, sizeof(struct tcpcb));
	LIST_INIT(&tp->segq);
	LIST_INIT(&tp->timeq);
	tp->t_peermss = tcp_mssdflt;
	tp->t_ourmss = tcp_mssdflt;
	tp->t_segsz = tcp_mssdflt;

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
	tp->t_inpcb = inp;
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
	inp->inp_ip.ip_ttl = ip_defttl;
	inp->inp_ppcb = (caddr_t)tp;
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, errno)
	register struct tcpcb *tp;
	int errno;
{
	struct socket *so = tp->t_inpcb->inp_socket;

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
	register struct tcpcb *tp;
{
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
#ifdef RTV_RTT
	register struct rtentry *rt;

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
	    (rt = inp->inp_route.ro_rt) &&
	    !in_nullhost(satosin(rt_key(rt))->sin_addr)) {
		register u_long i = 0;

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

	if (tp->t_template)
		pool_put(&tcp_template_pool, tp->t_template);
	pool_put(&tcpcb_pool, tp);
	inp->inp_ppcb = 0;
	soisdisconnected(so);
	in_pcbdetach(inp);
	tcpstat.tcps_closed++;
	return ((struct tcpcb *)0);
}

int
tcp_freeq(tp)
	struct tcpcb *tp;
{
	register struct ipqent *qe;
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
	register struct inpcb *inp;
	register struct tcpcb *tp;

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
	register struct tcpcb *tp = (struct tcpcb *)inp->inp_ppcb;
	register struct socket *so = inp->inp_socket;

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

void *
tcp_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	register void *v;
{
	register struct ip *ip = v;
	register struct tcphdr *th;
	extern int inetctlerrmap[];
	void (*notify) __P((struct inpcb *, int)) = tcp_notify;
	int errno;
	int nmatch;

	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	errno = inetctlerrmap[cmd];
	if (cmd == PRC_QUENCH)
		notify = tcp_quench;
	else if (PRC_IS_REDIRECT(cmd))
		notify = in_rtchange, ip = 0;
	else if (cmd == PRC_MSGSIZE && ip_mtudisc)
		notify = tcp_mtudisc, ip = 0;
	else if (cmd == PRC_HOSTDEAD)
		ip = 0;
	else if (errno == 0)
		return NULL;
	if (ip) {
		th = (struct tcphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		nmatch = in_pcbnotify(&tcbtable, satosin(sa)->sin_addr,
		    th->th_dport, ip->ip_src, th->th_sport, errno, notify);
		if (nmatch == 0 && syn_cache_count &&
		    (inetctlerrmap[cmd] == EHOSTUNREACH ||
		    inetctlerrmap[cmd] == ENETUNREACH ||
		    inetctlerrmap[cmd] == EHOSTDOWN))
			syn_cache_unreach(ip, th);
	} else
		(void)in_pcbnotifyall(&tcbtable, satosin(sa)->sin_addr, errno,
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


/*
 * Compute the MSS to advertise to the peer.  Called only during
 * the 3-way handshake.  If we are the server (peer initiated
 * connection), we are called with a pointer to the interface
 * on which the SYN packet arrived.  If we are the client (we 
 * initiated connection), we are called with a pointer to the
 * interface out which this connection should go.
 */
u_long
tcp_mss_to_advertise(ifp)
	const struct ifnet *ifp;
{
	extern u_long in_maxmtu;
	u_long mss = 0;

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

	if (mss > sizeof(struct tcpiphdr))
		mss -= sizeof(struct tcpiphdr);

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
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
#if defined(RTV_SPIPE) || defined(RTV_SSTHRESH)
	struct rtentry *rt = in_pcbrtentry(inp);
#endif
	u_long bufsize;
	int mss;

	/*
	 * As per RFC1122, use the default MSS value, unless they 
	 * sent us an offer.  Do not accept offers less than 32 bytes.
	 */
	mss = tcp_mssdflt;
	if (offer)
		mss = offer;
	mss = max(mss, 32);		/* sanity */
	tp->t_peermss = mss;
	mss -= (tcp_optlen(tp) + ip_optlen(tp->t_inpcb));

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
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
#ifdef RTV_RPIPE
	struct rtentry *rt = in_pcbrtentry(inp);
#endif
	u_long bufsize;

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
	struct rtentry *rt;
	int rtt;

	if ((rt = in_pcbrtentry(tp->t_inpcb)) == NULL)
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
	 * add randomness about this connection, but do not estimate
	 * entropy from the timing, since the physical device driver would
	 * have done that for us.
	 */
#if NRND > 0
	if (tp != NULL)
		rnd_add_data(NULL, tp, len, 0);
#endif

	/*
	 * randomize.
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
