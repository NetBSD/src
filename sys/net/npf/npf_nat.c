/*	$NetBSD: npf_nat.c,v 1.12 2012/03/11 18:27:59 rmind Exp $	*/

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
 * Sessions, translation entries and their life-cycle
 *
 *	NAT module relies on session management module.  Each translated
 *	session has an associated translation entry (npf_nat_t), which
 *	contains information used for backwards stream translation, i.e.
 *	original IP address with port and translation port, allocated from
 *	the port map.  Each NAT entry is associated with the policy, which
 *	contains translation IP address.  Allocated port is returned to the
 *	port map and NAT entry is destroyed when session expires.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_nat.c,v 1.12 2012/03/11 18:27:59 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/bitops.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/cprng.h>

#include <net/pfil.h>
#include <netinet/in.h>

#include "npf_impl.h"

/*
 * NPF portmap structure.
 */
typedef struct {
	u_int			p_refcnt;
	uint32_t		p_bitmap[0];
} npf_portmap_t;

/* Portmap range: [ 1024 .. 65535 ] */
#define	PORTMAP_FIRST		(1024)
#define	PORTMAP_SIZE		((65536 - PORTMAP_FIRST) / 32)
#define	PORTMAP_FILLED		((uint32_t)~0)
#define	PORTMAP_MASK		(31)
#define	PORTMAP_SHIFT		(5)

#define	PORTMAP_MEM_SIZE	\
    (sizeof(npf_portmap_t) + (PORTMAP_SIZE * sizeof(uint32_t)))

/*
 * NAT policy structure.
 */
struct npf_natpolicy {
	LIST_HEAD(, npf_nat)	n_nat_list;
	kmutex_t		n_lock;
	kcondvar_t		n_cv;
	npf_portmap_t *		n_portmap;
	int			n_type;
	u_int			n_flags;
	size_t			n_addr_sz;
	npf_addr_t		n_taddr;
	in_port_t		n_tport;
};

#define	NPF_NP_CMP_START	offsetof(npf_natpolicy_t, n_type)
#define	NPF_NP_CMP_SIZE		(sizeof(npf_natpolicy_t) - NPF_NP_CMP_START)

/*
 * NAT translation entry for a session.
 */
struct npf_nat {
	/* Association (list entry and a link pointer) with NAT policy. */
	LIST_ENTRY(npf_nat)	nt_entry;
	npf_natpolicy_t *	nt_natpolicy;
	npf_session_t *		nt_session;
	/* Original address and port (for backwards translation). */
	npf_addr_t		nt_oaddr;
	in_port_t		nt_oport;
	/* Translation port (for redirects). */
	in_port_t		nt_tport;
	/* ALG (if any) associated with this NAT entry. */
	npf_alg_t *		nt_alg;
	uintptr_t		nt_alg_arg;
};

static pool_cache_t		nat_cache	__read_mostly;

/*
 * npf_nat_sys{init,fini}: initialise/destroy NAT subsystem structures.
 */

void
npf_nat_sysinit(void)
{

	nat_cache = pool_cache_init(sizeof(npf_nat_t), coherency_unit,
	    0, 0, "npfnatpl", NULL, IPL_NET, NULL, NULL, NULL);
	KASSERT(nat_cache != NULL);
}

void
npf_nat_sysfini(void)
{

	/* NAT policies should already be destroyed. */
	pool_cache_destroy(nat_cache);
}

/*
 * npf_nat_newpolicy: create a new NAT policy.
 *
 * => Shares portmap if policy is on existing translation address.
 * => XXX: serialise at upper layer.
 */
npf_natpolicy_t *
npf_nat_newpolicy(prop_dictionary_t natdict, npf_ruleset_t *nrlset)
{
	npf_natpolicy_t *np;
	prop_object_t obj;
	npf_portmap_t *pm;

	np = kmem_zalloc(sizeof(npf_natpolicy_t), KM_SLEEP);

	/* Translation type and flags. */
	prop_dictionary_get_int32(natdict, "type", &np->n_type);
	prop_dictionary_get_uint32(natdict, "flags", &np->n_flags);

	/* Should be exclusively either inbound or outbound NAT. */
	if (((np->n_type == NPF_NATIN) ^ (np->n_type == NPF_NATOUT)) == 0) {
		kmem_free(np, sizeof(npf_natpolicy_t));
		return NULL;
	}
	mutex_init(&np->n_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	cv_init(&np->n_cv, "npfnatcv");
	LIST_INIT(&np->n_nat_list);

	/* Translation IP. */
	obj = prop_dictionary_get(natdict, "translation-ip");
	np->n_addr_sz = prop_data_size(obj);
	KASSERT(np->n_addr_sz > 0 && np->n_addr_sz <= sizeof(npf_addr_t));
	memcpy(&np->n_taddr, prop_data_data_nocopy(obj), np->n_addr_sz);

	/* Translation port (for redirect case). */
	prop_dictionary_get_uint16(natdict, "translation-port", &np->n_tport);

	/* Determine if port map is needed. */
	np->n_portmap = NULL;
	if ((np->n_flags & NPF_NAT_PORTMAP) == 0) {
		/* No port map. */
		return np;
	}

	/*
	 * Inspect NAT policies in the ruleset for port map sharing.
	 * Note that npf_ruleset_sharepm() will increase the reference count.
	 */
	if (!npf_ruleset_sharepm(nrlset, np)) {
		/* Allocate a new port map for the NAT policy. */
		pm = kmem_zalloc(PORTMAP_MEM_SIZE, KM_SLEEP);
		pm->p_refcnt = 1;
		KASSERT((uintptr_t)pm->p_bitmap == (uintptr_t)pm + sizeof(*pm));
		np->n_portmap = pm;
	} else {
		KASSERT(np->n_portmap != NULL);
	}
	return np;
}

/*
 * npf_nat_freepolicy: free NAT policy and, on last reference, free portmap.
 *
 * => Called from npf_rule_free() during the reload via npf_ruleset_destroy().
 */
void
npf_nat_freepolicy(npf_natpolicy_t *np)
{
	npf_portmap_t *pm = np->n_portmap;
	npf_session_t *se;
	npf_nat_t *nt;

	/* De-associate all entries from the policy. */
	mutex_enter(&np->n_lock);
	LIST_FOREACH(nt, &np->n_nat_list, nt_entry) {
		se = nt->nt_session; /* XXXSMP */
		if (se == NULL) {
			continue;
		}
		npf_session_expire(se);
	}
	while (!LIST_EMPTY(&np->n_nat_list)) {
		cv_wait(&np->n_cv, &np->n_lock);
	}
	mutex_exit(&np->n_lock);

	/* Destroy the port map, on last reference. */
	if (pm && --pm->p_refcnt == 0) {
		KASSERT((np->n_flags & NPF_NAT_PORTMAP) != 0);
		kmem_free(pm, PORTMAP_MEM_SIZE);
	}
	cv_destroy(&np->n_cv);
	mutex_destroy(&np->n_lock);
	kmem_free(np, sizeof(npf_natpolicy_t));
}

/*
 * npf_nat_matchpolicy: compare two NAT policies.
 *
 * => Return 0 on match, and non-zero otherwise.
 */
bool
npf_nat_matchpolicy(npf_natpolicy_t *np, npf_natpolicy_t *mnp)
{
	void *np_raw, *mnp_raw;
	/*
	 * Compare the relevant NAT policy information (in raw form),
	 * which is enough for matching criterion.
	 */
	KASSERT(np && mnp && np != mnp);
	np_raw = (uint8_t *)np + NPF_NP_CMP_START;
	mnp_raw = (uint8_t *)mnp + NPF_NP_CMP_START;
	return (memcmp(np_raw, mnp_raw, NPF_NP_CMP_SIZE) == 0);
}

bool
npf_nat_sharepm(npf_natpolicy_t *np, npf_natpolicy_t *mnp)
{
	npf_portmap_t *pm, *mpm;

	KASSERT(np && mnp && np != mnp);

	/* Using port map and having equal translation address? */
	if ((np->n_flags & mnp->n_flags & NPF_NAT_PORTMAP) == 0) {
		return false;
	}
	if (np->n_addr_sz != mnp->n_addr_sz) {
		return false;
	}
	if (memcmp(&np->n_taddr, &mnp->n_taddr, np->n_addr_sz) != 0) {
		return false;
	}
	/* If NAT policy has an old port map - drop the reference. */
	mpm = mnp->n_portmap;
	if (mpm) {
		/* Note: at this point we cannot hold a last reference. */
		KASSERT(mpm->p_refcnt > 1);
		mpm->p_refcnt--;
	}
	/* Share the port map. */
	pm = np->n_portmap;
	mnp->n_portmap = pm;
	pm->p_refcnt++;
	return true;
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

	idx = cprng_fast32() % PORTMAP_SIZE;
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
 * npf_nat_takeport: allocate specific port in the NAT policy portmap.
 */
static bool
npf_nat_takeport(npf_natpolicy_t *np, in_port_t port)
{
	npf_portmap_t *pm = np->n_portmap;
	uint32_t map, nmap;
	u_int idx, bit;

	port = ntohs(port) - PORTMAP_FIRST;
	idx = port >> PORTMAP_SHIFT;
	bit = port & PORTMAP_MASK;
	map = pm->p_bitmap[idx];
	nmap = map | (1 << bit);
	if (map == nmap) {
		/* Already taken. */
		return false;
	}
	return atomic_cas_32(&pm->p_bitmap[idx], map, nmap) == map;
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
npf_nat_inspect(npf_cache_t *npc, nbuf_t *nbuf, ifnet_t *ifp, const int di)
{
	npf_ruleset_t *rlset;
	npf_natpolicy_t *np;
	npf_rule_t *rl;

	npf_core_enter();
	rlset = npf_core_natset();
	rl = npf_ruleset_inspect(npc, nbuf, rlset, ifp, di, NPF_LAYER_3);
	if (rl == NULL) {
		npf_core_exit();
		return NULL;
	}
	np = npf_rule_getnat(rl);
	if (np == NULL) {
		npf_core_exit();
		return NULL;
	}
	return np;
}

/*
 * npf_nat_create: create a new NAT translation entry.
 */
static npf_nat_t *
npf_nat_create(npf_cache_t *npc, npf_natpolicy_t *np)
{
	const int proto = npf_cache_ipproto(npc);
	npf_nat_t *nt;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_LAYER4));

	/* New NAT association. */
	nt = pool_cache_get(nat_cache, PR_NOWAIT);
	if (nt == NULL){
		return NULL;
	}
	npf_stats_inc(NPF_STAT_NAT_CREATE);
	nt->nt_natpolicy = np;
	nt->nt_session = NULL;
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
		goto out;
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
out:
	mutex_enter(&np->n_lock);
	LIST_INSERT_HEAD(&np->n_nat_list, nt, nt_entry);
	mutex_exit(&np->n_lock);
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
	KASSERT((np->n_flags & NPF_NAT_PORTS) != 0 || port == 0);

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
		KASSERT(npf_iscached(npc, NPC_TCP) || npf_iscached(npc, NPC_UDP));
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
 *	  association already exists.  In such case, determine whether it
 *	  is a "forwards" or "backwards" stream.
 *	- Perform translation: rewrite source or destination fields,
 *	  depending on translation type and direction.
 *	- Associate a NAT policy with a session (may establish a new).
 */
int
npf_do_nat(npf_cache_t *npc, npf_session_t *se, nbuf_t *nbuf,
    ifnet_t *ifp, const int di)
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
	 * Note: no need to lock, since reference on session is held.
	 */
	if (se && (nt = npf_session_retnat(se, di, &forw)) != NULL) {
		np = nt->nt_natpolicy;
		new = false;
		goto translate;
	}

	/*
	 * Inspect the packet for a NAT policy, if there is no session.
	 * Note: acquires the lock (releases, if not found).
	 */
	np = npf_nat_inspect(npc, nbuf, ifp, di);
	if (np == NULL) {
		/* If packet does not match - done. */
		return 0;
	}
	forw = true;

	/*
	 * Create a new NAT entry.  Note: it is safe to unlock, since the
	 * NAT policy wont be desotroyed while there are list entries, which
	 * are removed only on session expiration.  Currently, NAT entry is
	 * not yet associated with any session.
	 */
	nt = npf_nat_create(npc, np);
	if (nt == NULL) {
		npf_core_exit();
		return ENOMEM;
	}
	npf_core_exit();
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
		nse = npf_session_establish(npc, nbuf, di);
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
		/*
		 * Associate NAT translation entry with the session.
		 * Note: packet now has a translated address in the cache.
		 */
		nt->nt_session = se;
		error = npf_session_setnat(se, nt, di);
out:
		if (error) {
			/* If session was for NAT only - expire it. */
			if (nse) {
				npf_session_expire(nse);
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
 * npf_nat_gettrans: return translation IP address and port.
 */
void
npf_nat_gettrans(npf_nat_t *nt, npf_addr_t **addr, in_port_t *port)
{
	npf_natpolicy_t *np = nt->nt_natpolicy;

	*addr = &np->n_taddr;
	*port = nt->nt_tport;
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

	/* Return any taken port to the portmap. */
	if ((np->n_flags & NPF_NAT_PORTMAP) != 0 && nt->nt_tport) {
		npf_nat_putport(np, nt->nt_tport);
	}

	/* Remove NAT entry from the list, notify any waiters if last entry. */
	mutex_enter(&np->n_lock);
	LIST_REMOVE(nt, nt_entry);
	if (LIST_EMPTY(&np->n_nat_list)) {
		cv_broadcast(&np->n_cv);
	}
	mutex_exit(&np->n_lock);

	/* Free structure, increase the counter. */
	pool_cache_put(nat_cache, nt);
	npf_stats_inc(NPF_STAT_NAT_DESTROY);
}

/*
 * npf_nat_save: construct NAT entry and reference to the NAT policy.
 */
int
npf_nat_save(prop_dictionary_t sedict, prop_array_t natlist, npf_nat_t *nt)
{
	npf_natpolicy_t *np = nt->nt_natpolicy;
	prop_object_iterator_t it;
	prop_dictionary_t npdict;
	prop_data_t nd, npd;
	uintptr_t itnp;

	/* Set NAT entry data. */
	nd = prop_data_create_data(nt, sizeof(npf_nat_t));
	prop_dictionary_set(sedict, "nat-data", nd);
	prop_object_release(nd);

	/* Find or create a NAT policy. */
	it = prop_array_iterator(natlist);
	while ((npdict = prop_object_iterator_next(it)) != NULL) {
		CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
		prop_dictionary_get_uint64(npdict, "id-ptr", (uint64_t *)&itnp);
		if (itnp == (uintptr_t)np) {
			break;
		}
	}
	if (npdict == NULL) {
		/* Create NAT policy dictionary and copy the data. */
		npdict = prop_dictionary_create();
		npd = prop_data_create_data(np, sizeof(npf_natpolicy_t));
		prop_dictionary_set(npdict, "nat-policy-data", npd);
		prop_object_release(npd);

		CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
		prop_dictionary_set_uint64(npdict, "id-ptr", (uintptr_t)np);
		prop_array_add(natlist, npdict);
		prop_object_release(npdict);
	}
	prop_dictionary_set(sedict, "nat-policy", npdict);
	prop_object_release(npdict);
	return 0;
}

/*
 * npf_nat_restore: find a matching NAT policy and restore NAT entry.
 *
 * => Caller should lock the active NAT ruleset.
 */
npf_nat_t *
npf_nat_restore(prop_dictionary_t sedict, npf_session_t *se)
{
	const npf_natpolicy_t *onp;
	const npf_nat_t *ntraw;
	prop_object_t obj;
	npf_natpolicy_t *np;
	npf_rule_t *rl;
	npf_nat_t *nt;

	/* Get raw NAT entry. */
	obj = prop_dictionary_get(sedict, "nat-data");
	ntraw = prop_data_data_nocopy(obj);
	if (ntraw == NULL || prop_data_size(obj) != sizeof(npf_nat_t)) {
		return NULL;
	}

	/* Find a stored NAT policy information. */
	obj = prop_dictionary_get(
	    prop_dictionary_get(sedict, "nat-policy"), "nat-policy-data");
	onp = prop_data_data_nocopy(obj);
	if (onp == NULL || prop_data_size(obj) != sizeof(npf_natpolicy_t)) {
		return NULL;
	}

	/* Match if there is an existing NAT policy. */
	rl = npf_ruleset_matchnat(npf_core_natset(), __UNCONST(onp));
	if (rl == NULL) {
		return NULL;
	}
	np = npf_rule_getnat(rl);
	KASSERT(np != NULL);

	/* Take a specific port from port-map. */
	if (!npf_nat_takeport(np, ntraw->nt_tport)) {
		return NULL;
	}

	/* Create and return NAT entry for association. */
	nt = pool_cache_get(nat_cache, PR_WAITOK);
	memcpy(nt, ntraw, sizeof(npf_nat_t));
	LIST_INSERT_HEAD(&np->n_nat_list, nt, nt_entry);
	nt->nt_natpolicy = np;
	nt->nt_session = se;
	nt->nt_alg = NULL;
	return nt;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_nat_dump(npf_nat_t *nt)
{
	npf_natpolicy_t *np;
	struct in_addr ip;

	np = nt->nt_natpolicy;
	memcpy(&ip, &np->n_taddr, sizeof(ip));
	printf("\tNATP(%p): type %d flags 0x%x taddr %s tport %d\n",
	    np, np->n_type, np->n_flags, inet_ntoa(ip), np->n_tport);
	memcpy(&ip, &nt->nt_oaddr, sizeof(ip));
	printf("\tNAT: original address %s oport %d tport %d\n",
	    inet_ntoa(ip), ntohs(nt->nt_oport), ntohs(nt->nt_tport));
	if (nt->nt_alg) {
		printf("\tNAT ALG = %p, ARG = %p\n",
		    nt->nt_alg, (void *)nt->nt_alg_arg);
	}
}

#endif
