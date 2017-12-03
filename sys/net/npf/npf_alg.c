/*	$NetBSD: npf_alg.c,v 1.5.2.4 2017/12/03 11:39:03 jdolecek Exp $	*/

/*-
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
 * NPF interface for the Application Level Gateways (ALGs).
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg.c,v 1.5.2.4 2017/12/03 11:39:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/pserialize.h>
#include <sys/mutex.h>
#include <net/pfil.h>
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
	u_int		na_slot;
};

struct npf_algset {
	/* List of ALGs and the count. */
	npf_alg_t	alg_list[NPF_MAX_ALGS];
	u_int		alg_count;

	/* Matching, inspection and translation functions. */
	npfa_funcs_t	alg_funcs[NPF_MAX_ALGS];
};

static const char	alg_prefix[] = "npf_alg_";
#define	NPF_EXT_PREFLEN	(sizeof(alg_prefix) - 1)

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

	for (u_int i = 0; i < aset->alg_count; i++) {
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
		char modname[NPF_EXT_PREFLEN + 64];
		snprintf(modname, sizeof(modname), "%s%s", alg_prefix, name);
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
	u_int i;

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

	/* Assign the functions. */
	afuncs = &aset->alg_funcs[i];
	afuncs->match = funcs->match;
	afuncs->translate = funcs->translate;
	afuncs->inspect = funcs->inspect;

	aset->alg_count = MAX(aset->alg_count, i + 1);
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
	u_int i = alg->na_slot;
	npfa_funcs_t *afuncs;

	/* Deactivate the functions first. */
	npf_config_enter(npf);
	afuncs = &aset->alg_funcs[i];
	afuncs->match = NULL;
	afuncs->translate = NULL;
	afuncs->inspect = NULL;
	pserialize_perform(npf->qsbr);

	/* Finally, unregister the ALG. */
	npf_ruleset_freealg(npf_config_natset(npf), alg);
	alg->na_name = NULL;
	npf_config_exit(npf);

	return 0;
}

/*
 * npf_alg_match: call ALG matching inspectors, determine if any ALG matches.
 */
bool
npf_alg_match(npf_cache_t *npc, npf_nat_t *nt, int di)
{
	npf_algset_t *aset = npc->npc_ctx->algset;
	bool match = false;
	int s;

	s = pserialize_read_enter();
	for (u_int i = 0; i < aset->alg_count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];

		if (f->match && f->match(npc, nt, di)) {
			match = true;
			break;
		}
	}
	pserialize_read_exit(s);
	return match;
}

/*
 * npf_alg_exec: execute ALG hooks for translation.
 */
void
npf_alg_exec(npf_cache_t *npc, npf_nat_t *nt, bool forw)
{
	npf_algset_t *aset = npc->npc_ctx->algset;
	int s;

	s = pserialize_read_enter();
	for (u_int i = 0; i < aset->alg_count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];

		if (f->translate) {
			f->translate(npc, nt, forw);
		}
	}
	pserialize_read_exit(s);
}

npf_conn_t *
npf_alg_conn(npf_cache_t *npc, int di)
{
	npf_algset_t *aset = npc->npc_ctx->algset;
	npf_conn_t *con = NULL;
	int s;

	s = pserialize_read_enter();
	for (u_int i = 0; i < aset->alg_count; i++) {
		const npfa_funcs_t *f = &aset->alg_funcs[i];

		if (!f->inspect)
			continue;
		if ((con = f->inspect(npc, di)) != NULL)
			break;
	}
	pserialize_read_exit(s);
	return con;
}

prop_array_t
npf_alg_export(npf_t *npf)
{
	prop_array_t alglist = prop_array_create();
	npf_algset_t *aset = npf->algset;

	KASSERT(npf_config_locked_p(npf));

	for (u_int i = 0; i < aset->alg_count; i++) {
		const npf_alg_t *alg = &aset->alg_list[i];

		if (alg->na_name == NULL) {
			continue;
		}
		prop_dictionary_t algdict = prop_dictionary_create();
		prop_dictionary_set_cstring(algdict, "name", alg->na_name);
		prop_array_add(alglist, algdict);
		prop_object_release(algdict);
	}
	return alglist;
}
