/*	$NetBSD: npf_nat.c,v 1.3 2010/11/11 06:30:39 rmind Exp $	*/

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
 * Translation types
 *
 *	There are two types of translation: outbound (NPF_NATOUT) and
 *	inbound (NPF_NATIN).  It should not be confused with connection
 *	direction.
 *
 *	Outbound NAT rewrites:
 *	- Source on "forwards" stream.
 *	- Destination on "backwards" stream.
 *	Inbound NAT rewrites:
 *	- Destination on "forwards" stream.
 *	- Source on "backwards" stream.
 *
 *	It should be noted that bi-directional NAT is a combined outbound
 *	and inbound translation, therefore constructed as two policies.
 *
 * NAT policies and port maps
 *
 *	NAT (translation) policy is applied when a packet matches the rule.
 *	Apart from filter criteria, NAT policy has a translation IP address
 *	and associated port map.  Port map is a bitmap used to reserve and
 *	use unique TCP/UDP ports for translation.  Port maps are unique to
 *	the IP addresses, therefore multiple NAT policies with the same IP
 *	will share the same port map.
 *
 * NAT sessions and translation entries
 *
 *	NAT module relies on session management module.  Each "NAT" session
 *	has an associated translation entry (npf_nat_t).  It contains saved
 *	i.e. original IP address with port and translation port, allocated
 *	from the port map.  Each NAT translation entry is associated with
 *	the policy, which contains translation IP address.  Allocated port
 *	is returned to the port map and translation entry destroyed when
 *	"NAT" session expires.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_nat.c,v 1.3 2010/11/11 06:30:39 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

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
	int				n_type;
	int				n_flags;
	npf_portmap_t *			n_portmap;
	size_t				n_addr_sz;
	npf_addr_t			n_taddr;
	in_port_t			n_tport;
};

/* NAT translation entry for a session. */ 
struct npf_nat {
	npf_natpolicy_t *		nt_natpolicy;
	/* Original address and port (for backwards translation). */
	npf_addr_t			nt_oaddr;
	in_port_t			nt_oport;
	/* Translation port (for redirects). */
	in_port_t			nt_tport;
	/* ALG (if any) associated with this NAT entry. */
	npf_alg_t *			nt_alg;
	uintptr_t			nt_alg_arg;
};

static npf_ruleset_t *			nat_ruleset	__read_mostly;
static LIST_HEAD(, npf_natpolicy)	nat_policy_list	__read_mostly;
static pool_cache_t			nat_cache	__read_mostly;

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
 * npf_nat_newpolicy: create a new NAT policy.
 *
 * => Shares portmap if policy is on existing translation address.
 * => XXX: serialise at upper layer.
 */
npf_natpolicy_t *
npf_nat_newpolicy(int type, int flags, const npf_addr_t *taddr,
    size_t addr_sz, in_port_t tport)
{
	npf_natpolicy_t *np, *it;
	npf_portmap_t *pm;

	np = kmem_zalloc(sizeof(npf_natpolicy_t), KM_SLEEP);
	if (np == NULL) {
		return NULL;
	}
	KASSERT(type == NPF_NATIN || type == NPF_NATOUT);
	np->n_type = type;
	np->n_flags = flags;
	np->n_addr_sz = addr_sz;
	memcpy(&np->n_taddr, taddr, sizeof(npf_addr_t));
	np->n_tport = tport;

	pm = NULL;
	if ((flags & NPF_NAT_PORTMAP) == 0) {
		goto nopm;
	}

	/* Search for a NAT policy using the same translation address. */
	LIST_FOREACH(it, &nat_policy_list, n_entry) {
		if (memcmp(&it->n_taddr, &np->n_taddr, sizeof(npf_addr_t))) {
			continue;
		}
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
nopm:
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
	if (pm && --pm->p_refcnt == 0) {
		KASSERT((np->n_flags & NPF_NAT_PORTMAP) != 0);
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
	KASSERT(oldnset != NULL);
	npf_ruleset_destroy(oldnset);
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
			/* This bitmap is filled, next. */
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
 * npf_nat_inspect: inspect packet against NAT ruleset and return a policy.
 */
static npf_natpolicy_t *
npf_nat_inspect(npf_cache_t *npc, nbuf_t *nbuf, struct ifnet *ifp, const int di)
{
	npf_rule_t *rl;

	rl = npf_ruleset_match(nat_ruleset, npc, nbuf, ifp, di, NPF_LAYER_3);

	return rl ? npf_rule_getnat(rl) : NULL;
}

/*
 * npf_nat_create: create a new NAT translation entry.
 */
static npf_nat_t *
npf_nat_create(npf_cache_t *npc, npf_natpolicy_t *np)
{
	const int proto = npf_cache_ipproto(npc);
	npf_nat_t *nt;

	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_LAYER4));

	/* New NAT association. */
	nt = pool_cache_get(nat_cache, PR_NOWAIT);
	if (nt == NULL){
		return NULL;
	}
	nt->nt_natpolicy = np;
	nt->nt_alg = NULL;

	/* Save the original address which may be rewritten. */
	if (np->n_type == NPF_NATOUT) {
		/* Source (local) for Outbound NAT. */
		memcpy(&nt->nt_oaddr, npc->npc_srcip, npc->npc_ipsz);
	} else {
		/* Destination (external) for Inbound NAT. */
		KASSERT(np->n_type == NPF_NATIN);
		memcpy(&nt->nt_oaddr, npc->npc_dstip, npc->npc_ipsz);
	}

	/*
	 * Port translation, if required, and if it is TCP/UDP.
	 */
	if ((np->n_flags & NPF_NAT_PORTS) == 0 ||
	    (proto != IPPROTO_TCP && proto != IPPROTO_UDP)) {
		nt->nt_oport = 0;
		nt->nt_tport = 0;
		return nt;
	}
	/* Save the relevant TCP/UDP port. */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &npc->npc_l4.tcp;
		nt->nt_oport = (np->n_type == NPF_NATOUT) ?
		    th->th_sport : th->th_dport;
	} else {
		struct udphdr *uh = &npc->npc_l4.udp;
		nt->nt_oport = (np->n_type == NPF_NATOUT) ?
		    uh->uh_sport : uh->uh_dport;
	}

	/* Get a new port for translation. */
	if ((np->n_flags & NPF_NAT_PORTMAP) != 0) {
		nt->nt_tport = npf_nat_getport(np);
	} else {
		nt->nt_tport = np->n_tport;
	}
	return nt;
}

/*
 * npf_nat_translate: perform address and/or port translation.
 */
static int
npf_nat_translate(npf_cache_t *npc, nbuf_t *nbuf, npf_nat_t *nt,
    const bool forw, const int di)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	npf_natpolicy_t *np = nt->nt_natpolicy;
	npf_addr_t *addr;
	in_port_t port;

	KASSERT(npf_iscached(npc, NPC_IP46));

	if (forw) {
		/* "Forwards" stream: use translation address/port. */
		KASSERT(
		    (np->n_type == NPF_NATIN && di == PFIL_IN) ^
		    (np->n_type == NPF_NATOUT && di == PFIL_OUT)
		);
		addr = &np->n_taddr;
		port = nt->nt_tport;
	} else {
		/* "Backwards" stream: use original address/port. */
		KASSERT(
		    (np->n_type == NPF_NATIN && di == PFIL_OUT) ^
		    (np->n_type == NPF_NATOUT && di == PFIL_IN)
		);
		addr = &nt->nt_oaddr;
		port = nt->nt_oport;
	}

	/* Execute ALG hook first. */
	npf_alg_exec(npc, nbuf, nt, di);

	/*
	 * Rewrite IP and/or TCP/UDP checksums first, since it will use
	 * the cache containing original values for checksum calculation.
	 */
	if (!npf_rwrcksum(npc, nbuf, n_ptr, di, addr, port)) {
		return EINVAL;
	}
	/*
	 * Address translation: rewrite source/destination address, depending
	 * on direction (PFIL_OUT - for source, PFIL_IN - for destination).
	 */
	if (!npf_rwrip(npc, nbuf, n_ptr, di, addr)) {
		return EINVAL;
	}
	if ((np->n_flags & NPF_NAT_PORTS) == 0) {
		/* Done. */
		return 0;
	}
	switch (npf_cache_ipproto(npc)) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_TCP | NPC_UDP));
		/* Rewrite source/destination port. */
		if (!npf_rwrport(npc, nbuf, n_ptr, di, port)) {
			return EINVAL;
		}
		break;
	case IPPROTO_ICMP:
		KASSERT(npf_iscached(npc, NPC_ICMP));
		/* Nothing. */
		break;
	default:
		return ENOTSUP;
	}
	return 0;
}

/*
 * npf_do_nat:
 *	- Inspect packet for a NAT policy, unless a session with a NAT
 *	  association already exists.  In such case, determine whether is
 *	  is a "forwards" or "backwards" stream.
 *	- Perform translation: rewrite source address if "forwards" stream
 *	  and destination address if "backwards".
 *	- Establish sessions or, if already exists, associate a NAT policy.
 */
int
npf_do_nat(npf_cache_t *npc, npf_session_t *se, nbuf_t *nbuf,
    struct ifnet *ifp, const int di)
{
	npf_session_t *nse = NULL;
	npf_natpolicy_t *np;
	npf_nat_t *nt;
	int error;
	bool forw, new;

	/* All relevant IPv4 data should be already cached. */
	if (!npf_iscached(npc, NPC_IP46) || !npf_iscached(npc, NPC_LAYER4)) {
		return 0;
	}

	/*
	 * Return the NAT entry associated with the session, if any.
	 * Determines whether the stream is "forwards" or "backwards".
	 */
	if (se && (nt = npf_session_retnat(se, di, &forw)) != NULL) {
		np = nt->nt_natpolicy;
		new = false;
		goto translate;
	}

	/* Inspect the packet for a NAT policy, if there is no session. */
	np = npf_nat_inspect(npc, nbuf, ifp, di);
	if (np == NULL) {
		/* If packet does not match - done. */
		return 0;
	}
	forw = true;

	/* Create a new NAT translation entry. */
	nt = npf_nat_create(npc, np);
	if (nt == NULL) {
		return ENOMEM;
	}
	new = true;

	/* Determine whether any ALG matches. */
	if (npf_alg_match(npc, nbuf, nt)) {
		KASSERT(nt->nt_alg != NULL);
	}

	/*
	 * If there is no local session (no "keep state" rule - unusual, but
	 * possible configuration), establish one before translation.  Note
	 * that it is not a "pass" session, therefore passing of "backwards"
	 * stream depends on other, stateless filtering rules.
	 */
	if (se == NULL) {
		nse = npf_session_establish(npc, nbuf, NULL, di);
		if (nse == NULL) {
			error = ENOMEM;
			goto out;
		}
		se = nse;
	}
translate:
	/* Perform the translation. */
	error = npf_nat_translate(npc, nbuf, nt, forw, di);
	if (error) {
		goto out;
	}

	if (__predict_false(new)) {
		npf_session_t *natse;
		/*
		 * Establish a new NAT session using translated address and
		 * associate NAT translation data with this session.
		 *
		 * Note: packet now has a translated address in the cache.
		 */
		natse = npf_session_establish(npc, nbuf, nt, di);
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
				/* XXX: Expire it?? */
			}
			/* Will free the structure and return the port. */
			npf_nat_expire(nt);
		}
		if (nse != NULL) {
			npf_session_release(nse);
		}
	}
	return error;
}

/*
 * npf_nat_getorig: return original IP address and port from translation entry.
 */
void
npf_nat_getorig(npf_nat_t *nt, npf_addr_t **addr, in_port_t *port)
{

	*addr = &nt->nt_oaddr;
	*port = nt->nt_oport;
}

/*
 * npf_nat_setalg: associate an ALG with the NAT entry.
 */
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
	npf_natpolicy_t *np = nt->nt_natpolicy;

	if ((np->n_flags & NPF_NAT_PORTMAP) != 0) {
		KASSERT(nt->nt_tport != 0);
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
		memcpy(&ip, &np->n_taddr, sizeof(ip));
		printf("\tNAT policy: type %d, flags 0x%x, taddr %s, tport = %d\n",
		    np->n_type, np->n_flags, inet_ntoa(ip), np->n_tport);
		if (nt == NULL) {
			continue;
		}
		memcpy(&ip, &nt->nt_oaddr, sizeof(ip));
		printf("\tNAT: original address %s, oport %d, tport = %d\n",
		    inet_ntoa(ip), ntohs(nt->nt_oport), ntohs(nt->nt_tport));
		if (nt->nt_alg) {
			printf("\tNAT ALG = %p, ARG = %p\n",
			    nt->nt_alg, (void *)nt->nt_alg_arg);
		}
		return;
	}
}

#endif
