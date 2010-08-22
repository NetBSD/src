/*	$NetBSD: npf_handler.c,v 1.1 2010/08/22 18:56:22 rmind Exp $	*/

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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_handler.c,v 1.1 2010/08/22 18:56:22 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#endif

#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <net/if.h>
#include <net/pfil.h>
#include <sys/socketvar.h>

#include "npf_impl.h"

/*
 * If npf_ph_if != NULL, pfil hooks are registers.  If NULL, not registered.
 * Used to check the state.  Locked by: softnet_lock + KERNEL_LOCK (XXX).
 */
static struct pfil_head *	npf_ph_if = NULL;
static struct pfil_head *	npf_ph_inet = NULL;

int	npf_packet_handler(void *, struct mbuf **, struct ifnet *, int);

/*
 * npf_ifhook: hook handling interface changes.
 */
static int
npf_ifhook(void *arg, struct mbuf **mp, struct ifnet *ifp, int di)
{

	return 0;
}

/*
 * npf_packet_handler: main packet handling routine.
 *
 * Note: packet flow and inspection logic is in strict order.
 */
int
npf_packet_handler(void *arg, struct mbuf **mp, struct ifnet *ifp, int di)
{
	const int layer = (const int)(long)arg;
	nbuf_t *nbuf = *mp;
	npf_cache_t npc;
	npf_session_t *se;
	npf_rule_t *rl;
	int error;

	/*
	 * Initialise packet information cache.
	 * Note: it is enough to clear the info bits.
	 */
	npc.npc_info = 0;

	/* Inspect the list of sessions. */
	se = npf_session_inspect(&npc, nbuf, ifp, di, layer);

	/* Inbound NAT. */
	if ((di & PFIL_IN) && (error = npf_natin(&npc, se, nbuf, layer)) != 0) {
		goto out;
	}

	/* If session found - we pass this packet. */
	if (se && npf_session_pass(se)) {
		error = 0;
	} else {
		/* Inspect ruleset using this packet. */
		rl = npf_ruleset_inspect(&npc, nbuf, ifp, di, layer);
		if (rl != NULL) {
			bool keepstate;
			/* Apply the rule. */
			error = npf_rule_apply(&npc, rl, &keepstate);
			if (error) {
				goto out;
			}
			/* Establish a session, if required. */
			if (keepstate) {
				se = npf_session_establish(&npc, NULL, di);
			}
		}
		/* No rules or "default" rule - pass. */
	}

	/* Outbound NAT. */
	if (di & PFIL_OUT) {
		error = npf_natout(&npc, se, nbuf, ifp, layer);
	}
out:
	/* Release reference on session. */
	if (se != NULL) {
		npf_session_release(se);
	}

	/*
	 * If error is set - drop the packet.
	 * Normally, ENETUNREACH is used to "block".
	 */
	if (error) {
		m_freem(*mp);
		*mp = NULL;
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
	error = pfil_add_hook(npf_packet_handler, (void *)NPF_LAYER_3,
	    PFIL_WAITOK | PFIL_ALL, npf_ph_inet);
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
		(void)pfil_remove_hook(npf_packet_handler, (void *)NPF_LAYER_3,
		    PFIL_ALL, npf_ph_inet);
		(void)pfil_remove_hook(npf_ifhook, NULL,
		    PFIL_IFADDR | PFIL_IFNET, npf_ph_if);

		npf_ph_if = NULL;
	}

	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);
}
