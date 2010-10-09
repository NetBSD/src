/*	$NetBSD: npf_ruleset.c,v 1.2.2.2 2010/10/09 03:32:37 yamt Exp $	*/

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
 * NPF ruleset module.
 *
 * Lock order:
 *
 *	ruleset_lock -> table_lock -> npf_table_t::t_lock
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ruleset.c,v 1.2.2.2 2010/10/09 03:32:37 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#endif

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/rwlock.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/pfil.h>

#include "npf_ncode.h"
#include "npf_impl.h"

struct npf_hook {
	void				(*hk_fn)(const npf_cache_t *, void *);
	void *				hk_arg;
	LIST_ENTRY(npf_hook)		hk_entry;
};

struct npf_ruleset {
	TAILQ_HEAD(, npf_rule)		rs_queue;
	npf_rule_t *			rs_default;
	int				_reserved;
};

/* Rule structure. */
struct npf_rule {
	/* List entry in the ruleset. */
	TAILQ_ENTRY(npf_rule)		r_entry;
	/* Optional: sub-ruleset, NAT policy. */
	npf_ruleset_t			r_subset;
	npf_natpolicy_t *		r_nat;
	/* Rule priority: (highest) 0, 1, 2 ... n (lowest). */
	u_int				r_priority;
	/* N-code to process. */
	void *				r_ncode;
	size_t				r_nc_size;
	/* Attributes of this rule. */
	int				r_attr;
	/* Interface. */
	u_int				r_ifid;
	/* Hit counter. */
	u_long				r_hitcount;
	/* List of hooks to process on match. */
	LIST_HEAD(, npf_hook)		r_hooks;
};

/* Global ruleset, its lock, cache and NAT ruleset. */
static npf_ruleset_t *			ruleset;
static krwlock_t			ruleset_lock;
static pool_cache_t			rule_cache;

/*
 * npf_ruleset_sysinit: initialise ruleset structures.
 */
int
npf_ruleset_sysinit(void)
{

	rule_cache = pool_cache_init(sizeof(npf_rule_t), coherency_unit,
	    0, 0, "npfrlpl", NULL, IPL_NONE, NULL, NULL, NULL);
	if (rule_cache == NULL) {
		return ENOMEM;
	}
	rw_init(&ruleset_lock);
	ruleset = npf_ruleset_create();
	return 0;
}

void
npf_ruleset_sysfini(void)
{

	npf_ruleset_destroy(ruleset);
	rw_destroy(&ruleset_lock);
	pool_cache_destroy(rule_cache);
}

npf_ruleset_t *
npf_ruleset_create(void)
{
	npf_ruleset_t *rlset;

	rlset = kmem_zalloc(sizeof(npf_ruleset_t), KM_SLEEP);
	TAILQ_INIT(&rlset->rs_queue);
	return rlset;
}

void
npf_ruleset_destroy(npf_ruleset_t *rlset)
{
	npf_rule_t *rl;

	while ((rl = TAILQ_FIRST(&rlset->rs_queue)) != NULL) {
		TAILQ_REMOVE(&rlset->rs_queue, rl, r_entry);
		npf_rule_free(rl);
	}
	kmem_free(rlset, sizeof(npf_ruleset_t));
}

/*
 * npf_ruleset_insert: insert the rule into the specified ruleset.
 *
 * Note: multiple rules at the same priority are allowed.
 */
void
npf_ruleset_insert(npf_ruleset_t *rlset, npf_rule_t *rl)
{
	npf_rule_t *it;

	if (rl->r_attr & NPF_RULE_DEFAULT) {
		rlset->rs_default = rl;
		return;
	}
	TAILQ_FOREACH(it, &rlset->rs_queue, r_entry) {
		/* Rule priority: (highest) 0, 1, 2, 4 ... n (lowest). */
		if (it->r_priority > rl->r_priority)
			break;
	}
	if (it == NULL) {
		TAILQ_INSERT_TAIL(&rlset->rs_queue, rl, r_entry);
	} else {
		TAILQ_INSERT_BEFORE(it, rl, r_entry);
	}
}

/*
 * npf_ruleset_reload: atomically load new ruleset and tableset,
 * and destroy old structures.
 */
void
npf_ruleset_reload(npf_ruleset_t *nrlset, npf_tableset_t *ntblset)
{
	npf_ruleset_t *oldrlset;
	npf_tableset_t *oldtblset;

	/*
	 * Swap old ruleset with the new.
	 * XXX: Rework to be fully lock-less; later.
	 */
	rw_enter(&ruleset_lock, RW_WRITER);
	oldrlset = atomic_swap_ptr(&ruleset, nrlset);

	/*
	 * Setup a new tableset.  It will lock the global tableset lock,
	 * therefore ensures atomicity.  We shall free the old table-set.
	 */
	oldtblset = npf_tableset_reload(ntblset);
	KASSERT(oldtblset != NULL);
	/* Unlock.  Everything goes "live" now. */
	rw_exit(&ruleset_lock);

	npf_tableset_destroy(oldtblset);
	npf_ruleset_destroy(oldrlset);
}

/*
 * npf_rule_alloc: allocate a rule and copy ncode from user-space.
 */
npf_rule_t *
npf_rule_alloc(int attr, pri_t pri, int ifidx, void *nc, size_t sz)
{
	npf_rule_t *rl;
	int errat;

	/* Perform validation & building of n-code. */
	if (nc && npf_ncode_validate(nc, sz, &errat)) {
		return NULL;
	}
	/* Allocate a rule structure. */
	rl = pool_cache_get(rule_cache, PR_WAITOK);
	if (rl == NULL) {
		return NULL;
	}
	TAILQ_INIT(&rl->r_subset.rs_queue);
	LIST_INIT(&rl->r_hooks);
	rl->r_priority = pri;
	rl->r_attr = attr;
	rl->r_ifid = ifidx;
	rl->r_ncode = nc;
	rl->r_nc_size = sz;
	rl->r_hitcount = 0;
	rl->r_nat = NULL;
	return rl;
}

#if 0
/*
 * npf_activate_rule: activate rule by inserting it into the global ruleset.
 */
void
npf_activate_rule(npf_rule_t *rl)
{

	rw_enter(&ruleset_lock, RW_WRITER);
	npf_ruleset_insert(ruleset, rl);
	rw_exit(&ruleset_lock);
}

/*
 * npf_deactivate_rule: deactivate rule by removing it from the ruleset.
 */
void
npf_deactivate_rule(npf_rule_t *)
{

	rw_enter(&ruleset_lock, RW_WRITER);
	TAILQ_REMOVE(&ruleset->rs_queue, rl, r_entry);
	rw_exit(&ruleset_lock);
}
#endif

/*
 * npf_rule_free: free the specified rule.
 */
void
npf_rule_free(npf_rule_t *rl)
{

	if (rl->r_ncode) {
		/* Free n-code (if any). */
		npf_ncode_free(rl->r_ncode, rl->r_nc_size);
	}
	if (rl->r_nat) {
		/* Free NAT policy (if associated). */
		npf_nat_freepolicy(rl->r_nat);
	}
	pool_cache_put(rule_cache, rl);
}

/*
 * npf_rule_subset: return sub-ruleset, if any.
 * npf_rule_getnat: get NAT policy assigned to the rule.
 * npf_rule_setnat: assign NAT policy to the rule.
 */

npf_ruleset_t *
npf_rule_subset(npf_rule_t *rl)
{
	return &rl->r_subset;
}

npf_natpolicy_t *
npf_rule_getnat(const npf_rule_t *rl)
{
	return rl->r_nat;
}

void
npf_rule_setnat(npf_rule_t *rl, npf_natpolicy_t *np)
{
	rl->r_nat = np;
}

/*
 * npf_hook_register: register action hook in the rule.
 */
npf_hook_t *
npf_hook_register(npf_rule_t *rl,
    void (*fn)(const npf_cache_t *, void *), void *arg)
{
	npf_hook_t *hk;

	hk = kmem_alloc(sizeof(npf_hook_t), KM_SLEEP);
	if (hk != NULL) {
		hk->hk_fn = fn;
		hk->hk_arg = arg;
		rw_enter(&ruleset_lock, RW_WRITER);
		LIST_INSERT_HEAD(&rl->r_hooks, hk, hk_entry);
		rw_exit(&ruleset_lock);
	}
	return hk;
}

/*
 * npf_hook_unregister: unregister a specified hook.
 *
 * => Hook should have been registered in the rule.
 */
void
npf_hook_unregister(npf_rule_t *rl, npf_hook_t *hk)
{

	rw_enter(&ruleset_lock, RW_WRITER);
	LIST_REMOVE(hk, hk_entry);
	rw_exit(&ruleset_lock);
	kmem_free(hk, sizeof(npf_hook_t));
}

/*
 * npf_ruleset_match: inspect the packet against the given ruleset.
 *
 * Loop for each rule in the set and run n-code processor of each rule
 * against the packet (nbuf chain).
 */
npf_rule_t *
npf_ruleset_match(npf_ruleset_t *rlset, npf_cache_t *npc, nbuf_t *nbuf,
    struct ifnet *ifp, const int di, const int layer)
{
	npf_rule_t *final_rl = NULL, *rl;

	KASSERT(((di & PFIL_IN) != 0) ^ ((di & PFIL_OUT) != 0));

	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		KASSERT(!final_rl || rl->r_priority >= final_rl->r_priority);

		/* Match the interface. */
		if (rl->r_ifid && rl->r_ifid != ifp->if_index) {
			continue;
		}
		/* Match the direction. */
		if ((rl->r_attr & NPF_RULE_DIMASK) != NPF_RULE_DIMASK) {
			const int di_mask =
			    (di & PFIL_IN) ? NPF_RULE_IN : NPF_RULE_OUT;

			if ((rl->r_attr & di_mask) == 0)
				continue;
		}
		/* Process the n-code, if any. */
		const void *nc = rl->r_ncode;
		if (nc && npf_ncode_process(npc, nc, nbuf, layer)) {
			continue;
		}
		/* Set the matching rule and check for "final". */
		final_rl = rl;
		if (rl->r_attr & NPF_RULE_FINAL) {
			break;
		}
	}
	return final_rl;
}

/*
 * npf_ruleset_inspect: inspection of the main ruleset for filtering.
 * If sub-ruleset is found, inspect it.
 *
 * => If found, ruleset is kept read-locked.
 * => Caller should protect the nbuf chain.
 */
npf_rule_t *
npf_ruleset_inspect(npf_cache_t *npc, nbuf_t *nbuf,
    struct ifnet *ifp, const int di, const int layer)
{
	npf_ruleset_t *rlset = ruleset;
	npf_rule_t *rl;
	bool defed;

	defed = false;
	rw_enter(&ruleset_lock, RW_READER);
reinspect:
	rl = npf_ruleset_match(rlset, npc, nbuf, ifp, di, layer);

	/* If no final rule, then - default. */
	if (rl == NULL && !defed) {
		rl = ruleset->rs_default;
		defed = true;
	}
	/* Inspect the sub-ruleset, if any. */
	if (rl && !TAILQ_EMPTY(&rl->r_subset.rs_queue)) {
		rlset = &rl->r_subset;
		goto reinspect;
	}
	if (rl == NULL) {
		rw_exit(&ruleset_lock);
	}
	return rl;
}

/*
 * npf_rule_apply: apply the rule i.e. run hooks and return appropriate value.
 *
 * => Returns ENETUNREACH if "block" and 0 if "pass".
 * => Releases the ruleset lock.
 */
int
npf_rule_apply(const npf_cache_t *npc, npf_rule_t *rl,
    bool *keepstate, int *retfl)
{
	npf_hook_t *hk;

	KASSERT(rw_lock_held(&ruleset_lock));

	/* Update the "hit" counter. */
	if (rl->r_attr & NPF_RULE_COUNT) {
		atomic_inc_ulong(&rl->r_hitcount);
	}

	/* If not passing - drop the packet. */
	if ((rl->r_attr & NPF_RULE_PASS) == 0) {
		/* Determine whether any return message is needed. */
		*retfl = rl->r_attr & (NPF_RULE_RETRST | NPF_RULE_RETICMP);
		rw_exit(&ruleset_lock);
		return ENETUNREACH;
	}

	/* Passing.  Run the hooks. */
	LIST_FOREACH(hk, &rl->r_hooks, hk_entry) {
		KASSERT(hk->hk_fn != NULL);
		(*hk->hk_fn)(npc, hk->hk_arg);
	}
	*keepstate = (rl->r_attr & NPF_RULE_KEEPSTATE) != 0;
	rw_exit(&ruleset_lock);

	return 0;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_rulenc_dump(npf_rule_t *rl)
{
	uint32_t *op = rl->r_ncode;
	size_t n = rl->r_nc_size;

	while (n) {
		printf("\t> |0x%02x|\n", (uint32_t)*op);
		op++;
		n -= sizeof(*op);
	}
	printf("-> %s\n", (rl->r_attr & NPF_RULE_PASS) ? "pass" : "block");
}

#endif
