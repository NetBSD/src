/*	$NetBSD: tcp_input.c,v 1.73 1999/01/19 21:58:41 mycroft Exp $	*/

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
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
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
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 */

/*
 *	TODO list for SYN cache stuff:
 *
 *	Find room for a "state" field, which is needed to keep a
 *	compressed state for TIME_WAIT TCBs.  It's been noted already
 *	that this is fairly important for very high-volume web and
 *	mail servers, which use a large number of short-lived
 *	connections.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/pool.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>

#include <machine/stdarg.h>

int	tcprexmtthresh = 3;
struct	tcpiphdr tcp_saveti;

extern u_long sb_max;

#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

/* for modulo comparisons of timestamps */
#define TSTMP_LT(a,b)	((int)((a)-(b)) < 0)
#define TSTMP_GEQ(a,b)	((int)((a)-(b)) >= 0)

/*
 * Macro to compute ACK transmission behavior.  Delay the ACK unless
 * we have already delayed an ACK (must send an ACK every two segments).
 * We also ACK immediately if we received a PUSH and the ACK-on-PUSH
 * option is enabled.
 */
#define	TCP_SETUP_ACK(tp, ti) \
do { \
	if ((tp)->t_flags & TF_DELACK || \
	    (tcp_ack_on_push && (ti)->ti_flags & TH_PUSH)) \
		tp->t_flags |= TF_ACKNOW; \
	else \
		TCP_SET_DELACK(tp); \
} while (0)

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 * Set DELACK for segments received in order, but ack immediately
 * when segments are out of order (so fast retransmit can work).
 */
#define	TCP_REASS(tp, ti, m, so, flags) { \
	TCP_REASS_LOCK((tp)); \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->segq.lh_first == NULL && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
		TCP_SETUP_ACK(tp, ti); \
		(tp)->rcv_nxt += (ti)->ti_len; \
		flags = (ti)->ti_flags & TH_FIN; \
		tcpstat.tcps_rcvpack++;\
		tcpstat.tcps_rcvbyte += (ti)->ti_len;\
		sbappend(&(so)->so_rcv, (m)); \
		sorwakeup(so); \
	} else { \
		(flags) = tcp_reass((tp), (ti), (m)); \
		tp->t_flags |= TF_ACKNOW; \
	} \
	TCP_REASS_UNLOCK((tp)); \
}

int
tcp_reass(tp, ti, m)
	register struct tcpcb *tp;
	register struct tcpiphdr *ti;
	struct mbuf *m;
{
	register struct ipqent *p, *q, *nq, *tiqe = NULL;
	struct socket *so = tp->t_inpcb->inp_socket;
	int pkt_flags;
	tcp_seq pkt_seq;
	unsigned pkt_len;
	u_long rcvpartdupbyte = 0;
	u_long rcvoobyte;

	TCP_REASS_LOCK_CHECK(tp);

	/*
	 * Call with ti==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (ti == 0)
		goto present;

	rcvoobyte = ti->ti_len;
	/*
	 * Copy these to local variables because the tcpiphdr
	 * gets munged while we are collapsing mbufs.
	 */
	pkt_seq = ti->ti_seq;
	pkt_len = ti->ti_len;
	pkt_flags = ti->ti_flags;
	/*
	 * Find a segment which begins after this one does.
	 */
	for (p = NULL, q = tp->segq.lh_first; q != NULL; q = nq) {
		nq = q->ipqe_q.le_next;
		/*
		 * If the received segment is just right after this
		 * fragment, merge the two together and then check
		 * for further overlaps.
		 */
		if (q->ipqe_seq + q->ipqe_len == pkt_seq) {
#ifdef TCPREASS_DEBUG
			printf("tcp_reass[%p]: concat %u:%u(%u) to %u:%u(%u)\n",
			       tp, pkt_seq, pkt_seq + pkt_len, pkt_len,
			       q->ipqe_seq, q->ipqe_seq + q->ipqe_len, q->ipqe_len);
#endif
			pkt_len += q->ipqe_len;
			pkt_flags |= q->ipqe_flags;
			pkt_seq = q->ipqe_seq;
			m_cat(q->ipqe_m, m);
			m = q->ipqe_m;
			goto free_ipqe;
		}
		/*
		 * If the received segment is completely past this
		 * fragment, we need to go the next fragment.
		 */
		if (SEQ_LT(q->ipqe_seq + q->ipqe_len, pkt_seq)) {
			p = q;
			continue;
		}
		/*
		 * If the fragment is past the received segment, 
		 * it (or any following) can't be concatenated.
		 */
		if (SEQ_GT(q->ipqe_seq, pkt_seq + pkt_len))
			break;
		/*
		 * We've received all the data in this segment before.
		 * mark it as a duplicate and return.
		 */
		if (SEQ_LEQ(q->ipqe_seq, pkt_seq) &&
		    SEQ_GEQ(q->ipqe_seq + q->ipqe_len, pkt_seq + pkt_len)) {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += pkt_len;
			m_freem(m);
			if (tiqe != NULL)
				pool_put(&ipqent_pool, tiqe);
			return (0);
		}
		/*
		 * Received segment completely overlaps this fragment
		 * so we drop the fragment (this keeps the temporal
		 * ordering of segments correct).
		 */
		if (SEQ_GEQ(q->ipqe_seq, pkt_seq) &&
		    SEQ_LEQ(q->ipqe_seq + q->ipqe_len, pkt_seq + pkt_len)) {
			rcvpartdupbyte += q->ipqe_len;
			m_freem(q->ipqe_m);
			goto free_ipqe;
		}
		/*
		 * RX'ed segment extends past the end of the
		 * fragment.  Drop the overlapping bytes.  Then
		 * merge the fragment and segment then treat as
		 * a longer received packet.
		 */
		if (SEQ_LT(q->ipqe_seq, pkt_seq)
		    && SEQ_GT(q->ipqe_seq + q->ipqe_len, pkt_seq))  {
			int overlap = q->ipqe_seq + q->ipqe_len - pkt_seq;
#ifdef TCPREASS_DEBUG
			printf("tcp_reass[%p]: trim starting %d bytes of %u:%u(%u)\n",
			       tp, overlap,
			       pkt_seq, pkt_seq + pkt_len, pkt_len);
#endif
			m_adj(m, overlap);
			rcvpartdupbyte += overlap;
			m_cat(q->ipqe_m, m);
			m = q->ipqe_m;
			pkt_seq = q->ipqe_seq;
			pkt_len += q->ipqe_len - overlap;
			rcvoobyte -= overlap;
			goto free_ipqe;
		}
		/*
		 * RX'ed segment extends past the front of the
		 * fragment.  Drop the overlapping bytes on the
		 * received packet.  The packet will then be
		 * contatentated with this fragment a bit later.
		 */
		if (SEQ_GT(q->ipqe_seq, pkt_seq)
		    && SEQ_LT(q->ipqe_seq, pkt_seq + pkt_len))  {
			int overlap = pkt_seq + pkt_len - q->ipqe_seq;
#ifdef TCPREASS_DEBUG
			printf("tcp_reass[%p]: trim trailing %d bytes of %u:%u(%u)\n",
			       tp, overlap,
			       pkt_seq, pkt_seq + pkt_len, pkt_len);
#endif
			m_adj(m, -overlap);
			pkt_len -= overlap;
			rcvpartdupbyte += overlap;
			rcvoobyte -= overlap;
		}
		/*
		 * If the received segment immediates precedes this
		 * fragment then tack the fragment onto this segment
		 * and reinsert the data.
		 */
		if (q->ipqe_seq == pkt_seq + pkt_len) {
#ifdef TCPREASS_DEBUG
			printf("tcp_reass[%p]: append %u:%u(%u) to %u:%u(%u)\n",
			       tp, q->ipqe_seq, q->ipqe_seq + q->ipqe_len, q->ipqe_len,
			       pkt_seq, pkt_seq + pkt_len, pkt_len);
#endif
			pkt_len += q->ipqe_len;
			pkt_flags |= q->ipqe_flags;
			m_cat(m, q->ipqe_m);
			LIST_REMOVE(q, ipqe_q);
			LIST_REMOVE(q, ipqe_timeq);
			if (tiqe == NULL) {
			    tiqe = q;
			} else {
			    pool_put(&ipqent_pool, q);
			}
			break;
		}
		/*
		 * If the fragment is before the segment, remember it.
		 * When this loop is terminated, p will contain the
		 * pointer to fragment that is right before the received
		 * segment.
		 */
		if (SEQ_LEQ(q->ipqe_seq, pkt_seq))
			p = q;

		continue;

		/*
		 * This is a common operation.  It also will allow
		 * to save doing a malloc/free in most instances.
		 */
	  free_ipqe:
		LIST_REMOVE(q, ipqe_q);
		LIST_REMOVE(q, ipqe_timeq);
		if (tiqe == NULL) {
		    tiqe = q;
		} else {
		    pool_put(&ipqent_pool, q);
		}
	}

	/*
	 * Allocate a new queue entry since the received segment did not
	 * collapse onto any other out-of-order block; thus we are allocating
	 * a new block.  If it had collapsed, tiqe would not be NULL and
	 * we would be reusing it.
	 * XXX If we can't, just drop the packet.  XXX
	 */
	if (tiqe == NULL) {
		tiqe = pool_get(&ipqent_pool, PR_NOWAIT);
		if (tiqe == NULL) {
			tcpstat.tcps_rcvmemdrop++;
			m_freem(m);
			return (0);
		}
	}

	/*
	 * Update the counters.
	 */
	tcpstat.tcps_rcvoopack++;
	tcpstat.tcps_rcvoobyte += rcvoobyte;
	if (rcvpartdupbyte) {
	    tcpstat.tcps_rcvpartduppack++;
	    tcpstat.tcps_rcvpartdupbyte += rcvpartdupbyte;
	}

	/*
	 * Insert the new fragment queue entry into both queues.
	 */
	tiqe->ipqe_m = m;
	tiqe->ipqe_seq = pkt_seq;
	tiqe->ipqe_len = pkt_len;
	tiqe->ipqe_flags = pkt_flags;
	if (p == NULL) {
		LIST_INSERT_HEAD(&tp->segq, tiqe, ipqe_q);
#ifdef TCPREASS_DEBUG
		if (tiqe->ipqe_seq != tp->rcv_nxt)
			printf("tcp_reass[%p]: insert %u:%u(%u) at front\n",
			       tp, pkt_seq, pkt_seq + pkt_len, pkt_len);
#endif
	} else {
		LIST_INSERT_AFTER(p, tiqe, ipqe_q);
#ifdef TCPREASS_DEBUG
		printf("tcp_reass[%p]: insert %u:%u(%u) after %u:%u(%u)\n",
		       tp, pkt_seq, pkt_seq + pkt_len, pkt_len,
		       p->ipqe_seq, p->ipqe_seq + p->ipqe_len, p->ipqe_len);
#endif
	}

	LIST_INSERT_HEAD(&tp->timeq, tiqe, ipqe_timeq);

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVEESTABLISHED(tp->t_state) == 0)
		return (0);
	q = tp->segq.lh_first;
	if (q == NULL || q->ipqe_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && q->ipqe_len)
		return (0);

	tp->rcv_nxt += q->ipqe_len;
	pkt_flags = q->ipqe_flags & TH_FIN;

	LIST_REMOVE(q, ipqe_q);
	LIST_REMOVE(q, ipqe_timeq);
	if (so->so_state & SS_CANTRCVMORE)
		m_freem(q->ipqe_m);
	else
		sbappend(&so->so_rcv, q->ipqe_m);
	pool_put(&ipqent_pool, q);
	sorwakeup(so);
	return (pkt_flags);
}

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
void
#if __STDC__
tcp_input(struct mbuf *m, ...)
#else
tcp_input(m, va_alist)
	register struct mbuf *m;
#endif
{
	register struct tcpiphdr *ti;
	register struct inpcb *inp;
	caddr_t optp = NULL;
	int optlen = 0;
	int len, tlen, off, hdroptlen;
	register struct tcpcb *tp = 0;
	register int tiflags;
	struct socket *so = NULL;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate = 0;
	int iss = 0;
	u_long tiwin;
	struct tcp_opt_info opti;
	int iphlen;
	va_list ap;

	va_start(ap, m);
	iphlen = va_arg(ap, int);
	va_end(ap);

	tcpstat.tcps_rcvtotal++;

	opti.ts_present = 0;
	opti.maxseg = 0;

	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */
	ti = mtod(m, struct tcpiphdr *);
	if (iphlen > sizeof (struct ip))
		ip_stripoptions(m, (struct mbuf *)0);
	if (m->m_len < sizeof (struct tcpiphdr)) {
		if ((m = m_pullup(m, sizeof (struct tcpiphdr))) == 0) {
			tcpstat.tcps_rcvshort++;
			return;
		}
		ti = mtod(m, struct tcpiphdr *);
	}

	/*
	 * Checksum extended TCP header and data.
	 */
	tlen = ((struct ip *)ti)->ip_len;
	len = sizeof (struct ip) + tlen;
	bzero(ti->ti_x1, sizeof ti->ti_x1);
	ti->ti_len = (u_int16_t)tlen;
	HTONS(ti->ti_len);
	if (in_cksum(m, len) != 0) {
		tcpstat.tcps_rcvbadsum++;
		goto drop;
	}

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.		XXX
	 */
	off = ti->ti_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		tcpstat.tcps_rcvbadoff++;
		goto drop;
	}
	tlen -= off;
	ti->ti_len = tlen;
	if (off > sizeof (struct tcphdr)) {
		if (m->m_len < sizeof(struct ip) + off) {
			if ((m = m_pullup(m, sizeof (struct ip) + off)) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
			ti = mtod(m, struct tcpiphdr *);
		}
		optlen = off - sizeof (struct tcphdr);
		optp = mtod(m, caddr_t) + sizeof (struct tcpiphdr);
		/* 
		 * Do quick retrieval of timestamp options ("options
		 * prediction?").  If timestamp is the only option and it's
		 * formatted as recommended in RFC 1323 appendix A, we
		 * quickly get the values now and not bother calling
		 * tcp_dooptions(), etc.
		 */
		if ((optlen == TCPOLEN_TSTAMP_APPA ||
		     (optlen > TCPOLEN_TSTAMP_APPA &&
			optp[TCPOLEN_TSTAMP_APPA] == TCPOPT_EOL)) &&
		     *(u_int32_t *)optp == htonl(TCPOPT_TSTAMP_HDR) &&
		     (ti->ti_flags & TH_SYN) == 0) {
			opti.ts_present = 1;
			opti.ts_val = ntohl(*(u_int32_t *)(optp + 4));
			opti.ts_ecr = ntohl(*(u_int32_t *)(optp + 8));
			optp = NULL;	/* we've parsed the options */
		}
	}
	tiflags = ti->ti_flags;

	/*
	 * Convert TCP protocol specific fields to host format.
	 */
	NTOHL(ti->ti_seq);
	NTOHL(ti->ti_ack);
	NTOHS(ti->ti_win);
	NTOHS(ti->ti_urp);

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	inp = in_pcblookup_connect(&tcbtable, ti->ti_src, ti->ti_sport,
	    ti->ti_dst, ti->ti_dport);
	if (inp == 0) {
		++tcpstat.tcps_pcbhashmiss;
		inp = in_pcblookup_bind(&tcbtable, ti->ti_dst, ti->ti_dport);
		if (inp == 0) {
			++tcpstat.tcps_noport;
			goto dropwithreset;
		}
	}

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	tp = intotcpcb(inp);
	if (tp == 0)
		goto dropwithreset;
	if (tp->t_state == TCPS_CLOSED)
		goto drop;
	
	/* Unscale the window into a 32-bit value. */
	if ((tiflags & TH_SYN) == 0)
		tiwin = ti->ti_win << tp->snd_scale;
	else
		tiwin = ti->ti_win;

	so = inp->inp_socket;
	if (so->so_options & (SO_DEBUG|SO_ACCEPTCONN)) {
		if (so->so_options & SO_DEBUG) {
			ostate = tp->t_state;
			tcp_saveti = *ti;
		}
		if (so->so_options & SO_ACCEPTCONN) {
  			if ((tiflags & (TH_RST|TH_ACK|TH_SYN)) != TH_SYN) {
				if (tiflags & TH_RST) {
					syn_cache_reset(ti);
				} else if ((tiflags & (TH_ACK|TH_SYN)) ==
				    (TH_ACK|TH_SYN)) {
					/*
					 * Received a SYN,ACK.  This should
					 * never happen while we are in
					 * LISTEN.  Send an RST.
					 */
					goto badsyn;
				} else if (tiflags & TH_ACK) {
					so = syn_cache_get(so, m);
					if (so == NULL) {
						/*
						 * We don't have a SYN for
						 * this ACK; send an RST.
						 */
						goto badsyn;
					} else if (so ==
					    (struct socket *)(-1)) {
						/*
						 * We were unable to create
						 * the connection.  If the
						 * 3-way handshake was
						 * completed, and RST has
						 * been sent to the peer.
						 * Since the mbuf might be
						 * in use for the reply,
						 * do not free it.
						 */
						m = NULL;
					} else {
						/*
						 * We have created a
						 * full-blown connection.
						 */
						inp = sotoinpcb(so);
						tp = intotcpcb(inp);
						tiwin <<= tp->snd_scale;
						goto after_listen;
					}
  				} else {
					/*
					 * None of RST, SYN or ACK was set.
					 * This is an invalid packet for a
					 * TCB in LISTEN state.  Send a RST.
					 */
					goto badsyn;
				}
  			} else {
				/*
				 * Received a SYN.
				 */
				if (in_hosteq(ti->ti_src, ti->ti_dst) &&
				    ti->ti_sport == ti->ti_dport) {
					/*
					 * LISTEN socket received a SYN
					 * from itself?  This can't possibly
					 * be valid; drop the packet.
					 */
					tcpstat.tcps_badsyn++;
					goto drop;
				}
				/*
				 * SYN looks ok; create compressed TCP
				 * state for it.
				 */
				if (so->so_qlen <= so->so_qlimit &&
				    syn_cache_add(so, m, optp, optlen, &opti))
					m = NULL;
			}
			goto drop;
		}
	}

after_listen:
#ifdef DIAGNOSTIC
	/*
	 * Should not happen now that all embryonic connections
	 * are handled with compressed state.
	 */
	if (tp->t_state == TCPS_LISTEN)
		panic("tcp_input: TCPS_LISTEN");
#endif

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_idle = 0;
	if (TCPS_HAVEESTABLISHED(tp->t_state))
		TCP_TIMER_ARM(tp, TCPT_KEEP, tcp_keepidle);

	/*
	 * Process options.
	 */
	if (optp)
		tcp_dooptions(tp, optp, optlen, ti, &opti);

	/* 
	 * Header prediction: check for the two common cases
	 * of a uni-directional data xfer.  If the packet has
	 * no control flags, is in-sequence, the window didn't
	 * change and we're not retransmitting, it's a
	 * candidate.  If the length is zero and the ack moved
	 * forward, we're the sender side of the xfer.  Just
	 * free the data acked & wake any higher level process
	 * that was blocked waiting for space.  If the length
	 * is non-zero and the ack didn't move, we're the
	 * receiver side.  If we're getting packets in-order
	 * (the reassembly queue is empty), add the data to
	 * the socket buffer and note that we need a delayed ack.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tiflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ACK)) == TH_ACK &&
	    (!opti.ts_present || TSTMP_GEQ(opti.ts_val, tp->ts_recent)) &&
	    ti->ti_seq == tp->rcv_nxt &&
	    tiwin && tiwin == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {

		/* 
		 * If last ACK falls within this segment's sequence numbers,
		 *  record the timestamp.
		 */
		if (opti.ts_present &&
		    SEQ_LEQ(ti->ti_seq, tp->last_ack_sent) &&
		    SEQ_LT(tp->last_ack_sent, ti->ti_seq + ti->ti_len)) {
			tp->ts_recent_age = tcp_now;
			tp->ts_recent = opti.ts_val;
		}

		if (ti->ti_len == 0) {
			if (SEQ_GT(ti->ti_ack, tp->snd_una) &&
			    SEQ_LEQ(ti->ti_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd &&
			    tp->t_dupacks < tcprexmtthresh) {
				/*
				 * this is a pure ack for outstanding data.
				 */
				++tcpstat.tcps_predack;
				if (opti.ts_present)
					tcp_xmit_timer(tp,
					    tcp_now-opti.ts_ecr+1);
				else if (tp->t_rtt &&
				    SEQ_GT(ti->ti_ack, tp->t_rtseq))
					tcp_xmit_timer(tp, tp->t_rtt);
				acked = ti->ti_ack - tp->snd_una;
				tcpstat.tcps_rcvackpack++;
				tcpstat.tcps_rcvackbyte += acked;
				sbdrop(&so->so_snd, acked);
				tp->snd_una = ti->ti_ack;
				m_freem(m);

				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selwakeup/signal.  If data
				 * are ready to send, let tcp_output
				 * decide between more output or persist.
				 */
				if (tp->snd_una == tp->snd_max)
					TCP_TIMER_DISARM(tp, TCPT_REXMT);
				else if (TCP_TIMER_ISARMED(tp,
				    TCPT_PERSIST) == 0)
					TCP_TIMER_ARM(tp, TCPT_REXMT,
					    tp->t_rxtcur);

				sowwakeup(so);
				if (so->so_snd.sb_cc)
					(void) tcp_output(tp);
				return;
			}
		} else if (ti->ti_ack == tp->snd_una &&
		    tp->segq.lh_first == NULL &&
		    ti->ti_len <= sbspace(&so->so_rcv)) {
			/*
			 * this is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			++tcpstat.tcps_preddat;
			tp->rcv_nxt += ti->ti_len;
			tcpstat.tcps_rcvpack++;
			tcpstat.tcps_rcvbyte += ti->ti_len;
			/*
			 * Drop TCP, IP headers and TCP options then add data
			 * to socket buffer.
			 */
			m->m_data += sizeof(struct tcpiphdr)+off-sizeof(struct tcphdr);
			m->m_len -= sizeof(struct tcpiphdr)+off-sizeof(struct tcphdr);
			sbappend(&so->so_rcv, m);
			sorwakeup(so);
			TCP_SETUP_ACK(tp, ti);
			if (tp->t_flags & TF_ACKNOW)
				(void) tcp_output(tp);
			return;
		}
	}

	/*
	 * Drop TCP, IP headers and TCP options.
	 */
	hdroptlen  = sizeof(struct tcpiphdr) + off - sizeof(struct tcphdr);
	m->m_data += hdroptlen;
	m->m_len  -= hdroptlen;

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	{ int win;

	win = sbspace(&so->so_rcv);
	if (win < 0)
		win = 0;
	tp->rcv_wnd = imax(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK)
				tp = tcp_drop(tp, ECONNREFUSED);
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = ti->ti_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		TCP_TIMER_DISARM(tp, TCPT_REXMT);
		tp->irs = ti->ti_seq;
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		tcp_mss_from_peer(tp, opti.maxseg);

		/*
		 * Initialize the initial congestion window.  If we
		 * had to retransmit the SYN, we must initialize cwnd
		 * to 1 segment (i.e. the Loss Window).
		 */
		if (tp->t_flags & TF_SYN_REXMT)
			tp->snd_cwnd = tp->t_peermss;
		else
			tp->snd_cwnd = TCP_INITIAL_WINDOW(tcp_init_win,
			    tp->t_peermss);

		tcp_rmx_rtt(tp);
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			tcpstat.tcps_connects++;
			soisconnected(so);
			tcp_established(tp);
			/* Do window scaling on this connection? */
			if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
				(TF_RCVD_SCALE|TF_REQ_SCALE)) {
				tp->snd_scale = tp->requested_s_scale;
				tp->rcv_scale = tp->request_r_scale;
			}
			TCP_REASS_LOCK(tp);
			(void) tcp_reass(tp, (struct tcpiphdr *)0,
				(struct mbuf *)0);
			TCP_REASS_UNLOCK(tp);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt)
				tcp_xmit_timer(tp, tp->t_rtt);
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			m_adj(m, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			tcpstat.tcps_rcvpackafterwin++;
			tcpstat.tcps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;

	/*
	 * If the state is SYN_RECEIVED:
	 *	If seg contains an ACK, but not for our SYN, drop the input
	 *	and generate an RST.  See page 36, rfc793
	 */
	case TCPS_SYN_RECEIVED:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		break;
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check timestamp, if present.
	 * Then check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 * 
	 * RFC 1323 PAWS: If we have a timestamp reply on this segment
	 * and it's less than ts_recent, drop it.
	 */
	if (opti.ts_present && (tiflags & TH_RST) == 0 && tp->ts_recent &&
	    TSTMP_LT(opti.ts_val, tp->ts_recent)) {

		/* Check to see if ts_recent is over 24 days old.  */
		if ((int)(tcp_now - tp->ts_recent_age) > TCP_PAWS_IDLE) {
			/*
			 * Invalidate ts_recent.  If this segment updates
			 * ts_recent, the age will be reset later and ts_recent
			 * will get a valid value.  If it does not, setting
			 * ts_recent to zero will at least satisfy the
			 * requirement that zero be placed in the timestamp
			 * echo reply when ts_recent isn't valid.  The
			 * age isn't reset until we get a valid ts_recent
			 * because we don't want out-of-order segments to be
			 * dropped when ts_recent is old.
			 */
			tp->ts_recent = 0;
		} else {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += ti->ti_len;
			tcpstat.tcps_pawsdrop++;
			goto dropafterack;
		}
	}

	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1) 
				ti->ti_urp--;
			else {
				tiflags &= ~TH_URG;
				ti->ti_urp = 0;
			}
			todrop--;
		}
		if (todrop > ti->ti_len ||
		    (todrop == ti->ti_len && (tiflags & TH_FIN) == 0)) {
			/*
			 * Any valid FIN must be to the left of the window.
			 * At this point the FIN must be a duplicate or
			 * out of sequence; drop it.
			 */
			tiflags &= ~TH_FIN;
			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */
			tp->t_flags |= TF_ACKNOW;
			todrop = ti->ti_len;
			tcpstat.tcps_rcvdupbyte += todrop;
			tcpstat.tcps_rcvduppack++;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}
		m_adj(m, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;
		if (ti->ti_urp > todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && ti->ti_len) {
		tp = tcp_close(tp);
		tcpstat.tcps_rcvafterclose++;
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (ti->ti_seq+ti->ti_len) - (tp->rcv_nxt+tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= ti->ti_len) {
			tcpstat.tcps_rcvbyteafterwin += ti->ti_len;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {
				iss = tcp_new_iss(tp, sizeof(struct tcpcb),
						  tp->rcv_nxt);
				tp = tcp_close(tp);
				/*
				 * We have already advanced the mbuf
				 * pointers past the IP+TCP headers and
				 * options.  Restore those pointers before
				 * attempting to use the TCP header again.
				 */
				m->m_data -= hdroptlen;
				m->m_len  += hdroptlen;
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat.tcps_rcvbyteafterwin += todrop;
		m_adj(m, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If last ACK falls within this segment's sequence numbers,
	 * and the timestamp is newer, record it.
	 */
	if (opti.ts_present && TSTMP_GEQ(opti.ts_val, tp->ts_recent) &&
	    SEQ_LEQ(ti->ti_seq, tp->last_ack_sent) &&
	    SEQ_LT(tp->last_ack_sent, ti->ti_seq + ti->ti_len +
		   ((tiflags & (TH_SYN|TH_FIN)) != 0))) {
		tp->ts_recent_age = tcp_now;
		tp->ts_recent = opti.ts_val;
	}

	/*
	 * If the RST bit is set examine the state:
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */
	if (tiflags&TH_RST) switch (tp->t_state) {

	case TCPS_SYN_RECEIVED:
		so->so_error = ECONNREFUSED;
		goto close;

	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
		so->so_error = ECONNRESET;
	close:
		tp->t_state = TCPS_CLOSED;
		tcpstat.tcps_drops++;
		tp = tcp_close(tp);
		goto drop;

	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		tp = tcp_close(tp);
		goto drop;
	}

	/*
	 * If a SYN is in the window, then this is an
	 * error and we send an RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;
	
	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send an RST.
	 */
	case TCPS_SYN_RECEIVED:
		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max))
			goto dropwithreset;
		tcpstat.tcps_connects++;
		soisconnected(so);
		tcp_established(tp);
		/* Do window scaling? */
		if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
			(TF_RCVD_SCALE|TF_REQ_SCALE)) {
			tp->snd_scale = tp->requested_s_scale;
			tp->rcv_scale = tp->request_r_scale;
		}
		TCP_REASS_LOCK(tp);
		(void) tcp_reass(tp, (struct tcpiphdr *)0, (struct mbuf *)0);
		TCP_REASS_UNLOCK(tp);
		tp->snd_wl1 = ti->ti_seq - 1;
		/* fall into ... */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && tiwin == tp->snd_wnd) {
				tcpstat.tcps_rcvdupack++;
				/*
				 * If we have outstanding data (other than
				 * a window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.
				 *
				 * We know we're losing at the current
				 * window size so do congestion avoidance
				 * (set ssthresh to half the current window
				 * and pull our congestion window back to
				 * the new ssthresh).
				 *
				 * Dup acks mean that packets have left the
				 * network (they're now cached at the receiver) 
				 * so bump cwnd by the amount in the receiver
				 * to keep a constant cwnd packets in the
				 * network.
				 */
				if (TCP_TIMER_ISARMED(tp, TCPT_REXMT) == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					u_int win =
					    min(tp->snd_wnd, tp->snd_cwnd) / 
					    2 /	tp->t_segsz;
					if (SEQ_LT(ti->ti_ack, tp->snd_recover)) {
						/*
						 * False fast retransmit after
						 * timeout.  Do not cut window.
						 */
						tp->snd_cwnd += tp->t_segsz;
						tp->t_dupacks = 0;
						(void) tcp_output(tp);
						goto drop;
					}

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_segsz;
					tp->snd_recover = tp->snd_max;
					TCP_TIMER_DISARM(tp, TCPT_REXMT);
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_segsz;
					(void) tcp_output(tp);
					tp->snd_cwnd = tp->snd_ssthresh +
					       tp->t_segsz * tp->t_dupacks;
					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				} else if (tp->t_dupacks > tcprexmtthresh) {
					tp->snd_cwnd += tp->t_segsz;
					(void) tcp_output(tp);
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		/*
		 * If the congestion window was inflated to account
		 * for the other side's cached packets, retract it.
		 */
		if (!tcp_do_newreno) {
			if (tp->t_dupacks >= tcprexmtthresh &&
			    tp->snd_cwnd > tp->snd_ssthresh)
				tp->snd_cwnd = tp->snd_ssthresh;
			tp->t_dupacks = 0;
		} else if (tp->t_dupacks >= tcprexmtthresh
		    && !tcp_newreno(tp, ti)) {
			tp->snd_cwnd = tp->snd_ssthresh;
			/*
			 * Window inflation should have left us with approx.
			 * snd_ssthresh outstanding data.  But in case we 
			 * would be inclined to send a burst, better to do
			 * it via the slow start mechanism.
			 */
			if (SEQ_SUB(tp->snd_max, ti->ti_ack) < tp->snd_ssthresh)
				tp->snd_cwnd = SEQ_SUB(tp->snd_max, ti->ti_ack)
				     + tp->t_segsz;
			tp->t_dupacks = 0;
		}
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		tcpstat.tcps_rcvackpack++;
		tcpstat.tcps_rcvackbyte += acked;

		/*
		 * If we have a timestamp reply, update smoothed
		 * round trip time.  If no timestamp is present but
		 * transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (opti.ts_present)
			tcp_xmit_timer(tp, tcp_now - opti.ts_ecr + 1);
		else if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq))
			tcp_xmit_timer(tp,tp->t_rtt);

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			TCP_TIMER_DISARM(tp, TCPT_REXMT);
			needoutput = 1;
		} else if (TCP_TIMER_ISARMED(tp, TCPT_PERSIST) == 0)
			TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (segsz per packet).
		 * Otherwise open linearly: segsz per window
		 * (segsz^2 / cwnd per packet), plus a constant
		 * fraction of a packet (segsz/8) to help larger windows
		 * open quickly enough.
		 */
		{
		register u_int cw = tp->snd_cwnd;
		register u_int incr = tp->t_segsz;

		if (cw > tp->snd_ssthresh)
			incr = incr * incr / cw;
		if (!tcp_do_newreno || SEQ_GEQ(ti->ti_ack, tp->snd_recover))
			tp->snd_cwnd = min(cw + incr,TCP_MAXWIN<<tp->snd_scale);
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
		} else {
			sbdrop(&so->so_snd, acked);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		sowwakeup(so);
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 */
				if (so->so_state & SS_CANTRCVMORE) {
					soisdisconnected(so);
					if (tcp_maxidle > 0)
						TCP_TIMER_ARM(tp, TCPT_2MSL,
						    tcp_maxidle);
				}
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

	 	/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * TCPTV_MSL);
				soisdisconnected(so);
			}
			break;

		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_close(tp);
				goto drop;
			}
			break;

		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */
		case TCPS_TIME_WAIT:
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * TCPTV_MSL);
			goto dropafterack;
		}
	}

step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if (((tiflags & TH_ACK) && SEQ_LT(tp->snd_wl1, ti->ti_seq)) ||
	    (tp->snd_wl1 == ti->ti_seq && SEQ_LT(tp->snd_wl2, ti->ti_ack)) ||
	    (tp->snd_wl2 == ti->ti_ack && tiwin > tp->snd_wnd)) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && tiwin > tp->snd_wnd)
			tcpstat.tcps_rcvwinupd++;
		tp->snd_wnd = tiwin;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && ti->ti_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (ti->ti_urp + so->so_rcv.sb_cc > sb_max) {
			ti->ti_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section as the original 
		 * spec states (in one of two places).
		 */
		if (SEQ_GT(ti->ti_seq+ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;
			if (so->so_oobmark == 0)
				so->so_state |= SS_RCVATMARK;
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (ti->ti_urp <= (u_int16_t) ti->ti_len
#ifdef SO_OOBINLINE
		     && (so->so_options & SO_OOBINLINE) == 0
#endif
		     )
			tcp_pulloutofband(so, ti, m);
	} else
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
dodata:							/* XXX */

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((ti->ti_len || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, m, so, tiflags);
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
	} else {
		m_freem(m);
		tiflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.  Ignore a FIN received before
	 * the connection is fully established.
	 */
	if ((tiflags & TH_FIN) && TCPS_HAVEESTABLISHED(tp->t_state)) {
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {

	 	/*
		 * In ESTABLISHED STATE enter the CLOSE_WAIT state.
		 */
		case TCPS_ESTABLISHED:
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

	 	/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */
		case TCPS_FIN_WAIT_1:
			tp->t_state = TCPS_CLOSING;
			break;

	 	/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other 
		 * standard timers.
		 */
		case TCPS_FIN_WAIT_2:
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * TCPTV_MSL);
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */
		case TCPS_TIME_WAIT:
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * TCPTV_MSL);
			break;
		}
	}
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) tcp_output(tp);
	return;

badsyn:
	/*
	 * Received a bad SYN.  Increment counters and dropwithreset.
	 */
	tcpstat.tcps_badsyn++;
	tp = NULL;
	goto dropwithreset;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	return;

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast/multicast.
	 */
	if ((tiflags & TH_RST) || m->m_flags & (M_BCAST|M_MCAST) ||
	    IN_MULTICAST(ti->ti_dst.s_addr))
		goto drop;
	if (tiflags & TH_ACK)
		(void)tcp_respond(tp, ti, m, (tcp_seq)0, ti->ti_ack, TH_RST);
	else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		(void)tcp_respond(tp, ti, m, ti->ti_seq+ti->ti_len, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	return;

drop:
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	m_freem(m);
	return;
}

void
tcp_dooptions(tp, cp, cnt, ti, oi)
	struct tcpcb *tp;
	u_char *cp;
	int cnt;
	struct tcpiphdr *ti;
	struct tcp_opt_info *oi;
{
	u_int16_t mss;
	int opt, optlen;

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			continue;

		case TCPOPT_MAXSEG:
			if (optlen != TCPOLEN_MAXSEG)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			bcopy(cp + 2, &mss, sizeof(mss));
			oi->maxseg = ntohs(mss);
			break;

		case TCPOPT_WINDOW:
			if (optlen != TCPOLEN_WINDOW)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			tp->t_flags |= TF_RCVD_SCALE;
			tp->requested_s_scale = cp[2];
			if (tp->requested_s_scale > TCP_MAX_WINSHIFT) {
				log(LOG_ERR, "TCP: invalid wscale %d from "
				    "0x%08x, assuming %d\n",
				    tp->requested_s_scale,
				    ntohl(ti->ti_src.s_addr),
				    TCP_MAX_WINSHIFT);
				tp->requested_s_scale = TCP_MAX_WINSHIFT;
			}
			break;

		case TCPOPT_TIMESTAMP:
			if (optlen != TCPOLEN_TIMESTAMP)
				continue;
			oi->ts_present = 1;
			bcopy(cp + 2, &oi->ts_val, sizeof(oi->ts_val));
			NTOHL(oi->ts_val);
			bcopy(cp + 6, &oi->ts_ecr, sizeof(oi->ts_ecr));
			NTOHL(oi->ts_ecr);

			/* 
			 * A timestamp received in a SYN makes
			 * it ok to send timestamp requests and replies.
			 */
			if (ti->ti_flags & TH_SYN) {
				tp->t_flags |= TF_RCVD_TSTMP;
				tp->ts_recent = oi->ts_val;
				tp->ts_recent_age = tcp_now;
			}
			break;
		case TCPOPT_SACK_PERMITTED:
			if (optlen != TCPOLEN_SACK_PERMITTED)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			tp->t_flags &= ~TF_CANT_TXSACK;
			break;

		case TCPOPT_SACK:
			if (tp->t_flags & TF_IGNR_RXSACK)
				continue;
			if (optlen % 8 != 2 || optlen < 10)
				continue;
			cp += 2;
			optlen -= 2;
			for (; optlen > 0; cp -= 8, optlen -= 8) {
				tcp_seq lwe, rwe;
				bcopy((char *)cp, (char *) &lwe, sizeof(lwe));
				NTOHL(lwe);
				bcopy((char *)cp, (char *) &rwe, sizeof(rwe));
				NTOHL(rwe);
				/* tcp_mark_sacked(tp, lwe, rwe); */
			}
			break;
		}
	}
}

/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
void
tcp_pulloutofband(so, ti, m)
	struct socket *so;
	struct tcpiphdr *ti;
	register struct mbuf *m;
{
	int cnt = ti->ti_urp - 1;
	
	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			bcopy(cp+1, cp, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == 0)
			break;
	}
	panic("tcp_pulloutofband");
}

/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 */
void
tcp_xmit_timer(tp, rtt)
	register struct tcpcb *tp;
	short rtt;
{
	register short delta;
	short rttmin;

	tcpstat.tcps_rttupdated++;
	--rtt;
	if (tp->t_srtt != 0) {
		/*
		 * srtt is stored as fixed point with 3 bits after the
		 * binary point (i.e., scaled by 8).  The following magic
		 * is equivalent to the smoothing algorithm in rfc793 with
		 * an alpha of .875 (srtt = rtt/8 + srtt*7/8 in fixed
		 * point).  Adjust rtt to origin 0.
		 */
		delta = (rtt << 2) - (tp->t_srtt >> TCP_RTT_SHIFT);
		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1 << 2;
		/*
		 * We accumulate a smoothed rtt variance (actually, a
		 * smoothed mean difference), then set the retransmit
		 * timer to smoothed rtt + 4 times the smoothed variance.
		 * rttvar is stored as fixed point with 2 bits after the
		 * binary point (scaled by 4).  The following is
		 * equivalent to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).  This replaces
		 * rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (tp->t_rttvar >> TCP_RTTVAR_SHIFT);
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1 << 2;
	} else {
		/* 
		 * No rtt measurement yet - use the unsmoothed rtt.
		 * Set the variance to half the rtt (so our first
		 * retransmit happens at 3*rtt).
		 */
		tp->t_srtt = rtt << (TCP_RTT_SHIFT + 2);
		tp->t_rttvar = rtt << (TCP_RTTVAR_SHIFT + 2 - 1);
	}
	tp->t_rtt = 0;
	tp->t_rxtshift = 0;

	/*
	 * the retransmit should happen at rtt + 4 * rttvar.
	 * Because of the way we do the smoothing, srtt and rttvar
	 * will each average +1/2 tick of bias.  When we compute
	 * the retransmit timer, we want 1/2 tick of rounding and
	 * 1 extra tick because of +-1/2 tick uncertainty in the
	 * firing of the timer.  The bias will give us exactly the
	 * 1.5 tick we need.  But, because the bias is
	 * statistical, we have to test that we don't drop below
	 * the minimum feasible timer (which is 2 ticks).
	 */
	if (tp->t_rttmin > rtt + 2)
		rttmin = tp->t_rttmin;
	else
		rttmin = rtt + 2;
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp), rttmin, TCPTV_REXMTMAX);
	
	/*
	 * We received an ack for a packet that wasn't retransmitted;
	 * it is probably safe to discard any error indications we've
	 * received recently.  This isn't quite right, but close enough
	 * for now (a route might have failed after we sent a segment,
	 * and the return path might not be symmetrical).
	 */
	tp->t_softerror = 0;
}

/*
 * Checks for partial ack.  If partial ack arrives, force the retransmission
 * of the next unacknowledged segment, do not clear tp->t_dupacks, and return
 * 1.  By setting snd_nxt to ti_ack, this forces retransmission timer to
 * be started again.  If the ack advances at least to tp->snd_recover, return 0.
 */
int
tcp_newreno(tp, ti)
	struct tcpcb *tp;
	struct tcpiphdr *ti;
{
	if (SEQ_LT(ti->ti_ack, tp->snd_recover)) {
	        tcp_seq onxt = tp->snd_nxt;
	        tcp_seq ouna = tp->snd_una;  /* Haven't updated snd_una yet*/
	        u_long  ocwnd = tp->snd_cwnd;
		TCP_TIMER_DISARM(tp, TCPT_REXMT);
	        tp->t_rtt = 0;
	        tp->snd_nxt = ti->ti_ack;
	        tp->snd_cwnd = tp->t_segsz;
	        tp->snd_una = ti->ti_ack;
	        (void) tcp_output(tp);
	        tp->snd_cwnd = ocwnd;
	        tp->snd_una = ouna;
	        if (SEQ_GT(onxt, tp->snd_nxt))
	                tp->snd_nxt = onxt;
	        /*
	         * Partial window deflation.  Relies on fact that tp->snd_una
	         * not updated yet.
	         */
	        tp->snd_cwnd -= (ti->ti_ack - tp->snd_una - tp->t_segsz);
	        return 1;
	}
	return 0;
}


/*
 * TCP compressed state engine.  Currently used to hold compressed
 * state for SYN_RECEIVED.
 */

u_long	syn_cache_count;
u_int32_t syn_hash1, syn_hash2;

#define SYN_HASH(sa, sp, dp) \
	((((sa)->s_addr^syn_hash1)*(((((u_int32_t)(dp))<<16) + \
				     ((u_int32_t)(sp)))^syn_hash2)))

LIST_HEAD(, syn_cache_head) tcp_syn_cache_queue;

#define	SYN_CACHE_RM(sc, scp)						\
do {									\
	TAILQ_REMOVE(&(scp)->sch_queue, (sc), sc_queue);		\
	if (--(scp)->sch_length	== 0)					\
		LIST_REMOVE((scp), sch_headq);				\
	syn_cache_count--;						\
} while (0)

struct pool syn_cache_pool;

void
syn_cache_init()
{
	int i;

	/* Initialize the hash bucket queues. */
	for (i = 0; i < tcp_syn_cache_size; i++)
		TAILQ_INIT(&tcp_syn_cache[i].sch_queue);

	/* Initialize the active hash bucket cache. */
	LIST_INIT(&tcp_syn_cache_queue);

	/* Initialize the syn cache pool. */
	pool_init(&syn_cache_pool, sizeof(struct syn_cache), 0, 0, 0,
	    "synpl", 0, NULL, NULL, M_PCB);
}

void
syn_cache_insert(sc)
	struct syn_cache *sc;
{
	struct syn_cache_head *scp, *scp2, *sce;
	struct syn_cache *sc2;
	int s;

	/*
	 * If there are no entries in the hash table, reinitialize
	 * the hash secrets.
	 */
	if (syn_cache_count == 0) {
		struct timeval tv;
		microtime(&tv);
		syn_hash1 = random() ^ (u_long)&sc;
		syn_hash2 = random() ^ tv.tv_usec;
	}

	sc->sc_hash = SYN_HASH(&sc->sc_src, sc->sc_sport, sc->sc_dport);
	scp = &tcp_syn_cache[sc->sc_hash % tcp_syn_cache_size];

	/*
	 * Make sure that we don't overflow the per-bucket
	 * limit or the total cache size limit.
	 */
	s = splsoftnet();
	if (scp->sch_length >= tcp_syn_bucket_limit) {
		tcpstat.tcps_sc_bucketoverflow++;
		/*
		 * The bucket is full.  Toss the first (i.e. oldest)
		 * element in this bucket.
		 */
		sc2 = TAILQ_FIRST(&scp->sch_queue);
		SYN_CACHE_RM(sc2, scp);
		if (sc2->sc_ipopts)
			(void) m_free(sc2->sc_ipopts);
		pool_put(&syn_cache_pool, sc2);
	} else if (syn_cache_count >= tcp_syn_cache_limit) {
		tcpstat.tcps_sc_overflowed++;
		/*
		 * The cache is full.  Toss the first (i.e. oldest)
		 * element in the first non-empty bucket we can find.
		 */
		scp2 = scp;
		if (TAILQ_FIRST(&scp2->sch_queue) == NULL) {
			sce = &tcp_syn_cache[tcp_syn_cache_size];
			for (++scp2; scp2 != scp; scp2++) {
				if (scp2 >= sce)
					scp2 = &tcp_syn_cache[0];
				if (TAILQ_FIRST(&scp2->sch_queue) != NULL)
					break;
			}
		}
		sc2 = TAILQ_FIRST(&scp2->sch_queue);
		if (sc2 == NULL) {
			if (sc->sc_ipopts)
				(void) m_free(sc->sc_ipopts);
			pool_put(&syn_cache_pool, sc);
			return;
		}
		SYN_CACHE_RM(sc2, scp2);
		if (sc2->sc_ipopts)
			(void) m_free(sc2->sc_ipopts);
		pool_put(&syn_cache_pool, sc2);
	}

	/* Set entry's timer. */
	PRT_SLOW_ARM(sc->sc_timer, tcp_syn_cache_timeo);

	/* Put it into the bucket. */
	TAILQ_INSERT_TAIL(&scp->sch_queue, sc, sc_queue);
	if (++scp->sch_length == 1)
		LIST_INSERT_HEAD(&tcp_syn_cache_queue, scp, sch_headq);
	syn_cache_count++;

	tcpstat.tcps_sc_added++;
	splx(s);
}

/*
 * Walk down the cache list, looking for expired entries in each bucket.
 */
void
syn_cache_timer()
{
	struct syn_cache_head *scp, *nscp;
	struct syn_cache *sc, *nsc;
	int s;

	s = splsoftnet();
	for (scp = LIST_FIRST(&tcp_syn_cache_queue); scp != NULL; scp = nscp) {
#ifdef DIAGNOSTIC
		if (TAILQ_FIRST(&scp->sch_queue) == NULL)
			panic("syn_cache_timer: queue inconsistency");
#endif
		nscp = LIST_NEXT(scp, sch_headq);
		for (sc = TAILQ_FIRST(&scp->sch_queue);
		     sc != NULL && PRT_SLOW_ISEXPIRED(sc->sc_timer);
		     sc = nsc) {
			nsc = TAILQ_NEXT(sc, sc_queue);
			tcpstat.tcps_sc_timed_out++;
			SYN_CACHE_RM(sc, scp);
			if (sc->sc_ipopts)
				(void) m_free(sc->sc_ipopts);
			pool_put(&syn_cache_pool, sc);
		}
	}
	splx(s);
}

/*
 * Find an entry in the syn cache.
 */
struct syn_cache *
syn_cache_lookup(ti, headp)
	struct tcpiphdr *ti;
	struct syn_cache_head **headp;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	u_int32_t hash;
	int s;

	hash = SYN_HASH(&ti->ti_src, ti->ti_sport, ti->ti_dport);

	scp = &tcp_syn_cache[hash % tcp_syn_cache_size];
	*headp = scp;
	s = splsoftnet();
	for (sc = TAILQ_FIRST(&scp->sch_queue); sc != NULL;
	     sc = TAILQ_NEXT(sc, sc_queue)) {
		if (sc->sc_hash != hash)
			continue;
		if (sc->sc_src.s_addr == ti->ti_src.s_addr &&
		    sc->sc_sport == ti->ti_sport &&
		    sc->sc_dport == ti->ti_dport &&
		    sc->sc_dst.s_addr == ti->ti_dst.s_addr) {
			splx(s);
			return (sc);
		}
	}
	splx(s);
	return (NULL);
}

/*
 * This function gets called when we receive an ACK for a
 * socket in the LISTEN state.  We look up the connection
 * in the syn cache, and if its there, we pull it out of
 * the cache and turn it into a full-blown connection in
 * the SYN-RECEIVED state.
 *
 * The return values may not be immediately obvious, and their effects
 * can be subtle, so here they are:
 *
 *	NULL	SYN was not found in cache; caller should drop the
 *		packet and send an RST.
 *
 *	-1	We were unable to create the new connection, and are
 *		aborting it.  An ACK,RST is being sent to the peer
 *		(unless we got screwey sequence numbners; see below),
 *		because the 3-way handshake has been completed.  Caller
 *		should not free the mbuf, since we may be using it.  If
 *		we are not, we will free it.
 *
 *	Otherwise, the return value is a pointer to the new socket
 *	associated with the connection.
 */
struct socket *
syn_cache_get(so, m)
	struct socket *so;
	struct mbuf *m;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	register struct inpcb *inp;
	register struct tcpcb *tp = 0;
	register struct tcpiphdr *ti;
	struct sockaddr_in *sin;
	struct mbuf *am;
	long win;
	int s;

	ti = mtod(m, struct tcpiphdr *);
	s = splsoftnet();
	if ((sc = syn_cache_lookup(ti, &scp)) == NULL) {
		splx(s);
		return (NULL);
	}

	win = sbspace(&so->so_rcv);
	if (win > TCP_MAXWIN)
		win = TCP_MAXWIN;

	/*
	 * Verify the sequence and ack numbers.
	 */
	if ((ti->ti_ack != sc->sc_iss + 1) ||
	    SEQ_LEQ(ti->ti_seq, sc->sc_irs) ||
	    SEQ_GT(ti->ti_seq, sc->sc_irs + 1 + win)) {
		(void) syn_cache_respond(sc, m, ti, win, 0);
		splx(s);
		return ((struct socket *)(-1));
	}

	/* Remove this cache entry */
	SYN_CACHE_RM(sc, scp);
	splx(s);

	/*
	 * Ok, create the full blown connection, and set things up
	 * as they would have been set up if we had created the
	 * connection when the SYN arrived.  If we can't create
	 * the connection, abort it.
	 */
	so = sonewconn(so, SS_ISCONNECTED);
	if (so == NULL)
		goto resetandabort;

	inp = sotoinpcb(so);
	inp->inp_laddr = sc->sc_dst;
	inp->inp_lport = sc->sc_dport;
	in_pcbstate(inp, INP_BOUND);
	inp->inp_options = ip_srcroute();
	if (inp->inp_options == NULL) {
		inp->inp_options = sc->sc_ipopts;
		sc->sc_ipopts = NULL;
	}

	am = m_get(M_DONTWAIT, MT_SONAME);	/* XXX */
	if (am == NULL)
		goto resetandabort;
	am->m_len = sizeof(struct sockaddr_in);
	sin = mtod(am, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_addr = sc->sc_src;
	sin->sin_port = sc->sc_sport;
	bzero((caddr_t)sin->sin_zero, sizeof(sin->sin_zero));
	if (in_pcbconnect(inp, am)) {
		(void) m_free(am);
		goto resetandabort;
	}
	(void) m_free(am);

	tp = intotcpcb(inp);
	if (sc->sc_request_r_scale != 15) {
		tp->requested_s_scale = sc->sc_requested_s_scale;
		tp->request_r_scale = sc->sc_request_r_scale;
		tp->snd_scale = sc->sc_requested_s_scale;
		tp->rcv_scale = sc->sc_request_r_scale;
		tp->t_flags |= TF_RCVD_SCALE;
	}
	if (sc->sc_flags & SCF_TIMESTAMP)
		tp->t_flags |= TF_RCVD_TSTMP;

	tp->t_template = tcp_template(tp);
	if (tp->t_template == 0) {
		tp = tcp_drop(tp, ENOBUFS);	/* destroys socket */
		so = NULL;
		m_freem(m);
		goto abort;
	}

	tp->iss = sc->sc_iss;
	tp->irs = sc->sc_irs;
	tcp_sendseqinit(tp);
	tcp_rcvseqinit(tp);
	tp->t_state = TCPS_SYN_RECEIVED;
	TCP_TIMER_ARM(tp, TCPT_KEEP, TCPTV_KEEP_INIT);
	tcpstat.tcps_accepts++;

	/* Initialize tp->t_ourmss before we deal with the peer's! */
	tp->t_ourmss = sc->sc_ourmaxseg;
	tcp_mss_from_peer(tp, sc->sc_peermaxseg);

	/*
	 * Initialize the initial congestion window.  If we
	 * had to retransmit the SYN,ACK, we must initialize cwnd
	 * to 1 segment (i.e. the Loss Window).
	 */
	if (sc->sc_rexmt_count)
		tp->snd_cwnd = tp->t_peermss;
	else
		tp->snd_cwnd = TCP_INITIAL_WINDOW(tcp_init_win, tp->t_peermss);

	tcp_rmx_rtt(tp);
	tp->snd_wl1 = sc->sc_irs;
	tp->rcv_up = sc->sc_irs + 1;

	/*
	 * This is what whould have happened in tcp_ouput() when
	 * the SYN,ACK was sent.
	 */
	tp->snd_up = tp->snd_una;
	tp->snd_max = tp->snd_nxt = tp->iss+1;
	TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);
	if (win > 0 && SEQ_GT(tp->rcv_nxt+win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->last_ack_sent = tp->rcv_nxt;

	tcpstat.tcps_sc_completed++;
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	pool_put(&syn_cache_pool, sc);
	return (so);

resetandabort:
	(void) tcp_respond(NULL, ti, m, ti->ti_seq+ti->ti_len,
	    (tcp_seq)0, TH_RST|TH_ACK);
abort:
	if (so != NULL)
		(void) soabort(so);
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	pool_put(&syn_cache_pool, sc);
	tcpstat.tcps_sc_aborted++;
	return ((struct socket *)(-1));
}

/*
 * This function is called when we get a RST for a
 * non-existant connection, so that we can see if the
 * connection is in the syn cache.  If it is, zap it.
 */

void
syn_cache_reset(ti)
	register struct tcpiphdr *ti;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	int s = splsoftnet();

	if ((sc = syn_cache_lookup(ti, &scp)) == NULL) {
		splx(s);
		return;
	}
	if (SEQ_LT(ti->ti_seq,sc->sc_irs) ||
	    SEQ_GT(ti->ti_seq, sc->sc_irs+1)) {
		splx(s);
		return;
	}
	SYN_CACHE_RM(sc, scp);
	splx(s);
	tcpstat.tcps_sc_reset++;
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	pool_put(&syn_cache_pool, sc);
}

void
syn_cache_unreach(ip, th)
	struct ip *ip;
	struct tcphdr *th;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct tcpiphdr ti2;
	int s;

	ti2.ti_src.s_addr = ip->ip_dst.s_addr;
	ti2.ti_dst.s_addr = ip->ip_src.s_addr;
	ti2.ti_sport = th->th_dport;
	ti2.ti_dport = th->th_sport;

	s = splsoftnet();
	if ((sc = syn_cache_lookup(&ti2, &scp)) == NULL) {
		splx(s);
		return;
	}
	/* If the sequence number != sc_iss, then it's a bogus ICMP msg */
	if (ntohl (th->th_seq) != sc->sc_iss) {
		splx(s);
		return;
	}

	/*
	 * If we've rertransmitted 3 times and this is our second error,
	 * we remove the entry.  Otherwise, we allow it to continue on.
	 * This prevents us from incorrectly nuking an entry during a
	 * spurious network outage.
	 *
	 * See tcp_notify().
	 */
	if ((sc->sc_flags & SCF_UNREACH) == 0 || sc->sc_rexmt_count < 3) {
		sc->sc_flags |= SCF_UNREACH;
		splx(s);
		return;
	}

	SYN_CACHE_RM(sc, scp);
	splx(s);
	tcpstat.tcps_sc_unreach++;
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	pool_put(&syn_cache_pool, sc);
}

/*
 * Given a LISTEN socket and an inbound SYN request, add
 * this to the syn cache, and send back a segment:
 *	<SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
 * to the source.
 *
 * IMPORTANT NOTE: We do _NOT_ ACK data that might accompany the SYN.
 * Doing so would require that we hold onto the data and deliver it
 * to the application.  However, if we are the target of a SYN-flood
 * DoS attack, an attacker could send data which would eventually
 * consume all available buffer space if it were ACKed.  By not ACKing
 * the data, we avoid this DoS scenario.
 */

int
syn_cache_add(so, m, optp, optlen, oi)
	struct socket *so;
	struct mbuf *m;
	u_char *optp;
	int optlen;
	struct tcp_opt_info *oi;
{
	register struct tcpiphdr *ti;
	struct tcpcb tb, *tp;
	long win;
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct mbuf *ipopts;

	tp = sototcpcb(so);
	ti = mtod(m, struct tcpiphdr *);

	/*
	 * RFC1122 4.2.3.10, p. 104: discard bcast/mcast SYN
	 * in_broadcast() should never return true on a received
	 * packet with M_BCAST not set.
	 */
	if (m->m_flags & (M_BCAST|M_MCAST) ||
	    IN_MULTICAST(ti->ti_src.s_addr) ||
	    IN_MULTICAST(ti->ti_dst.s_addr))
		return (0);

	/*
	 * Initialize some local state.
	 */
	win = sbspace(&so->so_rcv);
	if (win > TCP_MAXWIN)
		win = TCP_MAXWIN;

	/*
	 * Remember the IP options, if any.
	 */
	ipopts = ip_srcroute();

	if (optp) {
		tb.t_flags = tcp_do_rfc1323 ? (TF_REQ_SCALE|TF_REQ_TSTMP) : 0;
		tcp_dooptions(&tb, optp, optlen, ti, oi);
	} else
		tb.t_flags = 0;

	/*
	 * See if we already have an entry for this connection.
	 * If we do, resend the SYN,ACK, and remember since the
	 * initial congestion window must be initialized to 1
	 * segment when the connection completes.
	 */
	if ((sc = syn_cache_lookup(ti, &scp)) != NULL) {
		tcpstat.tcps_sc_dupesyn++;
		sc->sc_rexmt_count++;
		if (sc->sc_rexmt_count == 0) {
			/*
			 * Eeek!  We rolled the counter.  Just set it
			 * to the max value.  This shouldn't ever happen,
			 * but there's no real reason to panic here, since
			 * the count doesn't have to be very precise.
			 */
			sc->sc_rexmt_count = USHRT_MAX;
		}

		if (ipopts) {
			/*
			 * If we were remembering a previous source route,
			 * forget it and use the new one we've been given.
			 */
			if (sc->sc_ipopts)
				(void) m_free(sc->sc_ipopts);
			sc->sc_ipopts = ipopts;
		}

		if (syn_cache_respond(sc, m, ti, win, tb.ts_recent) == 0) {
			tcpstat.tcps_sndacks++;
			tcpstat.tcps_sndtotal++;
		}
		return (1);
	}

	sc = pool_get(&syn_cache_pool, PR_NOWAIT);
	if (sc == NULL) {	
		if (ipopts)
			(void) m_free(ipopts);
		return (0);
	}

	/*
	 * Fill in the cache, and put the necessary IP and TCP
	 * options into the reply.
	 */
	sc->sc_src.s_addr = ti->ti_src.s_addr;
	sc->sc_dst.s_addr = ti->ti_dst.s_addr;
	sc->sc_sport = ti->ti_sport;
	sc->sc_dport = ti->ti_dport;
	sc->sc_flags = 0;
	sc->sc_ipopts = ipopts;
	sc->sc_irs = ti->ti_seq;
	sc->sc_iss = tcp_new_iss(sc, sizeof(struct syn_cache), 0);
	sc->sc_peermaxseg = oi->maxseg;
	sc->sc_ourmaxseg = tcp_mss_to_advertise(m->m_flags & M_PKTHDR ?
						m->m_pkthdr.rcvif : NULL);
	if (tcp_do_rfc1323 && (tb.t_flags & TF_RCVD_TSTMP))
		sc->sc_flags |= SCF_TIMESTAMP;
	if ((tb.t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
	    (TF_RCVD_SCALE|TF_REQ_SCALE)) {
		sc->sc_requested_s_scale = tb.requested_s_scale;
		sc->sc_request_r_scale = 0;
		while (sc->sc_request_r_scale < TCP_MAX_WINSHIFT &&
		    TCP_MAXWIN << sc->sc_request_r_scale <
		    so->so_rcv.sb_hiwat)
			sc->sc_request_r_scale++;
	} else {
		sc->sc_requested_s_scale = 15;
		sc->sc_request_r_scale = 15;
	}
	if (syn_cache_respond(sc, m, ti, win, tb.ts_recent) == 0) {
		syn_cache_insert(sc);
		tcpstat.tcps_sndacks++;
		tcpstat.tcps_sndtotal++;
	} else {
		if (sc->sc_ipopts)
			(void) m_free(sc->sc_ipopts);
		pool_put(&syn_cache_pool, sc);
		tcpstat.tcps_sc_dropped++;
	}
	return (1);
}

int
syn_cache_respond(sc, m, ti, win, ts)
	struct syn_cache *sc;
	struct mbuf *m;
	register struct tcpiphdr *ti;
	long win;
	u_long ts;
{
	u_int8_t *optp;
	int optlen;

	/*
	 * Tack on the TCP options.  If there isn't enough trailing
	 * space for them, move up the fixed header to make space.
	 */
	optlen = 4 + (sc->sc_request_r_scale != 15 ? 4 : 0) +
	    ((sc->sc_flags & SCF_TIMESTAMP) ? TCPOLEN_TSTAMP_APPA : 0);
	if (optlen > M_TRAILINGSPACE(m)) {
		if (M_LEADINGSPACE(m) >= optlen) {
			m->m_data -= optlen;
			m->m_len += optlen;
		} else {
			struct mbuf *m0 = m;
			if ((m = m_gethdr(M_DONTWAIT, MT_HEADER)) == NULL) {
				m_freem(m0);
				return (ENOBUFS);
			}
			MH_ALIGN(m, sizeof(*ti) + optlen);
			m->m_next = m0; /* this gets freed below */
		}
		bcopy((caddr_t)ti, mtod(m, caddr_t), sizeof(*ti));
		ti = mtod(m, struct tcpiphdr *);
	}

	optp = (u_int8_t *)(ti + 1);
	optp[0] = TCPOPT_MAXSEG;
	optp[1] = 4;
	optp[2] = (sc->sc_ourmaxseg >> 8) & 0xff;
	optp[3] = sc->sc_ourmaxseg & 0xff;
	optlen = 4;

	if (sc->sc_request_r_scale != 15) {
		*((u_int32_t *)(optp + optlen)) = htonl(TCPOPT_NOP << 24 |
		    TCPOPT_WINDOW << 16 | TCPOLEN_WINDOW << 8 |
		    sc->sc_request_r_scale);
		optlen += 4;
	}

	if (sc->sc_flags & SCF_TIMESTAMP) {
		u_int32_t *lp = (u_int32_t *)(optp + optlen);
		/* Form timestamp option as shown in appendix A of RFC 1323. */
		*lp++ = htonl(TCPOPT_TSTAMP_HDR);
		*lp++ = htonl(tcp_now);
		*lp   = htonl(ts);
		optlen += TCPOLEN_TSTAMP_APPA;
	}

	/*
	 * Toss any trailing mbufs.  No need to worry about
	 * m_len and m_pkthdr.len, since tcp_respond() will
	 * unconditionally set them.
	 */
	if (m->m_next) {
		m_freem(m->m_next);
		m->m_next = NULL;
  	}

	/*
	 * Fill in the fields that tcp_respond() will not touch, and
	 * then send the response.
	 */
	ti->ti_off = (sizeof(struct tcphdr) + optlen) >> 2;
	ti->ti_win = htons(win);
	return (tcp_respond(NULL, ti, m, sc->sc_irs + 1, sc->sc_iss,
	    TH_SYN|TH_ACK));
}
