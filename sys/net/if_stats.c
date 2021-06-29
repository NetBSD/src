/*	$NetBSD: if_stats.c,v 1.4 2021/06/29 21:19:58 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_stats.c,v 1.4 2021/06/29 21:19:58 riastradh Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#include <net/if.h>

#define	IF_STATS_SIZE	(sizeof(uint64_t) * IF_NSTATS)

/*
 * if_stats_init --
 *	Initialize statistics storage for a network interface.
 */
void
if_stats_init(ifnet_t * const ifp)
{
	ifp->if_stats = percpu_alloc(IF_STATS_SIZE);
}

/*
 * if_stats_fini --
 *	Tear down statistics storage for a network interface.
 */
void
if_stats_fini(ifnet_t * const ifp)
{
	percpu_t *pc = ifp->if_stats;
	ifp->if_stats = NULL;
	if (pc) {
		percpu_free(pc, IF_STATS_SIZE);
	}
}

struct if_stats_to_if_data_ctx {
	struct if_data * const ifi;
	const bool zero_stats;
};

static void
if_stats_to_if_data_cb(void *v1, void *v2, struct cpu_info *ci)
{
	const uint64_t * const local_counters = v1;
	struct if_stats_to_if_data_ctx *ctx = v2;

	int s = splnet();

	if (ctx->ifi) {
		ctx->ifi->ifi_ipackets   += local_counters[if_ipackets];
		ctx->ifi->ifi_ierrors    += local_counters[if_ierrors];
		ctx->ifi->ifi_opackets   += local_counters[if_opackets];
		ctx->ifi->ifi_oerrors    += local_counters[if_oerrors];
		ctx->ifi->ifi_collisions += local_counters[if_collisions];
		ctx->ifi->ifi_ibytes     += local_counters[if_ibytes];
		ctx->ifi->ifi_obytes     += local_counters[if_obytes];
		ctx->ifi->ifi_imcasts    += local_counters[if_imcasts];
		ctx->ifi->ifi_omcasts    += local_counters[if_omcasts];
		ctx->ifi->ifi_iqdrops    += local_counters[if_iqdrops];
		ctx->ifi->ifi_noproto    += local_counters[if_noproto];
	}

	if (ctx->zero_stats) {
		memset(v1, 0, IF_STATS_SIZE);
	}

	splx(s);
}

/*
 * if_stats_to_if_data --
 *	Collect the interface statistics and place them into the
 *	legacy if_data structure for reportig to user space.
 *	Optionally zeros the stats after collection.
 */
void
if_stats_to_if_data(ifnet_t * const ifp, struct if_data * const ifi,
		    const bool zero_stats)
{
	struct if_stats_to_if_data_ctx ctx = {
		.ifi = ifi,
		.zero_stats = zero_stats,
	};

	memset(ifi, 0, sizeof(*ifi));
	percpu_foreach_xcall(ifp->if_stats, XC_HIGHPRI_IPL(IPL_SOFTNET),
	    if_stats_to_if_data_cb, &ctx);
}
