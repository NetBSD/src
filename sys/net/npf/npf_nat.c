/*	$NetBSD: npf_nat.c,v 1.1 2010/08/22 18:56:22 rmind Exp $	*/

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
 * NPF network address port translation (NAPT).
 * Described in RFC 2663, RFC 3022.  Commonly just "NAT".
 *
 * Overview
 *
 *	There are few mechanisms: NAT policy, port map and translation.
 *	NAT module has a separate ruleset, where rules contain associated
 *	NAT policy, thus flexible filter criteria can be used.
 *
 * NAT policies and port maps
 *
 *	NAT policy is applied when a packet matches the rule.  Apart from
 *	filter criteria, NAT policy has a translation (gateway) IP address
 *	and associated port map.  Port map is a bitmap used to reserve and
 *	use unique TCP/UDP ports for translation.  Port maps are unique to
 *	the IP addresses, therefore multiple NAT policies with the same IP
 *	will share the same port map.
 *
 * NAT sessions and translation entries
 *
 *	NAT module relies on session management module.  Each "NAT" session
 *	has an associated translation entry (npf_nat_t).  It contains local
 *	i.e. original IP address with port and translation port, allocated
 *	from the port map.  Each NAT translation entry is associated with
 *	the policy, which contains translation IP address.  Allocated port
 *	is returned to the port map and translation entry destroyed when
 *	"NAT" session expires.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_nat.c,v 1.1 2010/08/22 18:56:22 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#endif

#include <sys/atomic.h>
#include <sys/bitops.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <net/pfil.h>
#include <netinet/in.h>

#include "npf_impl.h"

/*
 * NPF portmap structure.
 */
typedef struct {
	u_int				p_refcnt;
	uint32_t			p_bitmap[0];
} npf_portmap_t;

/* Portmap range: [ 1024 .. 65535 ] */
#define	PORTMAP_FIRST			(1024)
#define	PORTMAP_SIZE			((65536 - PORTMAP_FIRST) / 32)
#define	PORTMAP_FILLED			((uint32_t)~0)
#define	PORTMAP_MASK			(31)
#define	PORTMAP_SHIFT			(5)

/* NAT policy structure. */
struct npf_natpolicy {
	LIST_ENTRY(npf_natpolicy)	n_entry;
	in_addr_t			n_gw_ip;
	npf_portmap_t *			n_portmap;
};

/* NAT translation entry for a session. */ 
struct npf_nat {
	npf_natpolicy_t *		nt_natpolicy;
	/* Local address and port (for backwards translation). */
	in_addr_t			nt_laddr;
	in_port_t			nt_lport;
	/* Translation port (for forwards). */
	in_port_t			nt_tport;
	/* ALG (if any) associated with this NAT entry. */
	npf_alg_t *			nt_alg;
	uintptr_t			nt_alg_arg;
};

static npf_ruleset_t *			nat_ruleset;
static LIST_HEAD(, npf_natpolicy)	nat_policy_list;
static pool_cache_t			nat_cache;

/*
 * npf_nat_sys{init,fini}: initialise/destroy NAT subsystem structures.
 */

void
npf_nat_sysinit(void)
{

	nat_cache = pool_cache_init(sizeof(npf_nat_t), coherency_unit,
	    0, 0, "npfnatpl", NULL, IPL_NET, NULL, NULL, NULL);
	KASSERT(nat_cache != NULL);
	nat_ruleset = npf_ruleset_create();
	LIST_INIT(&nat_policy_list);
}

void
npf_nat_sysfini(void)
{

	/* Flush NAT policies. */
	npf_nat_reload(NULL);
	KASSERT(LIST_EMPTY(&nat_policy_list));
	pool_cache_destroy(nat_cache);
}

/*
 * npf_nat_newpolicy: allocate a new NAT policy.
 *
 * => Shares portmap if policy is on existing translation address.
 * => XXX: serialise at upper layer.
 */
npf_natpolicy_t *
npf_nat_newpolicy(in_addr_t gip)
{
	npf_natpolicy_t *np, *it;
	npf_portmap_t *pm;

	np = kmem_zalloc(sizeof(npf_natpolicy_t), KM_SLEEP);
	if (np == NULL) {
		return NULL;
	}
	np->n_gw_ip = gip;

	/* Search for a NAT policy using the same translation address. */
	pm = NULL;
	LIST_FOREACH(it, &nat_policy_list, n_entry) {
		if (it->n_gw_ip != np->n_gw_ip)
			continue;
		pm = it->n_portmap;
		break;
	}
	if (pm == NULL) {
		/* Allocate a new port map for the NAT policy. */
		pm = kmem_zalloc(sizeof(npf_portmap_t) +
		    (PORTMAP_SIZE * sizeof(uint32_t)), KM_SLEEP);
		if (pm == NULL) {
			kmem_free(np, sizeof(npf_natpolicy_t));
			return NULL;
		}
		pm->p_refcnt = 1;
		KASSERT((uintptr_t)pm->p_bitmap == (uintptr_t)pm + sizeof(*pm));
	} else {
		/* Share the port map. */
		pm->p_refcnt++;
	}
	np->n_portmap = pm;
	/*
	 * Note: old policies with new might co-exist in the list,
	 * while reload is in progress, but that is not an issue.
	 */
	LIST_INSERT_HEAD(&nat_policy_list, np, n_entry);
	return np;
}

/*
 * npf_nat_freepolicy: free NAT policy and, on last reference, free portmap.
 *
 * => Called from npf_rule_free() during the reload via npf_nat_reload().
 */
void
npf_nat_freepolicy(npf_natpolicy_t *np)
{
	npf_portmap_t *pm = np->n_portmap;

	LIST_REMOVE(np, n_entry);
	if (--pm->p_refcnt == 0) {
		kmem_free(pm, sizeof(npf_portmap_t) +
		    (PORTMAP_SIZE * sizeof(uint32_t)));
	}
	kmem_free(np, sizeof(npf_natpolicy_t));
}

/*
 * npf_nat_reload: activate new ruleset of NAT policies and destroy old.
 *
 * => Destruction of ruleset will perform npf_nat_freepolicy() for each policy.
 */
void
npf_nat_reload(npf_ruleset_t *nset)
{
	npf_ruleset_t *oldnset;

	oldnset = atomic_swap_ptr(&nat_ruleset, nset);
	if (oldnset) {
		npf_ruleset_destroy(oldnset);
	}
}

/*
 * npf_nat_getport: allocate and return a port in the NAT policy portmap.
 *
 * => Returns in network byte-order.
 * => Zero indicates failure.
 */
static in_port_t
npf_nat_getport(npf_natpolicy_t *np)
{
	npf_portmap_t *pm = np->n_portmap;
	u_int n = PORTMAP_SIZE, idx, bit;
	uint32_t map, nmap;

	idx = arc4random() % PORTMAP_SIZE;
	for (;;) {
		KASSERT(idx < PORTMAP_SIZE);
		map = pm->p_bitmap[idx];
		if (__predict_false(map == PORTMAP_FILLED)) {
			if (n-- == 0) {
				/* No space. */
				return 0;
			}
			/* This bitmap is sfilled, next. */
			idx = (idx ? idx : PORTMAP_SIZE) - 1;
			continue;
		}
		bit = ffs32(~map) - 1;
		nmap = map | (1 << bit);
		if (atomic_cas_32(&pm->p_bitmap[idx], map, nmap) == map) {
			/* Success. */
			break;
		}
	}
	return htons(PORTMAP_FIRST + (idx << PORTMAP_SHIFT) + bit);
}

/*
 * npf_nat_putport: return port as available in the NAT policy portmap.
 *
 * => Port should be in network byte-order.
 */
static void
npf_nat_putport(npf_natpolicy_t *np, in_port_t port)
{
	npf_portmap_t *pm = np->n_portmap;
	uint32_t map, nmap;
	u_int idx, bit;

	port = ntohs(port) - PORTMAP_FIRST;
	idx = port >> PORTMAP_SHIFT;
	bit = port & PORTMAP_MASK;
	do {
		map = pm->p_bitmap[idx];
		KASSERT(map | (1 << bit));
		nmap = map & ~(1 << bit);
	} while (atomic_cas_32(&pm->p_bitmap[idx], map, nmap) != map);
}

/*
 * npf_natout:
 *	- Inspect packet for a NAT policy, unless session with NAT
 *	  association already exists.
 *	- Perform "forwards" translation: rewrite source address, etc.
 *	- Establish sessions or if already exists, associate NAT policy.
 */
int
npf_natout(npf_cache_t *npc, npf_session_t *se, nbuf_t *nbuf,
    struct ifnet *ifp, const int layer)
{
	const int proto = npc->npc_proto;
	void *n_ptr = nbuf_dataptr(nbuf);
	npf_session_t *nse = NULL; /* XXXgcc */
	npf_natpolicy_t *np;
	npf_nat_t *nt;
	npf_rule_t *rl;
	in_addr_t gwip;
	in_port_t tport;
	int error;
	bool new;

	/* All relevant IPv4 data should be already cached. */
	if (!npf_iscached(npc, NPC_IP46 | NPC_ADDRS)) {
		return 0;
	}

	/* Detect if there is a linked session pointing to the NAT entry. */
	nt = se ? npf_session_retlinknat(se) : NULL;
	if (nt) {
		np = nt->nt_natpolicy;
		new = false;
		goto skip;
	}

	/* Inspect packet against NAT ruleset, return a policy. */
	rl = npf_ruleset_match(nat_ruleset, npc, nbuf, ifp, PFIL_OUT, layer);
	np = rl ? npf_rule_getnat(rl) : NULL;
	if (np == NULL) {
		/* If packet does not match - done. */
		return 0;
	}

	/* New NAT association. */
	nt = pool_cache_get(nat_cache, PR_NOWAIT);
	if (nt == NULL){
		return ENOMEM;
	}
	nt->nt_natpolicy = np;
	nt->nt_alg = NULL;
	new = true;

	/* Save local (source) address. */
	nt->nt_laddr = npc->npc_srcip;

	if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
		/* Also, save local TCP/UDP port. */
		KASSERT(npf_iscached(npc, NPC_PORTS));
		nt->nt_lport = npc->npc_sport;
		/* Get a new port for translation. */
		nt->nt_tport = npf_nat_getport(np);
	} else {
		nt->nt_lport = 0;
		nt->nt_tport = 0;
	}

	/* Match any ALGs. */
	npf_alg_exec(npc, nbuf, nt, PFIL_OUT);

	/* If there is no local session, establish one before translation. */
	if (se == NULL) {
		nse = npf_session_establish(npc, NULL, PFIL_OUT);
		if (nse == NULL) {
			error = ENOMEM;
			goto out;
		}
		se = nse;
	} else {
		nse = NULL;
	}
skip:
	if (layer == NPF_LAYER_2 && /* XXX */
	    (n_ptr = nbuf_advance(&nbuf, n_ptr, npc->npc_elen)) == NULL)
		return EINVAL;

	/* Execute ALG hooks first. */
	npf_alg_exec(npc, nbuf, nt, PFIL_OUT);

	gwip = np->n_gw_ip;
	tport = nt->nt_tport;

	/*
	 * Perform translation: rewrite source address et al.
	 * Note: cache may be used in npf_rwrport(), update only in the end.
	 */
	if (!npf_rwrip(npc, nbuf, n_ptr, PFIL_OUT, gwip)) {
		error = EINVAL;
		goto out;
	}
	if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
		KASSERT(tport != 0);
		if (!npf_rwrport(npc, nbuf, n_ptr, PFIL_OUT, tport, gwip)) {
			error = EINVAL;
			goto out;
		}
	}
	/* Success: cache new address and port (if any). */
	npc->npc_srcip = gwip;
	npc->npc_sport = tport;
	error = 0;

	if (__predict_false(new)) {
		npf_session_t *natse;
		/*
		 * Establish a new NAT session using translated address and
		 * associate NAT translation data with this session.
		 *
		 * Note: packet now has a translated address in the cache.
		 */
		natse = npf_session_establish(npc, nt, PFIL_OUT);
		if (natse == NULL) {
			error = ENOMEM;
			goto out;
		}
		/*
		 * Link local session with NAT session, if no link already.
		 */
		npf_session_link(se, natse);
		npf_session_release(natse);
out:
		if (error) {
			if (nse != NULL) {
				/* XXX: expire local session if new? */
			}
			/* Will free the structure and return the port. */
			npf_nat_expire(nt);
		}
		if (nse != NULL) {
			/* Drop the reference local session was new. */
			npf_session_release(nse);
		}
	}
	return error;
}

/*
 * npf_natin:
 *	- Inspect packet for a session with associated NAT policy.
 *	- Perform "backwards" translation: rewrite destination address, etc.
 */
int
npf_natin(npf_cache_t *npc, npf_session_t *se, nbuf_t *nbuf, const int layer)
{
	npf_nat_t *nt = se ? npf_session_retnat(se) : NULL;

	if (nt == NULL) {
		/* No association - no translation. */
		return 0;
	}
	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_ADDRS));

	void *n_ptr = nbuf_dataptr(nbuf);
	in_addr_t laddr = nt->nt_laddr;
	in_port_t lport = nt->nt_lport;

	if (layer == NPF_LAYER_2) {
		n_ptr = nbuf_advance(&nbuf, n_ptr, npc->npc_elen);
		if (n_ptr == NULL) {
			return EINVAL;
		}
	}

	/* Execute ALG hooks first. */
	npf_alg_exec(npc, nbuf, nt, PFIL_IN);

	/*
	 * Address translation: rewrite destination address.
	 * Note: cache will be used in npf_rwrport(), update only in the end.
	 */
	if (!npf_rwrip(npc, nbuf, n_ptr, PFIL_IN, laddr)) {
		return EINVAL;
	}
	switch (npc->npc_proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_PORTS));
		/* Rewrite destination port. */
		if (!npf_rwrport(npc, nbuf, n_ptr, PFIL_IN, lport, laddr)) {
			return EINVAL;
		}
		break;
	case IPPROTO_ICMP:
		/* None. */
		break;
	default:
		return ENOTSUP;
	}
	/* Cache new address and port. */
	npc->npc_dstip = laddr;
	npc->npc_dport = lport;
	return 0;
}

/*
 * npf_nat_getlocal: return local IP address and port from translation entry.
 */
void
npf_nat_getlocal(npf_nat_t *nt, in_addr_t *addr, in_port_t *port)
{

	*addr = nt->nt_laddr;
	*port = nt->nt_lport;
}

void
npf_nat_setalg(npf_nat_t *nt, npf_alg_t *alg, uintptr_t arg)
{

	nt->nt_alg = alg;
	nt->nt_alg_arg = arg;
}

/*
 * npf_nat_expire: free NAT-related data structures on session expiration.
 */
void
npf_nat_expire(npf_nat_t *nt)
{

	if (nt->nt_tport) {
		npf_natpolicy_t *np = nt->nt_natpolicy;
		npf_nat_putport(np, nt->nt_tport);
	}
	pool_cache_put(nat_cache, nt);
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_nat_dump(npf_nat_t *nt)
{
	npf_natpolicy_t *np;
	struct in_addr ip;

	if (nt) {
		np = nt->nt_natpolicy;
		goto skip;
	}
	LIST_FOREACH(np, &nat_policy_list, n_entry) {
skip:
		ip.s_addr = np->n_gw_ip;
		printf("\tNAT policy: gw_ip = %s\n", inet_ntoa(ip));
		if (nt == NULL) {
			continue;
		}
		ip.s_addr = nt->nt_laddr;
		printf("\tNAT: original address %s, lport %d, tport = %d\n",
		    inet_ntoa(ip), ntohs(nt->nt_lport), ntohs(nt->nt_tport));
		if (nt->nt_alg) {
			printf("\tNAT ALG = %p, ARG = %p\n",
			    nt->nt_alg, (void *)nt->nt_alg_arg);
		}
		return;
	}
}

#endif
