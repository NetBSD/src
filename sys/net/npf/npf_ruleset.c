/*	$NetBSD: npf_ruleset.c,v 1.14.2.1 2013/02/25 00:30:03 tls Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf_ruleset.c,v 1.14.2.1 2013/02/25 00:30:03 tls Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/mbuf.h>
#include <sys/types.h>

#include <net/bpf.h>
#include <net/pfil.h>
#include <net/if.h>

#include "npf_ncode.h"
#include "npf_impl.h"

struct npf_ruleset {
	/*
	 * - List of all rules.
	 * - Dynamic (i.e. named) rules.
	 * - G/C list for convenience.
	 */
	LIST_HEAD(, npf_rule)	rs_all;
	LIST_HEAD(, npf_rule)	rs_dynamic;
	LIST_HEAD(, npf_rule)	rs_gc;

	/* Unique ID counter. */
	uint64_t		rs_idcnt;

	/* Number of array slots and active rules. */
	u_int			rs_slots;
	u_int			rs_nitems;

	/* Array of ordered rules. */
	npf_rule_t *		rs_rules[];
};

struct npf_rule {
	/* Attributes, interface and skip slot. */
	uint32_t		r_attr;
	u_int			r_ifid;
	u_int			r_skip_to;

	/* Code to process, if any. */
	int			r_type;
	void *			r_code;
	size_t			r_clen;

	/* NAT policy (optional), rule procedure and subset. */
	npf_natpolicy_t *	r_natp;
	npf_rproc_t *		r_rproc;

	/* Rule priority: (highest) 1, 2 ... n (lowest). */
	pri_t			r_priority;

	/*
	 * Dynamic group: subset queue and a dynamic group list entry.
	 * Dynamic rule: entry and the parent rule (the group).
	 */
	union {
		TAILQ_HEAD(npf_ruleq, npf_rule) r_subset;
		TAILQ_ENTRY(npf_rule)	r_entry;
	} /* C11 */;
	union {
		LIST_ENTRY(npf_rule)	r_dentry;
		npf_rule_t *		r_parent;
	} /* C11 */;

	/* Rule ID and the original dictionary. */
	uint64_t		r_id;
	prop_dictionary_t	r_dict;

	/* Rule name and all-list entry. */
	char			r_name[NPF_RULE_MAXNAMELEN];
	LIST_ENTRY(npf_rule)	r_aentry;

	/* Key (optional). */
	uint8_t			r_key[NPF_RULE_MAXKEYLEN];
};

#define	NPF_DYNAMIC_GROUP_P(attr) \
    (((attr) & NPF_DYNAMIC_GROUP) == NPF_DYNAMIC_GROUP)

#define	NPF_DYNAMIC_RULE_P(attr) \
    (((attr) & NPF_DYNAMIC_GROUP) == NPF_RULE_DYNAMIC)

npf_ruleset_t *
npf_ruleset_create(size_t slots)
{
	size_t len = offsetof(npf_ruleset_t, rs_rules[slots]);
	npf_ruleset_t *rlset;

	rlset = kmem_zalloc(len, KM_SLEEP);
	LIST_INIT(&rlset->rs_dynamic);
	LIST_INIT(&rlset->rs_all);
	LIST_INIT(&rlset->rs_gc);
	rlset->rs_slots = slots;

	return rlset;
}

static void
npf_ruleset_unlink(npf_ruleset_t *rlset, npf_rule_t *rl)
{
	if (NPF_DYNAMIC_GROUP_P(rl->r_attr)) {
		LIST_REMOVE(rl, r_dentry);
	}
	if (NPF_DYNAMIC_RULE_P(rl->r_attr)) {
		npf_rule_t *rg = rl->r_parent;
		TAILQ_REMOVE(&rg->r_subset, rl, r_entry);
	}
	LIST_REMOVE(rl, r_aentry);
}

void
npf_ruleset_destroy(npf_ruleset_t *rlset)
{
	size_t len = offsetof(npf_ruleset_t, rs_rules[rlset->rs_slots]);
	npf_rule_t *rl;

	while ((rl = LIST_FIRST(&rlset->rs_all)) != NULL) {
		npf_ruleset_unlink(rlset, rl);
		npf_rule_free(rl);
	}
	KASSERT(LIST_EMPTY(&rlset->rs_dynamic));
	KASSERT(LIST_EMPTY(&rlset->rs_gc));
	kmem_free(rlset, len);
}

/*
 * npf_ruleset_insert: insert the rule into the specified ruleset.
 */
void
npf_ruleset_insert(npf_ruleset_t *rlset, npf_rule_t *rl)
{
	u_int n = rlset->rs_nitems;

	KASSERT(n < rlset->rs_slots);

	LIST_INSERT_HEAD(&rlset->rs_all, rl, r_aentry);
	if (NPF_DYNAMIC_GROUP_P(rl->r_attr)) {
		LIST_INSERT_HEAD(&rlset->rs_dynamic, rl, r_dentry);
	}

	rlset->rs_rules[n] = rl;
	rlset->rs_nitems++;

	if (rl->r_skip_to < ++n) {
		rl->r_skip_to = n;
	}
}

static npf_rule_t *
npf_ruleset_lookup(npf_ruleset_t *rlset, const char *name)
{
	npf_rule_t *rl;

	KASSERT(npf_config_locked_p());

	LIST_FOREACH(rl, &rlset->rs_dynamic, r_dentry) {
		KASSERT(NPF_DYNAMIC_GROUP_P(rl->r_attr));
		if (strncmp(rl->r_name, name, NPF_RULE_MAXNAMELEN) == 0)
			break;
	}
	return rl;
}

int
npf_ruleset_add(npf_ruleset_t *rlset, const char *rname, npf_rule_t *rl)
{
	npf_rule_t *rg, *it;
	pri_t priocmd;

	rg = npf_ruleset_lookup(rlset, rname);
	if (rg == NULL) {
		return ESRCH;
	}
	if (!NPF_DYNAMIC_RULE_P(rl->r_attr)) {
		return EINVAL;
	}

	/* Dynamic rule - assign a unique ID and save the parent. */
	rl->r_id = ++rlset->rs_idcnt;
	rl->r_parent = rg;

	/*
	 * Rule priority: (highest) 1, 2 ... n (lowest).
	 * Negative priority indicates an operation and is reset to zero.
	 */
	if ((priocmd = rl->r_priority) < 0) {
		rl->r_priority = 0;
	}

	switch (priocmd) {
	case NPF_PRI_FIRST:
		TAILQ_FOREACH(it, &rg->r_subset, r_entry) {
			if (rl->r_priority <= it->r_priority)
				break;
		}
		if (it) {
			TAILQ_INSERT_BEFORE(it, rl, r_entry);
		} else {
			TAILQ_INSERT_HEAD(&rg->r_subset, rl, r_entry);
		}
		break;
	case NPF_PRI_LAST:
	default:
		TAILQ_FOREACH(it, &rg->r_subset, r_entry) {
			if (rl->r_priority < it->r_priority)
				break;
		}
		if (it) {
			TAILQ_INSERT_BEFORE(it, rl, r_entry);
		} else {
			TAILQ_INSERT_TAIL(&rg->r_subset, rl, r_entry);
		}
		break;
	}

	/* Finally, add into the all-list. */
	LIST_INSERT_HEAD(&rlset->rs_all, rl, r_aentry);
	return 0;
}

int
npf_ruleset_remove(npf_ruleset_t *rlset, const char *rname, uint64_t id)
{
	npf_rule_t *rg, *rl;

	if ((rg = npf_ruleset_lookup(rlset, rname)) == NULL) {
		return ESRCH;
	}
	TAILQ_FOREACH(rl, &rg->r_subset, r_entry) {
		/* Compare ID.  On match, remove and return. */
		if (rl->r_id == id) {
			npf_ruleset_unlink(rlset, rl);
			LIST_INSERT_HEAD(&rlset->rs_gc, rl, r_aentry);
			return 0;
		}
	}
	return ENOENT;
}

int
npf_ruleset_remkey(npf_ruleset_t *rlset, const char *rname,
    const void *key, size_t len)
{
	npf_rule_t *rg, *rl;

	KASSERT(len && len <= NPF_RULE_MAXKEYLEN);

	if ((rg = npf_ruleset_lookup(rlset, rname)) == NULL) {
		return ESRCH;
	}

	/* Find the last in the list. */
	TAILQ_FOREACH_REVERSE(rl, &rg->r_subset, npf_ruleq, r_entry) {
		/* Compare the key.  On match, remove and return. */
		if (memcmp(rl->r_key, key, len) == 0) {
			npf_ruleset_unlink(rlset, rl);
			LIST_INSERT_HEAD(&rlset->rs_gc, rl, r_aentry);
			return 0;
		}
	}
	return ENOENT;
}

prop_dictionary_t
npf_ruleset_list(npf_ruleset_t *rlset, const char *rname)
{
	prop_dictionary_t rldict;
	prop_array_t rules;
	npf_rule_t *rg, *rl;

	if ((rg = npf_ruleset_lookup(rlset, rname)) == NULL) {
		return NULL;
	}
	if ((rldict = prop_dictionary_create()) == NULL) {
		return NULL;
	}
	if ((rules = prop_array_create()) == NULL) {
		prop_object_release(rldict);
		return NULL;
	}

	TAILQ_FOREACH(rl, &rg->r_subset, r_entry) {
		if (rl->r_dict && !prop_array_add(rules, rl->r_dict)) {
			prop_object_release(rldict);
			prop_object_release(rules);
			return NULL;
		}
	}

	if (!prop_dictionary_set(rldict, "rules", rules)) {
		prop_object_release(rldict);
		rldict = NULL;
	}
	prop_object_release(rules);
	return rldict;
}

int
npf_ruleset_flush(npf_ruleset_t *rlset, const char *rname)
{
	npf_rule_t *rg, *rl;

	if ((rg = npf_ruleset_lookup(rlset, rname)) == NULL) {
		return ESRCH;
	}
	while ((rl = TAILQ_FIRST(&rg->r_subset)) != NULL) {
		npf_ruleset_unlink(rlset, rl);
		LIST_INSERT_HEAD(&rlset->rs_gc, rl, r_aentry);
	}
	return 0;
}

void
npf_ruleset_gc(npf_ruleset_t *rlset)
{
	npf_rule_t *rl;

	while ((rl = LIST_FIRST(&rlset->rs_gc)) != NULL) {
		LIST_REMOVE(rl, r_aentry);
		npf_rule_free(rl);
	}
}

/*
 * npf_ruleset_reload: share the dynamic rules.
 *
 * => Active ruleset should be exclusively locked.
 */
void
npf_ruleset_reload(npf_ruleset_t *rlset, npf_ruleset_t *arlset)
{
	npf_rule_t *rg;

	KASSERT(npf_config_locked_p());

	LIST_FOREACH(rg, &rlset->rs_dynamic, r_dentry) {
		npf_rule_t *arg, *rl;

		if ((arg = npf_ruleset_lookup(arlset, rg->r_name)) == NULL) {
			continue;
		}

		/*
		 * Copy the list-head structure and move the rules from the
		 * old ruleset to the new by reinserting to a new all-rules
		 * list and resetting the parent rule.  Note that the rules
		 * are still active and therefore accessible for inspection
		 * via the old ruleset.
		 */
		memcpy(&rg->r_subset, &arg->r_subset, sizeof(rg->r_subset));
		TAILQ_FOREACH(rl, &rg->r_subset, r_entry) {
			LIST_REMOVE(rl, r_aentry);
			LIST_INSERT_HEAD(&rlset->rs_all, rl, r_aentry);
			rl->r_parent = rg;
		}
	}

	/* Inherit the ID counter. */
	rlset->rs_idcnt = arlset->rs_idcnt;
}

/*
 * npf_ruleset_matchnat: find a matching NAT policy in the ruleset.
 */
npf_rule_t *
npf_ruleset_matchnat(npf_ruleset_t *rlset, npf_natpolicy_t *mnp)
{
	npf_rule_t *rl;

	/* Find a matching NAT policy in the old ruleset. */
	LIST_FOREACH(rl, &rlset->rs_all, r_aentry) {
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
	LIST_FOREACH(rl, &rlset->rs_all, r_aentry) {
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
	npf_natpolicy_t *np;

	LIST_FOREACH(rl, &rlset->rs_all, r_aentry) {
		if ((np = rl->r_natp) != NULL) {
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

	/* Scan a new NAT ruleset against NAT policies in old ruleset. */
	LIST_FOREACH(rl, &nrlset->rs_all, r_aentry) {
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
 */
npf_rule_t *
npf_rule_alloc(prop_dictionary_t rldict)
{
	npf_rule_t *rl;
	const char *rname;

	/* Allocate a rule structure. */
	rl = kmem_zalloc(sizeof(npf_rule_t), KM_SLEEP);
	TAILQ_INIT(&rl->r_subset);
	rl->r_natp = NULL;

	/* Name (optional) */
	if (prop_dictionary_get_cstring_nocopy(rldict, "name", &rname)) {
		strlcpy(rl->r_name, rname, NPF_RULE_MAXNAMELEN);
	} else {
		rl->r_name[0] = '\0';
	}

	/* Attributes, priority and interface ID (optional). */
	prop_dictionary_get_uint32(rldict, "attributes", &rl->r_attr);
	prop_dictionary_get_int32(rldict, "priority", &rl->r_priority);
	prop_dictionary_get_uint32(rldict, "interface", &rl->r_ifid);

	/* Get the skip-to index.  No need to validate it. */
	prop_dictionary_get_uint32(rldict, "skip-to", &rl->r_skip_to);

	/* Key (optional). */
	prop_object_t obj = prop_dictionary_get(rldict, "key");
	const void *key = prop_data_data_nocopy(obj);

	if (key) {
		size_t len = prop_data_size(obj);
		if (len > NPF_RULE_MAXKEYLEN) {
			kmem_free(rl, sizeof(npf_rule_t));
			return NULL;
		}
		memcpy(rl->r_key, key, len);
	}

	if (NPF_DYNAMIC_RULE_P(rl->r_attr)) {
		rl->r_dict = prop_dictionary_copy(rldict);
	}

	return rl;
}

/*
 * npf_rule_setcode: assign filter code to the rule.
 *
 * => The code should be validated by the caller.
 */
void
npf_rule_setcode(npf_rule_t *rl, const int type, void *code, size_t size)
{
	rl->r_type = type;
	rl->r_code = code;
	rl->r_clen = size;
}

/*
 * npf_rule_setrproc: assign a rule procedure and hold a reference on it.
 */
void
npf_rule_setrproc(npf_rule_t *rl, npf_rproc_t *rp)
{
	npf_rproc_acquire(rp);
	rl->r_rproc = rp;
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
	if (rl->r_code) {
		/* Free n-code. */
		kmem_free(rl->r_code, rl->r_clen);
	}
	if (rl->r_dict) {
		/* Destroy the dictionary. */
		prop_object_release(rl->r_dict);
	}
	kmem_free(rl, sizeof(npf_rule_t));
}

/*
 * npf_rule_getid: return the unique ID of a rule.
 * npf_rule_getrproc: acquire a reference and return rule procedure, if any.
 * npf_rule_getnat: get NAT policy assigned to the rule.
 */

uint64_t
npf_rule_getid(const npf_rule_t *rl)
{
	KASSERT(NPF_DYNAMIC_RULE_P(rl->r_attr));
	return rl->r_id;
}

npf_rproc_t *
npf_rule_getrproc(npf_rule_t *rl)
{
	npf_rproc_t *rp = rl->r_rproc;

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

/*
 * npf_rule_inspect: match the interface, direction and run the filter code.
 * Returns true if rule matches, false otherise.
 */
static inline bool
npf_rule_inspect(npf_cache_t *npc, nbuf_t *nbuf, const npf_rule_t *rl,
    const int di_mask, const int layer)
{
	const ifnet_t *ifp = nbuf->nb_ifp;
	const void *code;

	/* Match the interface. */
	if (rl->r_ifid && rl->r_ifid != ifp->if_index) {
		return false;
	}

	/* Match the direction. */
	if ((rl->r_attr & NPF_RULE_DIMASK) != NPF_RULE_DIMASK) {
		if ((rl->r_attr & di_mask) == 0)
			return false;
	}

	/* Execute the code, if any. */
	if ((code = rl->r_code) == NULL) {
		return true;
	}

	switch (rl->r_type) {
	case NPF_CODE_NC:
		return npf_ncode_process(npc, code, nbuf, layer) == 0;
	case NPF_CODE_BPF: {
		struct mbuf *m = nbuf_head_mbuf(nbuf);
		size_t pktlen = m_length(m);
		return bpf_filter(code, (unsigned char *)m, pktlen, 0) != 0;
	}
	default:
		KASSERT(false);
	}
	return false;
}

/*
 * npf_rule_reinspect: re-inspect the dynamic rule by iterating its list.
 * This is only for the dynamic rules.  Subrules cannot have nested rules.
 */
static npf_rule_t *
npf_rule_reinspect(npf_cache_t *npc, nbuf_t *nbuf, const npf_rule_t *drl,
    const int di_mask, const int layer)
{
	npf_rule_t *final_rl = NULL, *rl;

	KASSERT(NPF_DYNAMIC_GROUP_P(drl->r_attr));

	TAILQ_FOREACH(rl, &drl->r_subset, r_entry) {
		if (!npf_rule_inspect(npc, nbuf, rl, di_mask, layer)) {
			continue;
		}
		if (rl->r_attr & NPF_RULE_FINAL) {
			return rl;
		}
		final_rl = rl;
	}
	return final_rl;
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
    const npf_ruleset_t *rlset, const int di, const int layer)
{
	const int di_mask = (di & PFIL_IN) ? NPF_RULE_IN : NPF_RULE_OUT;
	const u_int nitems = rlset->rs_nitems;
	npf_rule_t *final_rl = NULL;
	u_int n = 0;

	KASSERT(((di & PFIL_IN) != 0) ^ ((di & PFIL_OUT) != 0));

	while (n < nitems) {
		npf_rule_t *rl = rlset->rs_rules[n];
		const u_int skip_to = rl->r_skip_to;
		const uint32_t attr = rl->r_attr;

		KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
		KASSERT(!final_rl || rl->r_priority >= final_rl->r_priority);
		KASSERT(n < skip_to);

		/* Group is a barrier: return a matching if found any. */
		if ((attr & NPF_RULE_GROUP) != 0 && final_rl) {
			break;
		}

		/* Main inspection of the rule. */
		if (!npf_rule_inspect(npc, nbuf, rl, di_mask, layer)) {
			n = skip_to;
			continue;
		}

		if (NPF_DYNAMIC_GROUP_P(attr)) {
			/*
			 * If this is a dynamic rule, re-inspect the subrules.
			 * If it has any matching rule, then it is final.
			 */
			rl = npf_rule_reinspect(npc, nbuf, rl, di_mask, layer);
			if (rl != NULL) {
				final_rl = rl;
				break;
			}
		} else if ((attr & NPF_RULE_GROUP) == 0) {
			/*
			 * Groups themselves are not matching.
			 */
			final_rl = rl;
		}

		/* Set the matching rule and check for "final". */
		if (attr & NPF_RULE_FINAL) {
			break;
		}
		n++;
	}

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
	return final_rl;
}

/*
 * npf_rule_conclude: return decision and the flags for conclusion.
 *
 * => Returns ENETUNREACH if "block" and 0 if "pass".
 */
int
npf_rule_conclude(const npf_rule_t *rl, int *retfl)
{
	/* If not passing - drop the packet. */
	*retfl = rl->r_attr;
	return (rl->r_attr & NPF_RULE_PASS) ? 0 : ENETUNREACH;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_rulenc_dump(const npf_rule_t *rl)
{
	const uint32_t *op = rl->r_code;
	size_t n = rl->r_clen;

	while (n) {
		printf("\t> |0x%02x|\n", (uint32_t)*op);
		op++;
		n -= sizeof(*op);
	}
	printf("-> %s\n", (rl->r_attr & NPF_RULE_PASS) ? "pass" : "block");
}

#endif
