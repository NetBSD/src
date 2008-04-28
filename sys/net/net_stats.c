/*	$NetBSD: net_stats.c,v 1.3 2008/04/28 20:24:09 martin Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: net_stats.c,v 1.3 2008/04/28 20:24:09 martin Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/sysctl.h>

#include <net/net_stats.h>

/*
 * netstat_convert_to_user_cb --
 *	Internal routine used as a call-back for percpu data enumeration.
 */
static void
netstat_convert_to_user_cb(void *v1, void *v2, struct cpu_info *ci)
{
	const uint64_t *local_counters = v1;
	netstat_sysctl_context *ctx = v2;
	u_int i;

	for (i = 0; i < ctx->ctx_ncounters; i++)
		ctx->ctx_counters[i] += local_counters[i];
}

/*
 * netstat_convert_to_user --
 *	Internal routine to convert per-CPU network statistics into
 *	collated form.
 */
static void
netstat_convert_to_user(netstat_sysctl_context *ctx)
{

	memset(ctx->ctx_counters, 0, sizeof(uint64_t) * ctx->ctx_ncounters);
	percpu_foreach(ctx->ctx_stat, netstat_convert_to_user_cb, ctx);
}

/*
 * netstat_sysctl --
 *	Common routine for collating and reporting network statistics
 *	that are gathered per-CPU.  Statistics counters are assumed
 *	to be arrays of uint64_t's.
 */
int
netstat_sysctl(netstat_sysctl_context *ctx, SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	netstat_convert_to_user(ctx);
	node = *rnode;
	node.sysctl_data = ctx->ctx_counters;
	node.sysctl_size = sizeof(uint64_t) * ctx->ctx_ncounters;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}
