/*	$NetBSD: tcp_input.c,v 1.438.2.1 2024/10/08 11:24:50 martin Exp $	*/

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
 *      @(#)COPYRIGHT   1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 *      This product includes software developed at the Information
 *      Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

/*-
 * Copyright (c) 1997, 1998, 1999, 2001, 2005, 2006,
 * 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Kevin M. Lahey of the Numerical Aerospace Simulation
 * Facility, NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rui Paulo.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcp_input.c,v 1.438.2.1 2024/10/08 11:24:50 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_inet_csum.h"
#include "opt_tcp_debug.h"
#endif

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
#include <sys/domain.h>
#include <sys/kernel.h>
#ifdef TCP_SIGNATURE
#include <sys/md5.h>
#endif
#include <sys/lwp.h> /* for lwp0 */
#include <sys/cprng.h>

#include <net/if.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/in_offload.h>

#if NARP > 0
#include <netinet/if_inarp.h>
#endif
#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#ifdef TCP_SIGNATURE
#include <netinet6/scope6_var.h>
#endif
#endif

#ifndef INET6
#include <netinet/ip6.h>
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_private.h>
#include <netinet/tcp_congctl.h>
#include <netinet/tcp_debug.h>
#include <netinet/tcp_syncache.h>

#ifdef INET6
#include "faith.h"
#if defined(NFAITH) && NFAITH > 0
#include <net/if_faith.h>
#endif
#endif

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#endif	/* IPSEC*/

#include <netinet/tcp_vtw.h>

int	tcprexmtthresh = 3;
int	tcp_log_refused;

int	tcp_do_autorcvbuf = 1;
int	tcp_autorcvbuf_inc = 16 * 1024;
int	tcp_autorcvbuf_max = 256 * 1024;
int	tcp_msl = (TCPTV_MSL / PR_SLOWHZ);

int tcp_reass_maxqueuelen = 100;

static int tcp_rst_ppslim_count = 0;
static struct timeval tcp_rst_ppslim_last;
static int tcp_ackdrop_ppslim_count = 0;
static struct timeval tcp_ackdrop_ppslim_last;

#define TCP_PAWS_IDLE	(24U * 24 * 60 * 60 * PR_SLOWHZ)

/* for modulo comparisons of timestamps */
#define TSTMP_LT(a,b)	((int)((a)-(b)) < 0)
#define TSTMP_GEQ(a,b)	((int)((a)-(b)) >= 0)

/*
 * Neighbor Discovery, Neighbor Unreachability Detection Upper layer hint.
 */
static void
nd_hint(struct tcpcb *tp)
{
	struct route *ro = NULL;
	struct rtentry *rt;

	if (tp == NULL)
		return;

	ro = &tp->t_inpcb->inp_route;
	if (ro == NULL)
		return;

	rt = rtcache_validate(ro);
	if (rt == NULL)
		return;

	switch (tp->t_family) {
#if NARP > 0
	case AF_INET:
		arp_nud_hint(rt);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		nd6_nud_hint(rt);
		break;
#endif
	}

	rtcache_unref(rt, ro);
}

/*
 * Compute ACK transmission behavior.  Delay the ACK unless
 * we have already delayed an ACK (must send an ACK every two segments).
 * We also ACK immediately if we received a PUSH and the ACK-on-PUSH
 * option is enabled.
 */
static void
tcp_setup_ack(struct tcpcb *tp, const struct tcphdr *th)
{

	if (tp->t_flags & TF_DELACK ||
	    (tcp_ack_on_push && th->th_flags & TH_PUSH))
		tp->t_flags |= TF_ACKNOW;
	else
		TCP_SET_DELACK(tp);
}

static void
icmp_check(struct tcpcb *tp, const struct tcphdr *th, int acked)
{

	/*
	 * If we had a pending ICMP message that refers to data that have
	 * just been acknowledged, disregard the recorded ICMP message.
	 */
	if ((tp->t_flags & TF_PMTUD_PEND) &&
	    SEQ_GT(th->th_ack, tp->t_pmtud_th_seq))
		tp->t_flags &= ~TF_PMTUD_PEND;

	/*
	 * Keep track of the largest chunk of data
	 * acknowledged since last PMTU update
	 */
	if (tp->t_pmtud_mss_acked < acked)
		tp->t_pmtud_mss_acked = acked;
}

/*
 * Convert TCP protocol fields to host order for easier processing.
 */
static void
tcp_fields_to_host(struct tcphdr *th)
{

	NTOHL(th->th_seq);
	NTOHL(th->th_ack);
	NTOHS(th->th_win);
	NTOHS(th->th_urp);
}

/*
 * ... and reverse the above.
 */
static void
tcp_fields_to_net(struct tcphdr *th)
{

	HTONL(th->th_seq);
	HTONL(th->th_ack);
	HTONS(th->th_win);
	HTONS(th->th_urp);
}

static void
tcp_urp_drop(struct tcphdr *th, int todrop, int *tiflags)
{
	if (th->th_urp > todrop) {
		th->th_urp -= todrop;
	} else {
		*tiflags &= ~TH_URG;
		th->th_urp = 0;
	}
}

#ifdef TCP_CSUM_COUNTERS
#include <sys/device.h>

extern struct evcnt tcp_hwcsum_ok;
extern struct evcnt tcp_hwcsum_bad;
extern struct evcnt tcp_hwcsum_data;
extern struct evcnt tcp_swcsum;
#if defined(INET6)
extern struct evcnt tcp6_hwcsum_ok;
extern struct evcnt tcp6_hwcsum_bad;
extern struct evcnt tcp6_hwcsum_data;
extern struct evcnt tcp6_swcsum;
#endif /* defined(INET6) */

#define	TCP_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++

#else

#define	TCP_CSUM_COUNTER_INCR(ev)	/* nothing */

#endif /* TCP_CSUM_COUNTERS */

#ifdef TCP_REASS_COUNTERS
#include <sys/device.h>

extern struct evcnt tcp_reass_;
extern struct evcnt tcp_reass_empty;
extern struct evcnt tcp_reass_iteration[8];
extern struct evcnt tcp_reass_prependfirst;
extern struct evcnt tcp_reass_prepend;
extern struct evcnt tcp_reass_insert;
extern struct evcnt tcp_reass_inserttail;
extern struct evcnt tcp_reass_append;
extern struct evcnt tcp_reass_appendtail;
extern struct evcnt tcp_reass_overlaptail;
extern struct evcnt tcp_reass_overlapfront;
extern struct evcnt tcp_reass_segdup;
extern struct evcnt tcp_reass_fragdup;

#define	TCP_REASS_COUNTER_INCR(ev)	(ev)->ev_count++

#else

#define	TCP_REASS_COUNTER_INCR(ev)	/* nothing */

#endif /* TCP_REASS_COUNTERS */

static int tcp_reass(struct tcpcb *, const struct tcphdr *, struct mbuf *,
    int);

static void tcp4_log_refused(const struct ip *, const struct tcphdr *);
#ifdef INET6
static void tcp6_log_refused(const struct ip6_hdr *, const struct tcphdr *);
#endif

#if defined(MBUFTRACE)
struct mowner tcp_reass_mowner = MOWNER_INIT("tcp", "reass");
#endif /* defined(MBUFTRACE) */

static struct pool tcpipqent_pool;

void
tcpipqent_init(void)
{

	pool_init(&tcpipqent_pool, sizeof(struct ipqent), 0, 0, 0, "tcpipqepl",
	    NULL, IPL_VM);
}

struct ipqent *
tcpipqent_alloc(void)
{
	struct ipqent *ipqe;
	int s;

	s = splvm();
	ipqe = pool_get(&tcpipqent_pool, PR_NOWAIT);
	splx(s);

	return ipqe;
}

void
tcpipqent_free(struct ipqent *ipqe)
{
	int s;

	s = splvm();
	pool_put(&tcpipqent_pool, ipqe);
	splx(s);
}

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.
 */
static int
tcp_reass(struct tcpcb *tp, const struct tcphdr *th, struct mbuf *m, int tlen)
{
	struct ipqent *p, *q, *nq, *tiqe = NULL;
	struct socket *so = NULL;
	int pkt_flags;
	tcp_seq pkt_seq;
	unsigned pkt_len;
	u_long rcvpartdupbyte = 0;
	u_long rcvoobyte;
#ifdef TCP_REASS_COUNTERS
	u_int count = 0;
#endif
	uint64_t *tcps;

	so = tp->t_inpcb->inp_socket;

	TCP_REASS_LOCK_CHECK(tp);

	/*
	 * Call with th==NULL after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (th == NULL)
		goto present;

	m_claimm(m, &tcp_reass_mowner);

	rcvoobyte = tlen;
	/*
	 * Copy these to local variables because the TCP header gets munged
	 * while we are collapsing mbufs.
	 */
	pkt_seq = th->th_seq;
	pkt_len = tlen;
	pkt_flags = th->th_flags;

	TCP_REASS_COUNTER_INCR(&tcp_reass_);

	if ((p = TAILQ_LAST(&tp->segq, ipqehead)) != NULL) {
		/*
		 * When we miss a packet, the vast majority of time we get
		 * packets that follow it in order.  So optimize for that.
		 */
		if (pkt_seq == p->ipqe_seq + p->ipqe_len) {
			p->ipqe_len += pkt_len;
			p->ipqe_flags |= pkt_flags;
			m_cat(p->ipqe_m, m);
			m = NULL;
			tiqe = p;
			TAILQ_REMOVE(&tp->timeq, p, ipqe_timeq);
			TCP_REASS_COUNTER_INCR(&tcp_reass_appendtail);
			goto skip_replacement;
		}
		/*
		 * While we're here, if the pkt is completely beyond
		 * anything we have, just insert it at the tail.
		 */
		if (SEQ_GT(pkt_seq, p->ipqe_seq + p->ipqe_len)) {
			TCP_REASS_COUNTER_INCR(&tcp_reass_inserttail);
			goto insert_it;
		}
	}

	q = TAILQ_FIRST(&tp->segq);

	if (q != NULL) {
		/*
		 * If this segment immediately precedes the first out-of-order
		 * block, simply slap the segment in front of it and (mostly)
		 * skip the complicated logic.
		 */
		if (pkt_seq + pkt_len == q->ipqe_seq) {
			q->ipqe_seq = pkt_seq;
			q->ipqe_len += pkt_len;
			q->ipqe_flags |= pkt_flags;
			m_cat(m, q->ipqe_m);
			q->ipqe_m = m;
			tiqe = q;
			TAILQ_REMOVE(&tp->timeq, q, ipqe_timeq);
			TCP_REASS_COUNTER_INCR(&tcp_reass_prependfirst);
			goto skip_replacement;
		}
	} else {
		TCP_REASS_COUNTER_INCR(&tcp_reass_empty);
	}

	/*
	 * Find a segment which begins after this one does.
	 */
	for (p = NULL; q != NULL; q = nq) {
		nq = TAILQ_NEXT(q, ipqe_q);
#ifdef TCP_REASS_COUNTERS
		count++;
#endif

		/*
		 * If the received segment is just right after this
		 * fragment, merge the two together and then check
		 * for further overlaps.
		 */
		if (q->ipqe_seq + q->ipqe_len == pkt_seq) {
			pkt_len += q->ipqe_len;
			pkt_flags |= q->ipqe_flags;
			pkt_seq = q->ipqe_seq;
			m_cat(q->ipqe_m, m);
			m = q->ipqe_m;
			TCP_REASS_COUNTER_INCR(&tcp_reass_append);
			goto free_ipqe;
		}

		/*
		 * If the received segment is completely past this
		 * fragment, we need to go to the next fragment.
		 */
		if (SEQ_LT(q->ipqe_seq + q->ipqe_len, pkt_seq)) {
			p = q;
			continue;
		}

		/*
		 * If the fragment is past the received segment,
		 * it (or any following) can't be concatenated.
		 */
		if (SEQ_GT(q->ipqe_seq, pkt_seq + pkt_len)) {
			TCP_REASS_COUNTER_INCR(&tcp_reass_insert);
			break;
		}

		/*
		 * We've received all the data in this segment before.
		 * Mark it as a duplicate and return.
		 */
		if (SEQ_LEQ(q->ipqe_seq, pkt_seq) &&
		    SEQ_GEQ(q->ipqe_seq + q->ipqe_len, pkt_seq + pkt_len)) {
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVDUPPACK]++;
			tcps[TCP_STAT_RCVDUPBYTE] += pkt_len;
			TCP_STAT_PUTREF();
			tcp_new_dsack(tp, pkt_seq, pkt_len);
			m_freem(m);
			if (tiqe != NULL) {
				tcpipqent_free(tiqe);
			}
			TCP_REASS_COUNTER_INCR(&tcp_reass_segdup);
			goto out;
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
			TCP_REASS_COUNTER_INCR(&tcp_reass_fragdup);
			goto free_ipqe;
		}

		/*
		 * Received segment extends past the end of the fragment.
		 * Drop the overlapping bytes, merge the fragment and
		 * segment, and treat as a longer received packet.
		 */
		if (SEQ_LT(q->ipqe_seq, pkt_seq) &&
		    SEQ_GT(q->ipqe_seq + q->ipqe_len, pkt_seq))  {
			int overlap = q->ipqe_seq + q->ipqe_len - pkt_seq;
			m_adj(m, overlap);
			rcvpartdupbyte += overlap;
			m_cat(q->ipqe_m, m);
			m = q->ipqe_m;
			pkt_seq = q->ipqe_seq;
			pkt_len += q->ipqe_len - overlap;
			rcvoobyte -= overlap;
			TCP_REASS_COUNTER_INCR(&tcp_reass_overlaptail);
			goto free_ipqe;
		}

		/*
		 * Received segment extends past the front of the fragment.
		 * Drop the overlapping bytes on the received packet. The
		 * packet will then be concatenated with this fragment a
		 * bit later.
		 */
		if (SEQ_GT(q->ipqe_seq, pkt_seq) &&
		    SEQ_LT(q->ipqe_seq, pkt_seq + pkt_len))  {
			int overlap = pkt_seq + pkt_len - q->ipqe_seq;
			m_adj(m, -overlap);
			pkt_len -= overlap;
			rcvpartdupbyte += overlap;
			TCP_REASS_COUNTER_INCR(&tcp_reass_overlapfront);
			rcvoobyte -= overlap;
		}

		/*
		 * If the received segment immediately precedes this
		 * fragment then tack the fragment onto this segment
		 * and reinsert the data.
		 */
		if (q->ipqe_seq == pkt_seq + pkt_len) {
			pkt_len += q->ipqe_len;
			pkt_flags |= q->ipqe_flags;
			m_cat(m, q->ipqe_m);
			TAILQ_REMOVE(&tp->segq, q, ipqe_q);
			TAILQ_REMOVE(&tp->timeq, q, ipqe_timeq);
			tp->t_segqlen--;
			KASSERT(tp->t_segqlen >= 0);
			KASSERT(tp->t_segqlen != 0 ||
			    (TAILQ_EMPTY(&tp->segq) &&
			    TAILQ_EMPTY(&tp->timeq)));
			if (tiqe == NULL) {
				tiqe = q;
			} else {
				tcpipqent_free(q);
			}
			TCP_REASS_COUNTER_INCR(&tcp_reass_prepend);
			break;
		}

		/*
		 * If the fragment is before the segment, remember it.
		 * When this loop is terminated, p will contain the
		 * pointer to the fragment that is right before the
		 * received segment.
		 */
		if (SEQ_LEQ(q->ipqe_seq, pkt_seq))
			p = q;

		continue;

		/*
		 * This is a common operation.  It also will allow
		 * to save doing a malloc/free in most instances.
		 */
	  free_ipqe:
		TAILQ_REMOVE(&tp->segq, q, ipqe_q);
		TAILQ_REMOVE(&tp->timeq, q, ipqe_timeq);
		tp->t_segqlen--;
		KASSERT(tp->t_segqlen >= 0);
		KASSERT(tp->t_segqlen != 0 ||
		    (TAILQ_EMPTY(&tp->segq) && TAILQ_EMPTY(&tp->timeq)));
		if (tiqe == NULL) {
			tiqe = q;
		} else {
			tcpipqent_free(q);
		}
	}

#ifdef TCP_REASS_COUNTERS
	if (count > 7)
		TCP_REASS_COUNTER_INCR(&tcp_reass_iteration[0]);
	else if (count > 0)
		TCP_REASS_COUNTER_INCR(&tcp_reass_iteration[count]);
#endif

insert_it:
	/* limit tcp segments per reassembly queue */
	if (tp->t_segqlen > tcp_reass_maxqueuelen) {
		TCP_STATINC(TCP_STAT_RCVMEMDROP);
		m_freem(m);
		goto out;
	}

	/*
	 * Allocate a new queue entry (block) since the received segment
	 * did not collapse onto any other out-of-order block. If it had
	 * collapsed, tiqe would not be NULL and we would be reusing it.
	 *
	 * If the allocation fails, drop the packet.
	 */
	if (tiqe == NULL) {
		tiqe = tcpipqent_alloc();
		if (tiqe == NULL) {
			TCP_STATINC(TCP_STAT_RCVMEMDROP);
			m_freem(m);
			goto out;
		}
	}

	/*
	 * Update the counters.
	 */
	tp->t_rcvoopack++;
	tcps = TCP_STAT_GETREF();
	tcps[TCP_STAT_RCVOOPACK]++;
	tcps[TCP_STAT_RCVOOBYTE] += rcvoobyte;
	if (rcvpartdupbyte) {
	    tcps[TCP_STAT_RCVPARTDUPPACK]++;
	    tcps[TCP_STAT_RCVPARTDUPBYTE] += rcvpartdupbyte;
	}
	TCP_STAT_PUTREF();

	/*
	 * Insert the new fragment queue entry into both queues.
	 */
	tiqe->ipqe_m = m;
	tiqe->ipqe_seq = pkt_seq;
	tiqe->ipqe_len = pkt_len;
	tiqe->ipqe_flags = pkt_flags;
	if (p == NULL) {
		TAILQ_INSERT_HEAD(&tp->segq, tiqe, ipqe_q);
	} else {
		TAILQ_INSERT_AFTER(&tp->segq, p, tiqe, ipqe_q);
	}
	tp->t_segqlen++;

skip_replacement:
	TAILQ_INSERT_HEAD(&tp->timeq, tiqe, ipqe_timeq);

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVEESTABLISHED(tp->t_state) == 0)
		goto out;
	q = TAILQ_FIRST(&tp->segq);
	if (q == NULL || q->ipqe_seq != tp->rcv_nxt)
		goto out;
	if (tp->t_state == TCPS_SYN_RECEIVED && q->ipqe_len)
		goto out;

	tp->rcv_nxt += q->ipqe_len;
	pkt_flags = q->ipqe_flags & TH_FIN;
	nd_hint(tp);

	TAILQ_REMOVE(&tp->segq, q, ipqe_q);
	TAILQ_REMOVE(&tp->timeq, q, ipqe_timeq);
	tp->t_segqlen--;
	KASSERT(tp->t_segqlen >= 0);
	KASSERT(tp->t_segqlen != 0 ||
	    (TAILQ_EMPTY(&tp->segq) && TAILQ_EMPTY(&tp->timeq)));
	if (so->so_state & SS_CANTRCVMORE)
		m_freem(q->ipqe_m);
	else
		sbappendstream(&so->so_rcv, q->ipqe_m);
	tcpipqent_free(q);
	TCP_REASS_UNLOCK(tp);
	sorwakeup(so);
	return pkt_flags;

out:
	TCP_REASS_UNLOCK(tp);
	return 0;
}

#ifdef INET6
int
tcp6_input(struct mbuf **mp, int *offp, int proto)
{
	struct mbuf *m = *mp;

	/*
	 * draft-itojun-ipv6-tcp-to-anycast
	 * better place to put this in?
	 */
	if (m->m_flags & M_ANYCAST6) {
		struct ip6_hdr *ip6;
		if (m->m_len < sizeof(struct ip6_hdr)) {
			if ((m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
				TCP_STATINC(TCP_STAT_RCVSHORT);
				return IPPROTO_DONE;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_ADDR,
		    (char *)&ip6->ip6_dst - (char *)ip6);
		return IPPROTO_DONE;
	}

	tcp_input(m, *offp, proto);
	return IPPROTO_DONE;
}
#endif

static void
tcp4_log_refused(const struct ip *ip, const struct tcphdr *th)
{
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];

	if (ip) {
		in_print(src, sizeof(src), &ip->ip_src);
		in_print(dst, sizeof(dst), &ip->ip_dst);
	} else {
		strlcpy(src, "(unknown)", sizeof(src));
		strlcpy(dst, "(unknown)", sizeof(dst));
	}
	log(LOG_INFO,
	    "Connection attempt to TCP %s:%d from %s:%d\n",
	    dst, ntohs(th->th_dport),
	    src, ntohs(th->th_sport));
}

#ifdef INET6
static void
tcp6_log_refused(const struct ip6_hdr *ip6, const struct tcphdr *th)
{
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];

	if (ip6) {
		in6_print(src, sizeof(src), &ip6->ip6_src);
		in6_print(dst, sizeof(dst), &ip6->ip6_dst);
	} else {
		strlcpy(src, "(unknown v6)", sizeof(src));
		strlcpy(dst, "(unknown v6)", sizeof(dst));
	}
	log(LOG_INFO,
	    "Connection attempt to TCP [%s]:%d from [%s]:%d\n",
	    dst, ntohs(th->th_dport),
	    src, ntohs(th->th_sport));
}
#endif

/*
 * Checksum extended TCP header and data.
 */
int
tcp_input_checksum(int af, struct mbuf *m, const struct tcphdr *th,
    int toff, int off, int tlen)
{
	struct ifnet *rcvif;
	int s;

	/*
	 * XXX it's better to record and check if this mbuf is
	 * already checked.
	 */

	rcvif = m_get_rcvif(m, &s);
	if (__predict_false(rcvif == NULL))
		goto badcsum; /* XXX */

	switch (af) {
	case AF_INET:
		switch (m->m_pkthdr.csum_flags &
			((rcvif->if_csum_flags_rx & M_CSUM_TCPv4) |
			 M_CSUM_TCP_UDP_BAD | M_CSUM_DATA)) {
		case M_CSUM_TCPv4|M_CSUM_TCP_UDP_BAD:
			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_bad);
			goto badcsum;

		case M_CSUM_TCPv4|M_CSUM_DATA: {
			u_int32_t hw_csum = m->m_pkthdr.csum_data;

			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_data);
			if (m->m_pkthdr.csum_flags & M_CSUM_NO_PSEUDOHDR) {
				const struct ip *ip =
				    mtod(m, const struct ip *);

				hw_csum = in_cksum_phdr(ip->ip_src.s_addr,
				    ip->ip_dst.s_addr,
				    htons(hw_csum + tlen + off + IPPROTO_TCP));
			}
			if ((hw_csum ^ 0xffff) != 0)
				goto badcsum;
			break;
		}

		case M_CSUM_TCPv4:
			/* Checksum was okay. */
			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_ok);
			break;

		default:
			/*
			 * Must compute it ourselves.  Maybe skip checksum
			 * on loopback interfaces.
			 */
			if (__predict_true(!(rcvif->if_flags & IFF_LOOPBACK) ||
					   tcp_do_loopback_cksum)) {
				TCP_CSUM_COUNTER_INCR(&tcp_swcsum);
				if (in4_cksum(m, IPPROTO_TCP, toff,
					      tlen + off) != 0)
					goto badcsum;
			}
			break;
		}
		break;

#ifdef INET6
	case AF_INET6:
		switch (m->m_pkthdr.csum_flags &
			((rcvif->if_csum_flags_rx & M_CSUM_TCPv6) |
			 M_CSUM_TCP_UDP_BAD | M_CSUM_DATA)) {
		case M_CSUM_TCPv6|M_CSUM_TCP_UDP_BAD:
			TCP_CSUM_COUNTER_INCR(&tcp6_hwcsum_bad);
			goto badcsum;

#if 0 /* notyet */
		case M_CSUM_TCPv6|M_CSUM_DATA:
#endif

		case M_CSUM_TCPv6:
			/* Checksum was okay. */
			TCP_CSUM_COUNTER_INCR(&tcp6_hwcsum_ok);
			break;

		default:
			/*
			 * Must compute it ourselves.  Maybe skip checksum
			 * on loopback interfaces.
			 */
			if (__predict_true((m->m_flags & M_LOOP) == 0 ||
			    tcp_do_loopback_cksum)) {
				TCP_CSUM_COUNTER_INCR(&tcp6_swcsum);
				if (in6_cksum(m, IPPROTO_TCP, toff,
				    tlen + off) != 0)
					goto badcsum;
			}
		}
		break;
#endif /* INET6 */
	}
	m_put_rcvif(rcvif, &s);

	return 0;

badcsum:
	m_put_rcvif(rcvif, &s);
	TCP_STATINC(TCP_STAT_RCVBADSUM);
	return -1;
}

/*
 * When a packet arrives addressed to a vestigial tcpbp, we
 * nevertheless have to respond to it per the spec.
 *
 * This code is duplicated from the one in tcp_input().
 */
static void tcp_vtw_input(struct tcphdr *th, vestigial_inpcb_t *vp,
    struct mbuf *m, int tlen)
{
	int tiflags;
	int todrop;
	uint32_t t_flags = 0;
	uint64_t *tcps;

	tiflags = th->th_flags;
	todrop  = vp->rcv_nxt - th->th_seq;

	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			th->th_seq++;
			tcp_urp_drop(th, 1, &tiflags);
			todrop--;
		}
		if (todrop > tlen ||
		    (todrop == tlen && (tiflags & TH_FIN) == 0)) {
			/*
			 * Any valid FIN or RST must be to the left of the
			 * window.  At this point the FIN or RST must be a
			 * duplicate or out of sequence; drop it.
			 */
			if (tiflags & TH_RST)
				goto drop;
			tiflags &= ~(TH_FIN|TH_RST);

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */
			t_flags |= TF_ACKNOW;
			todrop = tlen;
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVDUPPACK] += 1;
			tcps[TCP_STAT_RCVDUPBYTE] += todrop;
			TCP_STAT_PUTREF();
		} else if ((tiflags & TH_RST) &&
		    th->th_seq != vp->rcv_nxt) {
			/*
			 * Test for reset before adjusting the sequence
			 * number for overlapping data.
			 */
			goto dropafterack_ratelim;
		} else {
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVPARTDUPPACK] += 1;
			tcps[TCP_STAT_RCVPARTDUPBYTE] += todrop;
			TCP_STAT_PUTREF();
		}

//		tcp_new_dsack(tp, th->th_seq, todrop);
//		hdroptlen += todrop;	/*drop from head afterwards*/

		th->th_seq += todrop;
		tlen -= todrop;
		tcp_urp_drop(th, todrop, &tiflags);
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if (tlen) {
		TCP_STATINC(TCP_STAT_RCVAFTERCLOSE);
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (th->th_seq + tlen) - (vp->rcv_nxt + vp->rcv_wnd);

	if (todrop > 0) {
		TCP_STATINC(TCP_STAT_RCVPACKAFTERWIN);
		if (todrop >= tlen) {
			/*
			 * The segment actually starts after the window.
			 * th->th_seq + tlen - vp->rcv_nxt - vp->rcv_wnd >= tlen
			 * th->th_seq - vp->rcv_nxt - vp->rcv_wnd >= 0
			 * th->th_seq >= vp->rcv_nxt + vp->rcv_wnd
			 */
			TCP_STATADD(TCP_STAT_RCVBYTEAFTERWIN, tlen);

			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if ((tiflags & TH_SYN) &&
			    SEQ_GT(th->th_seq, vp->rcv_nxt)) {
				/*
				 * We only support this in the !NOFDREF case, which
				 * is to say: not here.
				 */
				goto dropwithreset;
			}

			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and (if not RST) ack.
			 */
			if (vp->rcv_wnd == 0 && th->th_seq == vp->rcv_nxt) {
				t_flags |= TF_ACKNOW;
				TCP_STATINC(TCP_STAT_RCVWINPROBE);
			} else {
				goto dropafterack;
			}
		} else {
			TCP_STATADD(TCP_STAT_RCVBYTEAFTERWIN, todrop);
		}
		m_adj(m, -todrop);
		tlen -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	if (tiflags & TH_RST) {
		if (th->th_seq != vp->rcv_nxt)
			goto dropafterack_ratelim;

		vtw_del(vp->ctl, vp->vtw);
		goto drop;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0) {
		if (t_flags & TF_ACKNOW)
			goto dropafterack;
		goto drop;
	}

	/*
	 * In TIME_WAIT state the only thing that should arrive
	 * is a retransmission of the remote FIN.  Acknowledge
	 * it and restart the finack timer.
	 */
	vtw_restart(vp);
	goto dropafterack;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	goto dropafterack2;

dropafterack_ratelim:
	/*
	 * We may want to rate-limit ACKs against SYN/RST attack.
	 */
	if (ppsratecheck(&tcp_ackdrop_ppslim_last, &tcp_ackdrop_ppslim_count,
	    tcp_ackdrop_ppslim) == 0) {
		/* XXX stat */
		goto drop;
	}
	/* ...fall into dropafterack2... */

dropafterack2:
	(void)tcp_respond(0, m, m, th, th->th_seq + tlen, th->th_ack, TH_ACK);
	return;

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 */
	if (tiflags & TH_RST)
		goto drop;

	if (tiflags & TH_ACK) {
		tcp_respond(0, m, m, th, (tcp_seq)0, th->th_ack, TH_RST);
	} else {
		if (tiflags & TH_SYN)
			++tlen;
		(void)tcp_respond(0, m, m, th, th->th_seq + tlen, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	return;
drop:
	m_freem(m);
}

/*
 * TCP input routine, follows pages 65-76 of RFC 793 very closely.
 */
void
tcp_input(struct mbuf *m, int off, int proto)
{
	struct tcphdr *th;
	struct ip *ip;
	struct inpcb *inp;
#ifdef INET6
	struct ip6_hdr *ip6;
#endif
	u_int8_t *optp = NULL;
	int optlen = 0;
	int len, tlen, hdroptlen = 0;
	struct tcpcb *tp = NULL;
	int tiflags;
	struct socket *so = NULL;
	int todrop, acked, ourfinisacked, needoutput = 0;
	bool dupseg;
#ifdef TCP_DEBUG
	short ostate = 0;
#endif
	u_long tiwin;
	struct tcp_opt_info opti;
	int thlen, iphlen;
	int af;		/* af on the wire */
	struct mbuf *tcp_saveti = NULL;
	uint32_t ts_rtt;
	uint8_t iptos;
	uint64_t *tcps;
	vestigial_inpcb_t vestige;

	vestige.valid = 0;

	MCLAIM(m, &tcp_rx_mowner);

	TCP_STATINC(TCP_STAT_RCVTOTAL);

	memset(&opti, 0, sizeof(opti));
	opti.ts_present = 0;
	opti.maxseg = 0;

	/*
	 * RFC1122 4.2.3.10, p. 104: discard bcast/mcast SYN.
	 *
	 * TCP is, by definition, unicast, so we reject all
	 * multicast outright.
	 *
	 * Note, there are additional src/dst address checks in
	 * the AF-specific code below.
	 */
	if (m->m_flags & (M_BCAST|M_MCAST)) {
		/* XXX stat */
		goto drop;
	}
#ifdef INET6
	if (m->m_flags & M_ANYCAST6) {
		/* XXX stat */
		goto drop;
	}
#endif

	M_REGION_GET(th, struct tcphdr *, m, off, sizeof(struct tcphdr));
	if (th == NULL) {
		TCP_STATINC(TCP_STAT_RCVSHORT);
		return;
	}

	/*
	 * Enforce alignment requirements that are violated in
	 * some cases, see kern/50766 for details.
	 */
	if (ACCESSIBLE_POINTER(th, struct tcphdr) == 0) {
		m = m_copyup(m, off + sizeof(struct tcphdr), 0);
		if (m == NULL) {
			TCP_STATINC(TCP_STAT_RCVSHORT);
			return;
		}
		th = (struct tcphdr *)(mtod(m, char *) + off);
	}
	KASSERT(ACCESSIBLE_POINTER(th, struct tcphdr));

	/*
	 * Get IP and TCP header.
	 * Note: IP leaves IP header in first mbuf.
	 */
	ip = mtod(m, struct ip *);
#ifdef INET6
	ip6 = mtod(m, struct ip6_hdr *);
#endif
	switch (ip->ip_v) {
	case 4:
		af = AF_INET;
		iphlen = sizeof(struct ip);

		if (IN_MULTICAST(ip->ip_dst.s_addr) ||
		    in_broadcast(ip->ip_dst, m_get_rcvif_NOMPSAFE(m)))
			goto drop;

		/* We do the checksum after PCB lookup... */
		len = ntohs(ip->ip_len);
		tlen = len - off;
		iptos = ip->ip_tos;
		break;
#ifdef INET6
	case 6:
		iphlen = sizeof(struct ip6_hdr);
		af = AF_INET6;

		/*
		 * Be proactive about unspecified IPv6 address in source.
		 * As we use all-zero to indicate unbounded/unconnected pcb,
		 * unspecified IPv6 address can be used to confuse us.
		 *
		 * Note that packets with unspecified IPv6 destination is
		 * already dropped in ip6_input.
		 */
		if (IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src)) {
			/* XXX stat */
			goto drop;
		}

		/*
		 * Make sure destination address is not multicast.
		 * Source address checked in ip6_input().
		 */
		if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
			/* XXX stat */
			goto drop;
		}

		/* We do the checksum after PCB lookup... */
		len = m->m_pkthdr.len;
		tlen = len - off;
		iptos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;
		break;
#endif
	default:
		m_freem(m);
		return;
	}


	/*
	 * Check that TCP offset makes sense, pull out TCP options and
	 * adjust length.
	 */
	thlen = th->th_off << 2;
	if (thlen < sizeof(struct tcphdr) || thlen > tlen) {
		TCP_STATINC(TCP_STAT_RCVBADOFF);
		goto drop;
	}
	tlen -= thlen;

	if (thlen > sizeof(struct tcphdr)) {
		M_REGION_GET(th, struct tcphdr *, m, off, thlen);
		if (th == NULL) {
			TCP_STATINC(TCP_STAT_RCVSHORT);
			return;
		}
		KASSERT(ACCESSIBLE_POINTER(th, struct tcphdr));
		optlen = thlen - sizeof(struct tcphdr);
		optp = ((u_int8_t *)th) + sizeof(struct tcphdr);

		/*
		 * Do quick retrieval of timestamp options.
		 *
		 * If timestamp is the only option and it's formatted as
		 * recommended in RFC 1323 appendix A, we quickly get the
		 * values now and don't bother calling tcp_dooptions(),
		 * etc.
		 */
		if ((optlen == TCPOLEN_TSTAMP_APPA ||
		     (optlen > TCPOLEN_TSTAMP_APPA &&
		      optp[TCPOLEN_TSTAMP_APPA] == TCPOPT_EOL)) &&
		    be32dec(optp) == TCPOPT_TSTAMP_HDR &&
		    (th->th_flags & TH_SYN) == 0) {
			opti.ts_present = 1;
			opti.ts_val = be32dec(optp + 4);
			opti.ts_ecr = be32dec(optp + 8);
			optp = NULL;	/* we've parsed the options */
		}
	}
	tiflags = th->th_flags;

	/*
	 * Checksum extended TCP header and data
	 */
	if (tcp_input_checksum(af, m, th, off, thlen, tlen))
		goto badcsum;

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	inp = NULL;
	switch (af) {
	case AF_INET:
		inp = inpcb_lookup(&tcbtable, ip->ip_src, th->th_sport,
		    ip->ip_dst, th->th_dport, &vestige);
		if (inp == NULL && !vestige.valid) {
			TCP_STATINC(TCP_STAT_PCBHASHMISS);
			inp = inpcb_lookup_bound(&tcbtable, ip->ip_dst,
			    th->th_dport);
		}
#ifdef INET6
		if (inp == NULL && !vestige.valid) {
			struct in6_addr s, d;

			/* mapped addr case */
			in6_in_2_v4mapin6(&ip->ip_src, &s);
			in6_in_2_v4mapin6(&ip->ip_dst, &d);
			inp = in6pcb_lookup(&tcbtable, &s,
			    th->th_sport, &d, th->th_dport, 0, &vestige);
			if (inp == NULL && !vestige.valid) {
				TCP_STATINC(TCP_STAT_PCBHASHMISS);
				inp = in6pcb_lookup_bound(&tcbtable, &d,
				    th->th_dport, 0);
			}
		}
#endif
		if (inp == NULL && !vestige.valid) {
			TCP_STATINC(TCP_STAT_NOPORT);
			if (tcp_log_refused &&
			    (tiflags & (TH_RST|TH_ACK|TH_SYN)) == TH_SYN) {
				tcp4_log_refused(ip, th);
			}
			tcp_fields_to_host(th);
			goto dropwithreset_ratelim;
		}
#if defined(IPSEC)
		if (ipsec_used) {
			if (inp && ipsec_in_reject(m, inp))
				goto drop;
		}
#endif /*IPSEC*/
		break;
#ifdef INET6
	case AF_INET6:
	    {
		int faith;

#if defined(NFAITH) && NFAITH > 0
		faith = faithprefix(&ip6->ip6_dst);
#else
		faith = 0;
#endif
		inp = in6pcb_lookup(&tcbtable, &ip6->ip6_src,
		    th->th_sport, &ip6->ip6_dst, th->th_dport, faith, &vestige);
		if (inp == NULL && !vestige.valid) {
			TCP_STATINC(TCP_STAT_PCBHASHMISS);
			inp = in6pcb_lookup_bound(&tcbtable, &ip6->ip6_dst,
			    th->th_dport, faith);
		}
		if (inp == NULL && !vestige.valid) {
			TCP_STATINC(TCP_STAT_NOPORT);
			if (tcp_log_refused &&
			    (tiflags & (TH_RST|TH_ACK|TH_SYN)) == TH_SYN) {
				tcp6_log_refused(ip6, th);
			}
			tcp_fields_to_host(th);
			goto dropwithreset_ratelim;
		}
#if defined(IPSEC)
		if (ipsec_used && inp && ipsec_in_reject(m, inp))
			goto drop;
#endif
		break;
	    }
#endif
	}

	tcp_fields_to_host(th);

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	tp = NULL;
	so = NULL;
	if (inp) {
		/* Check the minimum TTL for socket. */
		if (inp->inp_af == AF_INET && ip->ip_ttl < in4p_ip_minttl(inp))
			goto drop;

		tp = intotcpcb(inp);
		so = inp->inp_socket;
	} else if (vestige.valid) {
		/* We do not support the resurrection of vtw tcpcps. */
		tcp_vtw_input(th, &vestige, m, tlen);
		m = NULL;
		goto drop;
	}

	if (tp == NULL)
		goto dropwithreset_ratelim;
	if (tp->t_state == TCPS_CLOSED)
		goto drop;

	KASSERT(so->so_lock == softnet_lock);
	KASSERT(solocked(so));

	/* Unscale the window into a 32-bit value. */
	if ((tiflags & TH_SYN) == 0)
		tiwin = th->th_win << tp->snd_scale;
	else
		tiwin = th->th_win;

#ifdef INET6
	/* save packet options if user wanted */
	if (inp->inp_af == AF_INET6 && (inp->inp_flags & IN6P_CONTROLOPTS)) {
		if (inp->inp_options) {
			m_freem(inp->inp_options);
			inp->inp_options = NULL;
		}
		ip6_savecontrol(inp, &inp->inp_options, ip6, m);
	}
#endif

	if (so->so_options & SO_DEBUG) {
#ifdef TCP_DEBUG
		ostate = tp->t_state;
#endif

		tcp_saveti = NULL;
		if (iphlen + sizeof(struct tcphdr) > MHLEN)
			goto nosave;

		if (m->m_len > iphlen && (m->m_flags & M_EXT) == 0) {
			tcp_saveti = m_copym(m, 0, iphlen, M_DONTWAIT);
			if (tcp_saveti == NULL)
				goto nosave;
		} else {
			MGETHDR(tcp_saveti, M_DONTWAIT, MT_HEADER);
			if (tcp_saveti == NULL)
				goto nosave;
			MCLAIM(m, &tcp_mowner);
			tcp_saveti->m_len = iphlen;
			m_copydata(m, 0, iphlen,
			    mtod(tcp_saveti, void *));
		}

		if (M_TRAILINGSPACE(tcp_saveti) < sizeof(struct tcphdr)) {
			m_freem(tcp_saveti);
			tcp_saveti = NULL;
		} else {
			tcp_saveti->m_len += sizeof(struct tcphdr);
			memcpy(mtod(tcp_saveti, char *) + iphlen, th,
			    sizeof(struct tcphdr));
		}
nosave:;
	}

	if (so->so_options & SO_ACCEPTCONN) {
		union syn_cache_sa src;
		union syn_cache_sa dst;

		KASSERT(tp->t_state == TCPS_LISTEN);

		memset(&src, 0, sizeof(src));
		memset(&dst, 0, sizeof(dst));
		switch (af) {
		case AF_INET:
			src.sin.sin_len = sizeof(struct sockaddr_in);
			src.sin.sin_family = AF_INET;
			src.sin.sin_addr = ip->ip_src;
			src.sin.sin_port = th->th_sport;

			dst.sin.sin_len = sizeof(struct sockaddr_in);
			dst.sin.sin_family = AF_INET;
			dst.sin.sin_addr = ip->ip_dst;
			dst.sin.sin_port = th->th_dport;
			break;
#ifdef INET6
		case AF_INET6:
			src.sin6.sin6_len = sizeof(struct sockaddr_in6);
			src.sin6.sin6_family = AF_INET6;
			src.sin6.sin6_addr = ip6->ip6_src;
			src.sin6.sin6_port = th->th_sport;

			dst.sin6.sin6_len = sizeof(struct sockaddr_in6);
			dst.sin6.sin6_family = AF_INET6;
			dst.sin6.sin6_addr = ip6->ip6_dst;
			dst.sin6.sin6_port = th->th_dport;
			break;
#endif
		}

		if ((tiflags & (TH_RST|TH_ACK|TH_SYN)) != TH_SYN) {
			if (tiflags & TH_RST) {
				syn_cache_reset(&src.sa, &dst.sa, th);
			} else if ((tiflags & (TH_ACK|TH_SYN)) ==
			    (TH_ACK|TH_SYN)) {
				/*
				 * Received a SYN,ACK. This should never
				 * happen while we are in LISTEN. Send an RST.
				 */
				goto badsyn;
			} else if (tiflags & TH_ACK) {
				so = syn_cache_get(&src.sa, &dst.sa, th, so, m);
				if (so == NULL) {
					/*
					 * We don't have a SYN for this ACK;
					 * send an RST.
					 */
					goto badsyn;
				} else if (so == (struct socket *)(-1)) {
					/*
					 * We were unable to create the
					 * connection. If the 3-way handshake
					 * was completed, and RST has been
					 * sent to the peer. Since the mbuf
					 * might be in use for the reply, do
					 * not free it.
					 */
					m = NULL;
				} else {
					/*
					 * We have created a full-blown
					 * connection.
					 */
					inp = sotoinpcb(so);
					tp = intotcpcb(inp);
					if (tp == NULL)
						goto badsyn;	/*XXX*/
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

#ifdef INET6
			/*
			 * If deprecated address is forbidden, we do
			 * not accept SYN to deprecated interface
			 * address to prevent any new inbound
			 * connection from getting established.
			 * When we do not accept SYN, we send a TCP
			 * RST, with deprecated source address (instead
			 * of dropping it).  We compromise it as it is
			 * much better for peer to send a RST, and
			 * RST will be the final packet for the
			 * exchange.
			 *
			 * If we do not forbid deprecated addresses, we
			 * accept the SYN packet.  RFC2462 does not
			 * suggest dropping SYN in this case.
			 * If we decipher RFC2462 5.5.4, it says like
			 * this:
			 * 1. use of deprecated addr with existing
			 *    communication is okay - "SHOULD continue
			 *    to be used"
			 * 2. use of it with new communication:
			 *   (2a) "SHOULD NOT be used if alternate
			 *        address with sufficient scope is
			 *        available"
			 *   (2b) nothing mentioned otherwise.
			 * Here we fall into (2b) case as we have no
			 * choice in our source address selection - we
			 * must obey the peer.
			 *
			 * The wording in RFC2462 is confusing, and
			 * there are multiple description text for
			 * deprecated address handling - worse, they
			 * are not exactly the same.  I believe 5.5.4
			 * is the best one, so we follow 5.5.4.
			 */
			if (af == AF_INET6 && !ip6_use_deprecated) {
				struct in6_ifaddr *ia6;
				int s;
				struct ifnet *rcvif = m_get_rcvif(m, &s);
				if (rcvif == NULL)
					goto dropwithreset; /* XXX */
				if ((ia6 = in6ifa_ifpwithaddr(rcvif,
				    &ip6->ip6_dst)) &&
				    (ia6->ia6_flags & IN6_IFF_DEPRECATED)) {
					tp = NULL;
					m_put_rcvif(rcvif, &s);
					goto dropwithreset;
				}
				m_put_rcvif(rcvif, &s);
			}
#endif

			/*
			 * LISTEN socket received a SYN from itself? This
			 * can't possibly be valid; drop the packet.
			 */
			if (th->th_sport == th->th_dport) {
				int eq = 0;

				switch (af) {
				case AF_INET:
					eq = in_hosteq(ip->ip_src, ip->ip_dst);
					break;
#ifdef INET6
				case AF_INET6:
					eq = IN6_ARE_ADDR_EQUAL(&ip6->ip6_src,
					    &ip6->ip6_dst);
					break;
#endif
				}
				if (eq) {
					TCP_STATINC(TCP_STAT_BADSYN);
					goto drop;
				}
			}

			/*
			 * SYN looks ok; create compressed TCP
			 * state for it.
			 */
			if (so->so_qlen <= so->so_qlimit &&
			    syn_cache_add(&src.sa, &dst.sa, th, off,
			    so, m, optp, optlen, &opti))
				m = NULL;
		}

		goto drop;
	}

after_listen:
	/*
	 * From here on, we're dealing with !LISTEN.
	 */
	KASSERT(tp->t_state != TCPS_LISTEN);

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_rcvtime = tcp_now;
	if (TCPS_HAVEESTABLISHED(tp->t_state))
		TCP_TIMER_ARM(tp, TCPT_KEEP, tp->t_keepidle);

	/*
	 * Process options.
	 */
#ifdef TCP_SIGNATURE
	if (optp || (tp->t_flags & TF_SIGNATURE))
#else
	if (optp)
#endif
		if (tcp_dooptions(tp, optp, optlen, th, m, off, &opti) < 0)
			goto drop;

	if (TCP_SACK_ENABLED(tp)) {
		tcp_del_sackholes(tp, th);
	}

	if (TCP_ECN_ALLOWED(tp)) {
		if (tiflags & TH_CWR) {
			tp->t_flags &= ~TF_ECN_SND_ECE;
		}
		switch (iptos & IPTOS_ECN_MASK) {
		case IPTOS_ECN_CE:
			tp->t_flags |= TF_ECN_SND_ECE;
			TCP_STATINC(TCP_STAT_ECN_CE);
			break;
		case IPTOS_ECN_ECT0:
			TCP_STATINC(TCP_STAT_ECN_ECT);
			break;
		case IPTOS_ECN_ECT1:
			/* XXX: ignore for now -- rpaulo */
			break;
		}
		/*
		 * Congestion experienced.
		 * Ignore if we are already trying to recover.
		 */
		if ((tiflags & TH_ECE) && SEQ_GEQ(tp->snd_una, tp->snd_recover))
			tp->t_congctl->cong_exp(tp);
	}

	if (opti.ts_present && opti.ts_ecr) {
		/*
		 * Calculate the RTT from the returned time stamp and the
		 * connection's time base.  If the time stamp is later than
		 * the current time, or is extremely old, fall back to non-1323
		 * RTT calculation.  Since ts_rtt is unsigned, we can test both
		 * at the same time.
		 *
		 * Note that ts_rtt is in units of slow ticks (500
		 * ms).  Since most earthbound RTTs are < 500 ms,
		 * observed values will have large quantization noise.
		 * Our smoothed RTT is then the fraction of observed
		 * samples that are 1 tick instead of 0 (times 500
		 * ms).
		 *
		 * ts_rtt is increased by 1 to denote a valid sample,
		 * with 0 indicating an invalid measurement.  This
		 * extra 1 must be removed when ts_rtt is used, or
		 * else an erroneous extra 500 ms will result.
		 */
		ts_rtt = TCP_TIMESTAMP(tp) - opti.ts_ecr + 1;
		if (ts_rtt > TCP_PAWS_IDLE)
			ts_rtt = 0;
	} else {
		ts_rtt = 0;
	}

	/*
	 * Fast path: check for the two common cases of a uni-directional
	 * data transfer. If:
	 *    o We are in the ESTABLISHED state, and
	 *    o The packet has no control flags, and
	 *    o The packet is in-sequence, and
	 *    o The window didn't change, and
	 *    o We are not retransmitting
	 * It's a candidate.
	 *
	 * If the length (tlen) is zero and the ack moved forward, we're
	 * the sender side of the transfer. Just free the data acked and
	 * wake any higher level process that was blocked waiting for
	 * space.
	 *
	 * If the length is non-zero and the ack didn't move, we're the
	 * receiver side. If we're getting packets in-order (the reassembly
	 * queue is empty), add the data to the socket buffer and note
	 * that we need a delayed ack.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tiflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ECE|TH_CWR|TH_ACK))
	        == TH_ACK &&
	    (!opti.ts_present || TSTMP_GEQ(opti.ts_val, tp->ts_recent)) &&
	    th->th_seq == tp->rcv_nxt &&
	    tiwin && tiwin == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {

		/*
		 * If last ACK falls within this segment's sequence numbers,
		 * record the timestamp.
		 * NOTE that the test is modified according to the latest
		 * proposal of the tcplw@cray.com list (Braden 1993/04/26).
		 *
		 * note that we already know
		 *	TSTMP_GEQ(opti.ts_val, tp->ts_recent)
		 */
		if (opti.ts_present && SEQ_LEQ(th->th_seq, tp->last_ack_sent)) {
			tp->ts_recent_age = tcp_now;
			tp->ts_recent = opti.ts_val;
		}

		if (tlen == 0) {
			/* Ack prediction. */
			if (SEQ_GT(th->th_ack, tp->snd_una) &&
			    SEQ_LEQ(th->th_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd &&
			    tp->t_partialacks < 0) {
				/*
				 * this is a pure ack for outstanding data.
				 */
				if (ts_rtt)
					tcp_xmit_timer(tp, ts_rtt - 1);
				else if (tp->t_rtttime &&
				    SEQ_GT(th->th_ack, tp->t_rtseq))
					tcp_xmit_timer(tp,
					  tcp_now - tp->t_rtttime);
				acked = th->th_ack - tp->snd_una;
				tcps = TCP_STAT_GETREF();
				tcps[TCP_STAT_PREDACK]++;
				tcps[TCP_STAT_RCVACKPACK]++;
				tcps[TCP_STAT_RCVACKBYTE] += acked;
				TCP_STAT_PUTREF();
				nd_hint(tp);

				if (acked > (tp->t_lastoff - tp->t_inoff))
					tp->t_lastm = NULL;
				sbdrop(&so->so_snd, acked);
				tp->t_lastoff -= acked;

				icmp_check(tp, th, acked);

				tp->snd_una = th->th_ack;
				tp->snd_fack = tp->snd_una;
				if (SEQ_LT(tp->snd_high, tp->snd_una))
					tp->snd_high = tp->snd_una;
				/*
				 * drag snd_wl2 along so only newer
				 * ACKs can update the window size.
				 * also avoids the state where snd_wl2
				 * is eventually larger than th_ack and thus
				 * blocking the window update mechanism and
				 * the connection gets stuck for a loooong
				 * time in the zero sized send window state.
				 *
				 * see PR/kern 55567
				 */
				tp->snd_wl2 = tp->snd_una;

				m_freem(m);

				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selnotify/signal.  If data
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
				if (so->so_snd.sb_cc) {
					KERNEL_LOCK(1, NULL);
					(void)tcp_output(tp);
					KERNEL_UNLOCK_ONE(NULL);
				}
				if (tcp_saveti)
					m_freem(tcp_saveti);
				return;
			}
		} else if (th->th_ack == tp->snd_una &&
		    TAILQ_FIRST(&tp->segq) == NULL &&
		    tlen <= sbspace(&so->so_rcv)) {
			int newsize = 0;

			/*
			 * this is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			tp->rcv_nxt += tlen;

			/*
			 * Pull rcv_up up to prevent seq wrap relative to
			 * rcv_nxt.
			 */
			tp->rcv_up = tp->rcv_nxt;

			/*
			 * Pull snd_wl1 up to prevent seq wrap relative to
			 * th_seq.
			 */
			tp->snd_wl1 = th->th_seq;

			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_PREDDAT]++;
			tcps[TCP_STAT_RCVPACK]++;
			tcps[TCP_STAT_RCVBYTE] += tlen;
			TCP_STAT_PUTREF();
			nd_hint(tp);
		/*
		 * Automatic sizing enables the performance of large buffers
		 * and most of the efficiency of small ones by only allocating
		 * space when it is needed.
		 *
		 * On the receive side the socket buffer memory is only rarely
		 * used to any significant extent.  This allows us to be much
		 * more aggressive in scaling the receive socket buffer.  For
		 * the case that the buffer space is actually used to a large
		 * extent and we run out of kernel memory we can simply drop
		 * the new segments; TCP on the sender will just retransmit it
		 * later.  Setting the buffer size too big may only consume too
		 * much kernel memory if the application doesn't read() from
		 * the socket or packet loss or reordering makes use of the
		 * reassembly queue.
		 *
		 * The criteria to step up the receive buffer one notch are:
		 *  1. the number of bytes received during the time it takes
		 *     one timestamp to be reflected back to us (the RTT);
		 *  2. received bytes per RTT is within seven eighth of the
		 *     current socket buffer size;
		 *  3. receive buffer size has not hit maximal automatic size;
		 *
		 * This algorithm does one step per RTT at most and only if
		 * we receive a bulk stream w/o packet losses or reorderings.
		 * Shrinking the buffer during idle times is not necessary as
		 * it doesn't consume any memory when idle.
		 *
		 * TODO: Only step up if the application is actually serving
		 * the buffer to better manage the socket buffer resources.
		 */
			if (tcp_do_autorcvbuf &&
			    opti.ts_ecr &&
			    (so->so_rcv.sb_flags & SB_AUTOSIZE)) {
				if (opti.ts_ecr > tp->rfbuf_ts &&
				    opti.ts_ecr - tp->rfbuf_ts < PR_SLOWHZ) {
					if (tp->rfbuf_cnt >
					    (so->so_rcv.sb_hiwat / 8 * 7) &&
					    so->so_rcv.sb_hiwat <
					    tcp_autorcvbuf_max) {
						newsize =
						    uimin(so->so_rcv.sb_hiwat +
						    tcp_autorcvbuf_inc,
						    tcp_autorcvbuf_max);
					}
					/* Start over with next RTT. */
					tp->rfbuf_ts = 0;
					tp->rfbuf_cnt = 0;
				} else
					tp->rfbuf_cnt += tlen;	/* add up */
			}

			/*
			 * Drop TCP, IP headers and TCP options then add data
			 * to socket buffer.
			 */
			if (so->so_state & SS_CANTRCVMORE) {
				m_freem(m);
			} else {
				/*
				 * Set new socket buffer size.
				 * Give up when limit is reached.
				 */
				if (newsize)
					if (!sbreserve(&so->so_rcv,
					    newsize, so))
						so->so_rcv.sb_flags &= ~SB_AUTOSIZE;
				m_adj(m, off + thlen);
				sbappendstream(&so->so_rcv, m);
			}
			sorwakeup(so);
			tcp_setup_ack(tp, th);
			if (tp->t_flags & TF_ACKNOW) {
				KERNEL_LOCK(1, NULL);
				(void)tcp_output(tp);
				KERNEL_UNLOCK_ONE(NULL);
			}
			if (tcp_saveti)
				m_freem(tcp_saveti);
			return;
		}
	}

	/*
	 * Compute mbuf offset to TCP data segment.
	 */
	hdroptlen = off + thlen;

	/*
	 * Calculate amount of space in receive window. Receive window is
	 * amount of space in rcv queue, but not less than advertised
	 * window.
	 */
	{
		int win;
		win = sbspace(&so->so_rcv);
		if (win < 0)
			win = 0;
		tp->rcv_wnd = imax(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	/* Reset receive buffer auto scaling when not in bulk receive mode. */
	tp->rfbuf_ts = 0;
	tp->rfbuf_cnt = 0;

	switch (tp->t_state) {
	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if seg contains a ECE and ECN support is enabled, the stream
	 *	    is ECN capable.
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->iss) ||
		     SEQ_GT(th->th_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK)
				tp = tcp_drop(tp, ECONNREFUSED);
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = th->th_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
			if (SEQ_LT(tp->snd_high, tp->snd_una))
				tp->snd_high = tp->snd_una;
			TCP_TIMER_DISARM(tp, TCPT_REXMT);

			if ((tiflags & TH_ECE) && tcp_do_ecn) {
				tp->t_flags |= TF_ECN_PERMIT;
				TCP_STATINC(TCP_STAT_ECN_SHS);
			}
		}
		tp->irs = th->th_seq;
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
		else {
			int ss = tcp_init_win;
			if (inp->inp_af == AF_INET && in_localaddr(in4p_faddr(inp)))
				ss = tcp_init_win_local;
#ifdef INET6
			else if (inp->inp_af == AF_INET6 && in6_localaddr(&in6p_faddr(inp)))
				ss = tcp_init_win_local;
#endif
			tp->snd_cwnd = TCP_INITIAL_WINDOW(ss, tp->t_peermss);
		}

		tcp_rmx_rtt(tp);
		if (tiflags & TH_ACK) {
			TCP_STATINC(TCP_STAT_CONNECTS);
			/*
			 * move tcp_established before soisconnected
			 * because upcall handler can drive tcp_output
			 * functionality.
			 * XXX we might call soisconnected at the end of
			 * all processing
			 */
			tcp_established(tp);
			soisconnected(so);
			/* Do window scaling on this connection? */
			if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
			    (TF_RCVD_SCALE|TF_REQ_SCALE)) {
				tp->snd_scale = tp->requested_s_scale;
				tp->rcv_scale = tp->request_r_scale;
			}
			TCP_REASS_LOCK(tp);
			(void)tcp_reass(tp, NULL, NULL, tlen);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtttime)
				tcp_xmit_timer(tp, tcp_now - tp->t_rtttime);
		} else {
			tp->t_state = TCPS_SYN_RECEIVED;
		}

		/*
		 * Advance th->th_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		th->th_seq++;
		if (tlen > tp->rcv_wnd) {
			todrop = tlen - tp->rcv_wnd;
			m_adj(m, -todrop);
			tlen = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVPACKAFTERWIN]++;
			tcps[TCP_STAT_RCVBYTEAFTERWIN] += todrop;
			TCP_STAT_PUTREF();
		}
		tp->snd_wl1 = th->th_seq - 1;
		tp->rcv_up = th->th_seq;
		goto step6;

	/*
	 * If the state is SYN_RECEIVED:
	 *	If seg contains an ACK, but not for our SYN, drop the input
	 *	and generate an RST.  See page 36, rfc793
	 */
	case TCPS_SYN_RECEIVED:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->iss) ||
		     SEQ_GT(th->th_ack, tp->snd_max)))
			goto dropwithreset;
		break;
	}

	/*
	 * From here on, we're dealing with !LISTEN and !SYN_SENT.
	 */
	KASSERT(tp->t_state != TCPS_LISTEN &&
	    tp->t_state != TCPS_SYN_SENT);

	/*
	 * RFC1323 PAWS: if we have a timestamp reply on this segment and
	 * it's less than ts_recent, drop it.
	 */
	if (opti.ts_present && (tiflags & TH_RST) == 0 && tp->ts_recent &&
	    TSTMP_LT(opti.ts_val, tp->ts_recent)) {
		/* Check to see if ts_recent is over 24 days old.  */
		if (tcp_now - tp->ts_recent_age > TCP_PAWS_IDLE) {
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
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVDUPPACK]++;
			tcps[TCP_STAT_RCVDUPBYTE] += tlen;
			tcps[TCP_STAT_PAWSDROP]++;
			TCP_STAT_PUTREF();
			tcp_new_dsack(tp, th->th_seq, tlen);
			goto dropafterack;
		}
	}

	/*
	 * Check that at least some bytes of the segment are within the
	 * receive window. If segment begins before rcv_nxt, drop leading
	 * data (and SYN); if nothing left, just ack.
	 */
	todrop = tp->rcv_nxt - th->th_seq;
	dupseg = false;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			th->th_seq++;
			tcp_urp_drop(th, 1, &tiflags);
			todrop--;
		}
		if (todrop > tlen ||
		    (todrop == tlen && (tiflags & TH_FIN) == 0)) {
			/*
			 * Any valid FIN or RST must be to the left of the
			 * window.  At this point the FIN or RST must be a
			 * duplicate or out of sequence; drop it.
			 */
			if (tiflags & TH_RST)
				goto drop;
			tiflags &= ~(TH_FIN|TH_RST);

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */
			tp->t_flags |= TF_ACKNOW;
			todrop = tlen;
			dupseg = true;
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVDUPPACK]++;
			tcps[TCP_STAT_RCVDUPBYTE] += todrop;
			TCP_STAT_PUTREF();
		} else if ((tiflags & TH_RST) && th->th_seq != tp->rcv_nxt) {
			/*
			 * Test for reset before adjusting the sequence
			 * number for overlapping data.
			 */
			goto dropafterack_ratelim;
		} else {
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVPARTDUPPACK]++;
			tcps[TCP_STAT_RCVPARTDUPBYTE] += todrop;
			TCP_STAT_PUTREF();
		}
		tcp_new_dsack(tp, th->th_seq, todrop);
		hdroptlen += todrop;	/* drop from head afterwards (m_adj) */
		th->th_seq += todrop;
		tlen -= todrop;
		tcp_urp_drop(th, todrop, &tiflags);
	}

	/*
	 * If new data is received on a connection after the user processes
	 * are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && tlen) {
		tp = tcp_close(tp);
		TCP_STATINC(TCP_STAT_RCVAFTERCLOSE);
		goto dropwithreset;
	}

	/*
	 * If the segment ends after the window, drop trailing data (and
	 * PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (th->th_seq + tlen) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		TCP_STATINC(TCP_STAT_RCVPACKAFTERWIN);
		if (todrop >= tlen) {
			/*
			 * The segment actually starts after the window.
			 * th->th_seq + tlen - tp->rcv_nxt - tp->rcv_wnd >= tlen
			 * th->th_seq - tp->rcv_nxt - tp->rcv_wnd >= 0
			 * th->th_seq >= tp->rcv_nxt + tp->rcv_wnd
			 */
			TCP_STATADD(TCP_STAT_RCVBYTEAFTERWIN, tlen);

			/*
			 * If a new connection request is received while in
			 * TIME_WAIT, drop the old connection and start over
			 * if the sequence numbers are above the previous
			 * ones.
			 *
			 * NOTE: We need to put the header fields back into
			 * network order.
			 */
			if ((tiflags & TH_SYN) &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(th->th_seq, tp->rcv_nxt)) {
				tp = tcp_close(tp);
				tcp_fields_to_net(th);
				m_freem(tcp_saveti);
				tcp_saveti = NULL;
				goto findpcb;
			}

			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and (if not RST) ack.
			 */
			if (tp->rcv_wnd == 0 && th->th_seq == tp->rcv_nxt) {
				KASSERT(todrop == tlen);
				tp->t_flags |= TF_ACKNOW;
				TCP_STATINC(TCP_STAT_RCVWINPROBE);
			} else {
				goto dropafterack;
			}
		} else {
			TCP_STATADD(TCP_STAT_RCVBYTEAFTERWIN, todrop);
		}
		m_adj(m, -todrop);
		tlen -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If last ACK falls within this segment's sequence numbers,
	 *  record the timestamp.
	 * NOTE: 
	 * 1) That the test incorporates suggestions from the latest
	 *    proposal of the tcplw@cray.com list (Braden 1993/04/26).
	 * 2) That updating only on newer timestamps interferes with
	 *    our earlier PAWS tests, so this check should be solely
	 *    predicated on the sequence space of this segment.
	 * 3) That we modify the segment boundary check to be 
	 *        Last.ACK.Sent <= SEG.SEQ + SEG.Len  
	 *    instead of RFC1323's
	 *        Last.ACK.Sent < SEG.SEQ + SEG.Len,
	 *    This modified check allows us to overcome RFC1323's
	 *    limitations as described in Stevens TCP/IP Illustrated
	 *    Vol. 2 p.869. In such cases, we can still calculate the
	 *    RTT correctly when RCV.NXT == Last.ACK.Sent.
	 */
	if (opti.ts_present &&
	    SEQ_LEQ(th->th_seq, tp->last_ack_sent) &&
	    SEQ_LEQ(tp->last_ack_sent, th->th_seq + tlen +
	         ((tiflags & (TH_SYN|TH_FIN)) != 0))) {
		tp->ts_recent_age = tcp_now;
		tp->ts_recent = opti.ts_val;
	}

	/*
	 * If the RST bit is set examine the state:
	 *    RECEIVED state:
	 *        If passive open, return to LISTEN state.
	 *        If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT states:
	 *        Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT states:
	 *        Close the tcb.
	 */
	if (tiflags & TH_RST) {
		if (th->th_seq != tp->rcv_nxt)
			goto dropafterack_ratelim;

		switch (tp->t_state) {
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
			TCP_STATINC(TCP_STAT_DROPS);
			tp = tcp_close(tp);
			goto drop;

		case TCPS_CLOSING:
		case TCPS_LAST_ACK:
		case TCPS_TIME_WAIT:
			tp = tcp_close(tp);
			goto drop;
		}
	}

	/*
	 * Since we've covered the SYN-SENT and SYN-RECEIVED states above
	 * we must be in a synchronized state.  RFC793 states (under Reset
	 * Generation) that any unacceptable segment (an out-of-order SYN
	 * qualifies) received in a synchronized state must elicit only an
	 * empty acknowledgment segment ... and the connection remains in
	 * the same state.
	 */
	if (tiflags & TH_SYN) {
		if (tp->rcv_nxt == th->th_seq) {
			tcp_respond(tp, m, m, th, (tcp_seq)0, th->th_ack - 1,
			    TH_ACK);
			if (tcp_saveti)
				m_freem(tcp_saveti);
			return;
		}

		goto dropafterack_ratelim;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0) {
		if (tp->t_flags & TF_ACKNOW)
			goto dropafterack;
		goto drop;
	}

	/*
	 * From here on, we're doing ACK processing.
	 */

	switch (tp->t_state) {
	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send an RST.
	 */
	case TCPS_SYN_RECEIVED:
		if (SEQ_GT(tp->snd_una, th->th_ack) ||
		    SEQ_GT(th->th_ack, tp->snd_max))
			goto dropwithreset;
		TCP_STATINC(TCP_STAT_CONNECTS);
		soisconnected(so);
		tcp_established(tp);
		/* Do window scaling? */
		if ((tp->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
		    (TF_RCVD_SCALE|TF_REQ_SCALE)) {
			tp->snd_scale = tp->requested_s_scale;
			tp->rcv_scale = tp->request_r_scale;
		}
		TCP_REASS_LOCK(tp);
		(void)tcp_reass(tp, NULL, NULL, tlen);
		tp->snd_wl1 = th->th_seq - 1;
		/* FALLTHROUGH */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < th->th_ack <= tp->snd_max
	 * then advance tp->snd_una to th->th_ack and drop
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
		if (SEQ_LEQ(th->th_ack, tp->snd_una)) {
			if (tlen == 0 && !dupseg && tiwin == tp->snd_wnd) {
				TCP_STATINC(TCP_STAT_RCVDUPACK);
				/*
				 * If we have outstanding data (other than
				 * a window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.
				 */
				if (TCP_TIMER_ISARMED(tp, TCPT_REXMT) == 0 ||
				    th->th_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (tp->t_partialacks < 0 &&
				    (++tp->t_dupacks == tcprexmtthresh ||
				     TCP_FACK_FASTRECOV(tp))) {
					/*
					 * Do the fast retransmit, and adjust
					 * congestion control parameters.
					 */
					if (tp->t_congctl->fast_retransmit(tp, th)) {
						/* False fast retransmit */
						break;
					}
					goto drop;
				} else if (tp->t_dupacks > tcprexmtthresh) {
					tp->snd_cwnd += tp->t_segsz;
					KERNEL_LOCK(1, NULL);
					(void)tcp_output(tp);
					KERNEL_UNLOCK_ONE(NULL);
					goto drop;
				}
			} else {
				/*
				 * If the ack appears to be very old, only
				 * allow data that is in-sequence.  This
				 * makes it somewhat more difficult to insert
				 * forged data by guessing sequence numbers.
				 * Sent an ack to try to update the send
				 * sequence number on the other side.
				 */
				if (tlen && th->th_seq != tp->rcv_nxt &&
				    SEQ_LT(th->th_ack,
				    tp->snd_una - tp->max_sndwnd))
					goto dropafterack;
			}
			break;
		}
		/*
		 * If the congestion window was inflated to account
		 * for the other side's cached packets, retract it.
		 */
		tp->t_congctl->fast_retransmit_newack(tp, th);

		if (SEQ_GT(th->th_ack, tp->snd_max)) {
			TCP_STATINC(TCP_STAT_RCVACKTOOMUCH);
			goto dropafterack;
		}
		acked = th->th_ack - tp->snd_una;
		tcps = TCP_STAT_GETREF();
		tcps[TCP_STAT_RCVACKPACK]++;
		tcps[TCP_STAT_RCVACKBYTE] += acked;
		TCP_STAT_PUTREF();

		/*
		 * If we have a timestamp reply, update smoothed
		 * round trip time.  If no timestamp is present but
		 * transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (ts_rtt)
			tcp_xmit_timer(tp, ts_rtt - 1);
		else if (tp->t_rtttime && SEQ_GT(th->th_ack, tp->t_rtseq))
			tcp_xmit_timer(tp, tcp_now - tp->t_rtttime);

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (th->th_ack == tp->snd_max) {
			TCP_TIMER_DISARM(tp, TCPT_REXMT);
			needoutput = 1;
		} else if (TCP_TIMER_ISARMED(tp, TCPT_PERSIST) == 0)
			TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);

		/*
		 * New data has been acked, adjust the congestion window.
		 */
		tp->t_congctl->newack(tp, th);

		nd_hint(tp);
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
		} else {
			if (acked > (tp->t_lastoff - tp->t_inoff))
				tp->t_lastm = NULL;
			sbdrop(&so->so_snd, acked);
			tp->t_lastoff -= acked;
			if (tp->snd_wnd > acked)
				tp->snd_wnd -= acked;
			else
				tp->snd_wnd = 0;
			ourfinisacked = 0;
		}
		sowwakeup(so);

		icmp_check(tp, th, acked);

		tp->snd_una = th->th_ack;
		if (SEQ_GT(tp->snd_una, tp->snd_fack))
			tp->snd_fack = tp->snd_una;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;
		if (SEQ_LT(tp->snd_high, tp->snd_una))
			tp->snd_high = tp->snd_una;

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
					if (tp->t_maxidle > 0)
						TCP_TIMER_ARM(tp, TCPT_2MSL,
						    tp->t_maxidle);
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
				TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * tp->t_msl);
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
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * tp->t_msl);
			goto dropafterack;
		}
	}

step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if ((tiflags & TH_ACK) && (SEQ_LT(tp->snd_wl1, th->th_seq) ||
	    (tp->snd_wl1 == th->th_seq && (SEQ_LT(tp->snd_wl2, th->th_ack) ||
	    (tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd))))) {
		/* keep track of pure window updates */
		if (tlen == 0 &&
		    tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd)
			TCP_STATINC(TCP_STAT_RCVWINUPD);
		tp->snd_wnd = tiwin;
		tp->snd_wl1 = th->th_seq;
		tp->snd_wl2 = th->th_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && th->th_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (th->th_urp + so->so_rcv.sb_cc > sb_max) {
			th->th_urp = 0;			/* XXX */
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
		if (SEQ_GT(th->th_seq+th->th_urp, tp->rcv_up)) {
			tp->rcv_up = th->th_seq + th->th_urp;
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
		if (th->th_urp <= (u_int16_t)tlen &&
		    (so->so_options & SO_OOBINLINE) == 0)
			tcp_pulloutofband(so, th, m, hdroptlen);
	} else {
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
	}
dodata:

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgement of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * tcp_rcvd()).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((tlen || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * Handle the common case:
		 *  o Segment is the next to be received, and
		 *  o The queue is empty, and
		 *  o The connection is established
		 * In this case, we avoid calling tcp_reass.
		 *
		 * tcp_setup_ack: set DELACK for segments received in order,
		 * but ack immediately when segments are out of order (so that
		 * fast retransmit can work).
		 */
		TCP_REASS_LOCK(tp);
		if (th->th_seq == tp->rcv_nxt &&
		    TAILQ_FIRST(&tp->segq) == NULL &&
		    tp->t_state == TCPS_ESTABLISHED) {
			tcp_setup_ack(tp, th);
			tp->rcv_nxt += tlen;
			tiflags = th->th_flags & TH_FIN;
			tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_RCVPACK]++;
			tcps[TCP_STAT_RCVBYTE] += tlen;
			TCP_STAT_PUTREF();
			nd_hint(tp);
			if (so->so_state & SS_CANTRCVMORE) {
				m_freem(m);
			} else {
				m_adj(m, hdroptlen);
				sbappendstream(&(so)->so_rcv, m);
			}
			TCP_REASS_UNLOCK(tp);
			sorwakeup(so);
		} else {
			m_adj(m, hdroptlen);
			tiflags = tcp_reass(tp, th, m, tlen);
			tp->t_flags |= TF_ACKNOW;
		}

		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
	} else {
		m_freem(m);
		m = NULL;
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
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * tp->t_msl);
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */
		case TCPS_TIME_WAIT:
			TCP_TIMER_ARM(tp, TCPT_2MSL, 2 * tp->t_msl);
			break;
		}
	}
#ifdef TCP_DEBUG
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_INPUT, ostate, tp, tcp_saveti, 0);
#endif

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW)) {
		KERNEL_LOCK(1, NULL);
		(void)tcp_output(tp);
		KERNEL_UNLOCK_ONE(NULL);
	}
	if (tcp_saveti)
		m_freem(tcp_saveti);

	if (tp->t_state == TCPS_TIME_WAIT
	    && (so->so_state & SS_NOFDREF)
	    && (tp->t_inpcb || af != AF_INET || af != AF_INET6)
	    && ((af == AF_INET ? tcp4_vtw_enable : tcp6_vtw_enable) & 1) != 0
	    && TAILQ_EMPTY(&tp->segq)
	    && vtw_add(af, tp)) {
		;
	}
	return;

badsyn:
	/*
	 * Received a bad SYN.  Increment counters and dropwithreset.
	 */
	TCP_STATINC(TCP_STAT_BADSYN);
	tp = NULL;
	goto dropwithreset;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	goto dropafterack2;

dropafterack_ratelim:
	/*
	 * We may want to rate-limit ACKs against SYN/RST attack.
	 */
	if (ppsratecheck(&tcp_ackdrop_ppslim_last, &tcp_ackdrop_ppslim_count,
	    tcp_ackdrop_ppslim) == 0) {
		/* XXX stat */
		goto drop;
	}

dropafterack2:
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	KERNEL_LOCK(1, NULL);
	(void)tcp_output(tp);
	KERNEL_UNLOCK_ONE(NULL);
	if (tcp_saveti)
		m_freem(tcp_saveti);
	return;

dropwithreset_ratelim:
	/*
	 * We may want to rate-limit RSTs in certain situations,
	 * particularly if we are sending an RST in response to
	 * an attempt to connect to or otherwise communicate with
	 * a port for which we have no socket.
	 */
	if (ppsratecheck(&tcp_rst_ppslim_last, &tcp_rst_ppslim_count,
	    tcp_rst_ppslim) == 0) {
		/* XXX stat */
		goto drop;
	}

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 */
	if (tiflags & TH_RST)
		goto drop;
	if (tiflags & TH_ACK) {
		(void)tcp_respond(tp, m, m, th, (tcp_seq)0, th->th_ack, TH_RST);
	} else {
		if (tiflags & TH_SYN)
			tlen++;
		(void)tcp_respond(tp, m, m, th, th->th_seq + tlen, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	if (tcp_saveti)
		m_freem(tcp_saveti);
	return;

badcsum:
drop:
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp) {
		so = tp->t_inpcb->inp_socket;
#ifdef TCP_DEBUG
		if (so && (so->so_options & SO_DEBUG) != 0)
			tcp_trace(TA_DROP, ostate, tp, tcp_saveti, 0);
#endif
	}
	if (tcp_saveti)
		m_freem(tcp_saveti);
	m_freem(m);
	return;
}

#ifdef TCP_SIGNATURE
int
tcp_signature_apply(void *fstate, void *data, u_int len)
{

	MD5Update(fstate, (u_char *)data, len);
	return (0);
}

struct secasvar *
tcp_signature_getsav(struct mbuf *m)
{
	struct ip *ip;
	struct ip6_hdr *ip6;

	ip = mtod(m, struct ip *);
	switch (ip->ip_v) {
	case 4:
		ip = mtod(m, struct ip *);
		ip6 = NULL;
		break;
	case 6:
		ip = NULL;
		ip6 = mtod(m, struct ip6_hdr *);
		break;
	default:
		return (NULL);
	}

#ifdef IPSEC
	union sockaddr_union dst;

	/* Extract the destination from the IP header in the mbuf. */
	memset(&dst, 0, sizeof(union sockaddr_union));
	if (ip != NULL) {
		dst.sa.sa_len = sizeof(struct sockaddr_in);
		dst.sa.sa_family = AF_INET;
		dst.sin.sin_addr = ip->ip_dst;
	} else {
		dst.sa.sa_len = sizeof(struct sockaddr_in6);
		dst.sa.sa_family = AF_INET6;
		dst.sin6.sin6_addr = ip6->ip6_dst;
	}

	/*
	 * Look up an SADB entry which matches the address of the peer.
	 */
	return KEY_LOOKUP_SA(&dst, IPPROTO_TCP, htonl(TCP_SIG_SPI), 0, 0);
#else
	return NULL;
#endif
}

int
tcp_signature(struct mbuf *m, struct tcphdr *th, int thoff,
    struct secasvar *sav, char *sig)
{
	MD5_CTX ctx;
	struct ip *ip;
	struct ipovly *ipovly;
#ifdef INET6
	struct ip6_hdr *ip6;
	struct ip6_hdr_pseudo ip6pseudo;
#endif
	struct ippseudo ippseudo;
	struct tcphdr th0;
	int l, tcphdrlen;

	if (sav == NULL)
		return (-1);

	tcphdrlen = th->th_off * 4;

	switch (mtod(m, struct ip *)->ip_v) {
	case 4:
		MD5Init(&ctx);
		ip = mtod(m, struct ip *);
		memset(&ippseudo, 0, sizeof(ippseudo));
		ipovly = (struct ipovly *)ip;
		ippseudo.ippseudo_src = ipovly->ih_src;
		ippseudo.ippseudo_dst = ipovly->ih_dst;
		ippseudo.ippseudo_pad = 0;
		ippseudo.ippseudo_p = IPPROTO_TCP;
		ippseudo.ippseudo_len = htons(m->m_pkthdr.len - thoff);
		MD5Update(&ctx, (char *)&ippseudo, sizeof(ippseudo));
		break;
#if INET6
	case 6:
		MD5Init(&ctx);
		ip6 = mtod(m, struct ip6_hdr *);
		memset(&ip6pseudo, 0, sizeof(ip6pseudo));
		ip6pseudo.ip6ph_src = ip6->ip6_src;
		in6_clearscope(&ip6pseudo.ip6ph_src);
		ip6pseudo.ip6ph_dst = ip6->ip6_dst;
		in6_clearscope(&ip6pseudo.ip6ph_dst);
		ip6pseudo.ip6ph_len = htons(m->m_pkthdr.len - thoff);
		ip6pseudo.ip6ph_nxt = IPPROTO_TCP;
		MD5Update(&ctx, (char *)&ip6pseudo, sizeof(ip6pseudo));
		break;
#endif
	default:
		return (-1);
	}

	th0 = *th;
	th0.th_sum = 0;
	MD5Update(&ctx, (char *)&th0, sizeof(th0));

	l = m->m_pkthdr.len - thoff - tcphdrlen;
	if (l > 0)
		m_apply(m, thoff + tcphdrlen,
		    m->m_pkthdr.len - thoff - tcphdrlen,
		    tcp_signature_apply, &ctx);

	MD5Update(&ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
	MD5Final(sig, &ctx);

	return (0);
}
#endif

/*
 * Parse and process tcp options.
 *
 * Returns -1 if this segment should be dropped.  (eg. wrong signature)
 * Otherwise returns 0.
 */
int
tcp_dooptions(struct tcpcb *tp, const u_char *cp, int cnt, struct tcphdr *th,
    struct mbuf *m, int toff, struct tcp_opt_info *oi)
{
	u_int16_t mss;
	int opt, optlen = 0;
#ifdef TCP_SIGNATURE
	void *sigp = NULL;
	char sigbuf[TCP_SIGLEN];
	struct secasvar *sav = NULL;
#endif

	for (; cp && cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			if (cnt < 2)
				break;
			optlen = cp[1];
			if (optlen < 2 || optlen > cnt)
				break;
		}
		switch (opt) {

		default:
			continue;

		case TCPOPT_MAXSEG:
			if (optlen != TCPOLEN_MAXSEG)
				continue;
			if (!(th->th_flags & TH_SYN))
				continue;
			if (TCPS_HAVERCVDSYN(tp->t_state))
				continue;
			memcpy(&mss, cp + 2, sizeof(mss));
			oi->maxseg = ntohs(mss);
			break;

		case TCPOPT_WINDOW:
			if (optlen != TCPOLEN_WINDOW)
				continue;
			if (!(th->th_flags & TH_SYN))
				continue;
			if (TCPS_HAVERCVDSYN(tp->t_state))
				continue;
			tp->t_flags |= TF_RCVD_SCALE;
			tp->requested_s_scale = cp[2];
			if (tp->requested_s_scale > TCP_MAX_WINSHIFT) {
				char buf[INET6_ADDRSTRLEN];
				struct ip *ip = mtod(m, struct ip *);
#ifdef INET6
				struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
#endif

				switch (ip->ip_v) {
				case 4:
					in_print(buf, sizeof(buf),
					    &ip->ip_src);
					break;
#ifdef INET6
				case 6:
					in6_print(buf, sizeof(buf),
					    &ip6->ip6_src);
					break;
#endif
				default:
					strlcpy(buf, "(unknown)", sizeof(buf));
					break;
				}

				log(LOG_ERR, "TCP: invalid wscale %d from %s, "
				    "assuming %d\n",
				    tp->requested_s_scale, buf,
				    TCP_MAX_WINSHIFT);
				tp->requested_s_scale = TCP_MAX_WINSHIFT;
			}
			break;

		case TCPOPT_TIMESTAMP:
			if (optlen != TCPOLEN_TIMESTAMP)
				continue;
			oi->ts_present = 1;
			memcpy(&oi->ts_val, cp + 2, sizeof(oi->ts_val));
			NTOHL(oi->ts_val);
			memcpy(&oi->ts_ecr, cp + 6, sizeof(oi->ts_ecr));
			NTOHL(oi->ts_ecr);

			if (!(th->th_flags & TH_SYN))
				continue;
			if (TCPS_HAVERCVDSYN(tp->t_state))
				continue;
			/*
			 * A timestamp received in a SYN makes
			 * it ok to send timestamp requests and replies.
			 */
			tp->t_flags |= TF_RCVD_TSTMP;
			tp->ts_recent = oi->ts_val;
			tp->ts_recent_age = tcp_now;
                        break;

		case TCPOPT_SACK_PERMITTED:
			if (optlen != TCPOLEN_SACK_PERMITTED)
				continue;
			if (!(th->th_flags & TH_SYN))
				continue;
			if (TCPS_HAVERCVDSYN(tp->t_state))
				continue;
			if (tcp_do_sack) {
				tp->t_flags |= TF_SACK_PERMIT;
				tp->t_flags |= TF_WILL_SACK;
			}
			break;

		case TCPOPT_SACK:
			tcp_sack_option(tp, th, cp, optlen);
			break;
#ifdef TCP_SIGNATURE
		case TCPOPT_SIGNATURE:
			if (optlen != TCPOLEN_SIGNATURE)
				continue;
			if (sigp &&
			    !consttime_memequal(sigp, cp + 2, TCP_SIGLEN))
				return (-1);

			sigp = sigbuf;
			memcpy(sigbuf, cp + 2, TCP_SIGLEN);
			tp->t_flags |= TF_SIGNATURE;
			break;
#endif
		}
	}

#ifndef TCP_SIGNATURE
	return 0;
#else
	if (tp->t_flags & TF_SIGNATURE) {
		sav = tcp_signature_getsav(m);
		if (sav == NULL && tp->t_state == TCPS_LISTEN)
			return (-1);
	}

	if ((sigp ? TF_SIGNATURE : 0) ^ (tp->t_flags & TF_SIGNATURE))
		goto out;

	if (sigp) {
		char sig[TCP_SIGLEN];

		tcp_fields_to_net(th);
		if (tcp_signature(m, th, toff, sav, sig) < 0) {
			tcp_fields_to_host(th);
			goto out;
		}
		tcp_fields_to_host(th);

		if (!consttime_memequal(sig, sigp, TCP_SIGLEN)) {
			TCP_STATINC(TCP_STAT_BADSIG);
			goto out;
		} else
			TCP_STATINC(TCP_STAT_GOODSIG);

		key_sa_recordxfer(sav, m);
		KEY_SA_UNREF(&sav);
	}
	return 0;
out:
	if (sav != NULL)
		KEY_SA_UNREF(&sav);
	return -1;
#endif
}

/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
void
tcp_pulloutofband(struct socket *so, struct tcphdr *th,
    struct mbuf *m, int off)
{
	int cnt = off + th->th_urp - 1;

	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, char *) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			memmove(cp, cp + 1, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == NULL)
			break;
	}
	panic("tcp_pulloutofband");
}

/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 *
 * rtt is in units of slow ticks (typically 500 ms) -- essentially the
 * difference of two timestamps.
 */
void
tcp_xmit_timer(struct tcpcb *tp, uint32_t rtt)
{
	int32_t delta;

	TCP_STATINC(TCP_STAT_RTTUPDATED);
	if (tp->t_srtt != 0) {
		/*
		 * Compute the amount to add to srtt for smoothing,
		 * *alpha, or 2^(-TCP_RTT_SHIFT).  Because
		 * srtt is stored in 1/32 slow ticks, we conceptually
		 * shift left 5 bits, subtract srtt to get the
		 * difference, and then shift right by TCP_RTT_SHIFT
		 * (3) to obtain 1/8 of the difference.
		 */
		delta = (rtt << 2) - (tp->t_srtt >> TCP_RTT_SHIFT);
		/* 
		 * This can never happen, because delta's lowest
		 * possible value is 1/8 of t_srtt.  But if it does,
		 * set srtt to some reasonable value, here chosen
		 * as 1/8 tick.
		 */
		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1 << 2;
		/*
		 * RFC2988 requires that rttvar be updated first.
		 * This code is compliant because "delta" is the old
		 * srtt minus the new observation (scaled).
		 *
		 * RFC2988 says:
		 *   rttvar = (1-beta) * rttvar + beta * |srtt-observed|
		 *
		 * delta is in units of 1/32 ticks, and has then been
		 * divided by 8.  This is equivalent to being in 1/16s
		 * units and divided by 4.  Subtract from it 1/4 of
		 * the existing rttvar to form the (signed) amount to
		 * adjust.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (tp->t_rttvar >> TCP_RTTVAR_SHIFT);
		/*
		 * As with srtt, this should never happen.  There is
		 * no support in RFC2988 for this operation.  But 1/4s
		 * as rttvar when faced with something arguably wrong
		 * is ok.
		 */
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1 << 2;

		/*
		 * If srtt exceeds .01 second, ensure we use the 'remote' MSL
		 * Problem is: it doesn't work.  Disabled by defaulting
		 * tcp_rttlocal to 0; see corresponding code in
		 * tcp_subr that selects local vs remote in a different way.
		 *
		 * The static branch prediction hint here should be removed
		 * when the rtt estimator is fixed and the rtt_enable code
		 * is turned back on.
		 */
		if (__predict_false(tcp_rttlocal) && tcp_msl_enable
		    && tp->t_srtt > tcp_msl_remote_threshold
		    && tp->t_msl  < tcp_msl_remote) {
			tp->t_msl = MIN(tcp_msl_remote, TCP_MAXMSL);
		}
	} else {
		/*
		 * This is the first measurement.  Per RFC2988, 2.2,
		 * set rtt=R and srtt=R/2.
		 * For srtt, storage representation is 1/32 ticks,
		 * so shift left by 5.
		 * For rttvar, storage representation is 1/16 ticks,
		 * So shift left by 4, but then right by 1 to halve.
		 */
		tp->t_srtt = rtt << (TCP_RTT_SHIFT + 2);
		tp->t_rttvar = rtt << (TCP_RTTVAR_SHIFT + 2 - 1);
	}
	tp->t_rtttime = 0;
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
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
	    uimax(tp->t_rttmin, rtt + 2), TCPTV_REXMTMAX);

	/*
	 * We received an ack for a packet that wasn't retransmitted;
	 * it is probably safe to discard any error indications we've
	 * received recently.  This isn't quite right, but close enough
	 * for now (a route might have failed after we sent a segment,
	 * and the return path might not be symmetrical).
	 */
	tp->t_softerror = 0;
}
