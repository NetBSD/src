/*	$NetBSD: tcp_var.h,v 1.151 2007/12/25 18:33:47 perry Exp $	*/

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
 * Copyright (c) 1997, 1998, 1999, 2001, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	@(#)tcp_var.h	8.4 (Berkeley) 5/24/95
 */

#ifndef _NETINET_TCP_VAR_H_
#define _NETINET_TCP_VAR_H_

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_mbuftrace.h"
#include "rnd.h"
#endif

/*
 * Kernel variables for tcp.
 */

#include <sys/callout.h>

#ifdef TCP_SIGNATURE
/*
 * Defines which are needed by the xform_tcp module and tcp_[in|out]put
 * for SADB verification and lookup.
 */
#define	TCP_SIGLEN	16	/* length of computed digest in bytes */
#define	TCP_KEYLEN_MIN	1	/* minimum length of TCP-MD5 key */
#define	TCP_KEYLEN_MAX	80	/* maximum length of TCP-MD5 key */
/*
 * Only a single SA per host may be specified at this time. An SPI is
 * needed in order for the KEY_ALLOCSA() lookup to work.
 */
#define	TCP_SIG_SPI	0x1000
#endif /* TCP_SIGNATURE */

/*
 * SACK option block.
 */
struct sackblk {
	tcp_seq left;		/* Left edge of sack block. */
	tcp_seq right;		/* Right edge of sack block. */
};

TAILQ_HEAD(sackhead, sackhole);
struct sackhole {
	tcp_seq start;
	tcp_seq end;
	tcp_seq rxmit;

	TAILQ_ENTRY(sackhole) sackhole_q;
};

/*
 * Tcp control block, one per tcp; fields:
 */
struct tcpcb {
	int	t_family;		/* address family on the wire */
	struct ipqehead segq;		/* sequencing queue */
	int	t_segqlen;		/* length of the above */
	callout_t t_timer[TCPT_NTIMERS];/* tcp timers */
	short	t_state;		/* state of this connection */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	uint32_t t_rxtcur;		/* current retransmit value */
	short	t_dupacks;		/* consecutive dup acks recd */
	short	t_partialacks;		/* partials acks during fast rexmit */
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
#define	TF_REASSEMBLING	0x1000		/* we're busy reassembling */
#define	TF_DEAD		0x2000		/* dead and to-be-released */
#define	TF_PMTUD_PEND	0x4000		/* Path MTU Discovery pending */
#define	TF_ECN_PERMIT	0x10000		/* other side said is ECN-ready */
#define	TF_ECN_SND_CWR	0x20000		/* ECN CWR in queue */
#define	TF_ECN_SND_ECE	0x40000		/* ECN ECE in queue */
#define	TF_SIGNATURE	0x400000	/* require MD5 digests (RFC2385) */


	struct	mbuf *t_template;	/* skeletal packet for transmit */
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
	struct	in6pcb *t_in6pcb;	/* back pointer to internet pcb */
	callout_t t_delack_ch;		/* delayed ACK callout */
/*
 * The following fields are used as in the protocol specification.
 * See RFC793, Dec. 1981, page 21.
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
	tcp_seq	snd_high;		/* NewReno false fast rexmit seq */
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
/* auto-sizing variables */
	u_int rfbuf_cnt;		/* recv buffer autoscaling byte count */
	uint32_t rfbuf_ts;		/* recv buffer autoscaling timestamp */

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

/* RFC 3465 variables */
	u_long	t_bytes_acked;		/* ABC "bytes_acked" parameter */

/* SACK stuff */
#define TCP_SACK_MAX 3
#define TCPSACK_NONE 0
#define TCPSACK_HAVED 1
	u_char rcv_sack_flags;		/* SACK flags. */
	struct sackblk rcv_dsack_block;	/* RX D-SACK block. */
	struct ipqehead timeq;		/* time sequenced queue. */
	struct sackhead snd_holes;	/* TX SACK holes. */
	int	snd_numholes;		/* Number of TX SACK holes. */
	tcp_seq rcv_lastsack;		/* last seq number(+1) sack'd by rcv'r*/
	tcp_seq sack_newdata;		/* New data xmitted in this recovery
					   episode starts at this seq number*/
	tcp_seq snd_fack;		/* FACK TCP.  Forward-most data held by
					   peer. */

/* pointer for syn cache entries*/
	LIST_HEAD(, syn_cache) t_sc;	/* list of entries by this tcb */

/* prediction of next mbuf when using large window sizes */
	struct	mbuf *t_lastm;		/* last mbuf that data was sent from */
	int	t_inoff;		/* data offset in previous mbuf */
	int	t_lastoff;		/* last data address in mbuf chain */
	int	t_lastlen;		/* last length read from mbuf chain */

/* Path-MTU discovery blackhole detection */
	int t_mtudisc;			/* perform mtudisc for this tcb */
/* Path-MTU Discovery Information */
	u_int	t_pmtud_mss_acked;	/* MSS acked, lower bound for MTU */
	u_int	t_pmtud_mtu_sent;	/* MTU used, upper bound for MTU */
	tcp_seq	t_pmtud_th_seq;		/* TCP SEQ from ICMP payload */
	u_int	t_pmtud_nextmtu;	/* Advertised Next-Hop MTU from ICMP */
	u_short	t_pmtud_ip_len;		/* IP length from ICMP payload */
	u_short	t_pmtud_ip_hl;		/* IP header length from ICMP payload */

	uint8_t t_ecn_retries;		/* # of ECN setup retries */
	
	struct tcp_congctl *t_congctl;	/* per TCB congctl algorithm */

	/* Keepalive per socket */
	u_int	t_keepinit;
	u_int	t_keepidle;
	u_int	t_keepintvl;
	u_int	t_keepcnt;
	u_int	t_maxidle;		/* t_keepcnt * t_keepintvl */

};

/*
 * Macros to aid ECN TCP.
 */
#define TCP_ECN_ALLOWED(tp)	(tp->t_flags & TF_ECN_PERMIT)

/*
 * Macros to aid SACK/FACK TCP.
 */
#define TCP_SACK_ENABLED(tp)	(tp->t_flags & TF_WILL_SACK)
#define TCP_FACK_FASTRECOV(tp)	\
	(TCP_SACK_ENABLED(tp) && \
	(SEQ_GT(tp->snd_fack, tp->snd_una + tcprexmtthresh * tp->t_segsz)))

#ifdef _KERNEL
/*
 * TCP reassembly queue locks.
 */
static __inline int tcp_reass_lock_try (struct tcpcb *)
	__unused;
static __inline void tcp_reass_unlock (struct tcpcb *)
	__unused;

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
} while (/*CONSTCOND*/ 0)
#define	TCP_REASS_LOCK_CHECK(tp)					\
do {									\
	if (((tp)->t_flags & TF_REASSEMBLING) == 0) {			\
		printf("%s:%d: tcpcb %p reass lock not held\n",		\
		    __FILE__, __LINE__, tp);				\
		panic("tcp reass lock check");				\
	}								\
} while (/*CONSTCOND*/ 0)
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

#define	TOF_SIGNATURE	0x0040		/* signature option present */
#define	TOF_SIGLEN	0x0080		/* sigature length valid (RFC2385) */

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
	callout_t sc_timer;			/* rexmt timer */
	struct route sc_route;
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
#define	SCF_DEAD		0x0004		/* this entry to be released */
#define SCF_SACK_PERMIT		0x0008		/* peer will do SACK */
#define SCF_ECN_PERMIT		0x0010		/* peer will do ECN */
#define SCF_SIGNATURE	0x40			/* send MD5 digests */

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
	u_quad_t tcps_pmtublackhole;	/* PMTUD blackhole detected */

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
	u_quad_t tcps_delayed_free;	/* delayed pool_put() of tcpcb */

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
	u_quad_t tcps_sc_delayed_free;	/* # of delayed pool_put()s */

	u_quad_t tcps_selfquench;	/* # of ENOBUFS we get on output */
	u_quad_t tcps_badsig;		/* # of drops due to bad signature */
	u_quad_t tcps_goodsig;		/* # of packets with good signature */

	u_quad_t tcps_ecn_shs;		/* # of sucessful ECN handshakes */
	u_quad_t tcps_ecn_ce;		/* # of packets with CE bit */
	u_quad_t tcps_ecn_ect;		/* # of packets with ECT(0) bit */
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
#define	TCPCTL_INIT_WIN_LOCAL	26	/* initial window for local nets */
#define	TCPCTL_IDENT		27	/* rfc 931 identd */
#define	TCPCTL_ACKDROPRATELIMIT	28	/* SYN/RST -> ACK rate limit */
#define	TCPCTL_LOOPBACKCKSUM	29	/* do TCP checksum on loopback */
#define	TCPCTL_STATS		30	/* TCP statistics */
#define	TCPCTL_DEBUG		31	/* TCP debug sockets */
#define	TCPCTL_DEBX		32	/* # of tcp debug sockets */
#define	TCPCTL_DROP		33	/* drop tcp connection */
#define	TCPCTL_MAXID		34

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
	{ 0, 0 }, \
	{ "log_refused",CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ "rstppslimit", CTLTYPE_INT }, \
	{ "delack_ticks", CTLTYPE_INT }, \
	{ "init_win_local", CTLTYPE_INT }, \
	{ "ident", CTLTYPE_STRUCT }, \
	{ "ackdropppslimit", CTLTYPE_INT }, \
	{ "do_loopback_cksum", CTLTYPE_INT }, \
	{ "stats", CTLTYPE_STRUCT }, \
	{ "debug", CTLTYPE_STRUCT }, \
	{ "debx", CTLTYPE_INT }, \
	{ "drop", CTLTYPE_STRUCT }, \
}

#ifdef _KERNEL
extern	struct inpcbtable tcbtable;	/* head of queue of active tcpcb's */
extern	struct tcpstat tcpstat;	/* tcp statistics */
extern	u_int32_t tcp_now;	/* for RFC 1323 timestamps */
extern	int tcp_do_rfc1323;	/* enabled/disabled? */
extern	int tcp_do_sack;	/* SACK enabled/disabled? */
extern	int tcp_do_win_scale;	/* RFC1323 window scaling enabled/disabled? */
extern	int tcp_do_timestamps;	/* RFC1323 timestamps enabled/disabled? */
extern	int tcp_mssdflt;	/* default seg size */
extern	int tcp_minmss;		/* minimal seg size */
extern	int tcp_init_win;	/* initial window */
extern	int tcp_init_win_local;	/* initial window for local nets */
extern	int tcp_mss_ifmtu;	/* take MSS from interface, not in_maxmtu */
extern	int tcp_compat_42;	/* work around ancient broken TCP peers */
extern	int tcp_cwm;		/* enable Congestion Window Monitoring */
extern	int tcp_cwm_burstsize;	/* burst size allowed by CWM */
extern	int tcp_ack_on_push;	/* ACK immediately on PUSH */
extern	int tcp_syn_cache_limit; /* max entries for compressed state engine */
extern	int tcp_syn_bucket_limit;/* max entries per hash bucket */
extern	int tcp_log_refused;	/* log refused connections */
extern	int tcp_do_ecn;		/* TCP ECN enabled/disabled? */
extern	int tcp_ecn_maxretries;	/* Max ECN setup retries */
#if NRND > 0
extern	int tcp_do_rfc1948;	/* ISS by cryptographic hash */
#endif
extern int tcp_sack_tp_maxholes;	/* Max holes per connection. */
extern int tcp_sack_globalmaxholes;	/* Max holes per system. */
extern int tcp_sack_globalholes;	/* Number of holes present. */
extern int tcp_do_abc;			/* RFC3465 ABC enabled/disabled? */
extern int tcp_abc_aggressive;		/* 1: L=2*SMSS  0: L=1*SMSS */

extern	int tcp_rst_ppslim;
extern	int tcp_ackdrop_ppslim;

extern	int tcp_syn_cache_size;
extern	struct syn_cache_head tcp_syn_cache[];
extern	u_long syn_cache_count;

#ifdef MBUFTRACE
extern	struct mowner tcp_rx_mowner;
extern	struct mowner tcp_tx_mowner;
extern	struct mowner tcp_reass_mowner;
extern	struct mowner tcp_sock_mowner;
extern	struct mowner tcp_sock_rx_mowner;
extern	struct mowner tcp_sock_tx_mowner;
extern	struct mowner tcp_mowner;
#endif

extern int tcp_do_autorcvbuf;
extern int tcp_autorcvbuf_inc;
extern int tcp_autorcvbuf_max;
extern int tcp_do_autosndbuf;
extern int tcp_autosndbuf_inc;
extern int tcp_autosndbuf_max;


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
	{ 0 },					\
	{ 1, 0, &tcp_log_refused },		\
	{ 0 },					\
	{ 1, 0, &tcp_rst_ppslim },		\
	{ 1, 0, &tcp_delack_ticks },		\
	{ 1, 0, &tcp_init_win_local },		\
	{ 1, 0, &tcp_ackdrop_ppslim },		\
}

#ifdef __NO_STRICT_ALIGNMENT
#define	TCP_HDR_ALIGNED_P(th)	1
#else
#define	TCP_HDR_ALIGNED_P(th)	((((vaddr_t)(th)) & 3) == 0)
#endif

struct secasvar;

int	 tcp_attach(struct socket *);
void	 tcp_canceltimers(struct tcpcb *);
int	 tcp_timers_invoking(struct tcpcb*);
struct tcpcb *
	 tcp_close(struct tcpcb *);
int	 tcp_isdead(struct tcpcb *);
#ifdef INET6
void	 tcp6_ctlinput(int, const struct sockaddr *, void *);
#endif
void	 *tcp_ctlinput(int, const struct sockaddr *, void *);
int	 tcp_ctloutput(int, struct socket *, int, int, struct mbuf **);
struct tcpcb *
	 tcp_disconnect(struct tcpcb *);
struct tcpcb *
	 tcp_drop(struct tcpcb *, int);
#ifdef TCP_SIGNATURE
int	 tcp_signature_apply(void *, void *, u_int);
struct secasvar *tcp_signature_getsav(struct mbuf *, struct tcphdr *);
int	 tcp_signature(struct mbuf *, struct tcphdr *, int, struct secasvar *,
	    char *);
#endif
void	 tcp_drain(void);
void	 tcp_established(struct tcpcb *);
void	 tcp_init(void);
#ifdef INET6
int	 tcp6_input(struct mbuf **, int *, int);
#endif
void	 tcp_input(struct mbuf *, ...);
u_int	 tcp_hdrsz(struct tcpcb *);
u_long	 tcp_mss_to_advertise(const struct ifnet *, int);
void	 tcp_mss_from_peer(struct tcpcb *, int);
void	 tcp_tcpcb_template(void);
struct tcpcb *
	 tcp_newtcpcb(int, void *);
void	 tcp_notify(struct inpcb *, int);
#ifdef INET6
void	 tcp6_notify(struct in6pcb *, int);
#endif
u_int	 tcp_optlen(struct tcpcb *);
int	 tcp_output(struct tcpcb *);
void	 tcp_pulloutofband(struct socket *,
	    struct tcphdr *, struct mbuf *, int);
void	 tcp_quench(struct inpcb *, int);
#ifdef INET6
void	 tcp6_quench(struct in6pcb *, int);
#endif
void	 tcp_mtudisc(struct inpcb *, int);

struct ipqent *tcpipqent_alloc(void);
void	 tcpipqent_free(struct ipqent *);

int	 tcp_respond(struct tcpcb *, struct mbuf *, struct mbuf *,
	    struct tcphdr *, tcp_seq, tcp_seq, int);
void	 tcp_rmx_rtt(struct tcpcb *);
void	 tcp_setpersist(struct tcpcb *);
#ifdef TCP_SIGNATURE
int	 tcp_signature_compute(struct mbuf *, struct tcphdr *, int, int,
	    int, u_char *, u_int);
#endif
void	 tcp_slowtimo(void);
struct mbuf *
	 tcp_template(struct tcpcb *);
void	 tcp_trace(short, short, struct tcpcb *, struct mbuf *, int);
struct tcpcb *
	 tcp_usrclosed(struct tcpcb *);
int	 tcp_usrreq(struct socket *,
	    int, struct mbuf *, struct mbuf *, struct mbuf *, struct lwp *);
void	 tcp_xmit_timer(struct tcpcb *, uint32_t);
tcp_seq	 tcp_new_iss(struct tcpcb *, tcp_seq);
tcp_seq  tcp_new_iss1(void *, void *, u_int16_t, u_int16_t, size_t,
	    tcp_seq);

void	 tcp_new_dsack(struct tcpcb *, tcp_seq, u_int32_t);
void	 tcp_sack_option(struct tcpcb *, const struct tcphdr *,
	    const u_char *, int);
void	 tcp_del_sackholes(struct tcpcb *, const struct tcphdr *);
void	 tcp_free_sackholes(struct tcpcb *);
void	 tcp_sack_adjust(struct tcpcb *tp);
struct sackhole *tcp_sack_output(struct tcpcb *tp, int *sack_bytes_rexmt);
void	 tcp_sack_newack(struct tcpcb *, const struct tcphdr *);
int	 tcp_sack_numblks(const struct tcpcb *);
#define	TCP_SACK_OPTLEN(nblks)	((nblks) * 8 + 2 + 2)

int	 syn_cache_add(struct sockaddr *, struct sockaddr *,
		struct tcphdr *, unsigned int, struct socket *,
		struct mbuf *, u_char *, int, struct tcp_opt_info *);
void	 syn_cache_unreach(const struct sockaddr *, const struct sockaddr *,
	   struct tcphdr *);
struct socket *syn_cache_get(struct sockaddr *, struct sockaddr *,
		struct tcphdr *, unsigned int, unsigned int,
		struct socket *so, struct mbuf *);
void	 syn_cache_init(void);
void	 syn_cache_insert(struct syn_cache *, struct tcpcb *);
struct syn_cache *syn_cache_lookup(const struct sockaddr *, const struct sockaddr *,
		struct syn_cache_head **);
void	 syn_cache_reset(struct sockaddr *, struct sockaddr *,
		struct tcphdr *);
int	 syn_cache_respond(struct syn_cache *, struct mbuf *);
void	 syn_cache_timer(void *);
void	 syn_cache_cleanup(struct tcpcb *);

int	 tcp_input_checksum(int, struct mbuf *, const struct tcphdr *, int, int,
    int);
#endif

#endif /* !_NETINET_TCP_VAR_H_ */
