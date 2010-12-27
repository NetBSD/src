/*	$NetBSD: npf_ruleset.c,v 1.5 2010/12/27 14:58:55 uebayasi Exp $	*/

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
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ruleset.c,v 1.5 2010/12/27 14:58:55 uebayasi Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <net/pfil.h>
#include <net/if.h>
#endif

#include "npf_ncode.h"
#include "npf_impl.h"

/* Ruleset structre (queue and default rule). */
struct npf_ruleset {
	TAILQ_HEAD(, npf_rule)	rs_queue;
	npf_rule_t *		rs_default;
};

/* Rule hook entry. */
struct npf_hook {
	void			(*hk_fn)(npf_cache_t *, nbuf_t *, void *);
	void *			hk_arg;
	LIST_ENTRY(npf_hook)	hk_entry;
};

/* Rule processing structure. */
struct npf_rproc {
	/* Reference count. */
	u_int			rp_refcnt;
	/* Normalization options. */
	bool			rp_rnd_ipid;
	bool			rp_no_df;
	u_int			rp_minttl;
	u_int			rp_maxmss;
	/* Logging interface. */
	u_int			rp_log_ifid;
};

/* Rule structure. */
struct npf_rule {
	TAILQ_ENTRY(npf_rule)	r_entry;
	/* Optional: sub-ruleset, NAT policy. */
	npf_ruleset_t		r_subset;
	npf_natpolicy_t *	r_natp;
	/* Rule priority: (highest) 0, 1, 2 ... n (lowest). */
	u_int			r_priority;
	/* N-code to process. */
	void *			r_ncode;
	size_t			r_nc_size;
	/* Attributes of this rule. */
	uint32_t		r_attr;
	/* Interface. */
	u_int			r_ifid;
	/* Hit counter. */
	u_long			r_hitcount;
	/* Rule processing data. */
	npf_rproc_t *		r_rproc;
	/* List of hooks to process on match. */
	kmutex_t		r_hooks_lock;
	LIST_HEAD(, npf_hook)	r_hooks;
};

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
 * npf_ruleset_matchnat: find a matching NAT policy in the ruleset.
 */
npf_rule_t *
npf_ruleset_matchnat(npf_ruleset_t *rlset, npf_natpolicy_t *mnp)
{
	npf_rule_t *rl;

	/* Find a matching NAT policy in the old ruleset. */
	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		if (npf_nat_matchpolicy(rl->r_natp, mnp))
			break;
	}
	return rl;
}

/*
 * npf_ruleset_natreload: minimum reload of NAT policies by maching
 * two (active  and new) NAT rulesets.
 *
 * => Active ruleset should be exclusively locked.
 */
void
npf_ruleset_natreload(npf_ruleset_t *nrlset, npf_ruleset_t *arlset)
{
	npf_natpolicy_t *np, *anp;
	npf_rule_t *rl, *arl;

	KASSERT(npf_core_locked());

	/* Scan a new NAT ruleset against NAT policies in old ruleset. */
	TAILQ_FOREACH(rl, &nrlset->rs_queue, r_entry) {
		np = rl->r_natp;
		arl = npf_ruleset_matchnat(arlset, np);
		if (arl == NULL) {
			continue;
		}
		/* On match - we exchange NAT policies. */
		anp = arl->r_natp;
		rl->r_natp = anp;
		arl->r_natp = np;
	}
}

npf_rproc_t *
npf_rproc_create(prop_dictionary_t rpdict)
{
	npf_rproc_t *rp;
	prop_object_t obj;

	rp = kmem_alloc(sizeof(npf_rproc_t), KM_SLEEP);
	rp->rp_refcnt = 1;

	/* Logging interface ID (integer). */
	obj = prop_dictionary_get(rpdict, "log-interface");
	rp->rp_log_ifid = prop_number_integer_value(obj);

	/* Randomize IP ID (bool). */
	obj = prop_dictionary_get(rpdict, "randomize-id");
	rp->rp_rnd_ipid = prop_bool_true(obj);

	/* IP_DF flag cleansing (bool). */
	obj = prop_dictionary_get(rpdict, "no-df");
	rp->rp_no_df = prop_bool_true(obj);

	/* Minimum IP TTL (integer). */
	obj = prop_dictionary_get(rpdict, "min-ttl");
	rp->rp_minttl = prop_number_integer_value(obj);

	/* Maximum TCP MSS (integer). */
	obj = prop_dictionary_get(rpdict, "max-mss");
	rp->rp_maxmss = prop_number_integer_value(obj);

	return rp;
}

npf_rproc_t *
npf_rproc_return(npf_rule_t *rl)
{
	npf_rproc_t *rp = rl->r_rproc;

	if (rp) {
		atomic_inc_uint(&rp->rp_refcnt);
	}
	return rp;
}

void
npf_rproc_release(npf_rproc_t *rp)
{

	/* Destroy on last reference. */
	if (atomic_dec_uint_nv(&rp->rp_refcnt) != 0) {
		return;
	}
	kmem_free(rp, sizeof(npf_rproc_t));
}

void
npf_rproc_run(npf_cache_t *npc, nbuf_t *nbuf, npf_rproc_t *rp)
{

	KASSERT(rp->rp_refcnt > 0);

	/* Normalize the packet, if required. */
	(void)npf_normalize(npc, nbuf,
	    rp->rp_rnd_ipid, rp->rp_no_df, rp->rp_minttl, rp->rp_maxmss);

	/* Log packet, if required. */
	if (rp->rp_log_ifid) {
		npf_log_packet(npc, nbuf, rp->rp_log_ifid);
	}

}

/*
 * npf_rule_alloc: allocate a rule and copy ncode from user-space.
 *
 * => N-code should be validated by the caller.
 */
npf_rule_t *
npf_rule_alloc(prop_dictionary_t rldict, void *nc, size_t nc_size)
{
	npf_rule_t *rl;
	prop_object_t obj;
#ifdef DIAGNOSTIC
	int errat;
#endif

	/* Allocate a rule structure. */
	rl = kmem_alloc(sizeof(npf_rule_t), KM_SLEEP);
	TAILQ_INIT(&rl->r_subset.rs_queue);
	mutex_init(&rl->r_hooks_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	LIST_INIT(&rl->r_hooks);
	rl->r_hitcount = 0;
	rl->r_natp = NULL;

	/* N-code. */
	KASSERT(nc == NULL || npf_ncode_validate(nc, nc_size, &errat) == 0);
	rl->r_ncode = nc;
	rl->r_nc_size = nc_size;

	/* Attributes (integer). */
	obj = prop_dictionary_get(rldict, "attributes");
	rl->r_attr = prop_number_integer_value(obj);

	/* Priority (integer). */
	obj = prop_dictionary_get(rldict, "priority");
	rl->r_priority = prop_number_integer_value(obj);

	/* Interface ID (integer). */
	obj = prop_dictionary_get(rldict, "interface");
	rl->r_ifid = prop_number_integer_value(obj);

	/* Create rule processing structure, if any. */
	if (rl->r_attr & (NPF_RULE_LOG | NPF_RULE_NORMALIZE)) {
		rl->r_rproc = npf_rproc_create(rldict);
	} else {
		rl->r_rproc = NULL;
	}
	return rl;
}

/*
 * npf_rule_free: free the specified rule.
 */
void
npf_rule_free(npf_rule_t *rl)
{
	npf_natpolicy_t *np = rl->r_natp;
	npf_rproc_t *rp = rl->r_rproc;

	if (np) {
		/* Free NAT policy. */
		npf_nat_freepolicy(np);
	}
	if (rp) {
		/* Release/free rule processing structure. */
		npf_rproc_release(rp);
	}
	if (rl->r_ncode) {
		/* Free n-code. */
		npf_ncode_free(rl->r_ncode, rl->r_nc_size);
	}
	mutex_destroy(&rl->r_hooks_lock);
	kmem_free(rl, sizeof(npf_rule_t));
}

/*
 * npf_rule_subset: return sub-ruleset, if any.
 * npf_rule_getnat: get NAT policy assigned to the rule.
 */

npf_ruleset_t *
npf_rule_subset(npf_rule_t *rl)
{
	return &rl->r_subset;
}

npf_natpolicy_t *
npf_rule_getnat(const npf_rule_t *rl)
{
	return rl->r_natp;
}

/*
 * npf_rule_setnat: assign NAT policy to the rule and insert into the
 * NAT policy list in the ruleset.
 */
void
npf_rule_setnat(npf_rule_t *rl, npf_natpolicy_t *np)
{

	KASSERT(rl->r_natp == NULL);
	rl->r_natp = np;
}

/*
 * npf_hook_register: register action hook in the rule.
 */
npf_hook_t *
npf_hook_register(npf_rule_t *rl,
    void (*fn)(npf_cache_t *, nbuf_t *, void *), void *arg)
{
	npf_hook_t *hk;

	hk = kmem_alloc(sizeof(npf_hook_t), KM_SLEEP);
	if (hk != NULL) {
		hk->hk_fn = fn;
		hk->hk_arg = arg;
		mutex_enter(&rl->r_hooks_lock);
		LIST_INSERT_HEAD(&rl->r_hooks, hk, hk_entry);
		mutex_exit(&rl->r_hooks_lock);
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

	mutex_enter(&rl->r_hooks_lock);
	LIST_REMOVE(hk, hk_entry);
	mutex_exit(&rl->r_hooks_lock);
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
	npf_ruleset_t *rlset;
	npf_rule_t *rl;
	bool defed;

	defed = false;
	npf_core_enter();
	rlset = npf_core_ruleset();
reinspect:
	rl = npf_ruleset_match(rlset, npc, nbuf, ifp, di, layer);

	/* If no final rule, then - default. */
	if (rl == NULL && !defed) {
		npf_ruleset_t *mainrlset = npf_core_ruleset();
		rl = mainrlset->rs_default;
		defed = true;
	}
	/* Inspect the sub-ruleset, if any. */
	if (rl && !TAILQ_EMPTY(&rl->r_subset.rs_queue)) {
		rlset = &rl->r_subset;
		goto reinspect;
	}
	if (rl == NULL) {
		npf_core_exit();
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
npf_rule_apply(npf_cache_t *npc, nbuf_t *nbuf, npf_rule_t *rl, int *retfl)
{
	npf_hook_t *hk;
	int error;

	KASSERT(npf_core_locked());

	/* Update the "hit" counter. */
	if (rl->r_attr & NPF_RULE_COUNT) {
		atomic_inc_ulong(&rl->r_hitcount);
	}

	/* If not passing - drop the packet. */
	if ((rl->r_attr & NPF_RULE_PASS) == 0) {
		error = ENETUNREACH;
		goto done;
	}
	error = 0;

	/* Passing.  Run the hooks. */
	LIST_FOREACH(hk, &rl->r_hooks, hk_entry) {
		KASSERT(hk->hk_fn != NULL);
		(*hk->hk_fn)(npc, nbuf, hk->hk_arg);
	}
done:
	*retfl = rl->r_attr;
	npf_core_exit();
	return error;
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
