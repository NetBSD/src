/*	$NetBSD: npf_state.c,v 1.3.4.3 2011/05/31 03:05:07 rmind Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * NPF state engine to track connections.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_state.c,v 1.3.4.3 2011/05/31 03:05:07 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/mutex.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>

#include "npf_impl.h"

/* TCP session expiration table. */
static const u_int tcp_expire_table[ ] __read_mostly = {
	/* Initial synchronisation.  Timeout: 30 sec and 1 minute. */
	[TCPS_SYN_SENT]		= 30,
	[TCPS_SYN_RECEIVED]	= 60,
	/* Established (synchronised).  Timeout: 24 hours. */
	[TCPS_ESTABLISHED]	= 60 * 60 * 24,
	[TCPS_FIN_WAIT_1]	= 60 * 60 * 24,
	[TCPS_FIN_WAIT_2]	= 60 * 60 * 24,
	/* UNUSED [TCPS_CLOSE_WAIT]	= 60 * 60 * 24, */
	/* Closure.  Timeout: 4 minutes (2 * MSL). */
	[TCPS_CLOSING]		= 60 * 4,
	[TCPS_LAST_ACK]		= 60 * 4,
	[TCPS_TIME_WAIT]	= 60 * 4,
	/* Fully closed.  Timeout immediately. */
	[TCPS_CLOSED]		= 0
};

/* Session expiration table. */
static const u_int expire_table[ ] __read_mostly = {
	[IPPROTO_UDP]		= 60,		/* 1 min */
	[IPPROTO_ICMP]		= 30		/* 30 sec */
};

#define	MAXACKWINDOW		66000

static bool
npf_tcp_inwindow(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst,
    const bool forw)
{
	const struct tcphdr * const th = &npc->npc_l4.tcp;
	const int tcpfl = th->th_flags;
	npf_tcpstate_t *fstate, *tstate;
	int tcpdlen, wscale, ackskew;
	tcp_seq seq, ack, end;
	uint32_t win;

	KASSERT(npf_iscached(npc, NPC_TCP));
	tcpdlen = npf_tcpsaw(__UNCONST(npc), &seq, &ack, &win);
	end = seq + tcpdlen;
	if (tcpfl & TH_SYN) {
		end++;
	}
	if (tcpfl & TH_FIN) {
		end++;
	}

	/*
	 * Perform SEQ/ACK numbers check against boundaries.  Reference:
	 *
	 *	Rooij G., "Real stateful TCP packet filtering in IP Filter",
	 *	10th USENIX Security Symposium invited talk, Aug. 2001.
	 */

	fstate = &nst->nst_tcpst[forw ? 0 : 1];
	tstate = &nst->nst_tcpst[forw ? 1 : 0];
	win = win ? (win << fstate->nst_wscale) : 1;

	if (tcpfl == TH_SYN) {
		/*
		 * First SYN or re-transmission of SYN.  Initialize all
		 * values.  State of other side will get set with a SYN-ACK
		 * reply (see below).
		 */
		fstate->nst_seqend = end;
		fstate->nst_ackend = end;
		fstate->nst_maxwin = win;
		tstate->nst_ackend = 0;
		tstate->nst_ackend = 0;
		tstate->nst_maxwin = 0;
		/*
		 * Handle TCP Window Scaling (RFC 1323).  Both sides may
		 * send this option in their SYN packets.
		 */
		if (npf_fetch_tcpopts(npc, nbuf, NULL, &wscale)) {
			fstate->nst_wscale = wscale;
		} else {
			fstate->nst_wscale = 0;
		}
		tstate->nst_wscale = 0;
		/* Done. */
		return true;
	}
	if (fstate->nst_seqend == 0) {
		/*
		 * Should be a SYN-ACK reply to SYN.  If SYN is not set,
		 * then we are in the middle connection and lost tracking.
		 */
		fstate->nst_seqend = end;
		fstate->nst_ackend = end + 1;
		fstate->nst_maxwin = 1;

		/* Handle TCP Window Scaling (must be ignored if no SYN). */
		if (tcpfl & TH_SYN) {
			fstate->nst_wscale =
			    npf_fetch_tcpopts(npc, nbuf, NULL, &wscale) ?
			    wscale : 0;
		}
	}
	if ((tcpfl & TH_ACK) == 0) {
		/* Pretend that an ACK was sent. */
		ack = tstate->nst_seqend;
	} else if ((tcpfl & (TH_ACK|TH_RST)) == (TH_ACK|TH_RST) && ack == 0) {
		/* Workaround for some TCP stacks. */
		ack = tstate->nst_seqend;
	}
	if (seq == end) {
		/* If packet contains no data - assume it is valid. */
		end = fstate->nst_seqend;
		seq = end;
	}

	/*
	 * Determine whether the data is within previously noted window,
	 * that is, upper boundary for valid data (I).
	 */
	if (!SEQ_GEQ(fstate->nst_ackend, end)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP1);
		return false;
	}
	/* Lower boundary (II), which is no more than one window back. */
	if (!SEQ_GEQ(seq, fstate->nst_seqend - tstate->nst_maxwin)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP2);
		return false;
	}
	/*
	 * Boundaries for valid acknowledgments (III, IV) - on predicted
	 * window up or down, since packets may be fragmented.
	 */
	ackskew = tstate->nst_seqend - ack;
	if (ackskew < -MAXACKWINDOW || ackskew > MAXACKWINDOW) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP3);
		return false;
	}

	/*
	 * Packet is passed now.
	 *
	 * Negative ackskew might be due to fragmented packets.  Since the
	 * total length of the packet is unknown - bump the boundary.
	 */
	if (ackskew < 0) {
		tstate->nst_seqend = end;
	}
	/* Keep track of the maximum window seen. */
	if (fstate->nst_maxwin < win) {
		fstate->nst_maxwin = win;
	}
	if (SEQ_GT(end, fstate->nst_seqend)) {
		fstate->nst_seqend = end;
	}
	/* Note the window for upper boundary. */
	if (SEQ_GEQ(ack + win, tstate->nst_ackend)) {
		tstate->nst_ackend = ack + win;
	}
	return true;
}

static inline bool
npf_state_tcp(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst,
    const bool forw)
{
	const struct tcphdr * const th = &npc->npc_l4.tcp;
	const int tcpfl = th->th_flags, state = nst->nst_state;
#if 0
	/* Determine whether TCP packet really belongs to this connection. */
	if (!npf_tcp_inwindow(npc, nbuf, nst, forw)) {
		return false;
	}
#endif
	/*
	 * Handle 3-way handshake (SYN -> SYN,ACK -> ACK), connection
	 * reset (RST), half-open connections, connection closure, etc.
	 */
	if (__predict_false(tcpfl & TH_RST)) {
		nst->nst_state = TCPS_CLOSED;
		return true;
	}
	switch (state) {
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_2:
		/* Common case - connection is established. */
		if ((tcpfl & (TH_SYN | TH_ACK | TH_FIN)) == TH_ACK) {
			return true;
		}
		/* Otherwise, can only be a FIN. */
		if ((tcpfl & TH_FIN) == 0) {
			break;
		}
		/* XXX see below TCPS_CLOSE_WAIT */
		if (state != TCPS_FIN_WAIT_2) {
			/* First FIN: closure of one end. */
			nst->nst_state = TCPS_FIN_WAIT_1;
		} else {
			/* Second FIN: connection closure, wait for ACK. */
			nst->nst_state = TCPS_LAST_ACK;
		}
		return true;
	case TCPS_SYN_SENT:
		/* After SYN expecting SYN-ACK. */
		if (tcpfl == (TH_SYN | TH_ACK) && !forw) {
			/* Received backwards SYN-ACK. */
			nst->nst_state = TCPS_SYN_RECEIVED;
			return true;
		}
		if (tcpfl == TH_SYN && forw) {
			/* Re-transmission of SYN. */
			return true;
		}
		break;
	case TCPS_SYN_RECEIVED:
		/* SYN-ACK was seen, expecting ACK. */
		if ((tcpfl & (TH_SYN | TH_ACK | TH_FIN)) == TH_ACK) {
			/* ACK - establish connection. */
			nst->nst_state = TCPS_ESTABLISHED;
			return true;
		}
		if (tcpfl == (TH_SYN | TH_ACK)) {
			/* Re-transmission of SYN-ACK. */
			return true;
		}
		break;
	case TCPS_CLOSE_WAIT:
		/* UNUSED */
	case TCPS_FIN_WAIT_1:
		/*
		 * XXX: FIN re-transmission is not handled, use TCPS_CLOSE_WAIT.
		 */
		/*
		 * First FIN was seen, expecting ACK.  However, we may receive
		 * a simultaneous FIN or exchange of FINs with FIN-ACK.
		 */
		if ((tcpfl & (TH_ACK | TH_FIN)) == (TH_ACK | TH_FIN)) {
			/* Exchange of FINs with ACK.  Wait for last ACK. */
			nst->nst_state = TCPS_LAST_ACK;
			return true;
		} else if (tcpfl & TH_ACK) {
			/* ACK of first FIN. */
			nst->nst_state = TCPS_FIN_WAIT_2;
			return true;
		} else if (tcpfl & TH_FIN) {
			/* Simultaneous FIN.  Need to wait for ACKs. */
			nst->nst_state = TCPS_CLOSING;
			return true;
		}
		break;
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		/* Expecting only ACK. */
		if ((tcpfl & (TH_SYN | TH_ACK | TH_FIN)) != TH_ACK) {
			return false;
		}
		switch (state) {
		case TCPS_CLOSING:
			/* One ACK noted, wait for last one. */
			nst->nst_state = TCPS_LAST_ACK;
			break;
		case TCPS_LAST_ACK:
			/* Last ACK received, quiet wait now. */
			nst->nst_state = TCPS_TIME_WAIT;
			break;
		}
		return true;
	case TCPS_CLOSED:
		/* XXX: Drop or pass? */
		break;
	default:
		npf_state_dump(nst);
		KASSERT(false);
	}
	return false;
}

bool
npf_state_init(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst)
{
	const int proto = npf_cache_ipproto(npc);

	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_LAYER4));

	mutex_init(&nst->nst_lock, MUTEX_DEFAULT, IPL_SOFTNET);

	if (proto == IPPROTO_TCP) {
		const struct tcphdr *th = &npc->npc_l4.tcp;

		/* TCP case: must be SYN. */
		KASSERT(npf_iscached(npc, NPC_TCP));
		if (th->th_flags != TH_SYN) {
			npf_stats_inc(NPF_STAT_INVALID_STATE);
			return false;
		}
		/* Initial values for TCP window and sequence tracking. */
		if (!npf_tcp_inwindow(npc, nbuf, nst, true)) {
			npf_stats_inc(NPF_STAT_INVALID_STATE);
			return false;
		}
	}

	/*
	 * Initial state: SYN sent, waiting for response from the other side.
	 * Note: for UDP or ICMP, reuse SYN-sent flag to note response.
	 */
	nst->nst_state = TCPS_SYN_SENT;
	return true;
}

void
npf_state_destroy(npf_state_t *nst)
{

	mutex_destroy(&nst->nst_lock);
}

bool
npf_state_inspect(const npf_cache_t *npc, nbuf_t *nbuf,
    npf_state_t *nst, const bool forw)
{
	const int proto = npf_cache_ipproto(npc);
	bool ret;

	mutex_enter(&nst->nst_lock);
	switch (proto) {
	case IPPROTO_TCP:
		/* Handle TCP. */
		ret = npf_state_tcp(npc, nbuf, nst, forw);
		break;
	default:
		/*
		 * Handle UDP or ICMP response for opening session.
		 */
		if (nst->nst_state == TCPS_SYN_SENT && !forw) {
			nst->nst_state= TCPS_ESTABLISHED;
		}
		ret = true;
	}
	mutex_exit(&nst->nst_lock);
	if (__predict_false(!ret)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE);
	}
	return ret;
}

/*
 * npf_state_etime: return session expiration time according to the state.
 */
int
npf_state_etime(const npf_state_t *nst, const int proto)
{
	const int state = nst->nst_state;

	if (__predict_true(proto == IPPROTO_TCP)) {
		return tcp_expire_table[state];
	}
	return expire_table[proto];
}

void
npf_state_dump(npf_state_t *nst)
{
#if defined(DDB) || defined(_NPF_TESTING)
	npf_tcpstate_t *fst = &nst->nst_tcpst[0], *tst = &nst->nst_tcpst[1];

	printf("\tstate (%p) %d:\n\t\t"
	    "F { seqend %u ackend %u mwin %u wscale %u }\n\t\t"
	    "T { seqend %u ackend %u mwin %u wscale %u }\n",
	    nst, nst->nst_state,
	    fst->nst_seqend, fst->nst_ackend, fst->nst_maxwin, fst->nst_wscale,
	    tst->nst_seqend, tst->nst_ackend, tst->nst_maxwin, tst->nst_wscale
	);
#endif
}
