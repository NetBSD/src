/*	$NetBSD: npf_state_tcp.c,v 1.3.2.2 2012/06/26 14:49:10 riz Exp $	*/

/*-
 * Copyright (c) 2010-2012 The NetBSD Foundation, Inc.
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
 * NPF TCP state engine for connection tracking.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_state_tcp.c,v 1.3.2.2 2012/06/26 14:49:10 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#ifndef _KERNEL
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>

#include "npf_impl.h"

/*
 * NPF TCP states.  Note: these states are different from the TCP FSM
 * states of RFC 793.  The packet filter is a man-in-the-middle.
 */
#define	NPF_TCPS_OK		(-1)
#define	NPF_TCPS_CLOSED		0
#define	NPF_TCPS_SYN_SENT	1
#define	NPF_TCPS_SIMSYN_SENT	2
#define	NPF_TCPS_SYN_RECEIVED	3
#define	NPF_TCPS_ESTABLISHED	4
#define	NPF_TCPS_FIN_SEEN	5
#define	NPF_TCPS_CLOSE_WAIT	6
#define	NPF_TCPS_FIN_WAIT	7
#define	NPF_TCPS_CLOSING	8
#define	NPF_TCPS_LAST_ACK	9
#define	NPF_TCPS_TIME_WAIT	10

#define	NPF_TCP_NSTATES		11

/*
 * TCP connection timeout table (in seconds).
 */
static u_int npf_tcp_timeouts[] __read_mostly = {
	/* Closed, timeout nearly immediately. */
	[NPF_TCPS_CLOSED]	= 10,
	/* Unsynchronised states. */
	[NPF_TCPS_SYN_SENT]	= 30,
	[NPF_TCPS_SIMSYN_SENT]	= 30,
	[NPF_TCPS_SYN_RECEIVED]	= 60,
	/* Established, timeout: 24 hours. */
	[NPF_TCPS_ESTABLISHED]	= 60 * 60 * 24,
	/* Closure cases, timeout: 4 minutes (2 * MSL). */
	[NPF_TCPS_FIN_SEEN]	= 60 * 2 * 2,
	[NPF_TCPS_CLOSE_WAIT]	= 60 * 2 * 2,
	[NPF_TCPS_FIN_WAIT]	= 60 * 2 * 2,
	[NPF_TCPS_CLOSING]	= 30,
	[NPF_TCPS_LAST_ACK]	= 30,
	[NPF_TCPS_TIME_WAIT]	= 60 * 2 * 2,
};

#define	NPF_TCP_MAXACKWIN	66000

/*
 * List of TCP flag cases and conversion of flags to a case (index).
 */

#define	TCPFC_INVALID		0
#define	TCPFC_SYN		1
#define	TCPFC_SYNACK		2
#define	TCPFC_ACK		3
#define	TCPFC_FIN		4
#define	TCPFC_COUNT		5

static inline u_int
npf_tcpfl2case(const int tcpfl)
{
	u_int i, c;

	CTASSERT(TH_FIN == 0x01);
	CTASSERT(TH_SYN == 0x02);
	CTASSERT(TH_ACK == 0x10);

	/*
	 * Flags are shifted to use three least significant bits, thus each
	 * flag combination has a unique number ranging from 0 to 7, e.g.
	 * TH_SYN | TH_ACK has number 6, since (0x02 | (0x10 >> 2)) == 6.
	 * However, the requirement is to have number 0 for invalid cases,
	 * such as TH_SYN | TH_FIN, and to have the same number for TH_FIN
	 * and TH_FIN|TH_ACK cases.  Thus, we generate a mask assigning 3
	 * bits for each number, which contains the actual case numbers:
	 *
	 * TCPFC_SYNACK	<< (6 << 2) == 0x2000000 (6 - SYN,ACK)
	 * TCPFC_FIN	<< (5 << 2) == 0x0400000 (5 - FIN,ACK)
	 * ...
	 *
	 * Hence, OR'ed mask value is 0x2430140.
	 */
	i = (tcpfl & (TH_SYN | TH_FIN)) | ((tcpfl & TH_ACK) >> 2);
	c = (0x2430140 >> (i << 2)) & 7;

	KASSERT(c < TCPFC_COUNT);
	return c;
}

/*
 * NPF transition table of a tracked TCP connection.
 *
 * There is a single state, which is changed in the following way:
 *
 * new_state = npf_tcp_fsm[old_state][direction][npf_tcpfl2case(tcp_flags)];
 *
 * Note that this state is different from the state in each end (host).
 */

static const int npf_tcp_fsm[NPF_TCP_NSTATES][2][TCPFC_COUNT] = {
	[NPF_TCPS_CLOSED] = {
		[NPF_FLOW_FORW] = {
			/* Handshake (1): initial SYN. */
			[TCPFC_SYN]	= NPF_TCPS_SYN_SENT,
		},
	},
	[NPF_TCPS_SYN_SENT] = {
		[NPF_FLOW_FORW] = {
			/* SYN may be retransmitted. */
			[TCPFC_SYN]	= NPF_TCPS_OK,
		},
		[NPF_FLOW_BACK] = {
			/* Handshake (2): SYN-ACK is expected. */
			[TCPFC_SYNACK]	= NPF_TCPS_SYN_RECEIVED,
			/* Simultaneous initiation - SYN. */
			[TCPFC_SYN]	= NPF_TCPS_SIMSYN_SENT,
		},
	},
	[NPF_TCPS_SIMSYN_SENT] = {
		[NPF_FLOW_FORW] = {
			/* Original SYN re-transmission. */
			[TCPFC_SYN]	= NPF_TCPS_OK,
			/* SYN-ACK response to simultaneous SYN. */
			[TCPFC_SYNACK]	= NPF_TCPS_SYN_RECEIVED,
		},
		[NPF_FLOW_BACK] = {
			/* Simultaneous SYN re-transmission.*/
			[TCPFC_SYN]	= NPF_TCPS_OK,
			/* SYN-ACK response to original SYN. */
			[TCPFC_SYNACK]	= NPF_TCPS_SYN_RECEIVED,
			/* FIN may be sent early. */
			[TCPFC_FIN]	= NPF_TCPS_FIN_SEEN,
		},
	},
	[NPF_TCPS_SYN_RECEIVED] = {
		[NPF_FLOW_FORW] = {
			/* Handshake (3): ACK is expected. */
			[TCPFC_ACK]	= NPF_TCPS_ESTABLISHED,
			/* FIN may be sent early. */
			[TCPFC_FIN]	= NPF_TCPS_FIN_SEEN,
		},
		[NPF_FLOW_BACK] = {
			/* SYN-ACK may be retransmitted. */
			[TCPFC_SYNACK]	= NPF_TCPS_OK,
			/* XXX: ACK of late SYN in simultaneous case? */
			[TCPFC_ACK]	= NPF_TCPS_OK,
			/* FIN may be sent early. */
			[TCPFC_FIN]	= NPF_TCPS_FIN_SEEN,
		},
	},
	[NPF_TCPS_ESTABLISHED] = {
		/*
		 * Regular ACKs (data exchange) or FIN.
		 * FIN packets may have ACK set.
		 */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			/* FIN by the sender. */
			[TCPFC_FIN]	= NPF_TCPS_FIN_SEEN,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			/* FIN by the receiver. */
			[TCPFC_FIN]	= NPF_TCPS_FIN_SEEN,
		},
	},
	[NPF_TCPS_FIN_SEEN] = {
		/*
		 * FIN was seen.  If ACK only, connection is half-closed now,
		 * need to determine which end is closed (sender or receiver).
		 * However, both FIN and FIN-ACK may race here - in which
		 * case we are closing immediately.
		 */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_CLOSE_WAIT,
			[TCPFC_FIN]	= NPF_TCPS_CLOSING,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_FIN_WAIT,
			[TCPFC_FIN]	= NPF_TCPS_CLOSING,
		},
	},
	[NPF_TCPS_CLOSE_WAIT] = {
		/* Sender has sent the FIN and closed its end. */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			[TCPFC_FIN]	= NPF_TCPS_LAST_ACK,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			[TCPFC_FIN]	= NPF_TCPS_LAST_ACK,
		},
	},
	[NPF_TCPS_FIN_WAIT] = {
		/* Receiver has closed its end. */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			[TCPFC_FIN]	= NPF_TCPS_LAST_ACK,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_OK,
			[TCPFC_FIN]	= NPF_TCPS_LAST_ACK,
		},
	},
	[NPF_TCPS_CLOSING] = {
		/* Race of FINs - expecting ACK. */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_LAST_ACK,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_LAST_ACK,
		},
	},
	[NPF_TCPS_LAST_ACK] = {
		/* FINs exchanged - expecting last ACK. */
		[NPF_FLOW_FORW] = {
			[TCPFC_ACK]	= NPF_TCPS_TIME_WAIT,
		},
		[NPF_FLOW_BACK] = {
			[TCPFC_ACK]	= NPF_TCPS_TIME_WAIT,
		},
	},
	[NPF_TCPS_TIME_WAIT] = {
		/* May re-open the connection as per RFC 1122. */
		[NPF_FLOW_FORW] = {
			[TCPFC_SYN]	= NPF_TCPS_SYN_SENT,
		},
	},
};

/*
 * npf_tcp_inwindow: determine whether the packet is in the TCP window
 * and thus part of the connection we are tracking.
 */
static bool
npf_tcp_inwindow(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst,
    const int di)
{
	const struct tcphdr * const th = &npc->npc_l4.tcp;
	const int tcpfl = th->th_flags;
	npf_tcpstate_t *fstate, *tstate;
	int tcpdlen, wscale, ackskew;
	tcp_seq seq, ack, end;
	uint32_t win;

	KASSERT(npf_iscached(npc, NPC_TCP));
	KASSERT(di == NPF_FLOW_FORW || di == NPF_FLOW_BACK);

	/*
	 * Perform SEQ/ACK numbers check against boundaries.  Reference:
	 *
	 *	Rooij G., "Real stateful TCP packet filtering in IP Filter",
	 *	10th USENIX Security Symposium invited talk, Aug. 2001.
	 *
	 * There are four boundaries defined as following:
	 *	I)   SEQ + LEN	<= MAX { SND.ACK + MAX(SND.WIN, 1) }
	 *	II)  SEQ	>= MAX { SND.SEQ + SND.LEN - MAX(RCV.WIN, 1) }
	 *	III) ACK	<= MAX { RCV.SEQ + RCV.LEN }
	 *	IV)  ACK	>= MAX { RCV.SEQ + RCV.LEN } - MAXACKWIN
	 *
	 * Let these members of npf_tcpstate_t be the maximum seen values of:
	 *	nst_end		- SEQ + LEN
	 *	nst_maxend	- ACK + MAX(WIN, 1)
	 *	nst_maxwin	- MAX(WIN, 1)
	 */

	tcpdlen = npf_tcpsaw(__UNCONST(npc), &seq, &ack, &win);
	end = seq + tcpdlen;
	if (tcpfl & TH_SYN) {
		end++;
	}
	if (tcpfl & TH_FIN) {
		end++;
	}

	fstate = &nst->nst_tcpst[di];
	tstate = &nst->nst_tcpst[!di];
	win = win ? (win << fstate->nst_wscale) : 1;

	/*
	 * Initialise if the first packet.
	 * Note: only case when nst_maxwin is zero.
	 */
	if (__predict_false(fstate->nst_maxwin == 0)) {
		/*
		 * Normally, it should be the first SYN or a re-transmission
		 * of SYN.  The state of the other side will get set with a
		 * SYN-ACK reply (see below).
		 */
		fstate->nst_end = end;
		fstate->nst_maxend = end;
		fstate->nst_maxwin = win;
		tstate->nst_end = 0;
		tstate->nst_maxend = 0;
		tstate->nst_maxwin = 1;

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
	if (fstate->nst_end == 0) {
		/*
		 * Should be a SYN-ACK reply to SYN.  If SYN is not set,
		 * then we are in the middle of connection and lost tracking.
		 */
		fstate->nst_end = end;
		fstate->nst_maxend = end + 1;
		fstate->nst_maxwin = win;

		/* Handle TCP Window Scaling (must be ignored if no SYN). */
		if (tcpfl & TH_SYN) {
			fstate->nst_wscale =
			    npf_fetch_tcpopts(npc, nbuf, NULL, &wscale) ?
			    wscale : 0;
		}
	}

	if ((tcpfl & TH_ACK) == 0) {
		/* Pretend that an ACK was sent. */
		ack = tstate->nst_end;
	} else if ((tcpfl & (TH_ACK|TH_RST)) == (TH_ACK|TH_RST) && ack == 0) {
		/* Workaround for some TCP stacks. */
		ack = tstate->nst_end;
	}
	if (seq == end) {
		/* If packet contains no data - assume it is valid. */
		end = fstate->nst_end;
		seq = end;
	}
#if 0
	/* Strict in-order sequence for RST packets. */
	if ((tcpfl & TH_RST) != 0 && (fstate->nst_end - seq) > 1) {
		return false;
	}
#endif
	/*
	 * Determine whether the data is within previously noted window,
	 * that is, upper boundary for valid data (I).
	 */
	if (!SEQ_LEQ(end, fstate->nst_maxend)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP1);
		return false;
	}

	/* Lower boundary (II), which is no more than one window back. */
	if (!SEQ_GEQ(seq, fstate->nst_end - tstate->nst_maxwin)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP2);
		return false;
	}

	/*
	 * Boundaries for valid acknowledgments (III, IV) - one predicted
	 * window up or down, since packets may be fragmented.
	 */
	ackskew = tstate->nst_end - ack;
	if (ackskew < -NPF_TCP_MAXACKWIN ||
	    ackskew > (NPF_TCP_MAXACKWIN << fstate->nst_wscale)) {
		npf_stats_inc(NPF_STAT_INVALID_STATE_TCP3);
		return false;
	}

	/*
	 * Packet has been passed.
	 *
	 * Negative ackskew might be due to fragmented packets.  Since the
	 * total length of the packet is unknown - bump the boundary.
	 */

	if (ackskew < 0) {
		tstate->nst_end = ack;
	}
	/* Keep track of the maximum window seen. */
	if (fstate->nst_maxwin < win) {
		fstate->nst_maxwin = win;
	}
	if (SEQ_GT(end, fstate->nst_end)) {
		fstate->nst_end = end;
	}
	/* Note the window for upper boundary. */
	if (SEQ_GEQ(ack + win, tstate->nst_maxend)) {
		tstate->nst_maxend = ack + win;
	}
	return true;
}

/*
 * npf_state_tcp: inspect TCP segment, determine whether it belongs to
 * the connection and track its state.
 */
bool
npf_state_tcp(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst, int di)
{
	const struct tcphdr * const th = &npc->npc_l4.tcp;
	const int tcpfl = th->th_flags, state = nst->nst_state;
	int nstate;

	KASSERT(nst->nst_state == 0 || mutex_owned(&nst->nst_lock));

	/* Look for a transition to a new state. */
	if (__predict_true((tcpfl & TH_RST) == 0)) {
		const int flagcase = npf_tcpfl2case(tcpfl);
		nstate = npf_tcp_fsm[state][di][flagcase];
	} else if (state == NPF_TCPS_TIME_WAIT) {
		/* Prevent TIME-WAIT assassination (RFC 1337). */
		nstate = NPF_TCPS_OK;
	} else {
		nstate = NPF_TCPS_CLOSED;
	}

	/* Determine whether TCP packet really belongs to this connection. */
	if (!npf_tcp_inwindow(npc, nbuf, nst, di)) {
		return false;
	}
	if (__predict_true(nstate == NPF_TCPS_OK)) {
		return true;
	}

	nst->nst_state = nstate;
	return true;
}

int
npf_state_tcp_timeout(const npf_state_t *nst)
{
	const u_int state = nst->nst_state;

	KASSERT(state < NPF_TCP_NSTATES);
	return npf_tcp_timeouts[state];
}
