/*	$KAME: sctp_var.h,v 1.24 2005/03/06 16:04:19 itojun Exp $	*/
/*	$NetBSD: sctp_var.h,v 1.1 2015/10/13 21:28:35 rjs Exp $ */

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Cisco Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETINET_SCTP_VAR_H_
#define _NETINET_SCTP_VAR_H_

#include <sys/socketvar.h>
#include <netinet/sctp_uio.h>

/* SCTP Kernel structures */

/*
 * Names for SCTP sysctl objects
 */
#ifndef __APPLE__
#define	SCTPCTL_MAXDGRAM	    1	/* max datagram size */
#define	SCTPCTL_RECVSPACE	    2	/* default receive buffer space */
#define SCTPCTL_AUTOASCONF          3   /* auto asconf enable/disable flag */
#define SCTPCTL_ECN_ENABLE          4	/* Is ecn allowed */
#define SCTPCTL_ECN_NONCE           5   /* Is ecn nonce allowed */
#define SCTPCTL_STRICT_SACK         6	/* strictly require sack'd TSN's to be
					 * smaller than sndnxt.
					 */
#define SCTPCTL_NOCSUM_LO           7   /* Require that the Loopback NOT have
				         * the crc32 checksum on packets routed over
					 * it.
				         */
#define SCTPCTL_STRICT_INIT         8
#define SCTPCTL_PEER_CHK_OH         9
#define SCTPCTL_MAXBURST            10
#define SCTPCTL_MAXCHUNKONQ         11
#define SCTPCTL_DELAYED_SACK        12
#define SCTPCTL_HB_INTERVAL         13
#define SCTPCTL_PMTU_RAISE          14
#define SCTPCTL_SHUTDOWN_GUARD      15
#define SCTPCTL_SECRET_LIFETIME     16
#define SCTPCTL_RTO_MAX             17
#define SCTPCTL_RTO_MIN             18
#define SCTPCTL_RTO_INITIAL         19
#define SCTPCTL_INIT_RTO_MAX        20
#define SCTPCTL_COOKIE_LIFE         21
#define SCTPCTL_INIT_RTX_MAX        22
#define SCTPCTL_ASSOC_RTX_MAX       23
#define SCTPCTL_PATH_RTX_MAX        24
#define SCTPCTL_NR_OUTGOING_STREAMS 25
#ifdef SCTP_DEBUG
#define SCTPCTL_DEBUG               26
#define SCTPCTL_MAXID		    27
#else
#define SCTPCTL_MAXID		    26
#endif

#endif

#ifdef SCTP_DEBUG
#define SCTPCTL_NAMES { \
	{ 0, 0 }, \
	{ "maxdgram", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
	{ "autoasconf", CTLTYPE_INT }, \
	{ "ecn_enable", CTLTYPE_INT }, \
	{ "ecn_nonce", CTLTYPE_INT }, \
	{ "strict_sack", CTLTYPE_INT }, \
	{ "looback_nocsum", CTLTYPE_INT }, \
	{ "strict_init", CTLTYPE_INT }, \
	{ "peer_chkoh", CTLTYPE_INT }, \
	{ "maxburst", CTLTYPE_INT }, \
	{ "maxchunks", CTLTYPE_INT }, \
	{ "delayed_sack_time", CTLTYPE_INT }, \
	{ "heartbeat_interval", CTLTYPE_INT }, \
	{ "pmtu_raise_time", CTLTYPE_INT }, \
	{ "shutdown_guard_time", CTLTYPE_INT }, \
	{ "secret_lifetime", CTLTYPE_INT }, \
	{ "rto_max", CTLTYPE_INT }, \
	{ "rto_min", CTLTYPE_INT }, \
	{ "rto_initial", CTLTYPE_INT }, \
	{ "init_rto_max", CTLTYPE_INT }, \
	{ "valid_cookie_life", CTLTYPE_INT }, \
	{ "init_rtx_max", CTLTYPE_INT }, \
	{ "assoc_rtx_max", CTLTYPE_INT }, \
	{ "path_rtx_max", CTLTYPE_INT }, \
	{ "nr_outgoing_streams", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
}
#else
#define SCTPCTL_NAMES { \
	{ 0, 0 }, \
	{ "maxdgram", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
	{ "autoasconf", CTLTYPE_INT }, \
	{ "ecn_enable", CTLTYPE_INT }, \
	{ "ecn_nonce", CTLTYPE_INT }, \
	{ "strict_sack", CTLTYPE_INT }, \
	{ "looback_nocsum", CTLTYPE_INT }, \
	{ "strict_init", CTLTYPE_INT }, \
	{ "peer_chkoh", CTLTYPE_INT }, \
	{ "maxburst", CTLTYPE_INT }, \
	{ "maxchunks", CTLTYPE_INT }, \
	{ "delayed_sack_time", CTLTYPE_INT }, \
	{ "heartbeat_interval", CTLTYPE_INT }, \
	{ "pmtu_raise_time", CTLTYPE_INT }, \
	{ "shutdown_guard_time", CTLTYPE_INT }, \
	{ "secret_lifetime", CTLTYPE_INT }, \
	{ "rto_max", CTLTYPE_INT }, \
	{ "rto_min", CTLTYPE_INT }, \
	{ "rto_initial", CTLTYPE_INT }, \
	{ "init_rto_max", CTLTYPE_INT }, \
	{ "valid_cookie_life", CTLTYPE_INT }, \
	{ "init_rtx_max", CTLTYPE_INT }, \
	{ "assoc_rtx_max", CTLTYPE_INT }, \
	{ "path_rtx_max", CTLTYPE_INT }, \
	{ "nr_outgoing_streams", CTLTYPE_INT }, \
}
#endif

#if defined(_KERNEL)

extern const struct pr_usrreqs sctp_usrreqs;

int sctp_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
		      struct mbuf *, struct lwp *);

#define	sctp_sbspace(sb) ((long) (((sb)->sb_hiwat > (sb)->sb_cc) ? ((sb)->sb_hiwat - (sb)->sb_cc) : 0))

#define sctp_sbspace_sub(a,b) ((a > b) ? (a - b) : 0)

extern int	sctp_sendspace;
extern int	sctp_recvspace;
extern int      sctp_ecn;
extern int      sctp_ecn_nonce;

#define sctp_ucount_incr(val) { \
	val++; \
}

#define sctp_ucount_decr(val) { \
	if (val > 0) { \
		val--; \
	} else { \
		val = 0; \
	} \
}

#define sctp_flight_size_decrease(tp1) do { \
	if (tp1->whoTo->flight_size >= tp1->book_size) \
		tp1->whoTo->flight_size -= tp1->book_size; \
	else \
		tp1->whoTo->flight_size = 0; \
} while (0)

#define sctp_flight_size_increase(tp1) do { \
       (tp1)->whoTo->flight_size += (tp1)->book_size; \
} while (0)

#define sctp_total_flight_decrease(stcb, tp1) do { \
	if (stcb->asoc.total_flight >= tp1->book_size) { \
		stcb->asoc.total_flight -= tp1->book_size; \
		if (stcb->asoc.total_flight_count > 0) \
			stcb->asoc.total_flight_count--; \
	} else { \
		stcb->asoc.total_flight = 0; \
		stcb->asoc.total_flight_count = 0; \
	} \
} while (0)

#define sctp_total_flight_increase(stcb, tp1) do { \
       (stcb)->asoc.total_flight_count++; \
       (stcb)->asoc.total_flight += (tp1)->book_size; \
} while (0)


struct sctp_nets;
struct sctp_inpcb;
struct sctp_tcb;
struct sctphdr;

void*	sctp_ctlinput(int, const struct sockaddr *, void *);
int	sctp_ctloutput(int, struct socket *, struct sockopt *);
void	sctp_input(struct mbuf *, ... );
void	sctp_drain(void);
void	sctp_init(void);
int	sctp_shutdown(struct socket *);
void	sctp_notify(struct sctp_inpcb *, int, struct sctphdr *,
			 struct sockaddr *, struct sctp_tcb *,
			 struct sctp_nets *);
int sctp_rcvd(struct socket *, int, struct lwp *);
int sctp_send(struct socket *, struct mbuf *, struct sockaddr *,
		struct mbuf *, struct lwp *);

#if defined(INET6)
void ip_2_ip6_hdr(struct ip6_hdr *, struct ip *);
#endif

int sctp_bindx(struct socket *, int, struct sockaddr_storage *,
	int, int, struct lwp *);

/* can't use sctp_assoc_t here */
int sctp_peeloff(struct socket *, struct socket *, int, vaddr_t, int *);


sctp_assoc_t sctp_getassocid(struct sockaddr *);
int sctp_sockaddr(struct socket *, struct sockaddr *);
int sctp_peeraddr(struct socket *, struct sockaddr *);
int sctp_listen(struct socket *, struct lwp *);
int sctp_accept(struct socket *, struct sockaddr *);

int sctp_sysctl(int *, u_int, void *, size_t *, void *, size_t);

#endif /* _KERNEL */

#endif /* !_NETINET_SCTP_VAR_H_ */
