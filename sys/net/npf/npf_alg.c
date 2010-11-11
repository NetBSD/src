/*	$NetBSD: npf_alg.c,v 1.2 2010/11/11 06:30:39 rmind Exp $	*/

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
 *
 * XXX: locking
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg.c,v 1.2 2010/11/11 06:30:39 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <net/pfil.h>

#include "npf_impl.h"

/* NAT ALG structure for registration. */
struct npf_alg {
	LIST_ENTRY(npf_alg)		na_entry;
	npf_alg_t *			na_bptr;
	npf_algfunc_t			na_match_func;
	npf_algfunc_t			na_out_func;
	npf_algfunc_t			na_in_func;
	npf_algfunc_t			na_seid_func;
};

static LIST_HEAD(, npf_alg)		nat_alg_list	__read_mostly;

void
npf_alg_sysinit(void)
{

	LIST_INIT(&nat_alg_list);
}

void
npf_alg_sysfini(void)
{

	KASSERT(LIST_EMPTY(&nat_alg_list));
}

/*
 * npf_alg_register: register application-level gateway.
 *
 * XXX: Protected by module lock, but unify serialisation later.
 */
npf_alg_t *
npf_alg_register(npf_algfunc_t match, npf_algfunc_t out, npf_algfunc_t in,
    npf_algfunc_t seid)
{
	npf_alg_t *alg;

	alg = kmem_alloc(sizeof(npf_alg_t), KM_SLEEP);
	alg->na_bptr = alg;
	alg->na_match_func = match;
	alg->na_out_func = out;
	alg->na_in_func = in;
	alg->na_seid_func = seid;
	LIST_INSERT_HEAD(&nat_alg_list, alg, na_entry);
	return alg;
}

/*
 * npf_alg_unregister: unregister application-level gateway.
 */
int
npf_alg_unregister(npf_alg_t *alg)
{
	npf_alg_t *it;

	LIST_FOREACH(it, &nat_alg_list, na_entry) {
		if (alg == it)
			break;
	}
	if (it != NULL) {
		LIST_REMOVE(alg, na_entry);
	}
	/* TODO: Flush relevant sessions. */
	kmem_free(alg, sizeof(npf_alg_t));
	return 0;
}

/*
 * npf_alg_match: call ALG matching inspectors, determine if any ALG matches.
 */
bool
npf_alg_match(npf_cache_t *npc, nbuf_t *nbuf, npf_nat_t *nt)
{
	npf_alg_t *alg;
	npf_algfunc_t func;

	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		func = alg->na_match_func;
		if (func && func(npc, nbuf, nt)) {
			return true;
		}
	}
	return false;
}

/*
 * npf_alg_exec: execute ALG hooks for translation.
 */
void
npf_alg_exec(npf_cache_t *npc, nbuf_t *nbuf, npf_nat_t *nt, const int di)
{
	npf_alg_t *alg;

	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		if ((di & PFIL_OUT) != 0 && alg->na_out_func != NULL) {
			(alg->na_out_func)(npc, nbuf, nt);
			continue;
		}
		if ((di & PFIL_IN) != 0 && alg->na_in_func != NULL) {
			(alg->na_in_func)(npc, nbuf, nt);
			continue;
		}
	}
}

bool
npf_alg_sessionid(npf_cache_t *npc, nbuf_t *nbuf, npf_cache_t *key)
{
	npf_alg_t *alg;
	npf_algfunc_t func;

	LIST_FOREACH(alg, &nat_alg_list, na_entry) {
		func = alg->na_seid_func;
		if (func && func(npc, nbuf, (npf_nat_t *)key)) {
			return true;
		}
	}
	return false;
}
