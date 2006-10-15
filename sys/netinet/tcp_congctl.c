/*	$NetBSD: tcp_congctl.c,v 1.7 2006/10/15 17:53:30 rpaulo Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2001, 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
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
__KERNEL_RCSID(0, "$NetBSD: tcp_congctl.c,v 1.7 2006/10/15 17:53:30 rpaulo Exp $");

#include "opt_inet.h"
#include "opt_tcp_debug.h"
#include "opt_tcp_congctl.h"

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
#include <sys/lock.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
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

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_congctl.h>
#ifdef TCP_DEBUG
#include <netinet/tcp_debug.h>
#endif

/*
 * TODO:
 *   consider separating the actual implementations in another file.
 */

static int  tcp_reno_fast_retransmit(struct tcpcb *, struct tcphdr *);
static void tcp_reno_slow_retransmit(struct tcpcb *);
static void tcp_reno_fast_retransmit_newack(struct tcpcb *, struct tcphdr *);
static void tcp_reno_newack(struct tcpcb *, struct tcphdr *);
static void tcp_reno_congestion_exp(struct tcpcb *tp);

static int  tcp_newreno_fast_retransmit(struct tcpcb *, struct tcphdr *);
static void tcp_newreno_fast_retransmit_newack(struct tcpcb *,
	struct tcphdr *);
static void tcp_newreno_newack(struct tcpcb *, struct tcphdr *);


static void tcp_congctl_fillnames(void);

extern int tcprexmtthresh;

MALLOC_DEFINE(M_TCPCONGCTL, "tcpcongctl", "TCP congestion control structures");

/*
 * Used to list the available congestion control algorithms.
 */
struct tcp_congctlent {
	TAILQ_ENTRY(tcp_congctlent) congctl_ent;
	char               congctl_name[TCPCC_MAXLEN];
	struct tcp_congctl *congctl_ctl;
};
TAILQ_HEAD(, tcp_congctlent) tcp_congctlhd;

struct simplelock tcp_congctl_slock;

void
tcp_congctl_init(void)
{
	int r;
	
	TAILQ_INIT(&tcp_congctlhd);
	simple_lock_init(&tcp_congctl_slock);

	/* Base algorithms. */
	r = tcp_congctl_register("reno", &tcp_reno_ctl);
	KASSERT(r == 0);
	r = tcp_congctl_register("newreno", &tcp_newreno_ctl);
	KASSERT(r == 0);

	/* NewReno is the default. */
#ifndef TCP_CONGCTL_DEFAULT
#define TCP_CONGCTL_DEFAULT "newreno"
#endif

	r = tcp_congctl_select(NULL, TCP_CONGCTL_DEFAULT);
	KASSERT(r == 0);
}

/*
 * Register a congestion algorithm and select it if we have none.
 */
int
tcp_congctl_register(const char *name, struct tcp_congctl *tcc)
{
	struct tcp_congctlent *ntcc, *tccp;

	TAILQ_FOREACH(tccp, &tcp_congctlhd, congctl_ent) 
		if (!strcmp(name, tccp->congctl_name)) {
			/* name already registered */
			return EEXIST;
		}

	ntcc = malloc(sizeof(*ntcc), M_TCPCONGCTL, M_WAITOK);

	strlcpy(ntcc->congctl_name, name, sizeof(ntcc->congctl_name) - 1);
	ntcc->congctl_ctl = tcc;

	TAILQ_INSERT_TAIL(&tcp_congctlhd, ntcc, congctl_ent);
	tcp_congctl_fillnames();

	if (TAILQ_FIRST(&tcp_congctlhd) == ntcc)
		tcp_congctl_select(NULL, name);
		
	return 0;
}

int
tcp_congctl_unregister(const char *name)
{
	struct tcp_congctlent *tccp, *rtccp;
	unsigned int size;
	
	rtccp = NULL;
	size = 0;
	TAILQ_FOREACH(tccp, &tcp_congctlhd, congctl_ent) {
		if (!strcmp(name, tccp->congctl_name))
			rtccp = tccp;
		size++;
	}
	
	if (!rtccp)
		return ENOENT;

	if (size <= 1 || tcp_congctl_global == rtccp->congctl_ctl ||
	    rtccp->congctl_ctl->refcnt)
		return EBUSY;

	TAILQ_REMOVE(&tcp_congctlhd, rtccp, congctl_ent);
	free(rtccp, M_TCPCONGCTL);
	tcp_congctl_fillnames();

	return 0;
}

/*
 * Select a congestion algorithm by name.
 */
int
tcp_congctl_select(struct tcpcb *tp, const char *name)
{
	struct tcp_congctlent *tccp;

	KASSERT(name);

	TAILQ_FOREACH(tccp, &tcp_congctlhd, congctl_ent)
		if (!strcmp(name, tccp->congctl_name)) {
			if (tp) {
				simple_lock(&tcp_congctl_slock);
				tp->t_congctl->refcnt--;
				tp->t_congctl = tccp->congctl_ctl;
				tp->t_congctl->refcnt++;
				simple_unlock(&tcp_congctl_slock);
			} else {
				tcp_congctl_global = tccp->congctl_ctl;
				strlcpy(tcp_congctl_global_name,
				    tccp->congctl_name,
				    sizeof(tcp_congctl_global_name) - 1);
			}
			return 0;
		}
	
	return EINVAL;
}

/*
 * Returns the name of a congestion algorithm.
 */
const char *
tcp_congctl_bystruct(const struct tcp_congctl *tcc)
{
	struct tcp_congctlent *tccp;
	
	KASSERT(tcc);
	
	TAILQ_FOREACH(tccp, &tcp_congctlhd, congctl_ent)
		if (tccp->congctl_ctl == tcc)
			return tccp->congctl_name;

	return NULL;
}

static void
tcp_congctl_fillnames(void)
{
	struct tcp_congctlent *tccp;
	const char *delim = " ";
	
	tcp_congctl_avail[0] = '\0';
	TAILQ_FOREACH(tccp, &tcp_congctlhd, congctl_ent) {
		strlcat(tcp_congctl_avail, tccp->congctl_name,
		    sizeof(tcp_congctl_avail) - 1);
		if (TAILQ_NEXT(tccp, congctl_ent))
			strlcat(tcp_congctl_avail, delim, 
			    sizeof(tcp_congctl_avail) - 1);
	}	
	
}

/* ------------------------------------------------------------------------ */

/*
 * TCP/Reno congestion control.
 */
static void
tcp_reno_congestion_exp(struct tcpcb *tp)
{
	u_int win;

	/* 
	 * Halve the congestion window and reduce the
	 * slow start threshold.
	 */
	win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_segsz;
	if (win < 2)
		win = 2;

	tp->snd_ssthresh = win * tp->t_segsz;
	tp->snd_recover = tp->snd_max;
	tp->snd_cwnd = tp->snd_ssthresh;

	/*
	 * When using TCP ECN, notify the peer that
	 * we reduced the cwnd.
	 */
	if (TCP_ECN_ALLOWED(tp))
		tp->t_flags |= TF_ECN_SND_CWR;
}



static int
tcp_reno_fast_retransmit(struct tcpcb *tp, struct tcphdr *th)
{
	/*
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
	 *
	 * If we are using TCP/SACK, then enter
	 * Fast Recovery if the receiver SACKs
	 * data that is tcprexmtthresh * MSS
	 * bytes past the last ACKed segment,
	 * irrespective of the number of DupAcks.
	 */
	
	tcp_seq onxt;
	
	onxt = tp->snd_nxt;
	tcp_reno_congestion_exp(tp);
	tp->t_partialacks = 0;
	TCP_TIMER_DISARM(tp, TCPT_REXMT);
	tp->t_rtttime = 0;
	if (TCP_SACK_ENABLED(tp)) {
		tp->t_dupacks = tcprexmtthresh;
		tp->sack_newdata = tp->snd_nxt;
		tp->snd_cwnd = tp->t_segsz;
		(void) tcp_output(tp);
		return 0;
	}
	tp->snd_nxt = th->th_ack;
	tp->snd_cwnd = tp->t_segsz;
	(void) tcp_output(tp);
	tp->snd_cwnd = tp->snd_ssthresh + tp->t_segsz * tp->t_dupacks;
	if (SEQ_GT(onxt, tp->snd_nxt))
		tp->snd_nxt = onxt;
	
	return 0;
}

static void
tcp_reno_slow_retransmit(struct tcpcb *tp)
{
	u_int win;

	/*
	 * Close the congestion window down to one segment
	 * (we'll open it by one segment for each ack we get).
	 * Since we probably have a window's worth of unacked
	 * data accumulated, this "slow start" keeps us from
	 * dumping all that data as back-to-back packets (which
	 * might overwhelm an intermediate gateway).
	 *
	 * There are two phases to the opening: Initially we
	 * open by one mss on each ack.  This makes the window
	 * size increase exponentially with time.  If the
	 * window is larger than the path can handle, this
	 * exponential growth results in dropped packet(s)
	 * almost immediately.  To get more time between
	 * drops but still "push" the network to take advantage
	 * of improving conditions, we switch from exponential
	 * to linear window opening at some threshhold size.
	 * For a threshhold, we use half the current window
	 * size, truncated to a multiple of the mss.
	 *
	 * (the minimum cwnd that will give us exponential
	 * growth is 2 mss.  We don't allow the threshhold
	 * to go below this.)
	 */

	win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_segsz;
	if (win < 2)
		win = 2;
	/* Loss Window MUST be one segment. */
	tp->snd_cwnd = tp->t_segsz;
	tp->snd_ssthresh = win * tp->t_segsz;
	tp->t_partialacks = -1;
	tp->t_dupacks = 0;
}

static void
tcp_reno_fast_retransmit_newack(struct tcpcb *tp, struct tcphdr *th __unused)
{
	if (tp->t_partialacks < 0) {
		/*
		 * We were not in fast recovery.  Reset the duplicate ack
		 * counter.
		 */
		tp->t_dupacks = 0;
	} else {
		/*
		 * Clamp the congestion window to the crossover point and
		 * exit fast recovery.
		 */
		if (tp->snd_cwnd > tp->snd_ssthresh)
			tp->snd_cwnd = tp->snd_ssthresh;
		tp->t_partialacks = -1;
		tp->t_dupacks = 0;
	}
}

static void
tcp_reno_newack(struct tcpcb *tp, struct tcphdr *th __unused)
{
	/*
	 * When new data is acked, open the congestion window.
	 * If the window gives us less than ssthresh packets
	 * in flight, open exponentially (segsz per packet).
	 * Otherwise open linearly: segsz per window
	 * (segsz^2 / cwnd per packet).
	 */

	u_int cw = tp->snd_cwnd;
	u_int incr = tp->t_segsz;

	if (cw >= tp->snd_ssthresh)
		incr = incr * incr / cw;

	tp->snd_cwnd = min(cw + incr, TCP_MAXWIN << tp->snd_scale);
}

struct tcp_congctl tcp_reno_ctl = {
	.fast_retransmit = tcp_reno_fast_retransmit,
	.slow_retransmit = tcp_reno_slow_retransmit,
	.fast_retransmit_newack = tcp_reno_fast_retransmit_newack,
	.newack = tcp_reno_newack,
	.cong_exp = tcp_reno_congestion_exp,
};

/*
 * TCP/NewReno Congestion control.
 */
static int
tcp_newreno_fast_retransmit(struct tcpcb *tp, struct tcphdr *th)
{
	if (SEQ_LT(th->th_ack, tp->snd_high)) {
		/*
		 * False fast retransmit after timeout.
		 * Do not enter fast recovery
		 */
		tp->t_dupacks = 0;
		return 1;
	} else {
		/*
		 * Fast retransmit is same as reno.
		 */
		return tcp_reno_fast_retransmit(tp, th);
	}

	return 0;
}

/*
 * Implement the NewReno response to a new ack, checking for partial acks in
 * fast recovery.
 */
static void
tcp_newreno_fast_retransmit_newack(struct tcpcb *tp, struct tcphdr *th)
{
	if (tp->t_partialacks < 0) {
		/*
		 * We were not in fast recovery.  Reset the duplicate ack
		 * counter.
		 */
		tp->t_dupacks = 0;
	} else if (SEQ_LT(th->th_ack, tp->snd_recover)) {
		/*
		 * This is a partial ack.  Retransmit the first unacknowledged
		 * segment and deflate the congestion window by the amount of
		 * acknowledged data.  Do not exit fast recovery.
		 */
		tcp_seq onxt = tp->snd_nxt;
		u_long ocwnd = tp->snd_cwnd;

		/*
		 * snd_una has not yet been updated and the socket's send
		 * buffer has not yet drained off the ACK'd data, so we
		 * have to leave snd_una as it was to get the correct data
		 * offset in tcp_output().
		 */
		if (++tp->t_partialacks == 1)
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
	} else {
		/*
		 * Complete ack.  Inflate the congestion window to ssthresh
		 * and exit fast recovery.
		 *
		 * Window inflation should have left us with approx.
		 * snd_ssthresh outstanding data.  But in case we
		 * would be inclined to send a burst, better to do
		 * it via the slow start mechanism.
		 */
		if (SEQ_SUB(tp->snd_max, th->th_ack) < tp->snd_ssthresh)
			tp->snd_cwnd = SEQ_SUB(tp->snd_max, th->th_ack)
			    + tp->t_segsz;
		else
			tp->snd_cwnd = tp->snd_ssthresh;
		tp->t_partialacks = -1;
		tp->t_dupacks = 0;
	}
}

static void
tcp_newreno_newack(struct tcpcb *tp, struct tcphdr *th)
{
	/*
	 * If we are still in fast recovery (meaning we are using
	 * NewReno and we have only received partial acks), do not
	 * inflate the window yet.
	 */
	if (tp->t_partialacks < 0)
		tcp_reno_newack(tp, th);
}


struct tcp_congctl tcp_newreno_ctl = {
	.fast_retransmit = tcp_newreno_fast_retransmit,
	.slow_retransmit = tcp_reno_slow_retransmit,
	.fast_retransmit_newack = tcp_newreno_fast_retransmit_newack,
	.newack = tcp_newreno_newack,
	.cong_exp = tcp_reno_congestion_exp,
};


