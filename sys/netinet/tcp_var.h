/*	$NetBSD: tcp_var.h,v 1.77.2.5 2002/02/28 04:15:13 nathanw Exp $	*/

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
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1982, 1986, 1993, 1994, 1995
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
 *	@(#)tcp_var.h	8.4 (Berkeley) 5/24/95
 */

#ifndef _NETINET_TCP_VAR_H_
#define _NETINET_TCP_VAR_H_

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

/*
 * Kernel variables for tcp.
 */

#include <sys/callout.h>

/*
 * Tcp control block, one per tcp; fields:
 */
struct tcpcb {
	int	t_family;		/* address family on the wire */
	struct ipqehead segq;		/* sequencing queue */
	struct callout t_timer[TCPT_NTIMERS];/* tcp timers */
	short	t_state;		/* state of this connection */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	uint32_t t_rxtcur;		/* current retransmit value */
	short	t_dupacks;		/* consecutive dup acks recd */
	u_short	t_peermss;		/* peer's maximum segment size */
	u_short	t_ourmss;		/* our's maximum segment size */
	u_short t_segsz;		/* current segment size in use */
	char	t_force;		/* 1 if forcing out a byte */
	u_int	t_flags;
#define	TF_ACKNOW	0x0001		/* ack peer immediately */
#define	TF_DELACK	0x0002		/* ack, but try to delay it */
#define	TF_NODELAY	0x0004		/* don't delay packets to coalesce */
#define	TF_NOOPT	0x0008		/* don't use tcp options */
#define	TF_REQ_SCALE	0x0020		/* have/will request window scaling */
#define	TF_RCVD_SCALE	0x0040		/* other side has requested scaling */
#define	TF_REQ_TSTMP	0x0080		/* have/will request timestamps */
#define	TF_RCVD_TSTMP	0x0100		/* a timestamp was received in SYN */
#define	TF_SACK_PERMIT	0x0200		/* other side said I could SACK */
#define	TF_SYN_REXMT	0x0400		/* rexmit timer fired on SYN */
#define	TF_WILL_SACK	0x0800		/* try to use SACK */
#define	TF_CANT_TXSACK	0x1000		/* other side said I could not SACK */
#define	TF_IGNR_RXSACK	0x2000		/* ignore received SACK blocks */
#define	TF_REASSEMBLING	0x4000		/* we're busy reassembling */


	struct	mbuf *t_template;	/* skeletal packet for transmit */
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
	struct	in6pcb *t_in6pcb;	/* back pointer to internet pcb */
	struct	callout t_delack_ch;	/* delayed ACK callout */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq	snd_una;		/* send unacknowledged */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	snd_up;			/* send urgent pointer */
	tcp_seq	snd_wl1;		/* window update seg seq number */
	tcp_seq	snd_wl2;		/* window update seg ack number */
	tcp_seq	iss;			/* initial send sequence number */
	u_long	snd_wnd;		/* send window */
	tcp_seq snd_recover;		/* for use in fast recovery */
/* receive sequence variables */
	u_long	rcv_wnd;		/* receive window */
	tcp_seq	rcv_nxt;		/* receive next */
	tcp_seq	rcv_up;			/* receive urgent pointer */
	tcp_seq	irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq	rcv_adv;		/* advertised window */
/* retransmit variables */
	tcp_seq	snd_max;		/* highest sequence number sent;
					 * used to recognize retransmits
					 */
/* congestion control (for slow start, source quench, retransmit after loss) */
	u_long	snd_cwnd;		/* congestion-controlled window */
	u_long	snd_ssthresh;		/* snd_cwnd size threshhold for
					 * for slow start exponential to
					 * linear switch
					 */
/*
 * transmit timing stuff.  See below for scale of srtt and rttvar.
 * "Variance" is actually smoothed difference.
 */
	uint32_t t_rcvtime;		/* time last segment received */
	uint32_t t_rtttime;		/* time we started measuring rtt */
	tcp_seq	t_rtseq;		/* sequence number being timed */
	int32_t	t_srtt;			/* smoothed round-trip time */
	int32_t	t_rttvar;		/* variance in round-trip time */
	uint32_t t_rttmin;		/* minimum rtt allowed */
	u_long	max_sndwnd;		/* largest window peer has offered */

/* out-of-band data */
	char	t_oobflags;		/* have some */
	char	t_iobc;			/* input character */
#define	TCPOOB_HAVEDATA	0x01
#define	TCPOOB_HADDATA	0x02
	short	t_softerror;		/* possible error not yet reported */

/* RFC 1323 variables */
	u_char	snd_scale;		/* window scaling for send window */
	u_char	rcv_scale;		/* window scaling for recv window */
	u_char	request_r_scale;	/* pending window scaling */
	u_char	requested_s_scale;
	u_int32_t ts_recent;		/* timestamp echo data */
	u_int32_t ts_recent_age;	/* when last updated */
	u_int32_t ts_timebase;		/* our timebase */
	tcp_seq	last_ack_sent;

/* SACK stuff */
	struct ipqehead timeq;		/* time sequenced queue (for SACK) */

/* pointer for syn cache entries*/
	LIST_HEAD(, syn_cache) t_sc;	/* list of entries by this tcb */
};

#ifdef _KERNEL
/*
 * TCP reassembly queue locks.
 */
static __inline int tcp_reass_lock_try __P((struct tcpcb *))
	__attribute__((__unused__));
static __inline void tcp_reass_unlock __P((struct tcpcb *))
	__attribute__((__unused__));

static __inline int
tcp_reass_lock_try(tp)
	struct tcpcb *tp;
{
	int s;

	/*
	 * Use splvm() -- we're blocking things that would cause
	 * mbuf allocation.
	 */
	s = splvm();
	if (tp->t_flags & TF_REASSEMBLING) {
		splx(s);
		return (0);
	}
	tp->t_flags |= TF_REASSEMBLING;
	splx(s);
	return (1);
}

static __inline void
tcp_reass_unlock(tp)
	struct tcpcb *tp;
{
	int s;

	s = splvm();
	tp->t_flags &= ~TF_REASSEMBLING;
	splx(s);
}

#ifdef DIAGNOSTIC
#define	TCP_REASS_LOCK(tp)						\
do {									\
	if (tcp_reass_lock_try(tp) == 0) {				\
		printf("%s:%d: tcpcb %p reass already locked\n",	\
		    __FILE__, __LINE__, tp);				\
		panic("tcp_reass_lock");				\
	}								\
} while (0)
#define	TCP_REASS_LOCK_CHECK(tp)					\
do {									\
	if (((tp)->t_flags & TF_REASSEMBLING) == 0) {			\
		printf("%s:%d: tcpcb %p reass lock not held\n",		\
		    __FILE__, __LINE__, tp);				\
		panic("tcp reass lock check");				\
	}								\
} while (0)
#else
#define	TCP_REASS_LOCK(tp)	(void) tcp_reass_lock_try((tp))
#define	TCP_REASS_LOCK_CHECK(tp) /* nothing */
#endif

#define	TCP_REASS_UNLOCK(tp)	tcp_reass_unlock((tp))
#endif /* _KERNEL */

/*
 * Queue for delayed ACK processing.
 */
#ifdef _KERNEL
extern int tcp_delack_ticks;
void	tcp_delack(void *);

#define TCP_RESTART_DELACK(tp)						\
	callout_reset(&(tp)->t_delack_ch, tcp_delack_ticks,		\
	    tcp_delack, tp)

#define	TCP_SET_DELACK(tp)						\
do {									\
	if (((tp)->t_flags & TF_DELACK) == 0) {				\
		(tp)->t_flags |= TF_DELACK;				\
		TCP_RESTART_DELACK(tp);					\
	}								\
} while (/*CONSTCOND*/0)

#define	TCP_CLEAR_DELACK(tp)						\
do {									\
	if ((tp)->t_flags & TF_DELACK) {				\
		(tp)->t_flags &= ~TF_DELACK;				\
		callout_stop(&(tp)->t_delack_ch);			\
	}								\
} while (/*CONSTCOND*/0)
#endif /* _KERNEL */

/*
 * Compute the current timestamp for a connection.
 */
#define	TCP_TIMESTAMP(tp)	(tcp_now - (tp)->ts_timebase)

/*
 * Handy way of passing around TCP option info.
 */
struct tcp_opt_info {
	int		ts_present;
	u_int32_t	ts_val;
	u_int32_t	ts_ecr;
	u_int16_t	maxseg;
};

/*
 * Data for the TCP compressed state engine.
 */
union syn_cache_sa {
	struct sockaddr sa;
	struct sockaddr_in sin;
#if 1 /*def INET6*/
	struct sockaddr_in6 sin6;
#endif
};

struct syn_cache {
	TAILQ_ENTRY(syn_cache) sc_bucketq;	/* link on bucket list */
	struct callout sc_timer;		/* rexmt timer */
	union {					/* cached route */
		struct route route4;
#ifdef INET6
		struct route_in6 route6;
#endif
	} sc_route_u;
#define sc_route4	sc_route_u.route4
#ifdef INET6
#define sc_route6	sc_route_u.route6
#endif
	long sc_win;				/* advertised window */
	int sc_bucketidx;			/* our bucket index */
	u_int32_t sc_hash;
	u_int32_t sc_timestamp;			/* timestamp from SYN */
	u_int32_t sc_timebase;			/* our local timebase */
	union syn_cache_sa sc_src;
	union syn_cache_sa sc_dst;
	tcp_seq sc_irs;
	tcp_seq sc_iss;
	u_int sc_rxtcur;			/* current rxt timeout */
	u_int sc_rxttot;			/* total time spend on queues */
	u_short sc_rxtshift;			/* for computing backoff */
	u_short sc_flags;

#define	SCF_UNREACH		0x0001		/* we've had an unreach error */
#define	SCF_TIMESTAMP		0x0002		/* peer will do timestamps */

	struct mbuf *sc_ipopts;			/* IP options */
	u_int16_t sc_peermaxseg;
	u_int16_t sc_ourmaxseg;
	u_int8_t sc_request_r_scale	: 4,
		 sc_requested_s_scale	: 4;

	struct tcpcb *sc_tp;			/* tcb for listening socket */
	LIST_ENTRY(syn_cache) sc_tpq;		/* list of entries by same tp */
};

struct syn_cache_head {
	TAILQ_HEAD(, syn_cache) sch_bucket;	/* bucket entries */
	u_short sch_length;			/* # entries in bucket */
};

#define	intotcpcb(ip)	((struct tcpcb *)(ip)->inp_ppcb)
#ifdef INET6
#define	in6totcpcb(ip)	((struct tcpcb *)(ip)->in6p_ppcb)
#endif
#ifndef INET6
#define	sototcpcb(so)	(intotcpcb(sotoinpcb(so)))
#else
#define	sototcpcb(so)	(((so)->so_proto->pr_domain->dom_family == AF_INET) \
				? intotcpcb(sotoinpcb(so)) \
				: in6totcpcb(sotoin6pcb(so)))
#endif

/*
 * The smoothed round-trip time and estimated variance
 * are stored as fixed point numbers scaled by the values below.
 * For convenience, these scales are also used in smoothing the average
 * (smoothed = (1/scale)sample + ((scale-1)/scale)smoothed).
 * With these scales, srtt has 3 bits to the right of the binary point,
 * and thus an "ALPHA" of 0.875.  rttvar has 2 bits to the right of the
 * binary point, and is smoothed with an ALPHA of 0.75.
 */
#define	TCP_RTT_SHIFT		3	/* shift for srtt; 3 bits frac. */
#define	TCP_RTTVAR_SHIFT	2	/* multiplier for rttvar; 2 bits */

/*
 * The initial retransmission should happen at rtt + 4 * rttvar.
 * Because of the way we do the smoothing, srtt and rttvar
 * will each average +1/2 tick of bias.  When we compute
 * the retransmit timer, we want 1/2 tick of rounding and
 * 1 extra tick because of +-1/2 tick uncertainty in the
 * firing of the timer.  The bias will give us exactly the
 * 1.5 tick we need.  But, because the bias is
 * statistical, we have to test that we don't drop below
 * the minimum feasible timer (which is 2 ticks).
 * This macro assumes that the value of 1<<TCP_RTTVAR_SHIFT
 * is the same as the multiplier for rttvar.
 */
#define	TCP_REXMTVAL(tp) \
	((((tp)->t_srtt >> TCP_RTT_SHIFT) + (tp)->t_rttvar) >> 2)

/*
 * Compute the initial window for slow start.
 */
#define	TCP_INITIAL_WINDOW(iw, segsz) \
	(((iw) == 0) ? (min(4 * (segsz), max(2 * (segsz), 4380))) : \
	 ((segsz) * (iw)))

/*
 * TCP statistics.
 * Many of these should be kept per connection,
 * but that's inconvenient at the moment.
 */
struct	tcpstat {
	u_quad_t tcps_connattempt;	/* connections initiated */
	u_quad_t tcps_accepts;		/* connections accepted */
	u_quad_t tcps_connects;		/* connections established */
	u_quad_t tcps_drops;		/* connections dropped */
	u_quad_t tcps_conndrops;	/* embryonic connections dropped */
	u_quad_t tcps_closed;		/* conn. closed (includes drops) */
	u_quad_t tcps_segstimed;	/* segs where we tried to get rtt */
	u_quad_t tcps_rttupdated;	/* times we succeeded */
	u_quad_t tcps_delack;		/* delayed acks sent */
	u_quad_t tcps_timeoutdrop;	/* conn. dropped in rxmt timeout */
	u_quad_t tcps_rexmttimeo;	/* retransmit timeouts */
	u_quad_t tcps_persisttimeo;	/* persist timeouts */
	u_quad_t tcps_keeptimeo;	/* keepalive timeouts */
	u_quad_t tcps_keepprobe;	/* keepalive probes sent */
	u_quad_t tcps_keepdrops;	/* connections dropped in keepalive */
	u_quad_t tcps_persistdrops;	/* connections dropped in persist */
	u_quad_t tcps_connsdrained;	/* connections drained due to memory
					   shortage */

	u_quad_t tcps_sndtotal;		/* total packets sent */
	u_quad_t tcps_sndpack;		/* data packets sent */
	u_quad_t tcps_sndbyte;		/* data bytes sent */
	u_quad_t tcps_sndrexmitpack;	/* data packets retransmitted */
	u_quad_t tcps_sndrexmitbyte;	/* data bytes retransmitted */
	u_quad_t tcps_sndacks;		/* ack-only packets sent */
	u_quad_t tcps_sndprobe;		/* window probes sent */
	u_quad_t tcps_sndurg;		/* packets sent with URG only */
	u_quad_t tcps_sndwinup;		/* window update-only packets sent */
	u_quad_t tcps_sndctrl;		/* control (SYN|FIN|RST) packets sent */

	u_quad_t tcps_rcvtotal;		/* total packets received */
	u_quad_t tcps_rcvpack;		/* packets received in sequence */
	u_quad_t tcps_rcvbyte;		/* bytes received in sequence */
	u_quad_t tcps_rcvbadsum;	/* packets received with ccksum errs */
	u_quad_t tcps_rcvbadoff;	/* packets received with bad offset */
	u_quad_t tcps_rcvmemdrop;	/* packets dropped for lack of memory */
	u_quad_t tcps_rcvshort;		/* packets received too short */
	u_quad_t tcps_rcvduppack;	/* duplicate-only packets received */
	u_quad_t tcps_rcvdupbyte;	/* duplicate-only bytes received */
	u_quad_t tcps_rcvpartduppack;	/* packets with some duplicate data */
	u_quad_t tcps_rcvpartdupbyte;	/* dup. bytes in part-dup. packets */
	u_quad_t tcps_rcvoopack;	/* out-of-order packets received */
	u_quad_t tcps_rcvoobyte;	/* out-of-order bytes received */
	u_quad_t tcps_rcvpackafterwin;	/* packets with data after window */
	u_quad_t tcps_rcvbyteafterwin;	/* bytes rcvd after window */
	u_quad_t tcps_rcvafterclose;	/* packets rcvd after "close" */
	u_quad_t tcps_rcvwinprobe;	/* rcvd window probe packets */
	u_quad_t tcps_rcvdupack;	/* rcvd duplicate acks */
	u_quad_t tcps_rcvacktoomuch;	/* rcvd acks for unsent data */
	u_quad_t tcps_rcvackpack;	/* rcvd ack packets */
	u_quad_t tcps_rcvackbyte;	/* bytes acked by rcvd acks */
	u_quad_t tcps_rcvwinupd;	/* rcvd window update packets */
	u_quad_t tcps_pawsdrop;		/* segments dropped due to PAWS */
	u_quad_t tcps_predack;		/* times hdr predict ok for acks */
	u_quad_t tcps_preddat;		/* times hdr predict ok for data pkts */

	u_quad_t tcps_pcbhashmiss;	/* input packets missing pcb hash */
	u_quad_t tcps_noport;		/* no socket on port */
	u_quad_t tcps_badsyn;		/* received ack for which we have
					   no SYN in compressed state */

	/* These statistics deal with the SYN cache. */
	u_quad_t tcps_sc_added;		/* # of entries added */
	u_quad_t tcps_sc_completed;	/* # of connections completed */
	u_quad_t tcps_sc_timed_out;	/* # of entries timed out */
	u_quad_t tcps_sc_overflowed;	/* # dropped due to overflow */
	u_quad_t tcps_sc_reset;		/* # dropped due to RST */
	u_quad_t tcps_sc_unreach;	/* # dropped due to ICMP unreach */
	u_quad_t tcps_sc_bucketoverflow;/* # dropped due to bucket overflow */
	u_quad_t tcps_sc_aborted;	/* # of entries aborted (no mem) */
	u_quad_t tcps_sc_dupesyn;	/* # of duplicate SYNs received */
	u_quad_t tcps_sc_dropped;	/* # of SYNs dropped (no route/mem) */
	u_quad_t tcps_sc_collisions;	/* # of hash collisions */
	u_quad_t tcps_sc_retransmitted;	/* # of retransmissions */

	u_quad_t tcps_selfquench;	/* # of ENOBUFS we get on output */
};

/*
 * Names for TCP sysctl objects.
 */
#define	TCPCTL_RFC1323		1	/* RFC1323 timestamps/scaling */
#define	TCPCTL_SENDSPACE	2	/* default send buffer */
#define	TCPCTL_RECVSPACE	3	/* default recv buffer */
#define	TCPCTL_MSSDFLT		4	/* default seg size */
#define	TCPCTL_SYN_CACHE_LIMIT	5	/* max size of comp. state engine */
#define	TCPCTL_SYN_BUCKET_LIMIT	6	/* max size of hash bucket */
#if 0	/*obsoleted*/
#define	TCPCTL_SYN_CACHE_INTER	7	/* interval of comp. state timer */
#endif
#define	TCPCTL_INIT_WIN		8	/* initial window */
#define	TCPCTL_MSS_IFMTU	9	/* mss from interface, not in_maxmtu */
#define	TCPCTL_SACK		10	/* RFC2018 selective acknowledgement */
#define	TCPCTL_WSCALE		11	/* RFC1323 window scaling */
#define	TCPCTL_TSTAMP		12	/* RFC1323 timestamps */
#define	TCPCTL_COMPAT_42	13	/* 4.2BSD TCP bug work-arounds */
#define	TCPCTL_CWM		14	/* Congestion Window Monitoring */
#define	TCPCTL_CWM_BURSTSIZE	15	/* burst size allowed by CWM */
#define	TCPCTL_ACK_ON_PUSH	16	/* ACK immediately on PUSH */
#define	TCPCTL_KEEPIDLE		17	/* keepalive idle time */
#define	TCPCTL_KEEPINTVL	18	/* keepalive probe interval */
#define	TCPCTL_KEEPCNT		19	/* keepalive count */
#define	TCPCTL_SLOWHZ		20	/* PR_SLOWHZ (read-only) */
#define	TCPCTL_NEWRENO		21	/* NewReno Congestion Control */
#define TCPCTL_LOG_REFUSED	22	/* Log refused connections */
#if 0	/*obsoleted*/
#define	TCPCTL_RSTRATELIMIT	23	/* RST rate limit */
#endif
#define	TCPCTL_RSTPPSLIMIT	24	/* RST pps limit */
#define	TCPCTL_DELACK_TICKS	25	/* # ticks to delay ACK */
#define	TCPCTL_MAXID		26

#define	TCPCTL_NAMES { \
	{ 0, 0 }, \
	{ "rfc1323",	CTLTYPE_INT }, \
	{ "sendspace",	CTLTYPE_INT }, \
	{ "recvspace",	CTLTYPE_INT }, \
	{ "mssdflt",	CTLTYPE_INT }, \
	{ "syn_cache_limit", CTLTYPE_INT }, \
	{ "syn_bucket_limit", CTLTYPE_INT }, \
	{ 0, 0 },\
	{ "init_win", CTLTYPE_INT }, \
	{ "mss_ifmtu", CTLTYPE_INT }, \
	{ "sack", CTLTYPE_INT }, \
	{ "win_scale", CTLTYPE_INT }, \
	{ "timestamps", CTLTYPE_INT }, \
	{ "compat_42", CTLTYPE_INT }, \
	{ "cwm", CTLTYPE_INT }, \
	{ "cwm_burstsize", CTLTYPE_INT }, \
	{ "ack_on_push", CTLTYPE_INT }, \
	{ "keepidle",	CTLTYPE_INT }, \
	{ "keepintvl",	CTLTYPE_INT }, \
	{ "keepcnt",	CTLTYPE_INT }, \
	{ "slowhz",	CTLTYPE_INT }, \
	{ "newreno",	CTLTYPE_INT }, \
	{ "log_refused",CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ "rstppslimit", CTLTYPE_INT }, \
	{ "delack_ticks", CTLTYPE_INT }, \
}

#ifdef _KERNEL
struct	inpcbtable tcbtable;	/* head of queue of active tcpcb's */
#ifdef INET6
extern struct in6pcb tcb6;
#endif
struct	tcpstat tcpstat;	/* tcp statistics */
u_int32_t tcp_now;		/* for RFC 1323 timestamps */
extern	int tcp_do_rfc1323;	/* enabled/disabled? */
extern	int tcp_do_sack;	/* SACK enabled/disabled? */
extern	int tcp_do_win_scale;	/* RFC1323 window scaling enabled/disabled? */
extern	int tcp_do_timestamps;	/* RFC1323 timestamps enabled/disabled? */
extern	int tcp_do_newreno;	/* Use the New Reno algorithms */
extern	int tcp_mssdflt;	/* default seg size */
extern	int tcp_init_win;	/* initial window */
extern	int tcp_mss_ifmtu;	/* take MSS from interface, not in_maxmtu */
extern	int tcp_compat_42;	/* work around ancient broken TCP peers */
extern	int tcp_cwm;		/* enable Congestion Window Monitoring */
extern	int tcp_cwm_burstsize;	/* burst size allowed by CWM */
extern	int tcp_ack_on_push;	/* ACK immediately on PUSH */
extern	int tcp_syn_cache_limit; /* max entries for compressed state engine */
extern	int tcp_syn_bucket_limit;/* max entries per hash bucket */
extern	int tcp_log_refused;	/* log refused connections */

extern	int tcp_rst_ppslim;

extern	int tcp_syn_cache_size;
extern	struct syn_cache_head tcp_syn_cache[];
extern	u_long syn_cache_count;

#define	TCPCTL_VARIABLES { \
	{ 0 },					\
	{ 1, 0, &tcp_do_rfc1323 },		\
	{ 1, 0, &tcp_sendspace },		\
	{ 1, 0, &tcp_recvspace },		\
	{ 1, 0, &tcp_mssdflt },			\
	{ 1, 0, &tcp_syn_cache_limit },		\
	{ 1, 0, &tcp_syn_bucket_limit },	\
	{ 0 },					\
	{ 1, 0, &tcp_init_win },		\
	{ 1, 0, &tcp_mss_ifmtu },		\
	{ 1, 0, &tcp_do_sack },			\
	{ 1, 0, &tcp_do_win_scale },		\
	{ 1, 0, &tcp_do_timestamps },		\
	{ 1, 0, &tcp_compat_42 },		\
	{ 1, 0, &tcp_cwm },			\
	{ 1, 0, &tcp_cwm_burstsize },		\
	{ 1, 0, &tcp_ack_on_push },		\
	{ 1, 0, &tcp_keepidle },		\
	{ 1, 0, &tcp_keepintvl },		\
	{ 1, 0, &tcp_keepcnt },			\
	{ 1, 1, 0, PR_SLOWHZ },			\
	{ 1, 0, &tcp_do_newreno },		\
	{ 1, 0, &tcp_log_refused },		\
	{ 0 },					\
	{ 1, 0, &tcp_rst_ppslim },		\
	{ 1, 0, &tcp_delack_ticks },		\
}

int	 tcp_attach __P((struct socket *));
void	 tcp_canceltimers __P((struct tcpcb *));
struct tcpcb *
	 tcp_close __P((struct tcpcb *));
#ifdef INET6
void	 tcp6_ctlinput __P((int, struct sockaddr *, void *));
#endif
void	 *tcp_ctlinput __P((int, struct sockaddr *, void *));
int	 tcp_ctloutput __P((int, struct socket *, int, int, struct mbuf **));
struct tcpcb *
	 tcp_disconnect __P((struct tcpcb *));
struct tcpcb *
	 tcp_drop __P((struct tcpcb *, int));
void	 tcp_dooptions __P((struct tcpcb *,
	    u_char *, int, struct tcphdr *, struct tcp_opt_info *));
void	 tcp_drain __P((void));
void	 tcp_established __P((struct tcpcb *));
void	 tcp_init __P((void));
#ifdef INET6
int	 tcp6_input __P((struct mbuf **, int *, int));
#endif
void	 tcp_input __P((struct mbuf *, ...));
u_long	 tcp_mss_to_advertise __P((const struct ifnet *, int));
void	 tcp_mss_from_peer __P((struct tcpcb *, int));
struct tcpcb *
	 tcp_newtcpcb __P((int, void *));
void	 tcp_notify __P((struct inpcb *, int));
#ifdef INET6
void	 tcp6_notify __P((struct in6pcb *, int));
#endif
u_int	 tcp_optlen __P((struct tcpcb *));
int	 tcp_output __P((struct tcpcb *));
void	 tcp_pulloutofband __P((struct socket *,
	    struct tcphdr *, struct mbuf *, int));
void	 tcp_quench __P((struct inpcb *, int));
#ifdef INET6
void	 tcp6_quench __P((struct in6pcb *, int));
#endif
int	 tcp_reass __P((struct tcpcb *, struct tcphdr *, struct mbuf *, int *));
int	 tcp_respond __P((struct tcpcb *, struct mbuf *, struct mbuf *,
	    struct tcphdr *, tcp_seq, tcp_seq, int));
void	 tcp_rmx_rtt __P((struct tcpcb *));
void	 tcp_setpersist __P((struct tcpcb *));
void	 tcp_slowtimo __P((void));
struct mbuf *
	 tcp_template __P((struct tcpcb *));
void	 tcp_trace __P((int, int, struct tcpcb *, struct mbuf *, int));
struct tcpcb *
	 tcp_usrclosed __P((struct tcpcb *));
int	 tcp_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
int	 tcp_usrreq __P((struct socket *,
	    int, struct mbuf *, struct mbuf *, struct mbuf *, struct proc *));
void	 tcp_xmit_timer __P((struct tcpcb *, uint32_t));
tcp_seq	 tcp_new_iss __P((struct tcpcb *, tcp_seq));
tcp_seq  tcp_new_iss1 __P((void *, void *, u_int16_t, u_int16_t, size_t,
	    tcp_seq));

int	 syn_cache_add __P((struct sockaddr *, struct sockaddr *,
		struct tcphdr *, unsigned int, struct socket *,
		struct mbuf *, u_char *, int, struct tcp_opt_info *));
void	 syn_cache_unreach __P((struct sockaddr *, struct sockaddr *,
	   struct tcphdr *));
struct socket *syn_cache_get __P((struct sockaddr *, struct sockaddr *,
		struct tcphdr *, unsigned int, unsigned int,
		struct socket *so, struct mbuf *));
void	 syn_cache_init __P((void));
void	 syn_cache_insert __P((struct syn_cache *, struct tcpcb *));
struct syn_cache *syn_cache_lookup __P((struct sockaddr *, struct sockaddr *,
		struct syn_cache_head **));
void	 syn_cache_reset __P((struct sockaddr *, struct sockaddr *,
		struct tcphdr *));
int	 syn_cache_respond __P((struct syn_cache *, struct mbuf *));
void	 syn_cache_timer __P((void *));
void	 syn_cache_cleanup __P((struct tcpcb *));

int	tcp_newreno __P((struct tcpcb *, struct tcphdr *));
#endif

#endif /* _NETINET_TCP_VAR_H_ */
