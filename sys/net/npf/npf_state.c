/*	$NetBSD: npf_state.c,v 1.1 2010/11/11 06:30:39 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: npf_state.c,v 1.1 2010/11/11 06:30:39 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/mutex.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>

#include "npf_impl.h"

#define	MAXACKWINDOW		66000

/* Session expiration table.  XXX revisit later */
static const u_int expire_table[ ] = {
	[IPPROTO_TCP]		= 86400,	/* 24 hours */
	[IPPROTO_UDP]		= 120,		/* 2 min */
	[IPPROTO_ICMP]		= 30		/* 1 min */
};

static bool
npf_tcp_inwindow(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst,
    const bool forw)
{
	const struct tcphdr *th = &npc->npc_l4.tcp;
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
		return false;
	}
	/* Lower boundary (II), which is no more than one window back. */
	if (!SEQ_GEQ(seq, fstate->nst_seqend - tstate->nst_maxwin)) {
		return false;
	}
	/*
	 * Boundaries for valid acknowledgments (III, IV) - on predicted
	 * window up or down, since packets may be fragmented.
	 */
	ackskew = tstate->nst_seqend - ack;
	if (ackskew < -MAXACKWINDOW || ackskew > MAXACKWINDOW) {
		return false;
	}

	/*
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
	const struct tcphdr *th = &npc->npc_l4.tcp;
	const int tcpfl = th->th_flags;

	/*
	 * Handle 3-way handshake (SYN -> SYN,ACK -> ACK).
	 */
	switch (nst->nst_state) {
	case ST_ESTABLISHED:
		/* Common case - connection established. */
		if (tcpfl & TH_ACK) {
			/*
			 * Data transmission.
			 */
		} else if (tcpfl & TH_FIN) {
			/* XXX TODO */
		}
		break;
	case ST_OPENING:
		/* SYN has been sent, expecting SYN-ACK. */
		if (tcpfl == (TH_SYN | TH_ACK) && !forw) {
			/* Received backwards SYN-ACK. */
			nst->nst_state = ST_ACKNOWLEDGE;
		} else if (tcpfl == TH_SYN && forw) {
			/* Re-transmission of SYN. */
		} else {
			return false;
		}
		break;
	case ST_ACKNOWLEDGE:
		/* SYN-ACK was seen, expecting ACK. */
		if (tcpfl == TH_ACK && forw) {
			nst->nst_state = ST_ESTABLISHED;
		} else {
			return false;
		}
		break;
	case ST_CLOSING:
		/* XXX TODO */
		break;
	default:
		npf_state_dump(nst);
		KASSERT(false);
	}
	return npf_tcp_inwindow(npc, nbuf, nst, forw);
}

bool
npf_state_init(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst)
{
	const int proto = npf_cache_ipproto(npc);

	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_LAYER4));
	if (proto == IPPROTO_TCP) {
		const struct tcphdr *th = &npc->npc_l4.tcp;
		/* TCP case: must be SYN. */
		KASSERT(npf_iscached(npc, NPC_TCP));
		if (th->th_flags != TH_SYN) {
			return false;
		}
		/* Initial values for TCP window and sequence tracking. */
		if (!npf_tcp_inwindow(npc, nbuf, nst, true)) {
			return false;
		}
	}
	mutex_init(&nst->nst_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	nst->nst_state = ST_OPENING;
	return true;
}

void
npf_state_destroy(npf_state_t *nst)
{

	KASSERT(nst->nst_state != 0);
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
		/* Handle UDP or ICMP response for opening session. */
		if (nst->nst_state == ST_OPENING && !forw) {
			nst->nst_state = ST_ESTABLISHED;
		}
		ret = true;
	}
	mutex_exit(&nst->nst_lock);
	return ret;
}

int
npf_state_etime(const npf_state_t *nst, const int proto)
{

	if (nst->nst_state == ST_ESTABLISHED) {
		return expire_table[proto];
	}
	return 10;	/* XXX TODO */
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_state_dump(npf_state_t *nst)
{
	npf_tcpstate_t *fst = &nst->nst_tcpst[0], *tst = &nst->nst_tcpst[1];

	printf("\tstate (%p) %d:\n\t\t"
	    "F { seqend %u ackend %u mwin %u wscale %u }\n\t\t"
	    "T { seqend %u, ackend %u mwin %u wscale %u }\n",
	    nst, nst->nst_state,
	    fst->nst_seqend, fst->nst_ackend, fst->nst_maxwin, fst->nst_wscale,
	    tst->nst_seqend, tst->nst_ackend, tst->nst_maxwin, tst->nst_wscale
	);
}

#endif
