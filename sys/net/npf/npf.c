/*	$NetBSD: npf.c,v 1.12.2.5 2017/12/03 11:39:03 jdolecek Exp $	*/

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
 * NPF main: dynamic load/initialisation and unload routines.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.12.2.5 2017/12/03 11:39:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/percpu.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

__read_mostly static npf_t *	npf_kernel_ctx = NULL;

__dso_public int
npf_sysinit(unsigned nworkers)
{
	npf_bpf_sysinit();
	npf_tableset_sysinit();
	npf_nat_sysinit();
	return npf_worker_sysinit(nworkers);
}

__dso_public void
npf_sysfini(void)
{
	npf_worker_sysfini();
	npf_nat_sysfini();
	npf_tableset_sysfini();
	npf_bpf_sysfini();
}

__dso_public npf_t *
npf_create(int flags, const npf_mbufops_t *mbufops, const npf_ifops_t *ifops)
{
	npf_t *npf;

	npf = kmem_zalloc(sizeof(npf_t), KM_SLEEP);
	npf->qsbr = pserialize_create();
	npf->stats_percpu = percpu_alloc(NPF_STATS_SIZE);
	npf->mbufops = mbufops;

	npf_ifmap_init(npf, ifops);
	npf_conn_init(npf, flags);
	npf_alg_init(npf);
	npf_ext_init(npf);

	/* Load an empty configuration. */
	npf_config_init(npf);
	return npf;
}

__dso_public void
npf_destroy(npf_t *npf)
{
	/*
	 * Destroy the current configuration.  Note: at this point all
	 * handlers must be deactivated; we will drain any processing.
	 */
	npf_config_fini(npf);

	/* Finally, safe to destroy the subsystems. */
	npf_ext_fini(npf);
	npf_alg_fini(npf);
	npf_conn_fini(npf);
	npf_ifmap_fini(npf);

	pserialize_destroy(npf->qsbr);
	percpu_free(npf->stats_percpu, NPF_STATS_SIZE);
	kmem_free(npf, sizeof(npf_t));
}

__dso_public int
npf_load(npf_t *npf, void *ref, npf_error_t *err)
{
	return npfctl_load(npf, 0, ref);
}

__dso_public void
npf_gc(npf_t *npf)
{
	npf_conn_worker(npf);
}

__dso_public void
npf_thread_register(npf_t *npf)
{
	pserialize_register(npf->qsbr);
}

void
npf_setkernctx(npf_t *npf)
{
	npf_kernel_ctx = npf;
}

npf_t *
npf_getkernctx(void)
{
	return npf_kernel_ctx;
}

/*
 * NPF statistics interface.
 */

void
npf_stats_inc(npf_t *npf, npf_stats_t st)
{
	uint64_t *stats = percpu_getref(npf->stats_percpu);
	stats[st]++;
	percpu_putref(npf->stats_percpu);
}

void
npf_stats_dec(npf_t *npf, npf_stats_t st)
{
	uint64_t *stats = percpu_getref(npf->stats_percpu);
	stats[st]--;
	percpu_putref(npf->stats_percpu);
}

static void
npf_stats_collect(void *mem, void *arg, struct cpu_info *ci)
{
	uint64_t *percpu_stats = mem, *full_stats = arg;

	for (unsigned i = 0; i < NPF_STATS_COUNT; i++) {
		full_stats[i] += percpu_stats[i];
	}
}

/*
 * npf_stats: export collected statistics.
 */
__dso_public void
npf_stats(npf_t *npf, uint64_t *buf)
{
	memset(buf, 0, NPF_STATS_SIZE);
	percpu_foreach(npf->stats_percpu, npf_stats_collect, buf);
}
