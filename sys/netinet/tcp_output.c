/*	$NetBSD: tcp_output.c,v 1.79.4.1 2002/06/14 17:32:59 lukem Exp $	*/

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
 *	@(#)tcp_output.c	8.4 (Berkeley) 5/24/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcp_output.c,v 1.79.4.1 2002/06/14 17:32:59 lukem Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_tcp_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/domain.h>
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
#include <netinet6/ip6_var.h>
#endif

#include <netinet/tcp.h>
#define	TCPOUTFLAGS
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>

#ifdef notyet
extern struct mbuf *m_copypack();
#endif

#define MAX_TCPOPTLEN	32	/* max # bytes that go in options */

/*
 * Knob to enable Congestion Window Monitoring, and control the
 * the burst size it allows.  Default burst is 4 packets, per
 * the Internet draft.
 */
int	tcp_cwm = 0;
int	tcp_cwm_burstsize = 4;

#ifdef TCP_OUTPUT_COUNTERS
#include <sys/device.h>

extern struct evcnt tcp_output_bigheader;
extern struct evcnt tcp_output_copysmall;
extern struct evcnt tcp_output_copybig;
extern struct evcnt tcp_output_refbig;

#define	TCP_OUTPUT_COUNTER_INCR(ev)	(ev)->ev_count++
#else

#define	TCP_OUTPUT_COUNTER_INCR(ev)	/* nothing */

#endif /* TCP_OUTPUT_COUNTERS */

static
#ifndef GPROF
__inline
#endif
void
tcp_segsize(struct tcpcb *tp, int *txsegsizep, int *rxsegsizep)
{
#ifdef INET
	struct inpcb *inp = tp->t_inpcb;
#endif
#ifdef INET6
	struct in6pcb *in6p = tp->t_in6pcb;
#endif
	struct rtentry *rt;
	struct ifnet *ifp;
	int size;
	int iphlen;
	int optlen;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_segsize: both t_inpcb and t_in6pcb are set");
#endif
	switch (tp->t_family) {
#ifdef INET
	case AF_INET:
		iphlen = sizeof(struct ip);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		iphlen = sizeof(struct ip6_hdr);
		break;
#endif
	default:
		size = tcp_mssdflt;
		goto out;
	}

	rt = NULL;
#ifdef INET
	if (inp)
		rt = in_pcbrtentry(inp);
#endif
#ifdef INET6
	if (in6p)
		rt = in6_pcbrtentry(in6p);
#endif
	if (rt == NULL) {
		size = tcp_mssdflt;
		goto out;
	}

	ifp = rt->rt_ifp;

	size = tcp_mssdflt;
	if (rt->rt_rmx.rmx_mtu != 0)
		size = rt->rt_rmx.rmx_mtu - iphlen - sizeof(struct tcphdr);
	else if (ifp->if_flags & IFF_LOOPBACK)
		size = ifp->if_mtu - iphlen - sizeof(struct tcphdr);
#ifdef INET
	else if (inp && ip_mtudisc)
		size = ifp->if_mtu - iphlen - sizeof(struct tcphdr);
	else if (inp && in_localaddr(inp->inp_faddr))
		size = ifp->if_mtu - iphlen - sizeof(struct tcphdr);
#endif
#ifdef INET6
	else if (in6p) {
#ifdef INET
		if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr)) {
			/* mapped addr case */
			struct in_addr d;
			bcopy(&in6p->in6p_faddr.s6_addr32[3], &d, sizeof(d));
			if (ip_mtudisc || in_localaddr(d))
				size = ifp->if_mtu - iphlen - sizeof(struct tcphdr);
		} else
#endif
		{
			/*
			 * for IPv6, path MTU discovery is always turned on,
			 * or the node must use packet size <= 1280.
			 */
			size = ifp->if_mtu - iphlen - sizeof(struct tcphdr);
		}
	}
#endif
 out:
	/*
	 * Now we must make room for whatever extra TCP/IP options are in
	 * the packet.
	 */
	optlen = tcp_optlen(tp);

	/*
	 * XXX tp->t_ourmss should have the right size, but without this code
	 * fragmentation will occur... need more investigation
	 */
#ifdef INET
	if (inp) {
#ifdef IPSEC
		optlen += ipsec4_hdrsiz_tcp(tp);
#endif
		optlen += ip_optlen(inp);
	}
#endif
#ifdef INET6
#ifdef INET
	if (in6p && tp->t_family == AF_INET) {
#ifdef IPSEC
		optlen += ipsec4_hdrsiz_tcp(tp);
#endif
		/* XXX size -= ip_optlen(in6p); */
	} else
#endif
	if (in6p && tp->t_family == AF_INET6) {
#ifdef IPSEC
		optlen += ipsec6_hdrsiz_tcp(tp);
#endif
		optlen += ip6_optlen(in6p);
	}
#endif
	size -= optlen;

	/*
	 * *rxsegsizep holds *estimated* inbound segment size (estimation
	 * assumes that path MTU is the same for both ways).  this is only
	 * for silly window avoidance, do not use the value for other purposes.
	 *
	 * ipseclen is subtracted from both sides, this may not be right.
	 * I'm not quite sure about this (could someone comment).
	 */
	*txsegsizep = min(tp->t_peermss - optlen, size);
	*rxsegsizep = min(tp->t_ourmss - optlen, size);

	if (*txsegsizep != tp->t_segsz) {
		/*
		 * If the new segment size is larger, we don't want to 
		 * mess up the congestion window, but if it is smaller
		 * we'll have to reduce the congestion window to ensure
		 * that we don't get into trouble with initial windows
		 * and the rest.  In any case, if the segment size
		 * has changed, chances are the path has, too, and
		 * our congestion window will be different.
		 */
		if (*txsegsizep < tp->t_segsz) {
			tp->snd_cwnd = max((tp->snd_cwnd / tp->t_segsz) 
					   * *txsegsizep, *txsegsizep);
			tp->snd_ssthresh = max((tp->snd_ssthresh / tp->t_segsz) 
						* *txsegsizep, *txsegsizep);
		}
		tp->t_segsz = *txsegsizep;
	}
}

static
#ifndef GPROF
__inline
#endif
int
tcp_build_datapkt(struct tcpcb *tp, struct socket *so, int off,
    long len, int hdrlen, struct mbuf **mp)
{
	struct mbuf *m;

	if (tp->t_force && len == 1)
		tcpstat.tcps_sndprobe++;
	else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
		tcpstat.tcps_sndrexmitpack++;
		tcpstat.tcps_sndrexmitbyte += len;
	} else {
		tcpstat.tcps_sndpack++;
		tcpstat.tcps_sndbyte += len;
	}
#ifdef notyet
	if ((m = m_copypack(so->so_snd.sb_mb, off,
	    (int)len, max_linkhdr + hdrlen)) == 0)
		return (ENOBUFS);
	/*
	 * m_copypack left space for our hdr; use it.
	 */
	m->m_len += hdrlen;
	m->m_data -= hdrlen;
#else
	MGETHDR(m, M_DONTWAIT, MT_HEADER);
	if (__predict_false(m == NULL))
		return (ENOBUFS);

	/*
	 * XXX Because other code assumes headers will fit in
	 * XXX one header mbuf.
	 *
	 * (This code should almost *never* be run.)
	 */
	if (__predict_false((max_linkhdr + hdrlen) > MHLEN)) {
		TCP_OUTPUT_COUNTER_INCR(&tcp_output_bigheader);
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return (ENOBUFS);
		}
	}

	m->m_data += max_linkhdr;
	m->m_len = hdrlen;
	if (len <= M_TRAILINGSPACE(m)) {
		m_copydata(so->so_snd.sb_mb, off, (int) len,
		    mtod(m, caddr_t) + hdrlen);
		m->m_len += len;
		TCP_OUTPUT_COUNTER_INCR(&tcp_output_copysmall);
	} else {
		m->m_next = m_copy(so->so_snd.sb_mb, off, (int) len);
		if (m->m_next == NULL) {
			m_freem(m);
			return (ENOBUFS);
		}
#ifdef TCP_OUTPUT_COUNTERS
		if (m->m_next->m_flags & M_EXT)
			TCP_OUTPUT_COUNTER_INCR(&tcp_output_refbig);
		else
			TCP_OUTPUT_COUNTER_INCR(&tcp_output_copybig);
#endif /* TCP_OUTPUT_COUNTERS */
	}
#endif

	*mp = m;
	return (0);
}

/*
 * Tcp output routine: figure out what should be sent and send it.
 */
int
tcp_output(tp)
	struct tcpcb *tp;
{
	struct socket *so;
	struct route *ro;
	long len, win;
	int off, flags, error;
	struct mbuf *m;
	struct ip *ip;
#ifdef INET6
	struct ip6_hdr *ip6;
#endif
	struct tcphdr *th;
	u_char opt[MAX_TCPOPTLEN];
	unsigned optlen, hdrlen;
	int idle, sendalot, txsegsize, rxsegsize;
	int maxburst = TCP_MAXBURST;
	int af;		/* address family on the wire */
	int iphdrlen;

#ifdef DIAGNOSTIC
	if (tp->t_inpcb && tp->t_in6pcb)
		panic("tcp_output: both t_inpcb and t_in6pcb are set");
#endif
	so = NULL;
	ro = NULL;
	if (tp->t_inpcb) {
		so = tp->t_inpcb->inp_socket;
		ro = &tp->t_inpcb->inp_route;
	}
#ifdef INET6
	else if (tp->t_in6pcb) {
		so = tp->t_in6pcb->in6p_socket;
		ro = (struct route *)&tp->t_in6pcb->in6p_route;
	}
#endif

	switch (af = tp->t_family) {
#ifdef INET
	case AF_INET:
		if (tp->t_inpcb)
			break;
#ifdef INET6
		/* mapped addr case */
		if (tp->t_in6pcb)
			break;
#endif
		return EINVAL;
#endif
#ifdef INET6
	case AF_INET6:
		if (tp->t_in6pcb)
			break;
		return EINVAL;
#endif
	default:
		return EAFNOSUPPORT;
	}

	tcp_segsize(tp, &txsegsize, &rxsegsize);

	idle = (tp->snd_max == tp->snd_una);

	/*
	 * Restart Window computation.  From draft-floyd-incr-init-win-03:
	 *
	 *	Optionally, a TCP MAY set the restart window to the
	 *	minimum of the value used for the initial window and
	 *	the current value of cwnd (in other words, using a
	 *	larger value for the restart window should never increase
	 *	the size of cwnd).
	 */
	if (tcp_cwm) {
		/*
		 * Hughes/Touch/Heidemann Congestion Window Monitoring.
		 * Count the number of packets currently pending
		 * acknowledgement, and limit our congestion window
		 * to a pre-determined allowed burst size plus that count.
		 * This prevents bursting once all pending packets have
		 * been acknowledged (i.e. transmission is idle).
		 *
		 * XXX Link this to Initial Window?
		 */
		tp->snd_cwnd = min(tp->snd_cwnd,
		    (tcp_cwm_burstsize * txsegsize) +
		    (tp->snd_nxt - tp->snd_una));
	} else {
		if (idle && (tcp_now - tp->t_rcvtime) >= tp->t_rxtcur) {
			/*
			 * We have been idle for "a while" and no acks are
			 * expected to clock out any data we send --
			 * slow start to get ack "clock" running again.
			 */
			tp->snd_cwnd = min(tp->snd_cwnd,
			    TCP_INITIAL_WINDOW(tcp_init_win, txsegsize));
		}
	}

again:
	/*
	 * Determine length of data that should be transmitted, and
	 * flags that should be used.  If there is some data or critical
	 * controls (SYN, RST) to send, then transmit; otherwise,
	 * investigate further.
	 */
	sendalot = 0;
	off = tp->snd_nxt - tp->snd_una;
	win = min(tp->snd_wnd, tp->snd_cwnd);

	flags = tcp_outflags[tp->t_state];
	/*
	 * If in persist timeout with window of 0, send 1 byte.
	 * Otherwise, if window is small but nonzero
	 * and timer expired, we will send what we can
	 * and go to transmit state.
	 */
	if (tp->t_force) {
		if (win == 0) {
			/*
			 * If we still have some data to send, then
			 * clear the FIN bit.  Usually this would
			 * happen below when it realizes that we
			 * aren't sending all the data.  However,
			 * if we have exactly 1 byte of unset data,
			 * then it won't clear the FIN bit below,
			 * and if we are in persist state, we wind
			 * up sending the packet without recording
			 * that we sent the FIN bit.
			 *
			 * We can't just blindly clear the FIN bit,
			 * because if we don't have any more data
			 * to send then the probe will be the FIN
			 * itself.
			 */
			if (off < so->so_snd.sb_cc)
				flags &= ~TH_FIN;
			win = 1;
		} else {
			TCP_TIMER_DISARM(tp, TCPT_PERSIST);
			tp->t_rxtshift = 0;
		}
	}

	if (win < so->so_snd.sb_cc) {
		len = win - off;
		flags &= ~TH_FIN;
	} else
		len = so->so_snd.sb_cc - off;

	if (len < 0) {
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be -1.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit, pull snd_nxt back
		 * to (closed) window, and set the persist timer
		 * if it isn't already going.  If the window didn't
		 * close completely, just wait for an ACK.
		 *
		 * If we have a pending FIN, either it has already been
		 * transmitted or it is outside the window, so drop it.
		 * If the FIN has been transmitted, but this is not a
		 * retransmission, then len must be -1.  Therefore we also
		 * prevent here the sending of `gratuitous FINs'.  This
		 * eliminates the need to check for that case below (e.g.
		 * to back up snd_nxt before the FIN so that the sequence
		 * number is correct).
		 */
		len = 0;
		flags &= ~TH_FIN;
		if (win == 0) {
			TCP_TIMER_DISARM(tp, TCPT_REXMT);
			tp->t_rxtshift = 0;
			tp->snd_nxt = tp->snd_una;
			if (TCP_TIMER_ISARMED(tp, TCPT_PERSIST) == 0)
				tcp_setpersist(tp);
		}
	}
	if (len > txsegsize) {
		len = txsegsize;
		flags &= ~TH_FIN;
		sendalot = 1;
	}

	win = sbspace(&so->so_rcv);

	/*
	 * Sender silly window avoidance.  If connection is idle
	 * and can send all data, a maximum segment,
	 * at least a maximum default-size segment do it,
	 * or are forced, do it; otherwise don't bother.
	 * If peer's buffer is tiny, then send
	 * when window is at least half open.
	 * If retransmitting (possibly after persist timer forced us
	 * to send into a small window), then must resend.
	 */
	if (len) {
		if (len == txsegsize)
			goto send;
		if ((so->so_state & SS_MORETOCOME) == 0 &&
		    ((idle || tp->t_flags & TF_NODELAY) &&
		     len + off >= so->so_snd.sb_cc))
			goto send;
		if (tp->t_force)
			goto send;
		if (len >= tp->max_sndwnd / 2)
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
	}

	/*
	 * Compare available window to amount of window known to peer
	 * (as advertised window less next expected input).  If the
	 * difference is at least twice the size of the largest segment
	 * we expect to receive (i.e. two segments) or at least 50% of
	 * the maximum possible window, then want to send a window update
	 * to peer.
	 */
	if (win > 0) {
		/* 
		 * "adv" is the amount we can increase the window,
		 * taking into account that we are limited by
		 * TCP_MAXWIN << tp->rcv_scale.
		 */
		long adv = min(win, (long)TCP_MAXWIN << tp->rcv_scale) -
			(tp->rcv_adv - tp->rcv_nxt);

		if (adv >= (long) (2 * rxsegsize))
			goto send;
		if (2 * adv >= (long) so->so_rcv.sb_hiwat)
			goto send;
	}

	/*
	 * Send if we owe peer an ACK.
	 */
	if (tp->t_flags & TF_ACKNOW)
		goto send;
	if (flags & (TH_SYN|TH_FIN|TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are:
	 *	idle			not doing retransmits or persists
	 *	persisting		to move a small or zero window
	 *	(re)transmitting	and thereby not persisting
	 *
	 * tp->t_timer[TCPT_PERSIST]
	 *	is set when we are in persist state.
	 * tp->t_force
	 *	is set when we are called to send a persist packet.
	 * tp->t_timer[TCPT_REXMT]
	 *	is set when we are retransmitting
	 * The output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state.
	 * If nothing happens soon, send when timer expires:
	 * if window is nonzero, transmit what we can,
	 * otherwise force out a byte.
	 */
	if (so->so_snd.sb_cc && TCP_TIMER_ISARMED(tp, TCPT_REXMT) == 0 &&
	    TCP_TIMER_ISARMED(tp, TCPT_PERSIST) == 0) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}

	/*
	 * No reason to send a segment, just return.
	 */
	return (0);

send:
	/*
	 * Before ESTABLISHED, force sending of initial options
	 * unless TCP set not to do any options.
	 * NOTE: we assume that the IP/TCP header plus TCP options
	 * always fit in a single mbuf, leaving room for a maximum
	 * link header, i.e.
	 *	max_linkhdr + sizeof (struct tcpiphdr) + optlen <= MCLBYTES
	 */
	optlen = 0;
	switch (af) {
#ifdef INET
	case AF_INET:
		iphdrlen = sizeof(struct ip) + sizeof(struct tcphdr);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		iphdrlen = sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
		break;
#endif
	default:	/*pacify gcc*/
		iphdrlen = 0;
		break;
	}
	hdrlen = iphdrlen;
	if (flags & TH_SYN) {
		struct rtentry *rt;

		rt = NULL;
#ifdef INET
		if (tp->t_inpcb)
			rt = in_pcbrtentry(tp->t_inpcb);
#endif
#ifdef INET6
		if (tp->t_in6pcb)
			rt = in6_pcbrtentry(tp->t_in6pcb);
#endif

		tp->snd_nxt = tp->iss;
		tp->t_ourmss = tcp_mss_to_advertise(rt != NULL ? 
						    rt->rt_ifp : NULL, af);
		if ((tp->t_flags & TF_NOOPT) == 0) {
			opt[0] = TCPOPT_MAXSEG;
			opt[1] = 4;
			opt[2] = (tp->t_ourmss >> 8) & 0xff;
			opt[3] = tp->t_ourmss & 0xff;
			optlen = 4;
	 
			if ((tp->t_flags & TF_REQ_SCALE) &&
			    ((flags & TH_ACK) == 0 ||
			    (tp->t_flags & TF_RCVD_SCALE))) {
				*((u_int32_t *) (opt + optlen)) = htonl(
					TCPOPT_NOP << 24 |
					TCPOPT_WINDOW << 16 |
					TCPOLEN_WINDOW << 8 |
					tp->request_r_scale);
				optlen += 4;
			}
		}
 	}
 
 	/*
	 * Send a timestamp and echo-reply if this is a SYN and our side 
	 * wants to use timestamps (TF_REQ_TSTMP is set) or both our side
	 * and our peer have sent timestamps in our SYN's.
 	 */
 	if ((tp->t_flags & (TF_REQ_TSTMP|TF_NOOPT)) == TF_REQ_TSTMP &&
 	     (flags & TH_RST) == 0 &&
 	    ((flags & (TH_SYN|TH_ACK)) == TH_SYN ||
	     (tp->t_flags & TF_RCVD_TSTMP))) {
		u_int32_t *lp = (u_int32_t *)(opt + optlen);
 
 		/* Form timestamp option as shown in appendix A of RFC 1323. */
 		*lp++ = htonl(TCPOPT_TSTAMP_HDR);
 		*lp++ = htonl(TCP_TIMESTAMP(tp));
 		*lp   = htonl(tp->ts_recent);
 		optlen += TCPOLEN_TSTAMP_APPA;
 	}

 	hdrlen += optlen;
 
#ifdef DIAGNOSTIC
	if (len > txsegsize)
		panic("tcp data to be sent is larger than segment");
 	if (max_linkhdr + hdrlen > MCLBYTES)
		panic("tcphdr too big");
#endif

	/*
	 * Grab a header mbuf, attaching a copy of data to
	 * be transmitted, and initialize the header from
	 * the template for sends on this connection.
	 */
	if (len) {
		error = tcp_build_datapkt(tp, so, off, len, hdrlen, &m);
		if (error)
			goto out;
		/*
		 * If we're sending everything we've got, set PUSH.
		 * (This will keep happy those implementations which only
		 * give data to the user when a buffer fills or
		 * a PUSH comes in.)
		 */
		if (off + len == so->so_snd.sb_cc)
			flags |= TH_PUSH;
	} else {
		if (tp->t_flags & TF_ACKNOW)
			tcpstat.tcps_sndacks++;
		else if (flags & (TH_SYN|TH_FIN|TH_RST))
			tcpstat.tcps_sndctrl++;
		else if (SEQ_GT(tp->snd_up, tp->snd_una))
			tcpstat.tcps_sndurg++;
		else
			tcpstat.tcps_sndwinup++;

		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m != NULL && max_linkhdr + hdrlen > MHLEN) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				m = NULL;
			}
		}
		if (m == NULL) {
			error = ENOBUFS;
			goto out;
		}
		m->m_data += max_linkhdr;
		m->m_len = hdrlen;
	}
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	switch (af) {
#ifdef INET
	case AF_INET:
		ip = mtod(m, struct ip *);
#ifdef INET6
		ip6 = NULL;
#endif
		th = (struct tcphdr *)(ip + 1);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ip = NULL;
		ip6 = mtod(m, struct ip6_hdr *);
		th = (struct tcphdr *)(ip6 + 1);
		break;
#endif
	default:	/*pacify gcc*/
		ip = NULL;
#ifdef INET6
		ip6 = NULL;
#endif
		th = NULL;
		break;
	}
	if (tp->t_template == 0)
		panic("tcp_output");
	if (tp->t_template->m_len < iphdrlen)
		panic("tcp_output");
	bcopy(mtod(tp->t_template, caddr_t), mtod(m, caddr_t), iphdrlen);

	/*
	 * If we are doing retransmissions, then snd_nxt will
	 * not reflect the first unsent octet.  For ACK only
	 * packets, we do not want the sequence number of the
	 * retransmitted packet, we want the sequence number
	 * of the next unsent octet.  So, if there is no data
	 * (and no SYN or FIN), use snd_max instead of snd_nxt
	 * when filling in ti_seq.  But if we are in persist
	 * state, snd_max might reflect one byte beyond the
	 * right edge of the window, so use snd_nxt in that
	 * case, since we know we aren't doing a retransmission.
	 * (retransmit and persist are mutually exclusive...)
	 */
	if (len || (flags & (TH_SYN|TH_FIN)) ||
	    TCP_TIMER_ISARMED(tp, TCPT_PERSIST))
		th->th_seq = htonl(tp->snd_nxt);
	else
		th->th_seq = htonl(tp->snd_max);
	th->th_ack = htonl(tp->rcv_nxt);
	if (optlen) {
		bcopy((caddr_t)opt, (caddr_t)(th + 1), optlen);
		th->th_off = (sizeof (struct tcphdr) + optlen) >> 2;
	}
	th->th_flags = flags;
	/*
	 * Calculate receive window.  Don't shrink window,
	 * but avoid silly window syndrome.
	 */
	if (win < (long)(so->so_rcv.sb_hiwat / 4) && win < (long)rxsegsize)
		win = 0;
	if (win > (long)TCP_MAXWIN << tp->rcv_scale)
		win = (long)TCP_MAXWIN << tp->rcv_scale;
	if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
		win = (long)(tp->rcv_adv - tp->rcv_nxt);
	th->th_win = htons((u_int16_t) (win>>tp->rcv_scale));
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		u_int32_t urp = tp->snd_up - tp->snd_nxt;
		if (urp > IP_MAXPACKET)
			urp = IP_MAXPACKET;
		th->th_urp = htons((u_int16_t)urp);
		th->th_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull
		 * the urgent pointer to the left edge of the send window
		 * so that it doesn't drift into the send window on sequence
		 * number wraparound.
		 */
		tp->snd_up = tp->snd_una;		/* drag it along */

	/*
	 * Set ourselves up to be checksummed just before the packet
	 * hits the wire.
	 */
	switch (af) {
#ifdef INET
	case AF_INET:
		m->m_pkthdr.csum_flags = M_CSUM_TCPv4;
		m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
		if (len + optlen) {
			/* Fixup the pseudo-header checksum. */
			/* XXXJRT Not IP Jumbogram safe. */
			th->th_sum = in_cksum_addword(th->th_sum,
			    htons((u_int16_t) (len + optlen)));
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		/*
		 * XXX Actually delaying the checksum is Hard
		 * XXX (well, maybe not for Itojun, but it is
		 * XXX for me), but we can still take advantage
		 * XXX of the cached pseudo-header checksum.
		 */
		/* equals to hdrlen + len */
		m->m_pkthdr.len = sizeof(struct ip6_hdr)
			+ sizeof(struct tcphdr) + optlen + len;
#ifdef notyet
		m->m_pkthdr.csum_flags = M_CSUM_TCPv6;
		m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
#endif
		if (len + optlen) {
			/* Fixup the pseudo-header checksum. */
			/* XXXJRT: Not IPv6 Jumbogram safe. */
			th->th_sum = in_cksum_addword(th->th_sum,
			    htons((u_int16_t) (len + optlen)));
		}
#ifndef notyet
		th->th_sum = in6_cksum(m, 0, sizeof(struct ip6_hdr),
		    sizeof(struct tcphdr) + optlen + len);
#endif
		break;
#endif
	}

	/*
	 * In transmit state, time the transmission and arrange for
	 * the retransmit.  In persist state, just set snd_max.
	 */
	if (tp->t_force == 0 || TCP_TIMER_ISARMED(tp, TCPT_PERSIST) == 0) {
		tcp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment.
		 * There are no states in which we send both a SYN and a FIN,
		 * so we collapse the tests for these flags.
		 */
		if (flags & (TH_SYN|TH_FIN))
			tp->snd_nxt++;
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (tp->t_rtttime == 0) {
				tp->t_rtttime = tcp_now;
				tp->t_rtseq = startseq;
				tcpstat.tcps_segstimed++;
			}
		}

		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if (TCP_TIMER_ISARMED(tp, TCPT_REXMT) == 0 &&
		    tp->snd_nxt != tp->snd_una) {
			TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);
			if (TCP_TIMER_ISARMED(tp, TCPT_PERSIST)) {
				TCP_TIMER_DISARM(tp, TCPT_PERSIST);
				tp->t_rxtshift = 0;
			}
		}
	} else
		if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
			tp->snd_max = tp->snd_nxt + len;

#ifdef TCP_DEBUG
	/*
	 * Trace.
	 */
	if (so->so_options & SO_DEBUG) {
		/*
		 * need to recover version # field, which was overwritten
		 * on ip_cksum computation.
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
		tcp_trace(TA_OUTPUT, tp->t_state, tp, m, 0);
	}
#endif

	/*
	 * Fill in IP length and desired time to live and
	 * send to IP level.  There should be a better way
	 * to handle ttl and tos; we could keep them in
	 * the template, but need a way to checksum without them.
	 */
	m->m_pkthdr.len = hdrlen + len;

	switch (af) {
#ifdef INET
	case AF_INET:
		ip->ip_len = m->m_pkthdr.len;
		if (tp->t_inpcb) {
			ip->ip_ttl = tp->t_inpcb->inp_ip.ip_ttl;
			ip->ip_tos = tp->t_inpcb->inp_ip.ip_tos;
		}
#ifdef INET6
		else if (tp->t_in6pcb) {
			ip->ip_ttl = in6_selecthlim(tp->t_in6pcb, NULL); /*XXX*/
			ip->ip_tos = 0;	/*XXX*/
		}
#endif
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ip6->ip6_nxt = IPPROTO_TCP;
		if (tp->t_in6pcb) {
			/*
			 * we separately set hoplimit for every segment, since
			 * the user might want to change the value via
			 * setsockopt. Also, desired default hop limit might
			 * be changed via Neighbor Discovery.
			 */
			ip6->ip6_hlim = in6_selecthlim(tp->t_in6pcb,
				ro->ro_rt ? ro->ro_rt->rt_ifp : NULL);
		}
		/* ip6->ip6_flow = ??? */
		/* ip6_plen will be filled in ip6_output(). */
		break;
#endif
	}

#ifdef IPSEC
	if (ipsec_setsocket(m, so) != 0) {
		m_freem(m);
		error = ENOBUFS;
		goto out;
	}
#endif /*IPSEC*/

	switch (af) {
#ifdef INET
	case AF_INET:
	    {
		struct mbuf *opts;

		if (tp->t_inpcb)
			opts = tp->t_inpcb->inp_options;
		else
			opts = NULL;
		error = ip_output(m, opts, ro,
			(ip_mtudisc ? IP_MTUDISC : 0) |
			(so->so_options & SO_DONTROUTE),
			0);
		break;
	    }
#endif
#ifdef INET6
	case AF_INET6:
	    {
		struct ip6_pktopts *opts;

		if (tp->t_in6pcb)
			opts = tp->t_in6pcb->in6p_outputopts;
		else
			opts = NULL;
		error = ip6_output(m, opts, (struct route_in6 *)ro,
			so->so_options & SO_DONTROUTE, 0, NULL);
		break;
	    }
#endif
	default:
		error = EAFNOSUPPORT;
		break;
	}
	if (error) {
out:
		if (error == ENOBUFS) {
			tcpstat.tcps_selfquench++;
#ifdef INET
			if (tp->t_inpcb)
				tcp_quench(tp->t_inpcb, 0);
#endif
#ifdef INET6
			if (tp->t_in6pcb)
				tcp6_quench(tp->t_in6pcb, 0);
#endif
			error = 0;
		} else if ((error == EHOSTUNREACH || error == ENETDOWN) &&
		    TCPS_HAVERCVDSYN(tp->t_state)) {
			tp->t_softerror = error;
			error = 0;
		}

		/* Restart the delayed ACK timer, if necessary. */
		if (tp->t_flags & TF_DELACK)
			TCP_RESTART_DELACK(tp);

		return (error);
	}
	tcpstat.tcps_sndtotal++;
	if (tp->t_flags & TF_DELACK)
		tcpstat.tcps_delack++;

	/*
	 * Data sent (as far as we can tell).
	 * If this advertises a larger window than any other segment,
	 * then remember the size of the advertised window.
	 * Any pending ACK has now been sent.
	 */
	if (win > 0 && SEQ_GT(tp->rcv_nxt+win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->last_ack_sent = tp->rcv_nxt;
	tp->t_flags &= ~TF_ACKNOW;
	TCP_CLEAR_DELACK(tp);
#ifdef DIAGNOSTIC
	if (maxburst < 0)
		printf("tcp_output: maxburst exceeded by %d\n", -maxburst);
#endif
	if (sendalot && (!tcp_do_newreno || --maxburst))
		goto again;
	return (0);
}

void
tcp_setpersist(tp)
	struct tcpcb *tp;
{
	int t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> (1 + 2);
	int nticks;

	if (TCP_TIMER_ISARMED(tp, TCPT_REXMT))
		panic("tcp_output REXMT");
	/*
	 * Start/restart persistance timer.
	 */
	if (t < tp->t_rttmin)
		t = tp->t_rttmin;
	TCPT_RANGESET(nticks, t * tcp_backoff[tp->t_rxtshift],
	    TCPTV_PERSMIN, TCPTV_PERSMAX);
	TCP_TIMER_ARM(tp, TCPT_PERSIST, nticks);
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}
