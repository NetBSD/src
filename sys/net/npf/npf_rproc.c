/*	$NetBSD: npf_rproc.c,v 1.2 2012/02/20 00:18:20 rmind Exp $	*/

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
 * NPF rule procedure interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/kmem.h>

#include "npf_impl.h"

#define	NPF_RNAME_LEN		16

/* Rule procedure structure. */
struct npf_rproc {
	/* Name. */
	char			rp_name[NPF_RNAME_LEN];
	/* Reference count. */
	u_int			rp_refcnt;
	uint32_t		rp_flags;
	/* Normalisation options. */
	bool			rp_rnd_ipid;
	bool			rp_no_df;
	u_int			rp_minttl;
	u_int			rp_maxmss;
	/* Logging interface. */
	u_int			rp_log_ifid;
};

npf_rproc_t *
npf_rproc_create(prop_dictionary_t rpdict)
{
	npf_rproc_t *rp;
	const char *rname;

	rp = kmem_intr_zalloc(sizeof(npf_rproc_t), KM_SLEEP);
	rp->rp_refcnt = 1;

	/* Name and flags. */
	prop_dictionary_get_cstring_nocopy(rpdict, "name", &rname);
	strlcpy(rp->rp_name, rname, NPF_RNAME_LEN);
	prop_dictionary_get_uint32(rpdict, "flags", &rp->rp_flags);

	/* Logging interface ID (integer). */
	prop_dictionary_get_uint32(rpdict, "log-interface", &rp->rp_log_ifid);

	/* IP ID randomisation and IP_DF flag cleansing. */
	prop_dictionary_get_bool(rpdict, "randomize-id", &rp->rp_rnd_ipid);
	prop_dictionary_get_bool(rpdict, "no-df", &rp->rp_no_df);

	/* Minimum IP TTL and maximum TCP MSS. */
	prop_dictionary_get_uint32(rpdict, "min-ttl", &rp->rp_minttl);
	prop_dictionary_get_uint32(rpdict, "max-mss", &rp->rp_maxmss);

	return rp;
}

void
npf_rproc_acquire(npf_rproc_t *rp)
{

	atomic_inc_uint(&rp->rp_refcnt);
}

void
npf_rproc_release(npf_rproc_t *rp)
{

	/* Destroy on last reference. */
	KASSERT(rp->rp_refcnt > 0);
	if (atomic_dec_uint_nv(&rp->rp_refcnt) != 0) {
		return;
	}
	kmem_intr_free(rp, sizeof(npf_rproc_t));
}

void
npf_rproc_run(npf_cache_t *npc, nbuf_t *nbuf, npf_rproc_t *rp, int error)
{
	const uint32_t flags = rp->rp_flags;

	KASSERT(rp->rp_refcnt > 0);

	/* Normalise the packet, if required. */
	if ((flags & NPF_RPROC_NORMALIZE) != 0 && !error) {
		(void)npf_normalize(npc, nbuf,
		    rp->rp_rnd_ipid, rp->rp_no_df,
		    rp->rp_minttl, rp->rp_maxmss);
		npf_stats_inc(NPF_STAT_RPROC_NORM);
	}

	/* Log packet, if required. */
	if ((flags & NPF_RPROC_LOG) != 0) {
		npf_log_packet(npc, nbuf, rp->rp_log_ifid);
		npf_stats_inc(NPF_STAT_RPROC_LOG);
	}
}
