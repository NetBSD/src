/*	$NetBSD: npf_state.c,v 1.6 2011/11/29 20:05:30 rmind Exp $	*/

/*-
 * Copyright (c) 2010-2011 The NetBSD Foundation, Inc.
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
 * NPF state engine to track sessions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_state.c,v 1.6 2011/11/29 20:05:30 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/mutex.h>

#include "npf_impl.h"

/*
 * Generic session states and timeout table.
 *
 * Note: used for connnection-less protocols.
 */

#define	NPF_ANY_SESSION_CLOSED		0
#define	NPF_ANY_SESSION_NEW		1
#define	NPF_ANY_SESSION_ESTABLISHED	2
#define	NPF_ANY_SESSION_NSTATES		3

static const int npf_generic_fsm[NPF_ANY_SESSION_NSTATES][2] __read_mostly = {
	[NPF_ANY_SESSION_CLOSED] = {
		[NPF_FLOW_FORW]		= NPF_ANY_SESSION_NEW,
	},
	[NPF_ANY_SESSION_NEW] = {
		[NPF_FLOW_FORW]		= NPF_ANY_SESSION_NEW,
		[NPF_FLOW_BACK]		= NPF_ANY_SESSION_ESTABLISHED,
	},
	[NPF_ANY_SESSION_ESTABLISHED] = {
		[NPF_FLOW_FORW]		= NPF_ANY_SESSION_ESTABLISHED,
		[NPF_FLOW_BACK]		= NPF_ANY_SESSION_ESTABLISHED,
	},
};

static const u_int npf_generic_timeout[] __read_mostly = {
	[NPF_ANY_SESSION_CLOSED]	= 0,
	[NPF_ANY_SESSION_NEW]		= 30,
	[NPF_ANY_SESSION_ESTABLISHED]	= 60,
};

bool
npf_state_init(const npf_cache_t *npc, nbuf_t *nbuf, npf_state_t *nst)
{
	const int proto = npf_cache_ipproto(npc);
	bool ret;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_LAYER4));

	memset(nst, 0, sizeof(npf_state_t));
	mutex_init(&nst->nst_lock, MUTEX_DEFAULT, IPL_SOFTNET);

	switch (proto) {
	case IPPROTO_TCP:
		/* Pass to TCP state tracking engine. */
		ret = npf_state_tcp(npc, nbuf, nst, NPF_FLOW_FORW);
		break;
	case IPPROTO_UDP:
	case IPPROTO_ICMP:
		/* Generic. */
		nst->nst_state = npf_generic_fsm[nst->nst_state][NPF_FLOW_FORW];
		ret = true;
		break;
	default:
		ret = false;
	}
	return ret;
}

void
npf_state_destroy(npf_state_t *nst)
{

	nst->nst_state = 0;
	mutex_destroy(&nst->nst_lock);
}

bool
npf_state_inspect(const npf_cache_t *npc, nbuf_t *nbuf,
    npf_state_t *nst, const bool forw)
{
	const int proto = npf_cache_ipproto(npc);
	const int di = forw ? NPF_FLOW_FORW : NPF_FLOW_BACK;
	bool ret;

	mutex_enter(&nst->nst_lock);
	switch (proto) {
	case IPPROTO_TCP:
		/* Pass to TCP state tracking engine. */
		ret = npf_state_tcp(npc, nbuf, nst, di);
		break;
	case IPPROTO_UDP:
	case IPPROTO_ICMP:
		/* Generic. */
		nst->nst_state = npf_generic_fsm[nst->nst_state][di];
		ret = true;
		break;
	default:
		ret = false;
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
	int timeout = 0;

	switch (proto) {
	case IPPROTO_TCP:
		/* Pass to TCP state tracking engine. */
		timeout = npf_state_tcp_timeout(nst);
		break;
	case IPPROTO_UDP:
	case IPPROTO_ICMP:
		/* Generic. */
		timeout = npf_generic_timeout[state];
		break;
	default:
		KASSERT(false);
	}
	return timeout;
}

void
npf_state_dump(npf_state_t *nst)
{
#if defined(DDB) || defined(_NPF_TESTING)
	npf_tcpstate_t *fst = &nst->nst_tcpst[0], *tst = &nst->nst_tcpst[1];

	printf("\tstate (%p) %d:\n\t\t"
	    "F { end %u maxend %u mwin %u wscale %u }\n\t\t"
	    "T { end %u maxend %u mwin %u wscale %u }\n",
	    nst, nst->nst_state,
	    fst->nst_end, fst->nst_maxend, fst->nst_maxwin, fst->nst_wscale,
	    tst->nst_end, tst->nst_maxend, tst->nst_maxwin, tst->nst_wscale
	);
#endif
}
