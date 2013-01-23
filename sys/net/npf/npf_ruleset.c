/*	$NetBSD: npf_ruleset.c,v 1.7.6.3 2013/01/23 00:06:25 yamt Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ruleset.c,v 1.7.6.3 2013/01/23 00:06:25 yamt Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <net/pfil.h>
#include <net/if.h>

#include "npf_ncode.h"
#include "npf_impl.h"

/* Ruleset structure (queue and default rule). */
struct npf_ruleset {
	TAILQ_HEAD(, npf_rule)	rs_queue;
	npf_rule_t *		rs_default;
};

#define	NPF_RNAME_LEN		16

/* Rule structure. */
struct npf_rule {
	/* Rule name (optional) and list entry. */
	char			r_name[NPF_RNAME_LEN];
	TAILQ_ENTRY(npf_rule)	r_entry;
	/* Optional: sub-ruleset, NAT policy. */
	npf_ruleset_t		r_subset;
	npf_natpolicy_t *	r_natp;
	/* Rule priority: (highest) 0, 1, 2 ... n (lowest). */
	pri_t			r_priority;
	/* N-code to process. */
	void *			r_ncode;
	size_t			r_nc_size;
	/* Attributes of this rule. */
	uint32_t		r_attr;
	/* Interface. */
	u_int			r_ifid;
	/* Rule procedure data. */
	npf_rproc_t *		r_rproc;
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

npf_rule_t *
npf_ruleset_sharepm(npf_ruleset_t *rlset, npf_natpolicy_t *mnp)
{
	npf_natpolicy_t *np;
	npf_rule_t *rl;

	/* Find a matching NAT policy in the old ruleset. */
	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		/*
		 * NAT policy might not yet be set during the creation of
		 * the ruleset (in such case, rule is for our policy), or
		 * policies might be equal due to rule exchange on reload.
		 */
		np = rl->r_natp;
		if (np == NULL || np == mnp)
			continue;
		if (npf_nat_sharepm(np, mnp))
			break;
	}
	return rl;
}

/*
 * npf_ruleset_freealg: inspect the ruleset and disassociate specified
 * ALG from all NAT entries using it.
 */
void
npf_ruleset_freealg(npf_ruleset_t *rlset, npf_alg_t *alg)
{
	npf_rule_t *rl;

	KASSERT(npf_core_locked());

	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		npf_natpolicy_t *np = rl->r_natp;

		if (np != NULL) {
			npf_nat_freealg(np, alg);
		}
	}
}

/*
 * npf_ruleset_natreload: minimum reload of NAT policies by maching
 * two (active and new) NAT rulesets.
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
		/* Update other NAT policies to share portmap. */
		(void)npf_ruleset_sharepm(nrlset, anp);
	}
}

/*
 * npf_rule_alloc: allocate a rule and copy n-code from user-space.
 *
 * => N-code should be validated by the caller.
 */
npf_rule_t *
npf_rule_alloc(prop_dictionary_t rldict, npf_rproc_t *rp,
   void *nc, size_t nc_size)
{
	npf_rule_t *rl;
	const char *rname;
	int errat __unused;

	/* Allocate a rule structure. */
	rl = kmem_zalloc(sizeof(npf_rule_t), KM_SLEEP);
	TAILQ_INIT(&rl->r_subset.rs_queue);
	rl->r_natp = NULL;

	/* N-code. */
	KASSERT(nc == NULL || npf_ncode_validate(nc, nc_size, &errat) == 0);
	rl->r_ncode = nc;
	rl->r_nc_size = nc_size;

	/* Name (optional) */
	if (prop_dictionary_get_cstring_nocopy(rldict, "name", &rname)) {
		strlcpy(rl->r_name, rname, NPF_RNAME_LEN);
	} else {
		rl->r_name[0] = '\0';
	}

	/* Attributes, priority and interface ID (optional). */
	prop_dictionary_get_uint32(rldict, "attributes", &rl->r_attr);
	prop_dictionary_get_int32(rldict, "priority", &rl->r_priority);
	prop_dictionary_get_uint32(rldict, "interface", &rl->r_ifid);

	/* Rule procedure. */
	if (rp) {
		npf_rproc_acquire(rp);
	}
	rl->r_rproc = rp;

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
		/* Release rule procedure. */
		npf_rproc_release(rp);
	}
	if (rl->r_ncode) {
		/* Free n-code. */
		npf_ncode_free(rl->r_ncode, rl->r_nc_size);
	}
	kmem_free(rl, sizeof(npf_rule_t));
}

/*
 * npf_rule_subset: return sub-ruleset, if any.
 * npf_rule_getrproc: acquire a reference and return rule procedure, if any.
 * npf_rule_getnat: get NAT policy assigned to the rule.
 */

npf_ruleset_t *
npf_rule_subset(npf_rule_t *rl)
{
	return &rl->r_subset;
}

npf_rproc_t *
npf_rule_getrproc(npf_rule_t *rl)
{
	npf_rproc_t *rp = rl->r_rproc;

	KASSERT(npf_core_locked());
	if (rp) {
		npf_rproc_acquire(rp);
	}
	return rp;
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

npf_rule_t *
npf_ruleset_replace(const char *name, npf_ruleset_t *rlset)
{
	npf_ruleset_t orlset;
	npf_rule_t *rl;

	npf_core_enter(); /* XXX */
	rlset = npf_core_ruleset();
	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		if (rl->r_name[0] == '\0')
			continue;
		if (strncmp(rl->r_name, name, NPF_RNAME_LEN))
			continue;
		memcpy(&orlset, &rl->r_subset, sizeof(npf_ruleset_t));
		break;
	}
	npf_core_exit();
	return rl;
}

/*
 * npf_ruleset_inspect: inspect the packet against the given ruleset.
 *
 * Loop through the rules in the set and run n-code processor of each rule
 * against the packet (nbuf chain).  If sub-ruleset is found, inspect it.
 *
 * => Caller is responsible for nbuf chain protection.
 */
npf_rule_t *
npf_ruleset_inspect(npf_cache_t *npc, nbuf_t *nbuf,
    const npf_ruleset_t *mainrlset, const int di, const int layer)
{
	const ifnet_t *ifp = nbuf->nb_ifp;
	const int di_mask = (di & PFIL_IN) ? NPF_RULE_IN : NPF_RULE_OUT;
	const npf_ruleset_t *rlset = mainrlset;
	npf_rule_t *final_rl = NULL, *rl;
	bool defed = false;

	KASSERT(ifp != NULL);
	KASSERT(npf_core_locked());
	KASSERT(((di & PFIL_IN) != 0) ^ ((di & PFIL_OUT) != 0));
again:
	TAILQ_FOREACH(rl, &rlset->rs_queue, r_entry) {
		KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
		KASSERT(!final_rl || rl->r_priority >= final_rl->r_priority);

		/* Match the interface. */
		if (rl->r_ifid && rl->r_ifid != ifp->if_index) {
			continue;
		}
		/* Match the direction. */
		if ((rl->r_attr & NPF_RULE_DIMASK) != NPF_RULE_DIMASK) {
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

	/* If no final rule, then - default. */
	if (final_rl == NULL && !defed) {
		final_rl = mainrlset->rs_default;
		defed = true;
	}
	/* Inspect the sub-ruleset, if any. */
	if (final_rl && !TAILQ_EMPTY(&final_rl->r_subset.rs_queue)) {
		rlset = &final_rl->r_subset;
		final_rl = NULL;
		goto again;
	}

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
	return final_rl;
}

/*
 * npf_rule_apply: apply the rule and return appropriate value.
 *
 * => Returns ENETUNREACH if "block" and 0 if "pass".
 * => Releases the ruleset lock.
 */
int
npf_rule_apply(npf_cache_t *npc, nbuf_t *nbuf, npf_rule_t *rl, int *retfl)
{
	int error;

	KASSERT(npf_core_locked());

	/* If not passing - drop the packet. */
	error = (rl->r_attr & NPF_RULE_PASS) ? 0 : ENETUNREACH;

	*retfl = rl->r_attr;
	npf_core_exit();

	return error;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_rulenc_dump(const npf_rule_t *rl)
{
	const uint32_t *op = rl->r_ncode;
	size_t n = rl->r_nc_size;

	while (n) {
		printf("\t> |0x%02x|\n", (uint32_t)*op);
		op++;
		n -= sizeof(*op);
	}
	printf("-> %s\n", (rl->r_attr & NPF_RULE_PASS) ? "pass" : "block");
}

#endif
