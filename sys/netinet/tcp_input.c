/*	$NetBSD: tcp_input.c,v 1.122.2.6 2002/02/28 04:15:12 nathanw Exp $	*/

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
 * Copyright (c) 1997, 1998, 1999, 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcp_input.c,v 1.122.2.6 2002/02/28 04:15:12 nathanw Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_inet_csum.h"
#include "opt_tcp_debug.h"

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

#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#endif

#ifdef PULLDOWN_TEST
#ifndef INET6
/* always need ip6.h for IP6_EXTHDR_GET */
#include <netinet/ip6.h>
#endif
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>

#include <machine/stdarg.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif /*IPSEC*/
#ifdef INET6
#include "faith.h"
#if defined(NFAITH) && NFAITH > 0
#include <net/if_faith.h>
#endif
#endif

int	tcprexmtthresh = 3;
int	tcp_log_refused;

static int tcp_rst_ppslim_count = 0;
static struct timeval tcp_rst_ppslim_last;

#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

/* for modulo comparisons of timestamps */
#define TSTMP_LT(a,b)	((int)((a)-(b)) < 0)
#define TSTMP_GEQ(a,b)	((int)((a)-(b)) >= 0)

/*
 * Neighbor Discovery, Neighbor Unreachability Detection Upper layer hint.
 */
#ifdef INET6
#define ND6_HINT(tp) \
do { \
	if (tp && tp->t_in6pcb && tp->t_family == AF_INET6 \
	 && tp->t_in6pcb->in6p_route.ro_rt) { \
		nd6_nud_hint(tp->t_in6pcb->in6p_route.ro_rt, NULL, 0); \
	} \
} while (0)
#else
#define ND6_HINT(tp)
#endif

/*
 * Macro to compute ACK transmission behavior.  Delay the ACK unless
 * we have already delayed an ACK (must send an ACK every two segments).
 * We also ACK immediately if we received a PUSH and the ACK-on-PUSH
 * option is enabled.
 */
#define	TCP_SETUP_ACK(tp, th) \
do { \
	if ((tp)->t_flags & TF_DELACK || \
	    (tcp_ack_on_push && (th)->th_flags & TH_PUSH)) \
		tp->t_flags |= TF_ACKNOW; \
	else \
		TCP_SET_DELACK(tp); \
} while (0)

/*
 * Convert TCP protocol fields to host order for easier processing.
 */
#define	TCP_FIELDS_TO_HOST(th)						\
do {									\
	NTOHL((th)->th_seq);						\
	NTOHL((th)->th_ack);						\
	NTOHS((th)->th_win);						\
	NTOHS((th)->th_urp);						\
} while (0)

#ifdef TCP_CSUM_COUNTERS
#include <sys/device.h>

extern struct evcnt tcp_hwcsum_ok;
extern struct evcnt tcp_hwcsum_bad;
extern struct evcnt tcp_hwcsum_data;
extern struct evcnt tcp_swcsum;

#define	TCP_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++

#else

#define	TCP_CSUM_COUNTER_INCR(ev)	/* nothing */

#endif /* TCP_CSUM_COUNTERS */

int
tcp_reass(tp, th, m, tlen)
	struct tcpcb *tp;
	struct tcphdr *th;
	struct mbuf *m;
	int *tlen;
{
	struct ipqent *p, *q, *nq, *tiqe = NULL;
	struct socket *so = NULL;
	int pkt_flags;
	tcp_seq pkt_seq;
	unsigned pkt_len;
	u_long rcvpartdupbyte = 0;
	u_long rcvoobyte;

	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#ifdef INET6
	else if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif

	TCP_REASS_LOCK_CHECK(tp);

	/*
	 * Call with th==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (th == 0)
		goto present;

	rcvoobyte = *tlen;
	/*
	 * Copy these to local variables because the tcpiphdr
	 * gets munged while we are collapsing mbufs.
	 */
	pkt_seq = th->th_seq;
	pkt_len = *tlen;
	pkt_flags = th->th_flags;
	/*
	 * Find a segment which begins after this one does.
	 */
	for (p = NULL, q = LIST_FIRST(&tp->segq); q != NULL; q = nq) {
		nq = LIST_NEXT(q, ipqe_q);
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
	q = LIST_FIRST(&tp->segq);
	if (q == NULL || q->ipqe_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && q->ipqe_len)
		return (0);

	tp->rcv_nxt += q->ipqe_len;
	pkt_flags = q->ipqe_flags & TH_FIN;
	ND6_HINT(tp);

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

#ifdef INET6
int
tcp6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
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
				tcpstat.tcps_rcvshort++;
				return IPPROTO_DONE;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_ADDR,
		    (caddr_t)&ip6->ip6_dst - (caddr_t)ip6);
		return IPPROTO_DONE;
	}

	tcp_input(m, *offp, proto);
	return IPPROTO_DONE;
}
#endif

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
void
#if __STDC__
tcp_input(struct mbuf *m, ...)
#else
tcp_input(m, va_alist)
	struct mbuf *m;
#endif
{
	int proto;
	struct tcphdr *th;
	struct ip *ip;
	struct inpcb *inp;
#ifdef INET6
	struct ip6_hdr *ip6;
	struct in6pcb *in6p;
#endif
	caddr_t optp = NULL;
	int optlen = 0;
	int len, tlen, toff, hdroptlen = 0;
	struct tcpcb *tp = 0;
	int tiflags;
	struct socket *so = NULL;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate = 0;
	int iss = 0;
	u_long tiwin;
	struct tcp_opt_info opti;
	int off, iphlen;
	va_list ap;
	int af;		/* af on the wire */
	struct mbuf *tcp_saveti = NULL;

	va_start(ap, m);
	toff = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	tcpstat.tcps_rcvtotal++;

	bzero(&opti, sizeof(opti));
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

	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */
	ip = mtod(m, struct ip *);
#ifdef INET6
	ip6 = NULL;
#endif
	switch (ip->ip_v) {
#ifdef INET
	case 4:
		af = AF_INET;
		iphlen = sizeof(struct ip);
#ifndef PULLDOWN_TEST
		/* would like to get rid of this... */
		if (toff > sizeof (struct ip)) {
			ip_stripoptions(m, (struct mbuf *)0);
			toff = sizeof(struct ip);
		}
		if (m->m_len < toff + sizeof (struct tcphdr)) {
			if ((m = m_pullup(m, toff + sizeof (struct tcphdr))) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		}
		ip = mtod(m, struct ip *);
		th = (struct tcphdr *)(mtod(m, caddr_t) + toff);
#else
		ip = mtod(m, struct ip *);
		IP6_EXTHDR_GET(th, struct tcphdr *, m, toff,
			sizeof(struct tcphdr));
		if (th == NULL) {
			tcpstat.tcps_rcvshort++;
			return;
		}
#endif

		/*
		 * Make sure destination address is not multicast.
		 * Source address checked in ip_input().
		 */
		if (IN_MULTICAST(ip->ip_dst.s_addr)) {
			/* XXX stat */
			goto drop;
		}

		/* We do the checksum after PCB lookup... */
		len = ip->ip_len;
		tlen = len - toff;
		break;
#endif
#ifdef INET6
	case 6:
		ip = NULL;
		iphlen = sizeof(struct ip6_hdr);
		af = AF_INET6;
#ifndef PULLDOWN_TEST
		if (m->m_len < toff + sizeof(struct tcphdr)) {
			m = m_pullup(m, toff + sizeof(struct tcphdr));	/*XXX*/
			if (m == NULL) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		th = (struct tcphdr *)(mtod(m, caddr_t) + toff);
#else
		ip6 = mtod(m, struct ip6_hdr *);
		IP6_EXTHDR_GET(th, struct tcphdr *, m, toff,
			sizeof(struct tcphdr));
		if (th == NULL) {
			tcpstat.tcps_rcvshort++;
			return;
		}
#endif

		/* Be proactive about malicious use of IPv4 mapped address */
		if (IN6_IS_ADDR_V4MAPPED(&ip6->ip6_src) ||
		    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst)) {
			/* XXX stat */
			goto drop;
		}

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
		tlen = len - toff;
		break;
#endif
	default:
		m_freem(m);
		return;
	}

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.		XXX
	 */
	off = th->th_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		tcpstat.tcps_rcvbadoff++;
		goto drop;
	}
	tlen -= off;

	/* 
	 * tcp_input() has been modified to use tlen to mean the TCP data
	 * length throughout the function.  Other functions can use
	 * m->m_pkthdr.len as the basis for calculating the TCP data length.
	 * rja
	 */

	if (off > sizeof (struct tcphdr)) {
#ifndef PULLDOWN_TEST
		if (m->m_len < toff + off) {
			if ((m = m_pullup(m, toff + off)) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
			switch (af) {
#ifdef INET
			case AF_INET:
				ip = mtod(m, struct ip *);
				break;
#endif
#ifdef INET6
			case AF_INET6:
				ip6 = mtod(m, struct ip6_hdr *);
				break;
#endif
			}
			th = (struct tcphdr *)(mtod(m, caddr_t) + toff);
		}
#else
		IP6_EXTHDR_GET(th, struct tcphdr *, m, toff, off);
		if (th == NULL) {
			tcpstat.tcps_rcvshort++;
			return;
		}
		/*
		 * NOTE: ip/ip6 will not be affected by m_pulldown()
		 * (as they're before toff) and we don't need to update those.
		 */
#endif
		optlen = off - sizeof (struct tcphdr);
		optp = ((caddr_t)th) + sizeof(struct tcphdr);
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
		     (th->th_flags & TH_SYN) == 0) {
			opti.ts_present = 1;
			opti.ts_val = ntohl(*(u_int32_t *)(optp + 4));
			opti.ts_ecr = ntohl(*(u_int32_t *)(optp + 8));
			optp = NULL;	/* we've parsed the options */
		}
	}
	tiflags = th->th_flags;

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	inp = NULL;
#ifdef INET6
	in6p = NULL;
#endif
	switch (af) {
#ifdef INET
	case AF_INET:
		inp = in_pcblookup_connect(&tcbtable, ip->ip_src, th->th_sport,
		    ip->ip_dst, th->th_dport);
		if (inp == 0) {
			++tcpstat.tcps_pcbhashmiss;
			inp = in_pcblookup_bind(&tcbtable, ip->ip_dst, th->th_dport);
		}
#ifdef INET6
		if (inp == 0) {
			struct in6_addr s, d;

			/* mapped addr case */
			bzero(&s, sizeof(s));
			s.s6_addr16[5] = htons(0xffff);
			bcopy(&ip->ip_src, &s.s6_addr32[3], sizeof(ip->ip_src));
			bzero(&d, sizeof(d));
			d.s6_addr16[5] = htons(0xffff);
			bcopy(&ip->ip_dst, &d.s6_addr32[3], sizeof(ip->ip_dst));
			in6p = in6_pcblookup_connect(&tcb6, &s, th->th_sport,
				&d, th->th_dport, 0);
			if (in6p == 0) {
				++tcpstat.tcps_pcbhashmiss;
				in6p = in6_pcblookup_bind(&tcb6, &d,
					th->th_dport, 0);
			}
		}
#endif
#ifndef INET6
		if (inp == 0)
#else
		if (inp == 0 && in6p == 0)
#endif
		{
			++tcpstat.tcps_noport;
			if (tcp_log_refused && (tiflags & TH_SYN)) {
#ifndef INET6
				char src[4*sizeof "123"];
				char dst[4*sizeof "123"];
#else
				char src[INET6_ADDRSTRLEN];
				char dst[INET6_ADDRSTRLEN];
#endif
				if (ip) {
					strcpy(src, inet_ntoa(ip->ip_src));
					strcpy(dst, inet_ntoa(ip->ip_dst));
				}
#ifdef INET6
				else if (ip6) {
					strcpy(src, ip6_sprintf(&ip6->ip6_src));
					strcpy(dst, ip6_sprintf(&ip6->ip6_dst));
				}
#endif
				else {
					strcpy(src, "(unknown)");
					strcpy(dst, "(unknown)");
				}
				log(LOG_INFO,
				    "Connection attempt to TCP %s:%d from %s:%d\n",
				    dst, ntohs(th->th_dport),
				    src, ntohs(th->th_sport));
			}
			TCP_FIELDS_TO_HOST(th);
			goto dropwithreset_ratelim;
		}
#ifdef IPSEC
		if (inp && ipsec4_in_reject(m, inp)) {
			ipsecstat.in_polvio++;
			goto drop;
		}
#ifdef INET6
		else if (in6p && ipsec4_in_reject_so(m, in6p->in6p_socket)) {
			ipsecstat.in_polvio++;
			goto drop;
		}
#endif
#endif /*IPSEC*/
		break;
#endif /*INET*/
#ifdef INET6
	case AF_INET6:
	    {
		int faith;

#if defined(NFAITH) && NFAITH > 0
		faith = faithprefix(&ip6->ip6_dst);
#else
		faith = 0;
#endif
		in6p = in6_pcblookup_connect(&tcb6, &ip6->ip6_src, th->th_sport,
			&ip6->ip6_dst, th->th_dport, faith);
		if (in6p == NULL) {
			++tcpstat.tcps_pcbhashmiss;
			in6p = in6_pcblookup_bind(&tcb6, &ip6->ip6_dst,
				th->th_dport, faith);
		}
		if (in6p == NULL) {
			++tcpstat.tcps_noport;
			TCP_FIELDS_TO_HOST(th);
			goto dropwithreset_ratelim;
		}
#ifdef IPSEC
		if (ipsec6_in_reject(m, in6p)) {
			ipsec6stat.in_polvio++;
			goto drop;
		}
#endif /*IPSEC*/
		break;
	    }
#endif
	}

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	tp = NULL;
	so = NULL;
	if (inp) {
		tp = intotcpcb(inp);
		so = inp->inp_socket;
	}
#ifdef INET6
	else if (in6p) {
		tp = in6totcpcb(in6p);
		so = in6p->in6p_socket;
	}
#endif
	if (tp == 0) {
		TCP_FIELDS_TO_HOST(th);
		goto dropwithreset_ratelim;
	}
	if (tp->t_state == TCPS_CLOSED)
		goto drop;

	/*
	 * Checksum extended TCP header and data.
	 */
	switch (af) {
#ifdef INET
	case AF_INET:
		switch (m->m_pkthdr.csum_flags &
			((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_TCPv4) |
			 M_CSUM_TCP_UDP_BAD | M_CSUM_DATA)) {
		case M_CSUM_TCPv4|M_CSUM_TCP_UDP_BAD:
			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_bad);
			goto badcsum;

		case M_CSUM_TCPv4|M_CSUM_DATA:
			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_data);
			if ((m->m_pkthdr.csum_data ^ 0xffff) != 0)
				goto badcsum;
			break;

		case M_CSUM_TCPv4:
			/* Checksum was okay. */
			TCP_CSUM_COUNTER_INCR(&tcp_hwcsum_ok);
			break;

		default:
			/* Must compute it ourselves. */
			TCP_CSUM_COUNTER_INCR(&tcp_swcsum);
#ifndef PULLDOWN_TEST
		    {
			struct ipovly *ipov;
			ipov = (struct ipovly *)ip;
			bzero(ipov->ih_x1, sizeof ipov->ih_x1); 
			ipov->ih_len = htons(tlen + off);

			if (in_cksum(m, len) != 0)
				goto badcsum;
		    }
#else
			if (in4_cksum(m, IPPROTO_TCP, toff, tlen + off) != 0)
				goto badcsum;
#endif /* ! PULLDOWN_TEST */
			break;
		}
		break;
#endif /* INET4 */

#ifdef INET6
	case AF_INET6:
		if (in6_cksum(m, IPPROTO_TCP, toff, tlen + off) != 0)
			goto badcsum;
		break;
#endif /* INET6 */
	}

	TCP_FIELDS_TO_HOST(th);

	/* Unscale the window into a 32-bit value. */
	if ((tiflags & TH_SYN) == 0)
		tiwin = th->th_win << tp->snd_scale;
	else
		tiwin = th->th_win;

#ifdef INET6
	/* save packet options if user wanted */
	if (in6p && (in6p->in6p_flags & IN6P_CONTROLOPTS)) {
		if (in6p->in6p_options) {
			m_freem(in6p->in6p_options);
			in6p->in6p_options = 0;
		}
		ip6_savecontrol(in6p, &in6p->in6p_options, ip6, m);
	}
#endif

	if (so->so_options & (SO_DEBUG|SO_ACCEPTCONN)) {
		union syn_cache_sa src;
		union syn_cache_sa dst;

		bzero(&src, sizeof(src));
		bzero(&dst, sizeof(dst));
		switch (af) {
#ifdef INET
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
#endif
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
#endif /* INET6 */
		default:
			goto badsyn;	/*sanity*/
		}

		if (so->so_options & SO_DEBUG) {
			ostate = tp->t_state;

			tcp_saveti = NULL;
			if (iphlen + sizeof(struct tcphdr) > MHLEN)
				goto nosave;

			if (m->m_len > iphlen && (m->m_flags & M_EXT) == 0) {
				tcp_saveti = m_copym(m, 0, iphlen, M_DONTWAIT);
				if (!tcp_saveti)
					goto nosave;
			} else {
				MGETHDR(tcp_saveti, M_DONTWAIT, MT_HEADER);
				if (!tcp_saveti)
					goto nosave;
				tcp_saveti->m_len = iphlen;
				m_copydata(m, 0, iphlen,
				    mtod(tcp_saveti, caddr_t));
			}

			if (M_TRAILINGSPACE(tcp_saveti) < sizeof(struct tcphdr)) {
				m_freem(tcp_saveti);
				tcp_saveti = NULL;
			} else {
				tcp_saveti->m_len += sizeof(struct tcphdr);
				bcopy(th, mtod(tcp_saveti, caddr_t) + iphlen,
				    sizeof(struct tcphdr));
			}
			if (tcp_saveti) {
				/*
				 * need to recover version # field, which was
				 * overwritten on ip_cksum computation.
				 */
				struct ip *sip;
				sip = mtod(tcp_saveti, struct ip *);
				switch (af) {
#ifdef INET
				case AF_INET:
					sip->ip_v = 4;
					break;
#endif
#ifdef INET6
				case AF_INET6:
					sip->ip_v = 6;
					break;
#endif
				}
			}
	nosave:;
		}
		if (so->so_options & SO_ACCEPTCONN) {
  			if ((tiflags & (TH_RST|TH_ACK|TH_SYN)) != TH_SYN) {
				if (tiflags & TH_RST) {
					syn_cache_reset(&src.sa, &dst.sa, th);
				} else if ((tiflags & (TH_ACK|TH_SYN)) ==
				    (TH_ACK|TH_SYN)) {
					/*
					 * Received a SYN,ACK.  This should
					 * never happen while we are in
					 * LISTEN.  Send an RST.
					 */
					goto badsyn;
				} else if (tiflags & TH_ACK) {
					so = syn_cache_get(&src.sa, &dst.sa,
						th, toff, tlen, so, m);
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
						tp = NULL;
						inp = NULL;
#ifdef INET6
						in6p = NULL;
#endif
						switch (so->so_proto->pr_domain->dom_family) {
#ifdef INET
						case AF_INET:
							inp = sotoinpcb(so);
							tp = intotcpcb(inp);
							break;
#endif
#ifdef INET6
						case AF_INET6:
							in6p = sotoin6pcb(so);
							tp = in6totcpcb(in6p);
							break;
#endif
						}
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

				/*
				 * LISTEN socket received a SYN
				 * from itself?  This can't possibly
				 * be valid; drop the packet.
				 */
				if (th->th_sport == th->th_dport) {
					int i;

					switch (af) {
#ifdef INET
					case AF_INET:
						i = in_hosteq(ip->ip_src, ip->ip_dst);
						break;
#endif
#ifdef INET6
					case AF_INET6:
						i = IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &ip6->ip6_dst);
						break;
#endif
					default:
						i = 1;
					}
					if (i) {
						tcpstat.tcps_badsyn++;
						goto drop;
					}
				}

				/*
				 * SYN looks ok; create compressed TCP
				 * state for it.
				 */
				if (so->so_qlen <= so->so_qlimit &&
				    syn_cache_add(&src.sa, &dst.sa, th, tlen,
						so, m, optp, optlen, &opti))
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
	tp->t_rcvtime = tcp_now;
	if (TCPS_HAVEESTABLISHED(tp->t_state))
		TCP_TIMER_ARM(tp, TCPT_KEEP, tcp_keepidle);

	/*
	 * Process options.
	 */
	if (optp)
		tcp_dooptions(tp, optp, optlen, th, &opti);

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
	    th->th_seq == tp->rcv_nxt &&
	    tiwin && tiwin == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {

		/* 
		 * If last ACK falls within this segment's sequence numbers,
		 *  record the timestamp.
		 */
		if (opti.ts_present &&
		    SEQ_LEQ(th->th_seq, tp->last_ack_sent) &&
		    SEQ_LT(tp->last_ack_sent, th->th_seq + tlen)) {
			tp->ts_recent_age = TCP_TIMESTAMP(tp);
			tp->ts_recent = opti.ts_val;
		}

		if (tlen == 0) {
			if (SEQ_GT(th->th_ack, tp->snd_una) &&
			    SEQ_LEQ(th->th_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd &&
			    tp->t_dupacks < tcprexmtthresh) {
				/*
				 * this is a pure ack for outstanding data.
				 */
				++tcpstat.tcps_predack;
				if (opti.ts_present && opti.ts_ecr)
					tcp_xmit_timer(tp,
					  TCP_TIMESTAMP(tp) - opti.ts_ecr + 1);
				else if (tp->t_rtttime &&
				    SEQ_GT(th->th_ack, tp->t_rtseq))
					tcp_xmit_timer(tp,
					tcp_now - tp->t_rtttime);
				acked = th->th_ack - tp->snd_una;
				tcpstat.tcps_rcvackpack++;
				tcpstat.tcps_rcvackbyte += acked;
				ND6_HINT(tp);
				sbdrop(&so->so_snd, acked);
				/*
				 * We want snd_recover to track snd_una to
				 * avoid sequence wraparound problems for
				 * very large transfers.
				 */
				tp->snd_una = tp->snd_recover = th->th_ack;
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
				if (tcp_saveti)
					m_freem(tcp_saveti);
				return;
			}
		} else if (th->th_ack == tp->snd_una &&
		    LIST_FIRST(&tp->segq) == NULL &&
		    tlen <= sbspace(&so->so_rcv)) {
			/*
			 * this is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			++tcpstat.tcps_preddat;
			tp->rcv_nxt += tlen;
			tcpstat.tcps_rcvpack++;
			tcpstat.tcps_rcvbyte += tlen;
			ND6_HINT(tp);
			/*
			 * Drop TCP, IP headers and TCP options then add data
			 * to socket buffer.
			 */
			m_adj(m, toff + off);
			sbappend(&so->so_rcv, m);
			sorwakeup(so);
			TCP_SETUP_ACK(tp, th);
			if (tp->t_flags & TF_ACKNOW)
				(void) tcp_output(tp);
			if (tcp_saveti)
				m_freem(tcp_saveti);
			return;
		}
	}

	/*
	 * Compute mbuf offset to TCP data segment.
	 */
	hdroptlen = toff + off;

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
			tp->snd_una = tp->snd_recover = th->th_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
			TCP_TIMER_DISARM(tp, TCPT_REXMT);
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
		else
			tp->snd_cwnd = TCP_INITIAL_WINDOW(tcp_init_win,
			    tp->t_peermss);

		tcp_rmx_rtt(tp);
		if (tiflags & TH_ACK) {
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
			(void) tcp_reass(tp, NULL, (struct mbuf *)0, &tlen);
			TCP_REASS_UNLOCK(tp);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtttime)
				tcp_xmit_timer(tp, tcp_now - tp->t_rtttime);
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

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
			tcpstat.tcps_rcvpackafterwin++;
			tcpstat.tcps_rcvbyteafterwin += todrop;
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
		if ((int)(TCP_TIMESTAMP(tp) - tp->ts_recent_age) >
		    TCP_PAWS_IDLE) {
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
			tcpstat.tcps_rcvdupbyte += tlen;
			tcpstat.tcps_pawsdrop++;
			goto dropafterack;
		}
	}

	todrop = tp->rcv_nxt - th->th_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			th->th_seq++;
			if (th->th_urp > 1) 
				th->th_urp--;
			else {
				tiflags &= ~TH_URG;
				th->th_urp = 0;
			}
			todrop--;
		}
		if (todrop > tlen ||
		    (todrop == tlen && (tiflags & TH_FIN) == 0)) {
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
			todrop = tlen;
			tcpstat.tcps_rcvdupbyte += todrop;
			tcpstat.tcps_rcvduppack++;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}
		hdroptlen += todrop;	/*drop from head afterwards*/
		th->th_seq += todrop;
		tlen -= todrop;
		if (th->th_urp > todrop)
			th->th_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			th->th_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && tlen) {
		tp = tcp_close(tp);
		tcpstat.tcps_rcvafterclose++;
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (th->th_seq + tlen) - (tp->rcv_nxt+tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= tlen) {
			tcpstat.tcps_rcvbyteafterwin += tlen;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(th->th_seq, tp->rcv_nxt)) {
				iss = tcp_new_iss(tp, tp->snd_nxt);
				tp = tcp_close(tp);
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && th->th_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat.tcps_rcvbyteafterwin += todrop;
		m_adj(m, -todrop);
		tlen -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If last ACK falls within this segment's sequence numbers,
	 * and the timestamp is newer, record it.
	 */
	if (opti.ts_present && TSTMP_GEQ(opti.ts_val, tp->ts_recent) &&
	    SEQ_LEQ(th->th_seq, tp->last_ack_sent) &&
	    SEQ_LT(tp->last_ack_sent, th->th_seq + tlen +
		   ((tiflags & (TH_SYN|TH_FIN)) != 0))) {
		tp->ts_recent_age = TCP_TIMESTAMP(tp);
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
	if ((tiflags & TH_ACK) == 0) {
		if (tp->t_flags & TF_ACKNOW)
			goto dropafterack;
		else
			goto drop;
	}
	
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
		if (SEQ_GT(tp->snd_una, th->th_ack) ||
		    SEQ_GT(th->th_ack, tp->snd_max))
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
		(void) tcp_reass(tp, NULL, (struct mbuf *)0, &tlen);
		TCP_REASS_UNLOCK(tp);
		tp->snd_wl1 = th->th_seq - 1;
		/* fall into ... */

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
			if (tlen == 0 && tiwin == tp->snd_wnd) {
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
				    th->th_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					u_int win =
					    min(tp->snd_wnd, tp->snd_cwnd) / 
					    2 /	tp->t_segsz;
					if (tcp_do_newreno && SEQ_LT(th->th_ack,
					    tp->snd_recover)) {
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
					tp->t_rtttime = 0;
					tp->snd_nxt = th->th_ack;
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
		if (tcp_do_newreno == 0) {
			if (tp->t_dupacks >= tcprexmtthresh &&
			    tp->snd_cwnd > tp->snd_ssthresh)
				tp->snd_cwnd = tp->snd_ssthresh;
			tp->t_dupacks = 0;
		} else if (tp->t_dupacks >= tcprexmtthresh &&
			   tcp_newreno(tp, th) == 0) {
			tp->snd_cwnd = tp->snd_ssthresh;
			/*
			 * Window inflation should have left us with approx.
			 * snd_ssthresh outstanding data.  But in case we 
			 * would be inclined to send a burst, better to do
			 * it via the slow start mechanism.
			 */
			if (SEQ_SUB(tp->snd_max, th->th_ack) < tp->snd_ssthresh)
				tp->snd_cwnd = SEQ_SUB(tp->snd_max, th->th_ack)
				    + tp->t_segsz;
			tp->t_dupacks = 0;
		}
		if (SEQ_GT(th->th_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = th->th_ack - tp->snd_una;
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
		if (opti.ts_present && opti.ts_ecr)
			tcp_xmit_timer(tp, TCP_TIMESTAMP(tp) - opti.ts_ecr + 1);
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
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (segsz per packet).
		 * Otherwise open linearly: segsz per window
		 * (segsz^2 / cwnd per packet), plus a constant
		 * fraction of a packet (segsz/8) to help larger windows
		 * open quickly enough.
		 */
		{
		u_int cw = tp->snd_cwnd;
		u_int incr = tp->t_segsz;

		if (cw > tp->snd_ssthresh)
			incr = incr * incr / cw;
		if (tcp_do_newreno == 0 || SEQ_GEQ(th->th_ack, tp->snd_recover))
			tp->snd_cwnd = min(cw + incr,
			    TCP_MAXWIN << tp->snd_scale);
		}
		ND6_HINT(tp);
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
		/*
		 * We want snd_recover to track snd_una to
		 * avoid sequence wraparound problems for
		 * very large transfers.
		 */
		tp->snd_una = tp->snd_recover = th->th_ack;
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
	if ((tiflags & TH_ACK) && (SEQ_LT(tp->snd_wl1, th->th_seq) ||
	    (tp->snd_wl1 == th->th_seq && SEQ_LT(tp->snd_wl2, th->th_ack)) ||
	    (tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (tlen == 0 &&
		    tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd)
			tcpstat.tcps_rcvwinupd++;
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
		if (th->th_urp <= (u_int16_t) tlen
#ifdef SO_OOBINLINE
		     && (so->so_options & SO_OOBINLINE) == 0
#endif
		     )
			tcp_pulloutofband(so, th, m, hdroptlen);
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
	 * and arranging for acknowledgement of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((tlen || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * Insert segment ti into reassembly queue of tcp with
		 * control block tp.  Return TH_FIN if reassembly now includes
		 * a segment with FIN.  The macro form does the common case
		 * inline (segment is the next to be received on an
		 * established connection, and the queue is empty),
		 * avoiding linkage into and removal from the queue and
		 * repetition of various conversions.
		 * Set DELACK for segments received in order, but ack
		 * immediately when segments are out of order
		 * (so fast retransmit can work).
		 */
		/* NOTE: this was TCP_REASS() macro, but used only once */
		TCP_REASS_LOCK(tp);
		if (th->th_seq == tp->rcv_nxt &&
		    LIST_FIRST(&tp->segq) == NULL &&
		    tp->t_state == TCPS_ESTABLISHED) {
			TCP_SETUP_ACK(tp, th);
			tp->rcv_nxt += tlen;
			tiflags = th->th_flags & TH_FIN;
			tcpstat.tcps_rcvpack++;
			tcpstat.tcps_rcvbyte += tlen;
			ND6_HINT(tp);
			m_adj(m, hdroptlen);
			sbappend(&(so)->so_rcv, m);
			sorwakeup(so);
		} else {
			m_adj(m, hdroptlen);
			tiflags = tcp_reass(tp, th, m, &tlen);
			tp->t_flags |= TF_ACKNOW;
		}
		TCP_REASS_UNLOCK(tp);

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
#ifdef TCP_DEBUG
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_INPUT, ostate, tp, tcp_saveti, 0);
#endif

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) tcp_output(tp);
	if (tcp_saveti)
		m_freem(tcp_saveti);
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
	/* ...fall into dropwithreset... */

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 */
	if (tiflags & TH_RST)
		goto drop;
    {
	/*
	 * need to recover version # field, which was overwritten on
	 * ip_cksum computation.
	 */
	struct ip *sip;
	sip = mtod(m, struct ip *);
	switch (af) {
#ifdef INET
	case AF_INET:
		sip->ip_v = 4;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		sip->ip_v = 6;
		break;
#endif
	}
    }
	if (tiflags & TH_ACK)
		(void)tcp_respond(tp, m, m, th, (tcp_seq)0, th->th_ack, TH_RST);
	else {
		if (tiflags & TH_SYN)
			tlen++;
		(void)tcp_respond(tp, m, m, th, th->th_seq + tlen, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	if (tcp_saveti)
		m_freem(tcp_saveti);
	return;

badcsum:
	tcpstat.tcps_rcvbadsum++;
drop:
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp) {
		if (tp->t_inpcb)
			so = tp->t_inpcb->inp_socket;
#ifdef INET6
		else if (tp->t_in6pcb)
			so = tp->t_in6pcb->in6p_socket;
#endif
		else
			so = NULL;
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

void
tcp_dooptions(tp, cp, cnt, th, oi)
	struct tcpcb *tp;
	u_char *cp;
	int cnt;
	struct tcphdr *th;
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
			bcopy(cp + 2, &mss, sizeof(mss));
			oi->maxseg = ntohs(mss);
			break;

		case TCPOPT_WINDOW:
			if (optlen != TCPOLEN_WINDOW)
				continue;
			if (!(th->th_flags & TH_SYN))
				continue;
			tp->t_flags |= TF_RCVD_SCALE;
			tp->requested_s_scale = cp[2];
			if (tp->requested_s_scale > TCP_MAX_WINSHIFT) {
#if 0	/*XXX*/
				char *p;

				if (ip)
					p = ntohl(ip->ip_src);
#ifdef INET6
				else if (ip6)
					p = ip6_sprintf(&ip6->ip6_src);
#endif
				else
					p = "(unknown)";
				log(LOG_ERR, "TCP: invalid wscale %d from %s, "
				    "assuming %d\n",
				    tp->requested_s_scale, p,
				    TCP_MAX_WINSHIFT);
#else
				log(LOG_ERR, "TCP: invalid wscale %d, "
				    "assuming %d\n",
				    tp->requested_s_scale,
				    TCP_MAX_WINSHIFT);
#endif
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
			if (th->th_flags & TH_SYN) {
				tp->t_flags |= TF_RCVD_TSTMP;
				tp->ts_recent = oi->ts_val;
				tp->ts_recent_age = TCP_TIMESTAMP(tp);
			}
			break;
		case TCPOPT_SACK_PERMITTED:
			if (optlen != TCPOLEN_SACK_PERMITTED)
				continue;
			if (!(th->th_flags & TH_SYN))
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
tcp_pulloutofband(so, th, m, off)
	struct socket *so;
	struct tcphdr *th;
	struct mbuf *m;
	int off;
{
	int cnt = off + th->th_urp - 1;
	
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
	struct tcpcb *tp;
	uint32_t rtt;
{
	int32_t delta;

	tcpstat.tcps_rttupdated++;
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
	    max(tp->t_rttmin, rtt + 2), TCPTV_REXMTMAX);
	
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
 * 1.  By setting snd_nxt to th_ack, this forces retransmission timer to
 * be started again.  If the ack advances at least to tp->snd_recover, return 0.
 */
int
tcp_newreno(tp, th)
	struct tcpcb *tp;
	struct tcphdr *th;
{
	tcp_seq onxt = tp->snd_nxt;
	u_long ocwnd = tp->snd_cwnd;

	if (SEQ_LT(th->th_ack, tp->snd_recover)) {
		/*
		 * snd_una has not yet been updated and the socket's send
		 * buffer has not yet drained off the ACK'd data, so we
		 * have to leave snd_una as it was to get the correct data
		 * offset in tcp_output().
		 */
		TCP_TIMER_DISARM(tp, TCPT_REXMT);
	        tp->t_rtttime = 0;
	        tp->snd_nxt = th->th_ack;
		/*
		 * Set snd_cwnd to one segment beyond ACK'd offset.  snd_una
		 * is not yet updated when we're called.
		 */
		tp->snd_cwnd = tp->t_segsz + (th->th_ack - tp->snd_una);
	        (void) tcp_output(tp);
	        tp->snd_cwnd = ocwnd;
	        if (SEQ_GT(onxt, tp->snd_nxt))
	                tp->snd_nxt = onxt;
	        /*
	         * Partial window deflation.  Relies on fact that tp->snd_una
	         * not updated yet.
	         */
	        tp->snd_cwnd -= (th->th_ack - tp->snd_una - tp->t_segsz);
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
#ifndef INET6
#define	SYN_HASHALL(hash, src, dst) \
do {									\
	hash = SYN_HASH(&((struct sockaddr_in *)(src))->sin_addr,	\
		((struct sockaddr_in *)(src))->sin_port,		\
		((struct sockaddr_in *)(dst))->sin_port);		\
} while (0)
#else
#define SYN_HASH6(sa, sp, dp) \
	((((sa)->s6_addr32[0] ^ (sa)->s6_addr32[3] ^ syn_hash1) * \
	  (((((u_int32_t)(dp))<<16) + ((u_int32_t)(sp)))^syn_hash2)) \
	 & 0x7fffffff)

#define SYN_HASHALL(hash, src, dst) \
do {									\
	switch ((src)->sa_family) {					\
	case AF_INET:							\
		hash = SYN_HASH(&((struct sockaddr_in *)(src))->sin_addr, \
			((struct sockaddr_in *)(src))->sin_port,	\
			((struct sockaddr_in *)(dst))->sin_port);	\
		break;							\
	case AF_INET6:							\
		hash = SYN_HASH6(&((struct sockaddr_in6 *)(src))->sin6_addr, \
			((struct sockaddr_in6 *)(src))->sin6_port,	\
			((struct sockaddr_in6 *)(dst))->sin6_port);	\
		break;							\
	default:							\
		hash = 0;						\
	}								\
} while (/*CONSTCOND*/0)
#endif /* INET6 */

#define	SYN_CACHE_RM(sc)						\
do {									\
	TAILQ_REMOVE(&tcp_syn_cache[(sc)->sc_bucketidx].sch_bucket,	\
	    (sc), sc_bucketq);						\
	(sc)->sc_tp = NULL;						\
	LIST_REMOVE((sc), sc_tpq);					\
	tcp_syn_cache[(sc)->sc_bucketidx].sch_length--;			\
	callout_stop(&(sc)->sc_timer);					\
	syn_cache_count--;						\
} while (/*CONSTCOND*/0)

#define	SYN_CACHE_PUT(sc)						\
do {									\
	if ((sc)->sc_ipopts)						\
		(void) m_free((sc)->sc_ipopts);				\
	if ((sc)->sc_route4.ro_rt != NULL)				\
		RTFREE((sc)->sc_route4.ro_rt);				\
	pool_put(&syn_cache_pool, (sc));				\
} while (/*CONSTCOND*/0)

struct pool syn_cache_pool;

/*
 * We don't estimate RTT with SYNs, so each packet starts with the default
 * RTT and each timer step has a fixed timeout value.
 */
#define	SYN_CACHE_TIMER_ARM(sc)						\
do {									\
	TCPT_RANGESET((sc)->sc_rxtcur,					\
	    TCPTV_SRTTDFLT * tcp_backoff[(sc)->sc_rxtshift], TCPTV_MIN,	\
	    TCPTV_REXMTMAX);						\
	callout_reset(&(sc)->sc_timer,					\
	    (sc)->sc_rxtcur * (hz / PR_SLOWHZ), syn_cache_timer, (sc));	\
} while (/*CONSTCOND*/0)

#define	SYN_CACHE_TIMESTAMP(sc)	(tcp_now - (sc)->sc_timebase)

void
syn_cache_init()
{
	int i;

	/* Initialize the hash buckets. */
	for (i = 0; i < tcp_syn_cache_size; i++)
		TAILQ_INIT(&tcp_syn_cache[i].sch_bucket);

	/* Initialize the syn cache pool. */
	pool_init(&syn_cache_pool, sizeof(struct syn_cache), 0, 0, 0,
	    "synpl", 0, NULL, NULL, M_PCB);
}

void
syn_cache_insert(sc, tp)
	struct syn_cache *sc;
	struct tcpcb *tp;
{
	struct syn_cache_head *scp;
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

	SYN_HASHALL(sc->sc_hash, &sc->sc_src.sa, &sc->sc_dst.sa);
	sc->sc_bucketidx = sc->sc_hash % tcp_syn_cache_size;
	scp = &tcp_syn_cache[sc->sc_bucketidx];

	/*
	 * Make sure that we don't overflow the per-bucket
	 * limit or the total cache size limit.
	 */
	s = splsoftnet();
	if (scp->sch_length >= tcp_syn_bucket_limit) {
		tcpstat.tcps_sc_bucketoverflow++;
		/*
		 * The bucket is full.  Toss the oldest element in the
		 * bucket.  This will be the first entry in the bucket.
		 */
		sc2 = TAILQ_FIRST(&scp->sch_bucket);
#ifdef DIAGNOSTIC
		/*
		 * This should never happen; we should always find an
		 * entry in our bucket.
		 */
		if (sc2 == NULL)
			panic("syn_cache_insert: bucketoverflow: impossible");
#endif
		SYN_CACHE_RM(sc2);
		SYN_CACHE_PUT(sc2);
	} else if (syn_cache_count >= tcp_syn_cache_limit) {
		struct syn_cache_head *scp2, *sce;

		tcpstat.tcps_sc_overflowed++;
		/*
		 * The cache is full.  Toss the oldest entry in the
		 * first non-empty bucket we can find.
		 *
		 * XXX We would really like to toss the oldest
		 * entry in the cache, but we hope that this
		 * condition doesn't happen very often.
		 */
		scp2 = scp;
		if (TAILQ_EMPTY(&scp2->sch_bucket)) {
			sce = &tcp_syn_cache[tcp_syn_cache_size];
			for (++scp2; scp2 != scp; scp2++) {
				if (scp2 >= sce)
					scp2 = &tcp_syn_cache[0];
				if (! TAILQ_EMPTY(&scp2->sch_bucket))
					break;
			}
#ifdef DIAGNOSTIC
			/*
			 * This should never happen; we should always find a
			 * non-empty bucket.
			 */
			if (scp2 == scp)
				panic("syn_cache_insert: cacheoverflow: "
				    "impossible");
#endif
		}
		sc2 = TAILQ_FIRST(&scp2->sch_bucket);
		SYN_CACHE_RM(sc2);
		SYN_CACHE_PUT(sc2);
	}

	/*
	 * Initialize the entry's timer.
	 */
	sc->sc_rxttot = 0;
	sc->sc_rxtshift = 0;
	SYN_CACHE_TIMER_ARM(sc);

	/* Link it from tcpcb entry */
	LIST_INSERT_HEAD(&tp->t_sc, sc, sc_tpq);

	/* Put it into the bucket. */
	TAILQ_INSERT_TAIL(&scp->sch_bucket, sc, sc_bucketq);
	scp->sch_length++;
	syn_cache_count++;

	tcpstat.tcps_sc_added++;
	splx(s);
}

/*
 * Walk the timer queues, looking for SYN,ACKs that need to be retransmitted.
 * If we have retransmitted an entry the maximum number of times, expire
 * that entry.
 */
void
syn_cache_timer(void *arg)
{
	struct syn_cache *sc = arg;
	int s;

	s = splsoftnet();

	if (__predict_false(sc->sc_rxtshift == TCP_MAXRXTSHIFT)) {
		/* Drop it -- too many retransmissions. */
		goto dropit;
	}

	/*
	 * Compute the total amount of time this entry has
	 * been on a queue.  If this entry has been on longer
	 * than the keep alive timer would allow, expire it.
	 */
	sc->sc_rxttot += sc->sc_rxtcur;
	if (sc->sc_rxttot >= TCPTV_KEEP_INIT)
		goto dropit;

	tcpstat.tcps_sc_retransmitted++;
	(void) syn_cache_respond(sc, NULL);

	/* Advance the timer back-off. */
	sc->sc_rxtshift++;
	SYN_CACHE_TIMER_ARM(sc);

	splx(s);
	return;

 dropit:
	tcpstat.tcps_sc_timed_out++;
	SYN_CACHE_RM(sc);
	SYN_CACHE_PUT(sc);
	splx(s);
}

/*
 * Remove syn cache created by the specified tcb entry,
 * because this does not make sense to keep them
 * (if there's no tcb entry, syn cache entry will never be used)
 */
void
syn_cache_cleanup(tp)
	struct tcpcb *tp;
{
	struct syn_cache *sc, *nsc;
	int s;

	s = splsoftnet();

	for (sc = LIST_FIRST(&tp->t_sc); sc != NULL; sc = nsc) {
		nsc = LIST_NEXT(sc, sc_tpq);

#ifdef DIAGNOSTIC
		if (sc->sc_tp != tp)
			panic("invalid sc_tp in syn_cache_cleanup");
#endif
		SYN_CACHE_RM(sc);
		SYN_CACHE_PUT(sc);
	}
	/* just for safety */
	LIST_INIT(&tp->t_sc);

	splx(s);
}

/*
 * Find an entry in the syn cache.
 */
struct syn_cache *
syn_cache_lookup(src, dst, headp)
	struct sockaddr *src;
	struct sockaddr *dst;
	struct syn_cache_head **headp;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	u_int32_t hash;
	int s;

	SYN_HASHALL(hash, src, dst);

	scp = &tcp_syn_cache[hash % tcp_syn_cache_size];
	*headp = scp;
	s = splsoftnet();
	for (sc = TAILQ_FIRST(&scp->sch_bucket); sc != NULL;
	     sc = TAILQ_NEXT(sc, sc_bucketq)) {
		if (sc->sc_hash != hash)
			continue;
		if (!bcmp(&sc->sc_src, src, src->sa_len) &&
		    !bcmp(&sc->sc_dst, dst, dst->sa_len)) {
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
syn_cache_get(src, dst, th, hlen, tlen, so, m)
	struct sockaddr *src;
	struct sockaddr *dst;
	struct tcphdr *th;
	unsigned int hlen, tlen;
	struct socket *so;
	struct mbuf *m;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct inpcb *inp = NULL;
#ifdef INET6
	struct in6pcb *in6p = NULL;
#endif
	struct tcpcb *tp = 0;
	struct mbuf *am;
	int s;
	struct socket *oso;

	s = splsoftnet();
	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
		splx(s);
		return (NULL);
	}

	/*
	 * Verify the sequence and ack numbers.  Try getting the correct
	 * response again.
	 */
	if ((th->th_ack != sc->sc_iss + 1) ||
	    SEQ_LEQ(th->th_seq, sc->sc_irs) ||
	    SEQ_GT(th->th_seq, sc->sc_irs + 1 + sc->sc_win)) {
		(void) syn_cache_respond(sc, m);
		splx(s);
		return ((struct socket *)(-1));
	}

	/* Remove this cache entry */
	SYN_CACHE_RM(sc);
	splx(s);

	/*
	 * Ok, create the full blown connection, and set things up
	 * as they would have been set up if we had created the
	 * connection when the SYN arrived.  If we can't create
	 * the connection, abort it.
	 */
	/*
	 * inp still has the OLD in_pcb stuff, set the
	 * v6-related flags on the new guy, too.   This is
	 * done particularly for the case where an AF_INET6
	 * socket is bound only to a port, and a v4 connection
	 * comes in on that port.
	 * we also copy the flowinfo from the original pcb 
	 * to the new one.
	 */
    {
	struct inpcb *parentinpcb;

	parentinpcb = (struct inpcb *)so->so_pcb;

	oso = so;
	so = sonewconn(so, SS_ISCONNECTED);
	if (so == NULL)
		goto resetandabort;

	switch (so->so_proto->pr_domain->dom_family) {
#ifdef INET
	case AF_INET:
		inp = sotoinpcb(so);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		in6p = sotoin6pcb(so);
		break;
#endif
	}
    }
	switch (src->sa_family) {
#ifdef INET
	case AF_INET:
		if (inp) {
			inp->inp_laddr = ((struct sockaddr_in *)dst)->sin_addr;
			inp->inp_lport = ((struct sockaddr_in *)dst)->sin_port;
			inp->inp_options = ip_srcroute();
			in_pcbstate(inp, INP_BOUND);
			if (inp->inp_options == NULL) {
				inp->inp_options = sc->sc_ipopts;
				sc->sc_ipopts = NULL;
			}
		}
#ifdef INET6
		else if (in6p) {
			/* IPv4 packet to AF_INET6 socket */
			bzero(&in6p->in6p_laddr, sizeof(in6p->in6p_laddr));
			in6p->in6p_laddr.s6_addr16[5] = htons(0xffff);
			bcopy(&((struct sockaddr_in *)dst)->sin_addr,
				&in6p->in6p_laddr.s6_addr32[3],
				sizeof(((struct sockaddr_in *)dst)->sin_addr));
			in6p->in6p_lport = ((struct sockaddr_in *)dst)->sin_port;
			in6totcpcb(in6p)->t_family = AF_INET;
		}
#endif
		break;
#endif
#ifdef INET6
	case AF_INET6:
		if (in6p) {
			in6p->in6p_laddr = ((struct sockaddr_in6 *)dst)->sin6_addr;
			in6p->in6p_lport = ((struct sockaddr_in6 *)dst)->sin6_port;
#if 0
			in6p->in6p_flowinfo = ip6->ip6_flow & IPV6_FLOWINFO_MASK;
			/*inp->inp_options = ip6_srcroute();*/ /* soon. */
#endif
		}
		break;
#endif
	}
#ifdef INET6
	if (in6p && in6totcpcb(in6p)->t_family == AF_INET6 && sotoinpcb(oso)) {
		struct in6pcb *oin6p = sotoin6pcb(oso);
		/* inherit socket options from the listening socket */
		in6p->in6p_flags |= (oin6p->in6p_flags & IN6P_CONTROLOPTS);
		if (in6p->in6p_flags & IN6P_CONTROLOPTS) {
			m_freem(in6p->in6p_options);
			in6p->in6p_options = 0;
		}
		ip6_savecontrol(in6p, &in6p->in6p_options,
			mtod(m, struct ip6_hdr *), m);
	}
#endif

#ifdef IPSEC
	/*
	 * we make a copy of policy, instead of sharing the policy,
	 * for better behavior in terms of SA lookup and dead SA removal.
	 */
	if (inp) {
		/* copy old policy into new socket's */
		if (ipsec_copy_policy(sotoinpcb(oso)->inp_sp, inp->inp_sp))
			printf("tcp_input: could not copy policy\n");
	}
#ifdef INET6
	else if (in6p) {
		/* copy old policy into new socket's */
		if (ipsec_copy_policy(sotoin6pcb(oso)->in6p_sp, in6p->in6p_sp))
			printf("tcp_input: could not copy policy\n");
	}
#endif
#endif

	/*
	 * Give the new socket our cached route reference.
	 */
	if (inp)
		inp->inp_route = sc->sc_route4;		/* struct assignment */
#ifdef INET6
	else
		in6p->in6p_route = sc->sc_route6;
#endif
	sc->sc_route4.ro_rt = NULL;

	am = m_get(M_DONTWAIT, MT_SONAME);	/* XXX */
	if (am == NULL)
		goto resetandabort;
	am->m_len = src->sa_len;
	bcopy(src, mtod(am, caddr_t), src->sa_len);
	if (inp) {
		if (in_pcbconnect(inp, am)) {
			(void) m_free(am);
			goto resetandabort;
		}
	}
#ifdef INET6
	else if (in6p) {
		if (src->sa_family == AF_INET) {
			/* IPv4 packet to AF_INET6 socket */
			struct sockaddr_in6 *sin6;
			sin6 = mtod(am, struct sockaddr_in6 *);
			am->m_len = sizeof(*sin6);
			bzero(sin6, sizeof(*sin6));
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(*sin6);
			sin6->sin6_port = ((struct sockaddr_in *)src)->sin_port;
			sin6->sin6_addr.s6_addr16[5] = htons(0xffff);
			bcopy(&((struct sockaddr_in *)src)->sin_addr,
				&sin6->sin6_addr.s6_addr32[3],
				sizeof(sin6->sin6_addr.s6_addr32[3]));
		}
		if (in6_pcbconnect(in6p, am)) {
			(void) m_free(am);
			goto resetandabort;
		}
	}
#endif
	else {
		(void) m_free(am);
		goto resetandabort;
	}
	(void) m_free(am);

	if (inp)
		tp = intotcpcb(inp);
#ifdef INET6
	else if (in6p)
		tp = in6totcpcb(in6p);
#endif
	else
		tp = NULL;
	if (sc->sc_request_r_scale != 15) {
		tp->requested_s_scale = sc->sc_requested_s_scale;
		tp->request_r_scale = sc->sc_request_r_scale;
		tp->snd_scale = sc->sc_requested_s_scale;
		tp->rcv_scale = sc->sc_request_r_scale;
		tp->t_flags |= TF_RCVD_SCALE;
	}
	if (sc->sc_flags & SCF_TIMESTAMP)
		tp->t_flags |= TF_RCVD_TSTMP;
	tp->ts_timebase = sc->sc_timebase;

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
	if (sc->sc_rxtshift)
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
	if (sc->sc_win > 0 && SEQ_GT(tp->rcv_nxt + sc->sc_win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + sc->sc_win;
	tp->last_ack_sent = tp->rcv_nxt;

	tcpstat.tcps_sc_completed++;
	SYN_CACHE_PUT(sc);
	return (so);

resetandabort:
	(void) tcp_respond(NULL, m, m, th,
			   th->th_seq + tlen, (tcp_seq)0, TH_RST|TH_ACK);
abort:
	if (so != NULL)
		(void) soabort(so);
	SYN_CACHE_PUT(sc);
	tcpstat.tcps_sc_aborted++;
	return ((struct socket *)(-1));
}

/*
 * This function is called when we get a RST for a
 * non-existent connection, so that we can see if the
 * connection is in the syn cache.  If it is, zap it.
 */

void
syn_cache_reset(src, dst, th)
	struct sockaddr *src;
	struct sockaddr *dst;
	struct tcphdr *th;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	int s = splsoftnet();

	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
		splx(s);
		return;
	}
	if (SEQ_LT(th->th_seq, sc->sc_irs) ||
	    SEQ_GT(th->th_seq, sc->sc_irs+1)) {
		splx(s);
		return;
	}
	SYN_CACHE_RM(sc);
	splx(s);
	tcpstat.tcps_sc_reset++;
	SYN_CACHE_PUT(sc);
}

void
syn_cache_unreach(src, dst, th)
	struct sockaddr *src;
	struct sockaddr *dst;
	struct tcphdr *th;
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	int s;

	s = splsoftnet();
	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
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
	if ((sc->sc_flags & SCF_UNREACH) == 0 || sc->sc_rxtshift < 3) {
		sc->sc_flags |= SCF_UNREACH;
		splx(s);
		return;
	}

	SYN_CACHE_RM(sc);
	splx(s);
	tcpstat.tcps_sc_unreach++;
	SYN_CACHE_PUT(sc);
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
syn_cache_add(src, dst, th, hlen, so, m, optp, optlen, oi)
	struct sockaddr *src;
	struct sockaddr *dst;
	struct tcphdr *th;
	unsigned int hlen;
	struct socket *so;
	struct mbuf *m;
	u_char *optp;
	int optlen;
	struct tcp_opt_info *oi;
{
	struct tcpcb tb, *tp;
	long win;
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct mbuf *ipopts;

	tp = sototcpcb(so);

	/*
	 * RFC1122 4.2.3.10, p. 104: discard bcast/mcast SYN
	 *
	 * Note this check is performed in tcp_input() very early on.
	 */

	/*
	 * Initialize some local state.
	 */
	win = sbspace(&so->so_rcv);
	if (win > TCP_MAXWIN)
		win = TCP_MAXWIN;

	switch (src->sa_family) {
#ifdef INET
	case AF_INET:
		/*
		 * Remember the IP options, if any.
		 */
		ipopts = ip_srcroute();
		break;
#endif
	default:
		ipopts = NULL;
	}

	if (optp) {
		tb.t_flags = tcp_do_rfc1323 ? (TF_REQ_SCALE|TF_REQ_TSTMP) : 0;
		tcp_dooptions(&tb, optp, optlen, th, oi);
	} else
		tb.t_flags = 0;

	/*
	 * See if we already have an entry for this connection.
	 * If we do, resend the SYN,ACK.  We do not count this
	 * as a retransmission (XXX though maybe we should).
	 */
	if ((sc = syn_cache_lookup(src, dst, &scp)) != NULL) {
		tcpstat.tcps_sc_dupesyn++;
		if (ipopts) {
			/*
			 * If we were remembering a previous source route,
			 * forget it and use the new one we've been given.
			 */
			if (sc->sc_ipopts)
				(void) m_free(sc->sc_ipopts);
			sc->sc_ipopts = ipopts;
		}
		sc->sc_timestamp = tb.ts_recent;
		if (syn_cache_respond(sc, m) == 0) {
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
	callout_init(&sc->sc_timer);
	bzero(sc, sizeof(struct syn_cache));
	bcopy(src, &sc->sc_src, src->sa_len);
	bcopy(dst, &sc->sc_dst, dst->sa_len);
	sc->sc_flags = 0;
	sc->sc_ipopts = ipopts;
	sc->sc_irs = th->th_seq;
	switch (src->sa_family) {
#ifdef INET
	case AF_INET:
	    {
		struct sockaddr_in *srcin = (void *) src;
		struct sockaddr_in *dstin = (void *) dst;

		sc->sc_iss = tcp_new_iss1(&dstin->sin_addr,
		    &srcin->sin_addr, dstin->sin_port,
		    srcin->sin_port, sizeof(dstin->sin_addr), 0);
		break;
	    }
#endif /* INET */
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 *srcin6 = (void *) src;
		struct sockaddr_in6 *dstin6 = (void *) dst;

		sc->sc_iss = tcp_new_iss1(&dstin6->sin6_addr,
		    &srcin6->sin6_addr, dstin6->sin6_port,
		    srcin6->sin6_port, sizeof(dstin6->sin6_addr), 0);
		break;
	    }
#endif /* INET6 */
	}
	sc->sc_peermaxseg = oi->maxseg;
	sc->sc_ourmaxseg = tcp_mss_to_advertise(m->m_flags & M_PKTHDR ?
						m->m_pkthdr.rcvif : NULL,
						sc->sc_src.sa.sa_family);
	sc->sc_win = win;
	sc->sc_timebase = tcp_now;	/* see tcp_newtcpcb() */
	sc->sc_timestamp = tb.ts_recent;
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
	sc->sc_tp = tp;
	if (syn_cache_respond(sc, m) == 0) {
		syn_cache_insert(sc, tp);
		tcpstat.tcps_sndacks++;
		tcpstat.tcps_sndtotal++;
	} else {
		SYN_CACHE_PUT(sc);
		tcpstat.tcps_sc_dropped++;
	}
	return (1);
}

int
syn_cache_respond(sc, m)
	struct syn_cache *sc;
	struct mbuf *m;
{
	struct route *ro;
	u_int8_t *optp;
	int optlen, error;
	u_int16_t tlen;
	struct ip *ip = NULL;
#ifdef INET6
	struct ip6_hdr *ip6 = NULL;
#endif
	struct tcphdr *th;
	u_int hlen;

	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		hlen = sizeof(struct ip);
		ro = &sc->sc_route4;
		break;
#ifdef INET6
	case AF_INET6:
		hlen = sizeof(struct ip6_hdr);
		ro = (struct route *)&sc->sc_route6;
		break;
#endif
	default:
		if (m)
			m_freem(m);
		return EAFNOSUPPORT;
	}

	/* Compute the size of the TCP options. */
	optlen = 4 + (sc->sc_request_r_scale != 15 ? 4 : 0) +
	    ((sc->sc_flags & SCF_TIMESTAMP) ? TCPOLEN_TSTAMP_APPA : 0);

	tlen = hlen + sizeof(struct tcphdr) + optlen;

	/*
	 * Create the IP+TCP header from scratch.
	 */
	if (m)
		m_freem(m);
#ifdef DIAGNOSTIC
	if (max_linkhdr + tlen > MCLBYTES)
		return (ENOBUFS);
#endif
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && tlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		}
	}
	if (m == NULL)
		return (ENOBUFS);

	/* Fixup the mbuf. */
	m->m_data += max_linkhdr;
	m->m_len = m->m_pkthdr.len = tlen;
#ifdef IPSEC
	if (sc->sc_tp) {
		struct tcpcb *tp;
		struct socket *so;

		tp = sc->sc_tp;
		if (tp->t_inpcb)
			so = tp->t_inpcb->inp_socket;
#ifdef INET6
		else if (tp->t_in6pcb)
			so = tp->t_in6pcb->in6p_socket;
#endif
		else
			so = NULL;
		/* use IPsec policy on listening socket, on SYN ACK */
		if (ipsec_setsocket(m, so) != 0) {
			m_freem(m);
			return ENOBUFS;
		}
	}
#endif
	m->m_pkthdr.rcvif = NULL;
	memset(mtod(m, u_char *), 0, tlen);

	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		ip = mtod(m, struct ip *);
		ip->ip_dst = sc->sc_src.sin.sin_addr;
		ip->ip_src = sc->sc_dst.sin.sin_addr;
		ip->ip_p = IPPROTO_TCP;
		th = (struct tcphdr *)(ip + 1);
		th->th_dport = sc->sc_src.sin.sin_port;
		th->th_sport = sc->sc_dst.sin.sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_dst = sc->sc_src.sin6.sin6_addr;
		ip6->ip6_src = sc->sc_dst.sin6.sin6_addr;
		ip6->ip6_nxt = IPPROTO_TCP;
		/* ip6_plen will be updated in ip6_output() */
		th = (struct tcphdr *)(ip6 + 1);
		th->th_dport = sc->sc_src.sin6.sin6_port;
		th->th_sport = sc->sc_dst.sin6.sin6_port;
		break;
#endif
	default:
		th = NULL;
	}

	th->th_seq = htonl(sc->sc_iss);
	th->th_ack = htonl(sc->sc_irs + 1);
	th->th_off = (sizeof(struct tcphdr) + optlen) >> 2;
	th->th_flags = TH_SYN|TH_ACK;
	th->th_win = htons(sc->sc_win);
	/* th_sum already 0 */
	/* th_urp already 0 */

	/* Tack on the TCP options. */
	optp = (u_int8_t *)(th + 1);
	*optp++ = TCPOPT_MAXSEG;
	*optp++ = 4;
	*optp++ = (sc->sc_ourmaxseg >> 8) & 0xff;
	*optp++ = sc->sc_ourmaxseg & 0xff;

	if (sc->sc_request_r_scale != 15) {
		*((u_int32_t *)optp) = htonl(TCPOPT_NOP << 24 |
		    TCPOPT_WINDOW << 16 | TCPOLEN_WINDOW << 8 |
		    sc->sc_request_r_scale);
		optp += 4;
	}

	if (sc->sc_flags & SCF_TIMESTAMP) {
		u_int32_t *lp = (u_int32_t *)(optp);
		/* Form timestamp option as shown in appendix A of RFC 1323. */
		*lp++ = htonl(TCPOPT_TSTAMP_HDR);
		*lp++ = htonl(SYN_CACHE_TIMESTAMP(sc));
		*lp   = htonl(sc->sc_timestamp);
		optp += TCPOLEN_TSTAMP_APPA;
	}

	/* Compute the packet's checksum. */
	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		ip->ip_len = htons(tlen - hlen);
		th->th_sum = 0;
		th->th_sum = in_cksum(m, tlen);
		break;
#ifdef INET6
	case AF_INET6:
		ip6->ip6_plen = htons(tlen - hlen);
		th->th_sum = 0;
		th->th_sum = in6_cksum(m, IPPROTO_TCP, hlen, tlen - hlen);
		break;
#endif
	}

	/*
	 * Fill in some straggling IP bits.  Note the stack expects
	 * ip_len to be in host order, for convenience.
	 */
	switch (sc->sc_src.sa.sa_family) {
#ifdef INET
	case AF_INET:
		ip->ip_len = tlen;
		ip->ip_ttl = ip_defttl;
		/* XXX tos? */
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6->ip6_vfc |= IPV6_VERSION;
		ip6->ip6_plen = htons(tlen - hlen);
		/* ip6_hlim will be initialized afterwards */
		/* XXX flowlabel? */
		break;
#endif
	}

	switch (sc->sc_src.sa.sa_family) {
#ifdef INET
	case AF_INET:
		error = ip_output(m, sc->sc_ipopts, ro,
		    (ip_mtudisc ? IP_MTUDISC : 0),
		    NULL);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ip6->ip6_hlim = in6_selecthlim(NULL,
				ro->ro_rt ? ro->ro_rt->rt_ifp : NULL);

		error = ip6_output(m, NULL /*XXX*/, (struct route_in6 *)ro,
			0, NULL, NULL);
		break;
#endif
	default:
		error = EAFNOSUPPORT;
		break;
	}
	return (error);
}
