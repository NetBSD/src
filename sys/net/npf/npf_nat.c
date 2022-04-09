/*-
 * Copyright (c) 2014-2020 Mindaugas Rasiukevicius <rmind at noxt eu>
 * Copyright (c) 2010-2013 The NetBSD Foundation, Inc.
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
 * NPF network address port translation (NAPT) and other forms of NAT.
 * Described in RFC 2663, RFC 3022, etc.
 *
 * Overview
 *
 *	There are a few mechanisms: NAT policy, port map and translation.
 *	The NAT module has a separate ruleset where rules always have an
 *	associated NAT policy.
 *
 * Translation types
 *
 *	There are two types of translation: outbound (NPF_NATOUT) and
 *	inbound (NPF_NATIN).  It should not be confused with connection
 *	direction.  See npf_nat_which() for the description of how the
 *	addresses are rewritten.  The bi-directional NAT is a combined
 *	outbound and inbound translation, therefore is constructed as
 *	two policies.
 *
 * NAT policies and port maps
 *
 *	The NAT (translation) policy is applied when packet matches the
 *	rule.  Apart from the filter criteria, the NAT policy always has
 *	a translation IP address or a table.  If port translation is set,
 *	then NAT mechanism relies on port map mechanism.
 *
 * Connections, translation entries and their life-cycle
 *
 *	NAT relies on the connection tracking module.  Each translated
 *	connection has an associated translation entry (npf_nat_t) which
 *	contains information used for backwards stream translation, i.e.
 *	the original IP address with port and translation port, allocated
 *	from the port map.  Each NAT entry is associated with the policy,
 *	which contains translation IP address.  Allocated port is returned
 *	to the port map and NAT entry is destroyed when connection expires.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_nat.c,v 1.52 2022/04/09 23:38:33 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/proc.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

/*
 * NAT policy structure.
 */
struct npf_natpolicy {
	npf_t *			n_npfctx;
	kmutex_t		n_lock;
	LIST_HEAD(, npf_nat)	n_nat_list;
	unsigned		n_refcnt;
	uint64_t		n_id;

	/*
	 * Translation type, flags, address or table and the port.
	 * Additionally, there may be translation algorithm and any
	 * auxiliary data, e.g. NPTv6 adjustment value.
	 *
	 * NPF_NP_CMP_START mark starts here.
	 */
	unsigned		n_type;
	unsigned		n_flags;
	unsigned		n_alen;

	npf_addr_t		n_taddr;
	npf_netmask_t		n_tmask;
	in_port_t		n_tport;
	unsigned		n_tid;

	unsigned		n_algo;
	union {
		unsigned	n_rr_idx;
		uint16_t	n_npt66_adj;
	};
};

/*
 * Private flags - must be in the NPF_NAT_PRIVMASK range.
 */
#define	NPF_NAT_USETABLE	(0x01000000 & NPF_NAT_PRIVMASK)

#define	NPF_NP_CMP_START	offsetof(npf_natpolicy_t, n_type)
#define	NPF_NP_CMP_SIZE		(sizeof(npf_natpolicy_t) - NPF_NP_CMP_START)

/*
 * NAT entry for a connection.
 */
struct npf_nat {
	/* Associated NAT policy. */
	npf_natpolicy_t *	nt_natpolicy;

	uint16_t		nt_ifid;
	uint16_t		nt_alen;

	/*
	 * Translation address as well as the original address which is
	 * used for backwards translation.  The same for ports.
	 */
	npf_addr_t		nt_taddr;
	npf_addr_t		nt_oaddr;

	in_port_t		nt_oport;
	in_port_t		nt_tport;

	/* ALG (if any) associated with this NAT entry. */
	npf_alg_t *		nt_alg;
	uintptr_t		nt_alg_arg;

	LIST_ENTRY(npf_nat)	nt_entry;
	npf_conn_t *		nt_conn;
};

static pool_cache_t		nat_cache	__read_mostly;

/*
 * npf_nat_sys{init,fini}: initialize/destroy NAT subsystem structures.
 */

void
npf_nat_sysinit(void)
{
	nat_cache = pool_cache_init(sizeof(npf_nat_t), 0,
	    0, 0, "npfnatpl", NULL, IPL_NET, NULL, NULL, NULL);
	KASSERT(nat_cache != NULL);
}

void
npf_nat_sysfini(void)
{
	/* All NAT policies should already be destroyed. */
	pool_cache_destroy(nat_cache);
}

/*
 * npf_natpolicy_create: create a new NAT policy.
 */
npf_natpolicy_t *
npf_natpolicy_create(npf_t *npf, const nvlist_t *nat, npf_ruleset_t *rset)
{
	npf_natpolicy_t *np;
	const void *addr;
	size_t len;

	np = kmem_zalloc(sizeof(npf_natpolicy_t), KM_SLEEP);
	atomic_store_relaxed(&np->n_refcnt, 1);
	np->n_npfctx = npf;

	/* The translation type, flags and policy ID. */
	np->n_type = dnvlist_get_number(nat, "type", 0);
	np->n_flags = dnvlist_get_number(nat, "flags", 0) & ~NPF_NAT_PRIVMASK;
	np->n_id = dnvlist_get_number(nat, "nat-policy", 0);

	/* Should be exclusively either inbound or outbound NAT. */
	if (((np->n_type == NPF_NATIN) ^ (np->n_type == NPF_NATOUT)) == 0) {
		goto err;
	}
	mutex_init(&np->n_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	LIST_INIT(&np->n_nat_list);

	/*
	 * Translation IP, mask and port (if applicable).  If using the
	 * the table, specified by the ID, then the nat-addr/nat-mask will
	 * be used as a filter for the addresses selected from table.
	 */
	if (nvlist_exists_number(nat, "nat-table-id")) {
		if (np->n_flags & NPF_NAT_STATIC) {
			goto err;
		}
		np->n_tid = nvlist_get_number(nat, "nat-table-id");
		np->n_tmask = NPF_NO_NETMASK;
		np->n_flags |= NPF_NAT_USETABLE;
	} else {
		addr = dnvlist_get_binary(nat, "nat-addr", &len, NULL, 0);
		if (!addr || len == 0 || len > sizeof(npf_addr_t)) {
			goto err;
		}
		memcpy(&np->n_taddr, addr, len);
		np->n_alen = len;
		np->n_tmask = dnvlist_get_number(nat, "nat-mask", NPF_NO_NETMASK);
		if (npf_netmask_check(np->n_alen, np->n_tmask)) {
			goto err;
		}
	}
	np->n_tport = dnvlist_get_number(nat, "nat-port", 0);

	/*
	 * NAT algorithm.
	 */
	np->n_algo = dnvlist_get_number(nat, "nat-algo", 0);
	switch (np->n_algo) {
	case NPF_ALGO_NPT66:
		np->n_npt66_adj = dnvlist_get_number(nat, "npt66-adj", 0);
		break;
	case NPF_ALGO_NETMAP:
		break;
	case NPF_ALGO_IPHASH:
	case NPF_ALGO_RR:
	default:
		if (np->n_tmask != NPF_NO_NETMASK) {
			goto err;
		}
		break;
	}
	return np;
err:
	mutex_destroy(&np->n_lock);
	kmem_free(np, sizeof(npf_natpolicy_t));
	return NULL;
}

int
npf_natpolicy_export(const npf_natpolicy_t *np, nvlist_t *nat)
{
	nvlist_add_number(nat, "nat-policy", np->n_id);
	nvlist_add_number(nat, "type", np->n_type);
	nvlist_add_number(nat, "flags", np->n_flags);

	if (np->n_flags & NPF_NAT_USETABLE) {
		nvlist_add_number(nat, "nat-table-id", np->n_tid);
	} else {
		nvlist_add_binary(nat, "nat-addr", &np->n_taddr, np->n_alen);
		nvlist_add_number(nat, "nat-mask", np->n_tmask);
	}
	nvlist_add_number(nat, "nat-port", np->n_tport);
	nvlist_add_number(nat, "nat-algo", np->n_algo);

	switch (np->n_algo) {
	case NPF_ALGO_NPT66:
		nvlist_add_number(nat, "npt66-adj", np->n_npt66_adj);
		break;
	}
	return 0;
}

static void
npf_natpolicy_release(npf_natpolicy_t *np)
{
	KASSERT(atomic_load_relaxed(&np->n_refcnt) > 0);

#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif
	if (atomic_dec_uint_nv(&np->n_refcnt) != 0) {
		return;
	}
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif
	KASSERT(LIST_EMPTY(&np->n_nat_list));
	mutex_destroy(&np->n_lock);
	kmem_free(np, sizeof(npf_natpolicy_t));
}

/*
 * npf_natpolicy_destroy: free the NAT policy.
 *
 * => Called from npf_rule_free() during the reload via npf_ruleset_destroy().
 * => At this point, NAT policy cannot acquire new references.
 */
void
npf_natpolicy_destroy(npf_natpolicy_t *np)
{
	/*
	 * Drain the references.  If there are active NAT connections,
	 * then expire them and kick the worker.
	 */
	if (atomic_load_relaxed(&np->n_refcnt) > 1) {
		npf_nat_t *nt;

		mutex_enter(&np->n_lock);
		LIST_FOREACH(nt, &np->n_nat_list, nt_entry) {
			npf_conn_t *con = nt->nt_conn;
			KASSERT(con != NULL);
			npf_conn_expire(con);
		}
		mutex_exit(&np->n_lock);
		npf_worker_signal(np->n_npfctx);
	}
	KASSERT(atomic_load_relaxed(&np->n_refcnt) >= 1);

	/*
	 * Drop the initial reference, but it might not be the last one.
	 * If so, the last reference will be triggered via:
	 *
	 * npf_conn_destroy() -> npf_nat_destroy() -> npf_natpolicy_release()
	 */
	npf_natpolicy_release(np);
}

void
npf_nat_freealg(npf_natpolicy_t *np, npf_alg_t *alg)
{
	npf_nat_t *nt;

	mutex_enter(&np->n_lock);
	LIST_FOREACH(nt, &np->n_nat_list, nt_entry) {
		if (nt->nt_alg == alg) {
			npf_alg_destroy(np->n_npfctx, alg, nt, nt->nt_conn);
			nt->nt_alg = NULL;
		}
	}
	mutex_exit(&np->n_lock);
}

/*
 * npf_natpolicy_cmp: compare two NAT policies.
 *
 * => Return 0 on match, and non-zero otherwise.
 */
bool
npf_natpolicy_cmp(npf_natpolicy_t *np, npf_natpolicy_t *mnp)
{
	const void *np_raw, *mnp_raw;

	/*
	 * Compare the relevant NAT policy information (in its raw form)
	 * that is enough as a matching criteria.
	 */
	KASSERT(np && mnp && np != mnp);
	np_raw = (const uint8_t *)np + NPF_NP_CMP_START;
	mnp_raw = (const uint8_t *)mnp + NPF_NP_CMP_START;
	return memcmp(np_raw, mnp_raw, NPF_NP_CMP_SIZE) == 0;
}

void
npf_nat_setid(npf_natpolicy_t *np, uint64_t id)
{
	np->n_id = id;
}

uint64_t
npf_nat_getid(const npf_natpolicy_t *np)
{
	return np->n_id;
}

/*
 * npf_nat_which: tell which address (source or destination) should be
 * rewritten given the combination of the NAT type and flow direction.
 *
 * => Returns NPF_SRC or NPF_DST constant.
 */
static inline unsigned
npf_nat_which(const unsigned type, const npf_flow_t flow)
{
	unsigned which;

	/* The logic below relies on these values being 0 or 1. */
	CTASSERT(NPF_SRC == 0 && NPF_DST == 1);
	CTASSERT(NPF_FLOW_FORW == NPF_SRC && NPF_FLOW_BACK == NPF_DST);

	KASSERT(type == NPF_NATIN || type == NPF_NATOUT);
	KASSERT(flow == NPF_FLOW_FORW || flow == NPF_FLOW_BACK);

	/*
	 * Outbound NAT rewrites:
	 *
	 * - Source (NPF_SRC) on "forwards" stream.
	 * - Destination (NPF_DST) on "backwards" stream.
	 *
	 * Inbound NAT is other way round.
	 */
	which = (type == NPF_NATOUT) ? flow : !flow;
	KASSERT(which == NPF_SRC || which == NPF_DST);
	return which;
}

/*
 * npf_nat_inspect: inspect packet against NAT ruleset and return a policy.
 *
 * => Acquire a reference on the policy, if found.
 * => NAT lookup is protected by EBR.
 */
static npf_natpolicy_t *
npf_nat_inspect(npf_cache_t *npc, const unsigned di)
{
	npf_t *npf = npc->npc_ctx;
	int slock = npf_config_read_enter(npf);
	npf_ruleset_t *rlset = npf_config_natset(npf);
	npf_natpolicy_t *np;
	npf_rule_t *rl;

	rl = npf_ruleset_inspect(npc, rlset, di, NPF_LAYER_3);
	if (rl == NULL) {
		npf_config_read_exit(npf, slock);
		return NULL;
	}
	np = npf_rule_getnat(rl);
	atomic_inc_uint(&np->n_refcnt);
	npf_config_read_exit(npf, slock);
	return np;
}

static void
npf_nat_algo_netmap(const npf_cache_t *npc, const npf_natpolicy_t *np,
    const unsigned which, npf_addr_t *addr)
{
	const npf_addr_t *orig_addr = npc->npc_ips[which];

	/*
	 * NETMAP:
	 *
	 *	addr = net-addr | (orig-addr & ~mask)
	 */
	npf_addr_mask(&np->n_taddr, np->n_tmask, npc->npc_alen, addr);
	npf_addr_bitor(orig_addr, np->n_tmask, npc->npc_alen, addr);
}

static inline npf_addr_t *
npf_nat_getaddr(npf_cache_t *npc, npf_natpolicy_t *np, const unsigned alen)
{
	npf_tableset_t *ts = npf_config_tableset(np->n_npfctx);
	npf_table_t *t = npf_tableset_getbyid(ts, np->n_tid);
	unsigned idx;

	/*
	 * Dynamically select the translation IP address.
	 */
	switch (np->n_algo) {
	case NPF_ALGO_RR:
		idx = atomic_inc_uint_nv(&np->n_rr_idx);
		break;
	case NPF_ALGO_IPHASH:
	default:
		idx = npf_addr_mix(alen,
		    npc->npc_ips[NPF_SRC],
		    npc->npc_ips[NPF_DST]);
		break;
	}
	return npf_table_getsome(t, alen, idx);
}

/*
 * npf_nat_create: create a new NAT translation entry.
 *
 * => The caller must pass the NAT policy with a reference acquired for us.
 */
static npf_nat_t *
npf_nat_create(npf_cache_t *npc, npf_natpolicy_t *np, npf_conn_t *con)
{
	const unsigned proto = npc->npc_proto;
	const unsigned alen = npc->npc_alen;
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_t *npf = npc->npc_ctx;
	npf_addr_t *taddr;
	npf_nat_t *nt;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_LAYER4));

	/* Construct a new NAT entry and associate it with the connection. */
	nt = pool_cache_get(nat_cache, PR_NOWAIT);
	if (__predict_false(!nt)) {
		return NULL;
	}
	npf_stats_inc(npf, NPF_STAT_NAT_CREATE);
	nt->nt_natpolicy = np;
	nt->nt_conn = con;
	nt->nt_alg = NULL;

	/*
	 * Save the interface ID.
	 *
	 * Note: this can be different from the given connection if it
	 * was established on a different interface, using the global state
	 * mode (state.key.interface = 0).
	 */
	KASSERT(nbuf->nb_ifid != 0);
	nt->nt_ifid = nbuf->nb_ifid;

	/*
	 * Select the translation address.
	 */
	if (np->n_flags & NPF_NAT_USETABLE) {
		int slock = npf_config_read_enter(npf);
		taddr = npf_nat_getaddr(npc, np, alen);
		if (__predict_false(!taddr)) {
			npf_config_read_exit(npf, slock);
			pool_cache_put(nat_cache, nt);
			return NULL;
		}
		memcpy(&nt->nt_taddr, taddr, alen);
		npf_config_read_exit(npf, slock);

	} else if (np->n_algo == NPF_ALGO_NETMAP) {
		const unsigned which = npf_nat_which(np->n_type, NPF_FLOW_FORW);
		npf_nat_algo_netmap(npc, np, which, &nt->nt_taddr);
		taddr = &nt->nt_taddr;
	} else {
		/* Static IP address. */
		taddr = &np->n_taddr;
		memcpy(&nt->nt_taddr, taddr, alen);
	}
	nt->nt_alen = alen;

	/* Save the original address which may be rewritten. */
	if (np->n_type == NPF_NATOUT) {
		/* Outbound NAT: source (think internal) address. */
		memcpy(&nt->nt_oaddr, npc->npc_ips[NPF_SRC], alen);
	} else {
		/* Inbound NAT: destination (think external) address. */
		KASSERT(np->n_type == NPF_NATIN);
		memcpy(&nt->nt_oaddr, npc->npc_ips[NPF_DST], alen);
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
		const struct tcphdr *th = npc->npc_l4.tcp;
		nt->nt_oport = (np->n_type == NPF_NATOUT) ?
		    th->th_sport : th->th_dport;
	} else {
		const struct udphdr *uh = npc->npc_l4.udp;
		nt->nt_oport = (np->n_type == NPF_NATOUT) ?
		    uh->uh_sport : uh->uh_dport;
	}

	/* Get a new port for translation. */
	if ((np->n_flags & NPF_NAT_PORTMAP) != 0) {
		npf_portmap_t *pm = np->n_npfctx->portmap;
		nt->nt_tport = npf_portmap_get(pm, alen, taddr);
	} else {
		nt->nt_tport = np->n_tport;
	}
out:
	mutex_enter(&np->n_lock);
	LIST_INSERT_HEAD(&np->n_nat_list, nt, nt_entry);
	/* Note: we also consume the reference on policy. */
	mutex_exit(&np->n_lock);
	return nt;
}

/*
 * npf_dnat_translate: perform translation given the state data.
 */
static inline int
npf_dnat_translate(npf_cache_t *npc, npf_nat_t *nt, npf_flow_t flow)
{
	const npf_natpolicy_t *np = nt->nt_natpolicy;
	const unsigned which = npf_nat_which(np->n_type, flow);
	const npf_addr_t *addr;
	in_port_t port;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_LAYER4));

	if (flow == NPF_FLOW_FORW) {
		/* "Forwards" stream: use translation address/port. */
		addr = &nt->nt_taddr;
		port = nt->nt_tport;
	} else {
		/* "Backwards" stream: use original address/port. */
		addr = &nt->nt_oaddr;
		port = nt->nt_oport;
	}
	KASSERT((np->n_flags & NPF_NAT_PORTS) != 0 || port == 0);

	/* Execute ALG translation first. */
	if ((npc->npc_info & NPC_ALG_EXEC) == 0) {
		npc->npc_info |= NPC_ALG_EXEC;
		npf_alg_exec(npc, nt, flow);
		npf_recache(npc);
	}
	KASSERT(!nbuf_flag_p(npc->npc_nbuf, NBUF_DATAREF_RESET));

	/* Finally, perform the translation. */
	return npf_napt_rwr(npc, which, addr, port);
}

/*
 * npf_snat_translate: perform translation given the algorithm.
 */
static inline int
npf_snat_translate(npf_cache_t *npc, const npf_natpolicy_t *np, npf_flow_t flow)
{
	const unsigned which = npf_nat_which(np->n_type, flow);
	const npf_addr_t *taddr;
	npf_addr_t addr;

	KASSERT(np->n_flags & NPF_NAT_STATIC);

	switch (np->n_algo) {
	case NPF_ALGO_NETMAP:
		npf_nat_algo_netmap(npc, np, which, &addr);
		taddr = &addr;
		break;
	case NPF_ALGO_NPT66:
		return npf_npt66_rwr(npc, which, &np->n_taddr,
		    np->n_tmask, np->n_npt66_adj);
	default:
		taddr = &np->n_taddr;
		break;
	}
	return npf_napt_rwr(npc, which, taddr, np->n_tport);
}

/*
 * Associate NAT policy with an existing connection state.
 */
npf_nat_t *
npf_nat_share_policy(npf_cache_t *npc, npf_conn_t *con, npf_nat_t *src_nt)
{
	npf_natpolicy_t *np = src_nt->nt_natpolicy;
	npf_nat_t *nt;
	int ret;

	/* Create a new NAT entry. */
	nt = npf_nat_create(npc, np, con);
	if (__predict_false(nt == NULL)) {
		return NULL;
	}
	atomic_inc_uint(&np->n_refcnt);

	/* Associate the NAT translation entry with the connection. */
	ret = npf_conn_setnat(npc, con, nt, np->n_type);
	if (__predict_false(ret)) {
		/* Will release the reference. */
		npf_nat_destroy(con, nt);
		return NULL;
	}
	return nt;
}

/*
 * npf_nat_lookup: lookup the (dynamic) NAT state and return its entry,
 *
 * => Checks that the packet is on the interface where NAT policy is applied.
 * => Determines the flow direction in the context of the NAT policy.
 */
static npf_nat_t *
npf_nat_lookup(const npf_cache_t *npc, npf_conn_t *con,
    const unsigned di, npf_flow_t *flow)
{
	const nbuf_t *nbuf = npc->npc_nbuf;
	const npf_natpolicy_t *np;
	npf_nat_t *nt;

	if ((nt = npf_conn_getnat(con)) == NULL) {
		return NULL;
	}
	if (nt->nt_ifid != nbuf->nb_ifid) {
		return NULL;
	}

	np = nt->nt_natpolicy;
	KASSERT(atomic_load_relaxed(&np->n_refcnt) > 0);

	/*
	 * We rely on NPF_NAT{IN,OUT} being equal to PFIL_{IN,OUT}.
	 */
	CTASSERT(NPF_NATIN == PFIL_IN && NPF_NATOUT == PFIL_OUT);
	*flow = (np->n_type == di) ? NPF_FLOW_FORW : NPF_FLOW_BACK;
	return nt;
}

/*
 * npf_do_nat:
 *
 *	- Inspect packet for a NAT policy, unless a connection with a NAT
 *	  association already exists.  In such case, determine whether it
 *	  is a "forwards" or "backwards" stream.
 *
 *	- Perform translation: rewrite source or destination fields,
 *	  depending on translation type and direction.
 *
 *	- Associate a NAT policy with a connection (may establish a new).
 */
int
npf_do_nat(npf_cache_t *npc, npf_conn_t *con, const unsigned di)
{
	nbuf_t *nbuf = npc->npc_nbuf;
	npf_conn_t *ncon = NULL;
	npf_natpolicy_t *np;
	npf_flow_t flow;
	npf_nat_t *nt;
	int error;

	/* All relevant data should be already cached. */
	if (!npf_iscached(npc, NPC_IP46) || !npf_iscached(npc, NPC_LAYER4)) {
		return 0;
	}
	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	/*
	 * Return the NAT entry associated with the connection, if any.
	 * Determines whether the stream is "forwards" or "backwards".
	 * Note: no need to lock, since reference on connection is held.
	 */
	if (con && (nt = npf_nat_lookup(npc, con, di, &flow)) != NULL) {
		np = nt->nt_natpolicy;
		goto translate;
	}

	/*
	 * Inspect the packet for a NAT policy, if there is no connection.
	 * Note: acquires a reference if found.
	 */
	np = npf_nat_inspect(npc, di);
	if (np == NULL) {
		/* If packet does not match - done. */
		return 0;
	}
	flow = NPF_FLOW_FORW;

	/* Static NAT - just perform the translation. */
	if (np->n_flags & NPF_NAT_STATIC) {
		if (nbuf_cksum_barrier(nbuf, di)) {
			npf_recache(npc);
		}
		error = npf_snat_translate(npc, np, flow);
		npf_natpolicy_release(np);
		return error;
	}

	/*
	 * If there is no local connection (no "stateful" rule - unusual,
	 * but possible configuration), establish one before translation.
	 * Note that it is not a "pass" connection, therefore passing of
	 * "backwards" stream depends on other, stateless filtering rules.
	 */
	if (con == NULL) {
		ncon = npf_conn_establish(npc, di, true);
		if (ncon == NULL) {
			npf_natpolicy_release(np);
			return ENOMEM;
		}
		con = ncon;
	}

	/*
	 * Create a new NAT entry and associate with the connection.
	 * We will consume the reference on success (release on error).
	 */
	nt = npf_nat_create(npc, np, con);
	if (nt == NULL) {
		npf_natpolicy_release(np);
		error = ENOMEM;
		goto out;
	}

	/* Determine whether any ALG matches. */
	if (npf_alg_match(npc, nt, di)) {
		KASSERT(nt->nt_alg != NULL);
	}

	/* Associate the NAT translation entry with the connection. */
	error = npf_conn_setnat(npc, con, nt, np->n_type);
	if (error) {
		/* Will release the reference. */
		npf_nat_destroy(con, nt);
		goto out;
	}

translate:
	/* May need to process the delayed checksums first (XXX: NetBSD). */
	if (nbuf_cksum_barrier(nbuf, di)) {
		npf_recache(npc);
	}

	/* Perform the translation. */
	error = npf_dnat_translate(npc, nt, flow);
out:
	if (__predict_false(ncon)) {
		if (error) {
			/* It was created for NAT - just expire. */
			npf_conn_expire(ncon);
		}
		npf_conn_release(ncon);
	}
	return error;
}

/*
 * npf_nat_gettrans: return translation IP address and port.
 */
void
npf_nat_gettrans(npf_nat_t *nt, npf_addr_t **addr, in_port_t *port)
{
	*addr = &nt->nt_taddr;
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

npf_alg_t *
npf_nat_getalg(const npf_nat_t *nt)
{
	return nt->nt_alg;
}

uintptr_t
npf_nat_getalgarg(const npf_nat_t *nt)
{
	return nt->nt_alg_arg;
}

/*
 * npf_nat_destroy: destroy NAT structure (performed on connection expiration).
 */
void
npf_nat_destroy(npf_conn_t *con, npf_nat_t *nt)
{
	npf_natpolicy_t *np = nt->nt_natpolicy;
	npf_t *npf = np->n_npfctx;
	npf_alg_t *alg;

	/* Execute the ALG destroy callback, if any. */
	if ((alg = npf_nat_getalg(nt)) != NULL) {
		npf_alg_destroy(npf, alg, nt, con);
		nt->nt_alg = NULL;
	}

	/* Return taken port to the portmap. */
	if ((np->n_flags & NPF_NAT_PORTMAP) != 0 && nt->nt_tport) {
		npf_portmap_t *pm = npf->portmap;
		npf_portmap_put(pm, nt->nt_alen, &nt->nt_taddr, nt->nt_tport);
	}
	npf_stats_inc(np->n_npfctx, NPF_STAT_NAT_DESTROY);

	/*
	 * Remove the connection from the list and drop the reference on
	 * the NAT policy.  Note: this might trigger its destruction.
	 */
	mutex_enter(&np->n_lock);
	LIST_REMOVE(nt, nt_entry);
	mutex_exit(&np->n_lock);
	npf_natpolicy_release(np);

	pool_cache_put(nat_cache, nt);
}

/*
 * npf_nat_export: serialize the NAT entry with a NAT policy ID.
 */
void
npf_nat_export(npf_t *npf, const npf_nat_t *nt, nvlist_t *con_nv)
{
	npf_natpolicy_t *np = nt->nt_natpolicy;
	unsigned alen = nt->nt_alen;
	nvlist_t *nat_nv;

	nat_nv = nvlist_create(0);
	if (nt->nt_ifid) {
		char ifname[IFNAMSIZ];
		npf_ifmap_copyname(npf, nt->nt_ifid, ifname, sizeof(ifname));
		nvlist_add_string(nat_nv, "ifname", ifname);
	}
	nvlist_add_number(nat_nv, "alen", alen);

	nvlist_add_binary(nat_nv, "oaddr", &nt->nt_oaddr, alen);
	nvlist_add_number(nat_nv, "oport", nt->nt_oport);

	nvlist_add_binary(nat_nv, "taddr", &nt->nt_taddr, alen);
	nvlist_add_number(nat_nv, "tport", nt->nt_tport);

	nvlist_add_number(nat_nv, "nat-policy", np->n_id);
	nvlist_move_nvlist(con_nv, "nat", nat_nv);
}

/*
 * npf_nat_import: find the NAT policy and unserialize the NAT entry.
 */
npf_nat_t *
npf_nat_import(npf_t *npf, const nvlist_t *nat,
    npf_ruleset_t *natlist, npf_conn_t *con)
{
	npf_natpolicy_t *np;
	npf_nat_t *nt;
	const char *ifname;
	const void *taddr, *oaddr;
	size_t alen, len;
	uint64_t np_id;

	np_id = dnvlist_get_number(nat, "nat-policy", UINT64_MAX);
	if ((np = npf_ruleset_findnat(natlist, np_id)) == NULL) {
		return NULL;
	}
	nt = pool_cache_get(nat_cache, PR_WAITOK);
	memset(nt, 0, sizeof(npf_nat_t));

	ifname = dnvlist_get_string(nat, "ifname", NULL);
	if (ifname && (nt->nt_ifid = npf_ifmap_register(npf, ifname)) == 0) {
		goto err;
	}

	alen = dnvlist_get_number(nat, "alen", 0);
	if (alen == 0 || alen > sizeof(npf_addr_t)) {
		goto err;
	}

	taddr = dnvlist_get_binary(nat, "taddr", &len, NULL, 0);
	if (!taddr || len != alen) {
		goto err;
	}
	memcpy(&nt->nt_taddr, taddr, sizeof(npf_addr_t));

	oaddr = dnvlist_get_binary(nat, "oaddr", &len, NULL, 0);
	if (!oaddr || len != alen) {
		goto err;
	}
	memcpy(&nt->nt_oaddr, oaddr, sizeof(npf_addr_t));

	nt->nt_oport = dnvlist_get_number(nat, "oport", 0);
	nt->nt_tport = dnvlist_get_number(nat, "tport", 0);

	/* Take a specific port from port-map. */
	if ((np->n_flags & NPF_NAT_PORTMAP) != 0 && nt->nt_tport) {
		npf_portmap_t *pm = npf->portmap;

		if (!npf_portmap_take(pm, nt->nt_alen,
		    &nt->nt_taddr, nt->nt_tport)) {
			goto err;
		}
	}
	npf_stats_inc(npf, NPF_STAT_NAT_CREATE);

	/*
	 * Associate, take a reference and insert.  Unlocked/non-atomic
	 * since the policy is not yet globally visible.
	 */
	nt->nt_natpolicy = np;
	nt->nt_conn = con;
	atomic_store_relaxed(&np->n_refcnt,
	    atomic_load_relaxed(&np->n_refcnt) + 1);
	LIST_INSERT_HEAD(&np->n_nat_list, nt, nt_entry);
	return nt;
err:
	pool_cache_put(nat_cache, nt);
	return NULL;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_nat_dump(const npf_nat_t *nt)
{
	const npf_natpolicy_t *np;
	struct in_addr ip;

	np = nt->nt_natpolicy;
	memcpy(&ip, &nt->nt_taddr, sizeof(ip));
	printf("\tNATP(%p): type %u flags 0x%x taddr %s tport %d\n", np,
	    np->n_type, np->n_flags, inet_ntoa(ip), ntohs(np->n_tport));
	memcpy(&ip, &nt->nt_oaddr, sizeof(ip));
	printf("\tNAT: original address %s oport %d tport %d\n",
	    inet_ntoa(ip), ntohs(nt->nt_oport), ntohs(nt->nt_tport));
	if (nt->nt_alg) {
		printf("\tNAT ALG = %p, ARG = %p\n",
		    nt->nt_alg, (void *)nt->nt_alg_arg);
	}
}

#endif
