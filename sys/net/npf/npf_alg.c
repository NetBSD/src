/*	$NetBSD: npf_alg.c,v 1.2.16.5 2013/02/11 21:49:49 riz Exp $	*/

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
 * NPF interface for application level gateways (ALGs).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg.c,v 1.2.16.5 2013/02/11 21:49:49 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/pserialize.h>
#include <sys/mutex.h>
#include <net/pfil.h>

#include "npf_impl.h"

/* NAT ALG structure for registration. */
struct npf_alg {
	LIST_ENTRY(npf_alg)	na_entry;
	npf_alg_t *		na_bptr;
	npf_alg_func_t		na_match_func;
	npf_alg_func_t		na_tr_func;
	npf_alg_sfunc_t		na_se_func;
};

static LIST_HEAD(, npf_alg)	nat_alg_list	__cacheline_aligned;
static kmutex_t			nat_alg_lock	__cacheline_aligned;
static pserialize_t		nat_alg_psz	__cacheline_aligned;

void
npf_alg_sysinit(void)
{

	mutex_init(&nat_alg_lock, MUTEX_DEFAULT, IPL_NONE);
	nat_alg_psz = pserialize_create();
	LIST_INIT(&nat_alg_list);
}

void
npf_alg_sysfini(void)
{

	KASSERT(LIST_EMPTY(&nat_alg_list));
	pserialize_destroy(nat_alg_psz);
	mutex_destroy(&nat_alg_lock);
}

/*
 * npf_alg_register: register application-level gateway.
 */
npf_alg_t *
npf_alg_register(npf_alg_func_t mfunc, npf_alg_func_t tfunc,
    npf_alg_sfunc_t sfunc)
{
	npf_alg_t *alg;

	alg = kmem_zalloc(sizeof(npf_alg_t), KM_SLEEP);
	alg->na_bptr = alg;
	alg->na_match_func = mfunc;
	alg->na_tr_func = tfunc;
	alg->na_se_func = sfunc;

	mutex_enter(&nat_alg_lock);
	LIST_INSERT_HEAD(&nat_alg_list, alg, na_entry);
	mutex_exit(&nat_alg_lock);

	return alg;
}

/*
 * npf_alg_unregister: unregister application-level gateway.
 */
int
npf_alg_unregister(npf_alg_t *alg)
{
	mutex_enter(&nat_alg_lock);
	LIST_REMOVE(alg, na_entry);
	pserialize_perform(nat_alg_psz);
	mutex_exit(&nat_alg_lock);

	npf_config_enter();
	npf_ruleset_freealg(npf_config_natset(), alg);
	npf_config_exit();

	kmem_free(alg, sizeof(npf_alg_t));
	return 0;
}

/*
 * npf_alg_match: call ALG matching inspectors, determine if any ALG matches.
 */
bool
npf_alg_match(npf_cache_t *npc, nbuf_t *nbuf, npf_nat_t *nt, int di)
{
	npf_alg_t *alg;
	bool match = false;
	int s;

	s = pserialize_read_enter();
	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		npf_alg_func_t func = alg->na_match_func;

		if (func && func(npc, nbuf, nt, di)) {
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
npf_alg_exec(npf_cache_t *npc, nbuf_t *nbuf, npf_nat_t *nt, int di)
{
	npf_alg_t *alg;
	int s;

	s = pserialize_read_enter();
	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		npf_alg_func_t func;

		if ((func = alg->na_tr_func) != NULL) {
			(func)(npc, nbuf, nt, di);
		}
	}
	pserialize_read_exit(s);
}

npf_session_t *
npf_alg_session(npf_cache_t *npc, nbuf_t *nbuf, int di)
{
	npf_session_t *se = NULL;
	npf_alg_t *alg;
	int s;

	s = pserialize_read_enter();
	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		npf_alg_sfunc_t func = alg->na_se_func;

		if (func && (se = func(npc, nbuf, di)) != NULL) {
			break;
		}
	}
	pserialize_read_exit(s);
	return se;
}
