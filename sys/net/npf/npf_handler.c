/*	$NetBSD: npf_handler.c,v 1.7.6.1 2011/11/10 14:31:50 yamt Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
 * NPF packet handler.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_handler.c,v 1.7.6.1 2011/11/10 14:31:50 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <net/if.h>
#include <net/pfil.h>
#include <sys/socketvar.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include "npf_impl.h"

/*
 * If npf_ph_if != NULL, pfil hooks are registers.  If NULL, not registered.
 * Used to check the state.  Locked by: softnet_lock + KERNEL_LOCK (XXX).
 */
static struct pfil_head *	npf_ph_if = NULL;
static struct pfil_head *	npf_ph_inet = NULL;
static struct pfil_head *	npf_ph_inet6 = NULL;

static bool			default_pass = true;

int	npf_packet_handler(void *, struct mbuf **, ifnet_t *, int);

/*
 * npf_ifhook: hook handling interface changes.
 */
static int
npf_ifhook(void *arg, struct mbuf **mp, ifnet_t *ifp, int di)
{

	return 0;
}

/*
 * npf_packet_handler: main packet handling routine for layer 3.
 *
 * Note: packet flow and inspection logic is in strict order.
 */
int
npf_packet_handler(void *arg, struct mbuf **mp, ifnet_t *ifp, int di)
{
	nbuf_t *nbuf = *mp;
	npf_cache_t npc;
	npf_session_t *se;
	npf_ruleset_t *rlset;
	npf_rule_t *rl;
	npf_rproc_t *rp;
	int retfl, error, ret;

	/*
	 * Initialise packet information cache.
	 * Note: it is enough to clear the info bits.
	 */
	npc.npc_info = 0;
	error = 0;
	retfl = 0;
	rp = NULL;
	ret = 0;

	/* Cache everything.  Determine whether it is an IP fragment. */
	npf_cache_all(&npc, nbuf);

	if (npf_iscached(&npc, NPC_IPFRAG)) {
		/* Pass to IPv4 or IPv6 reassembly mechanism. */
		if (npf_iscached(&npc, NPC_IP4)) {
			struct ip *ip = nbuf_dataptr(*mp);
			ret = ip_reass_packet(mp, ip);
		} else {
			KASSERT(npf_iscached(&npc, NPC_IP6));
#ifdef INET6
			/*
			 * Note: frag6_input() offset is the start of the
			 * fragment header.
			 */
			size_t hlen = npf_cache_hlen(&npc, nbuf);
			ret = ip6_reass_packet(mp, hlen);
#else
			ret = -1;
#endif
		}

		if (ret) {
			error = EINVAL;
			se = NULL;
			goto out;
		}
		if (*mp == NULL) {
			/* More fragments should come; return. */
			return 0;
		}

		/*
		 * Reassembly is complete, we have the final packet.
		 * Cache again, since layer 3 daya is accessible now.
		 */
		nbuf = (nbuf_t *)*mp;
		npc.npc_info = 0;
		npf_cache_all(&npc, nbuf);
	}

	/* Inspect the list of sessions. */
	se = npf_session_inspect(&npc, nbuf, di);

	/* If "passing" session found - skip the ruleset inspection. */
	if (se && npf_session_pass(se, &rp)) {
		npf_stats_inc(NPF_STAT_PASS_SESSION);
		goto pass;
	}

	/* Acquire the lock, inspect the ruleset using this packet. */
	npf_core_enter();
	rlset = npf_core_ruleset();
	rl = npf_ruleset_inspect(&npc, nbuf, rlset, ifp, di, NPF_LAYER_3);
	if (rl == NULL) {
		if (default_pass) {
			npf_stats_inc(NPF_STAT_PASS_DEFAULT);
			goto pass;
		}
		npf_stats_inc(NPF_STAT_BLOCK_DEFAULT);
		error = ENETUNREACH;
		goto block;
	}

	/* Get rule procedure for assocation and/or execution. */
	KASSERT(rp == NULL);
	rp = npf_rproc_return(rl);

	/* Apply the rule, release the lock. */
	error = npf_rule_apply(&npc, nbuf, rl, &retfl);
	if (error) {
		npf_stats_inc(NPF_STAT_BLOCK_RULESET);
		goto block;
	}
	npf_stats_inc(NPF_STAT_PASS_RULESET);

	/* Establish a "pass" session, if required. */
	if ((retfl & NPF_RULE_KEEPSTATE) != 0 && !se) {
		se = npf_session_establish(&npc, nbuf, di);
		if (se == NULL) {
			error = ENOMEM;
			goto out;
		}
		npf_session_setpass(se, rp);
	}
pass:
	KASSERT(error == 0);
	/*
	 * Perform NAT.
	 */
	error = npf_do_nat(&npc, se, nbuf, ifp, di);
block:
	/*
	 * Perform rule procedure, if any.
	 */
	if (rp) {
		npf_rproc_run(&npc, nbuf, rp, error);
	}
out:
	/* Release the reference on session, or rule procedure. */
	if (se) {
		npf_session_release(se);
	} else if (rp) {
		npf_rproc_release(rp); /* XXXkmem */
	}

	/*
	 * If error is set - drop the packet.
	 * Normally, ENETUNREACH is used for "block".
	 */
	if (error) {
		/*
		 * Depending on flags and protocol, return TCP reset (RST)
		 * or ICMP destination unreachable
		 */
		if (retfl) {
			npf_return_block(&npc, nbuf, retfl);
		}
		if (error != ENETUNREACH) {
			NPF_PRINTF(("NPF: error in handler '%d'\n", error));
			npf_stats_inc(NPF_STAT_ERROR);
		}
		m_freem(*mp);
		*mp = NULL;
	} else {
		/*
		 * XXX: Disable for now, it will be set accordingly later,
		 * for optimisations (to reduce inspection).
		 */
		(*mp)->m_flags &= ~M_CANFASTFWD;
	}
	return error;
}

/*
 * npf_register_pfil: register pfil(9) hooks.
 */
int
npf_register_pfil(void)
{
	int error;

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);

	/* Check if pfil hooks are not already registered. */
	if (npf_ph_if) {
		error = EEXIST;
		goto fail;
	}

	/* Capture point of any activity in interfaces and IP layer. */
	npf_ph_if = pfil_head_get(PFIL_TYPE_IFNET, 0);
	npf_ph_inet = pfil_head_get(PFIL_TYPE_AF, AF_INET);
	npf_ph_inet6 = pfil_head_get(PFIL_TYPE_AF, AF_INET6);
	if (npf_ph_if == NULL || npf_ph_inet == NULL) {
		npf_ph_if = NULL;
		error = ENOENT;
		goto fail;
	}

	/* Interface re-config or attach/detach hook. */
	error = pfil_add_hook(npf_ifhook, NULL,
	    PFIL_WAITOK | PFIL_IFADDR | PFIL_IFNET, npf_ph_if);
	KASSERT(error == 0);

	/* Packet IN/OUT handler on all interfaces and IP layer. */
	error = pfil_add_hook(npf_packet_handler, NULL,
	    PFIL_WAITOK | PFIL_ALL, npf_ph_inet);
	KASSERT(error == 0);

	error = pfil_add_hook(npf_packet_handler, NULL,
	    PFIL_WAITOK | PFIL_ALL, npf_ph_inet6);
	KASSERT(error == 0);
fail:
	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);

	return error;
}

/*
 * npf_unregister: unregister pfil(9) hooks.
 */
void
npf_unregister_pfil(void)
{

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);

	if (npf_ph_if) {
		(void)pfil_remove_hook(npf_packet_handler, NULL,
		    PFIL_ALL, npf_ph_inet6);
		(void)pfil_remove_hook(npf_packet_handler, NULL,
		    PFIL_ALL, npf_ph_inet);
		(void)pfil_remove_hook(npf_ifhook, NULL,
		    PFIL_IFADDR | PFIL_IFNET, npf_ph_if);

		npf_ph_if = NULL;
	}

	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);
}
