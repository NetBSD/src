/*	$NetBSD: tcp_timer.c,v 1.55 2001/09/11 21:03:21 thorpej Exp $	*/

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
 * Copyright (c) 1997, 1998, 2001 The NetBSD Foundation, Inc.
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
 *	@(#)tcp_timer.c	8.2 (Berkeley) 5/24/95
 */

#include "opt_inet.h"
#include "opt_tcp_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

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
#include <netinet6/in6_pcb.h>
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#ifdef TCP_DEBUG
#include <netinet/tcp_debug.h>
#endif

/*
 * Various tunable timer parameters.  These are initialized in tcp_init(),
 * unless they are patched.
 */
int	tcp_keepidle = 0;
int	tcp_keepintvl = 0;
int	tcp_keepcnt = 0;		/* max idle probes */
int	tcp_maxpersistidle = 0;		/* max idle time in persist */
int	tcp_maxidle;			/* computed in tcp_slowtimo() */

/*
 * Time to delay the ACK.  This is initialized in tcp_init(), unless
 * its patched.
 */
int	tcp_delack_ticks = 0;

void	tcp_timer_rexmt(void *);
void	tcp_timer_persist(void *);
void	tcp_timer_keep(void *);
void	tcp_timer_2msl(void *);

tcp_timer_func_t tcp_timer_funcs[TCPT_NTIMERS] = {
	tcp_timer_rexmt,
	tcp_timer_persist,
	tcp_timer_keep,
	tcp_timer_2msl,
};

/*
 * Timer state initialization, called from tcp_init().
 */
void
tcp_timer_init(void)
{

	if (tcp_keepidle == 0)
		tcp_keepidle = TCPTV_KEEP_IDLE;

	if (tcp_keepintvl == 0)
		tcp_keepintvl = TCPTV_KEEPINTVL;

	if (tcp_keepcnt == 0)
		tcp_keepcnt = TCPTV_KEEPCNT;

	if (tcp_maxpersistidle == 0)
		tcp_maxpersistidle = TCPTV_KEEP_IDLE;

	if (tcp_delack_ticks == 0)
		tcp_delack_ticks = TCP_DELACK_TICKS;
}

/*
 * Callout to process delayed ACKs for a TCPCB.
 */
void
tcp_delack(void *arg)
{
	struct tcpcb *tp = arg;
	int s;

	/*
	 * If tcp_output() wasn't able to transmit the ACK
	 * for whatever reason, it will restart the delayed
	 * ACK callout.
	 */

	s = splsoftnet();
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	splx(s);
}

/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */
void
tcp_slowtimo()
{
	int s;

	s = splsoftnet();
	tcp_maxidle = tcp_keepcnt * tcp_keepintvl;
	tcp_iss_seq += TCP_ISSINCR;			/* increment iss */
	tcp_now++;					/* for timestamps */
	splx(s);
}

/*
 * Cancel all timers for TCP tp.
 */
void
tcp_canceltimers(tp)
	struct tcpcb *tp;
{
	int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		TCP_TIMER_DISARM(tp, i);
}

int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };

int	tcp_totbackoff = 511;	/* sum of tcp_backoff[] */

/*
 * TCP timer processing.
 */

void
tcp_timer_rexmt(void *arg)
{
	struct tcpcb *tp = arg;
	uint32_t rto;
	int s;
#ifdef TCP_DEBUG
	struct socket *so;
	short ostate;
#endif

	s = splsoftnet();

	callout_deactivate(&tp->t_timer[TCPT_REXMT]);

#ifdef TCP_DEBUG
#ifdef INET
	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif
	ostate = tp->t_state;
#endif /* TCP_DEBUG */

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */

	if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
		tp->t_rxtshift = TCP_MAXRXTSHIFT;
		tcpstat.tcps_timeoutdrop++;
		tp = tcp_drop(tp, tp->t_softerror ?
		    tp->t_softerror : ETIMEDOUT);
		goto out;
	}
	tcpstat.tcps_rexmttimeo++;
	rto = TCP_REXMTVAL(tp);
	if (rto < tp->t_rttmin)
		rto = tp->t_rttmin;
	TCPT_RANGESET(tp->t_rxtcur, rto * tcp_backoff[tp->t_rxtshift],
	    tp->t_rttmin, TCPTV_REXMTMAX);
	TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);
#if 0
	/* 
	 * If we are losing and we are trying path MTU discovery,
	 * try turning it off.  This will avoid black holes in
	 * the network which suppress or fail to send "packet
	 * too big" ICMP messages.  We should ideally do
	 * lots more sophisticated searching to find the right
	 * value here...
	 */
	if (ip_mtudisc && tp->t_rxtshift > TCP_MAXRXTSHIFT / 6) {
		struct rtentry *rt = NULL;

#ifdef INET
		if (tp->t_inpcb)
			rt = in_pcbrtentry(tp->t_inpcb);
#endif
#ifdef INET6
		if (tp->t_in6pcb)
			rt = in6_pcbrtentry(tp->t_in6pcb);
#endif

		/* XXX:  Black hole recovery code goes here */
	}
#endif /* 0 */
	/*
	 * If losing, let the lower level know and try for
	 * a better route.  Also, if we backed off this far,
	 * our srtt estimate is probably bogus.  Clobber it
	 * so we'll take the next rtt measurement as our srtt;
	 * move the current srtt into rttvar to keep the current
	 * retransmit times until then.
	 */
	if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
#ifdef INET
		if (tp->t_inpcb)
			in_losing(tp->t_inpcb);
#endif
#ifdef INET6
		if (tp->t_in6pcb)
			in6_losing(tp->t_in6pcb);
#endif
		tp->t_rttvar += (tp->t_srtt >> TCP_RTT_SHIFT);
		tp->t_srtt = 0;
	}
	tp->snd_nxt = tp->snd_una;
	/*
	 * If timing a segment in this window, stop the timer.
	 */
	tp->t_rtttime = 0;
	/*
	 * Remember if we are retransmitting a SYN, because if
	 * we do, set the initial congestion window must be set
	 * to 1 segment.
	 */
	if (tp->t_state == TCPS_SYN_SENT)
		tp->t_flags |= TF_SYN_REXMT;
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
	{
	u_int win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_segsz;
	if (win < 2)
		win = 2;
	/* Loss Window MUST be one segment. */
	tp->snd_cwnd = tp->t_segsz;
	tp->snd_ssthresh = win * tp->t_segsz;
	tp->t_dupacks = 0;
	}
	(void) tcp_output(tp);

 out:
#ifdef TCP_DEBUG
	if (tp && so->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, NULL,
		    PRU_SLOWTIMO | (TCPT_REXMT << 8));
#endif
	splx(s);
}

void
tcp_timer_persist(void *arg)
{
	struct tcpcb *tp = arg;
	struct socket *so;
	uint32_t rto;
	int s;
#ifdef TCP_DEBUG
	short ostate;
#endif

	s = splsoftnet();

	callout_deactivate(&tp->t_timer[TCPT_PERSIST]);

#ifdef INET
	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif

#ifdef TCP_DEBUG
	ostate = tp->t_state;
#endif

	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */

	/*
	 * Hack: if the peer is dead/unreachable, we do not
	 * time out if the window is closed.  After a full
	 * backoff, drop the connection if the idle time
	 * (no responses to probes) reaches the maximum
	 * backoff that we would use if retransmitting.
	 */
	rto = TCP_REXMTVAL(tp);
	if (rto < tp->t_rttmin)
		rto = tp->t_rttmin;
	if (tp->t_rxtshift == TCP_MAXRXTSHIFT &&
	    ((tcp_now - tp->t_rcvtime) >= tcp_maxpersistidle ||
	    (tcp_now - tp->t_rcvtime) >= rto * tcp_totbackoff)) {
		tcpstat.tcps_persistdrops++;
		tp = tcp_drop(tp, ETIMEDOUT);
		goto out;
	}
	tcpstat.tcps_persisttimeo++;
	tcp_setpersist(tp);
	tp->t_force = 1;
	(void) tcp_output(tp);
	tp->t_force = 0;

 out:
#ifdef TCP_DEBUG
	if (tp && so->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, NULL,
		    PRU_SLOWTIMO | (TCPT_PERSIST << 8));
#endif
	splx(s);
}

void
tcp_timer_keep(void *arg)
{
	struct tcpcb *tp = arg;
	struct socket *so;
	int s;
#ifdef TCP_DEBUG
	short ostate;
#endif

	s = splsoftnet();

	callout_deactivate(&tp->t_timer[TCPT_KEEP]);

#ifdef TCP_DEBUG
	ostate = tp->t_state;
#endif /* TCP_DEBUG */

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */

	tcpstat.tcps_keeptimeo++;
	if (TCPS_HAVEESTABLISHED(tp->t_state) == 0)
		goto dropit;
#ifdef INET
	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif
	if (so->so_options & SO_KEEPALIVE &&
	    tp->t_state <= TCPS_CLOSE_WAIT) {
	    	if ((tcp_maxidle > 0) &&
		    ((tcp_now - tp->t_rcvtime) >=
		     tcp_keepidle + tcp_maxidle))
			goto dropit;
		/*
		 * Send a packet designed to force a response
		 * if the peer is up and reachable:
		 * either an ACK if the connection is still alive,
		 * or an RST if the peer has closed the connection
		 * due to timeout or reboot.
		 * Using sequence number tp->snd_una-1
		 * causes the transmitted zero-length segment
		 * to lie outside the receive window;
		 * by the protocol spec, this requires the
		 * correspondent TCP to respond.
		 */
		tcpstat.tcps_keepprobe++;
		if (tcp_compat_42) {
			/*
			 * The keepalive packet must have nonzero
			 * length to get a 4.2 host to respond.
			 */
			(void)tcp_respond(tp, tp->t_template,
			    (struct mbuf *)NULL, NULL, tp->rcv_nxt - 1,
			    tp->snd_una - 1, 0);
		} else {
			(void)tcp_respond(tp, tp->t_template,
			    (struct mbuf *)NULL, NULL, tp->rcv_nxt,
			    tp->snd_una - 1, 0);
		}
		TCP_TIMER_ARM(tp, TCPT_KEEP, tcp_keepintvl);
	} else
		TCP_TIMER_ARM(tp, TCPT_KEEP, tcp_keepidle);

#ifdef TCP_DEBUG
	if (tp && so->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, NULL,
		    PRU_SLOWTIMO | (TCPT_KEEP << 8));
#endif
	splx(s);
	return;

 dropit:
	tcpstat.tcps_keepdrops++;
	(void) tcp_drop(tp, ETIMEDOUT);
	splx(s);
}

void
tcp_timer_2msl(void *arg)
{
	struct tcpcb *tp = arg;
	struct socket *so;
	int s;
#ifdef TCP_DEBUG
	short ostate;
#endif

	s = splsoftnet();

	callout_deactivate(&tp->t_timer[TCPT_2MSL]);

#ifdef INET
	if (tp->t_inpcb)
		so = tp->t_inpcb->inp_socket;
#endif
#ifdef INET6
	if (tp->t_in6pcb)
		so = tp->t_in6pcb->in6p_socket;
#endif

#ifdef TCP_DEBUG
	ostate = tp->t_state;
#endif

	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	if (tp->t_state != TCPS_TIME_WAIT &&
	    ((tcp_maxidle == 0) || ((tcp_now - tp->t_rcvtime) <= tcp_maxidle)))
		TCP_TIMER_ARM(tp, TCPT_2MSL, tcp_keepintvl);
	else
		tp = tcp_close(tp);

#ifdef TCP_DEBUG
	if (tp && so->so_options & SO_DEBUG)
		tcp_trace(TA_USER, ostate, tp, NULL,
		    PRU_SLOWTIMO | (TCPT_2MSL << 8));
#endif
	splx(s);
}
