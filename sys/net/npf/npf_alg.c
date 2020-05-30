/*-
 * Copyright (c) 2010-2019 The NetBSD Foundation, Inc.
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
 * NPF interface for the Application Level Gateways (ALGs).
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg.c,v 1.22 2020/05/30 14:16:56 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/module.h>
#endif

#include "npf_impl.h"

/*
 * NAT ALG description structure.  For more compact use of cache,
 * the functions are separated in their own arrays.  The number of
 * ALGs is expected to be very small.
 */

struct npf_alg {
	const char *	na_name;
	unsigned	na_slot;
};

struct npf_algset {
	/* List of ALGs and the count. */
	npf_alg_t	alg_list[NPF_MAX_ALGS];
	unsigned	alg_count;

	/* Matching, inspection and translation functions. */
	npfa_funcs_t	alg_funcs[NPF_MAX_ALGS];
};

#define	NPF_ALG_PREF	"npf_alg_"
#define	NPF_ALG_PREFLEN	(sizeof(NPF_ALG_PREF) - 1)

void
npf_alg_init(npf_t *npf)
{
	npf_algset_t *aset;

	aset = kmem_zalloc(sizeof(npf_algset_t), KM_SLEEP);
	npf->algset = aset;
}

void
npf_alg_fini(npf_t *npf)
{
	npf_algset_t *aset = npf->algset;

	kmem_free(aset, sizeof(npf_algset_t));
}

static npf_alg_t *
npf_alg_lookup(npf_t *npf, const char *name)
{
	npf_algset_t *aset = npf->algset;

	KASSERT(npf_config_locked_p(npf));

	for (unsigned i = 0; i < aset->alg_count; i++) {
		npf_alg_t *alg = &aset->alg_list[i];
		const char *aname = alg->na_name;

		if (aname && strcmp(aname, name) == 0)
			return alg;
	}
	return NULL;
}

npf_alg_t *
npf_alg_construct(npf_t *npf, const char *name)
{
	npf_alg_t *alg;

	npf_config_enter(npf);
	if ((alg = npf_alg_lookup(npf, name)) == NULL) {
		char modname[NPF_ALG_PREFLEN + 64];

		snprintf(modname, sizeof(modname), "%s%s", NPF_ALG_PREF, name);
		npf_config_exit(npf);

		if (module_autoload(modname, MODULE_CLASS_MISC) != 0) {
			return NULL;
		}
		npf_config_enter(npf);
		alg = npf_alg_lookup(npf, name);
	}
	npf_config_exit(npf);
	return alg;
}

/*
 * npf_alg_register: register application-level gateway.
 */
npf_alg_t *
npf_alg_register(npf_t *npf, const char *name, const npfa_funcs_t *funcs)
{
	npf_algset_t *aset = npf->algset;
	npfa_funcs_t *afuncs;
	npf_alg_t *alg;
	unsigned i;

	npf_config_enter(npf);
	if (npf_alg_lookup(npf, name) != NULL) {
		npf_config_exit(npf);
		return NULL;
	}

	/* Find a spare slot. */
	for (i = 0; i < NPF_MAX_ALGS; i++) {
		alg = &aset->alg_list[i];
		if (alg->na_name == NULL) {
			break;
		}
	}
	if (i == NPF_MAX_ALGS) {
		npf_config_exit(npf);
		return NULL;
	}

	/* Register the ALG. */
	alg->na_name = name;
	alg->na_slot = i;

	/*
	 * Assign the functions.  Make sure the 'destroy' gets visible first.
	 */
	afuncs = &aset->alg_funcs[i];
	atomic_store_relaxed(&afuncs->destroy, funcs->destroy);
	membar_producer();
	atomic_store_relaxed(&afuncs->translate, funcs->translate);
	atomic_store_relaxed(&afuncs->inspect, funcs->inspect);
	atomic_store_relaxed(&afuncs->match, funcs->match);
	membar_producer();

	atomic_store_relaxed(&aset->alg_count, MAX(aset->alg_count, i + 1));
	npf_config_exit(npf);

	return alg;
}

/*
 * npf_alg_unregister: unregister application-level gateway.
 */
int
npf_alg_unregister(npf_t *npf, npf_alg_t *alg)
{
	npf_algset_t *aset = npf->algset;
	unsigned i = alg->na_slot;
	npfa_funcs_t *afuncs;

	/* Deactivate the functions first. */
	npf_config_enter(npf);
	afuncs = &aset->alg_funcs[i];
	atomic_store_relaxed(&afuncs->match, NULL);
	atomic_store_relaxed(&afuncs->translate, NULL);
	atomic_store_relaxed(&afuncs->inspect, NULL);
	npf_config_sync(npf);

	/*
	 * Finally, unregister the ALG.  We leave the 'destroy' callback
	 * as the following will invoke it for the relevant connections.
	 */
	npf_ruleset_freealg(npf_config_natset(npf), alg);
	atomic_store_relaxed(&afuncs->destroy, NULL);
	alg->na_name = NULL;
	npf_config_exit(npf);

	return 0;
}

/*
 * npf_alg_match: call the ALG matching inspectors.
 *
 *	The purpose of the "matching" inspector function in the ALG API
 *	is to determine whether this connection matches the ALG criteria
 *	i.e. is concerning the ALG.  If yes, ALG can associate itself with
 *	the given NAT state structure and set/save an arbitrary parameter.
 *	This is done using the using the npf_nat_setalg() function.
 *
 *	=> This is called when the packet matches the dynamic NAT policy
 *	   and the NAT state entry is being created for it [NAT-ESTABLISH].
 */
bool
npf_alg_match(npf_cache_t *npc, npf_nat_t *nt, int di)
{
	npf_t *npf = npc->npc_ctx;
	npf_algset_t *aset = npf->algset;
	bool match = false;
	unsigned count;
	int s;

	KASSERTMSG(npf_iscached(npc, NPC_IP46), "expecting protocol number");

	s = npf_config_read_enter(npf);
	count = atomic_load_relaxed(&aset->alg_count);
	for (unsigned i = 0; i < count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];
		bool (*match_func)(npf_cache_t *, npf_nat_t *, int);

		match_func = atomic_load_relaxed(&f->match);
		if (match_func && match_func(npc, nt, di)) {
			match = true;
			break;
		}
	}
	npf_config_read_exit(npf, s);
	return match;
}

/*
 * npf_alg_exec: execute the ALG translation processors.
 *
 *	The ALG function would perform any additional packet translation
 *	or manipulation here.
 *
 *	=> This is called when the packet is being translated according
 *	   to the dynamic NAT logic [NAT-TRANSLATE].
 */
void
npf_alg_exec(npf_cache_t *npc, npf_nat_t *nt, const npf_flow_t flow)
{
	npf_t *npf = npc->npc_ctx;
	npf_algset_t *aset = npf->algset;
	unsigned count;
	int s;

	s = npf_config_read_enter(npf);
	count = atomic_load_relaxed(&aset->alg_count);
	for (unsigned i = 0; i < count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];
		bool (*translate_func)(npf_cache_t *, npf_nat_t *, npf_flow_t);

		translate_func = atomic_load_relaxed(&f->translate);
		if (translate_func) {
			translate_func(npc, nt, flow);
		}
	}
	npf_config_read_exit(npf, s);
}

/*
 * npf_alg_conn: query ALGs which may perform a custom state lookup.
 *
 *	The purpose of ALG connection inspection function is to provide
 *	ALGs with a mechanism to override the regular connection state
 *	lookup, if they need to.  For example, some ALGs may want to
 *	extract and use a different n-tuple to perform a lookup.
 *
 *	=> This is called at the beginning of the connection state lookup
 *	   function [CONN-LOOKUP].
 *
 *	=> Must use the npf_conn_lookup() function to perform the custom
 *	   connection state lookup and return the result.
 *
 *	=> Returning NULL will result in NPF performing a regular state
 *	   lookup for the packet.
 */
npf_conn_t *
npf_alg_conn(npf_cache_t *npc, int di)
{
	npf_t *npf = npc->npc_ctx;
	npf_algset_t *aset = npf->algset;
	npf_conn_t *con = NULL;
	unsigned count;
	int s;

	s = npf_config_read_enter(npf);
	count = atomic_load_relaxed(&aset->alg_count);
	for (unsigned i = 0; i < count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];
		npf_conn_t *(*inspect_func)(npf_cache_t *, int);

		inspect_func = atomic_load_relaxed(&f->inspect);
		if (inspect_func && (con = inspect_func(npc, di)) != NULL) {
			break;
		}
	}
	npf_config_read_exit(npf, s);
	return con;
}

/*
 * npf_alg_destroy: free the ALG structure associated with the NAT entry.
 */
void
npf_alg_destroy(npf_t *npf, npf_alg_t *alg, npf_nat_t *nat, npf_conn_t *con)
{
	npf_algset_t *aset = npf->algset;
	const npfa_funcs_t *f = &aset->alg_funcs[alg->na_slot];
	void (*destroy_func)(npf_t *, npf_nat_t *, npf_conn_t *);

	if ((destroy_func = atomic_load_relaxed(&f->destroy)) != NULL) {
		destroy_func(npf, nat, con);
	}
}

/*
 * npf_alg_export: serialise the configuration of ALGs.
 */
int
npf_alg_export(npf_t *npf, nvlist_t *nvl)
{
	npf_algset_t *aset = npf->algset;

	KASSERT(npf_config_locked_p(npf));

	for (unsigned i = 0; i < aset->alg_count; i++) {
		const npf_alg_t *alg = &aset->alg_list[i];
		nvlist_t *algdict;

		if (alg->na_name == NULL) {
			continue;
		}
		algdict = nvlist_create(0);
		nvlist_add_string(algdict, "name", alg->na_name);
		nvlist_append_nvlist_array(nvl, "algs", algdict);
		nvlist_destroy(algdict);
	}
	return 0;
}
