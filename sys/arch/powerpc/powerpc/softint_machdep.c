/*	$NetBSD: softint_machdep.c,v 1.2.2.2 2011/06/23 14:19:35 cherry Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#ifdef __HAVE_FAST_SOFTINTS
#include <powerpc/softint.h>

__CTASSERT(IPL_NONE < IPL_SOFTCLOCK);
__CTASSERT(IPL_SOFTCLOCK < IPL_SOFTBIO);
__CTASSERT(IPL_SOFTBIO < IPL_SOFTNET);
__CTASSERT(IPL_SOFTNET < IPL_SOFTSERIAL);
__CTASSERT(IPL_SOFTSERIAL < IPL_VM);
__CTASSERT(IPL_SOFTSERIAL < sizeof(uint32_t)*2);

static inline void
softint_deliver(struct cpu_info *ci, int ipl)
{
	const int si_level = IPL2SOFTINT(ipl);
	KASSERT(ci->ci_data.cpu_softints & (1 << ipl));
	ci->ci_data.cpu_softints ^= 1 << ipl;
	softint_fast_dispatch(ci->ci_softlwps[si_level], ipl);
	KASSERT(ci->ci_softlwps[si_level]->l_ctxswtch == 0);
	KASSERTMSG(ci->ci_cpl == IPL_HIGH,
	    ("%s: cpl (%d) != HIGH", __func__, ci->ci_cpl));
}

void
powerpc_softint(struct cpu_info *ci, int old_ipl, vaddr_t pc)
{
	const u_int softint_mask = (IPL_SOFTMASK << old_ipl) & IPL_SOFTMASK;
	u_int softints;

	KASSERTMSG(ci->ci_idepth == -1,
	    ("%s: cpu%u: idepth (%d) != -1", __func__,
	     cpu_index(ci), ci->ci_idepth));
	KASSERT(ci->ci_mtx_count == 0);
	KASSERT(ci->ci_cpl == IPL_HIGH);
	while ((softints = (ci->ci_data.cpu_softints & softint_mask)) != 0) {
		KASSERT(old_ipl < IPL_SOFTSERIAL);
		if (softints & (1 << IPL_SOFTSERIAL)) {
			softint_deliver(ci, IPL_SOFTSERIAL);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTNET);
		if (softints & (1 << IPL_SOFTNET)) {
			softint_deliver(ci, IPL_SOFTNET);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTBIO);
		if (softints & (1 << IPL_SOFTBIO)) {
			softint_deliver(ci, IPL_SOFTBIO);
			continue;
		}
		KASSERT(old_ipl < IPL_SOFTCLOCK);
		if (softints & (1 << IPL_SOFTCLOCK)) {
			softint_deliver(ci, IPL_SOFTCLOCK);
			continue;
		}
#ifdef __HAVE_PREEMPTION
		KASSERT(old_ipl == IPL_NONE);
		KASSERT(softints == (1 << IPL_NONE));
		ci->ci_data.cpu_softints ^= (1 << IPL_NONE);
		kpreempt(pc);
#endif
	}
}

void
powerpc_softint_init_md(lwp_t *l, u_int si_level, uintptr_t *machdep_p)
{
	struct cpu_info * const ci = l->l_cpu;

	*machdep_p = 1 << SOFTINT2IPL(si_level);
	KASSERT(*machdep_p & IPL_SOFTMASK);
	ci->ci_softlwps[si_level] = l;
}

void
powerpc_softint_trigger(uintptr_t machdep)
{
	struct cpu_info * const ci = curcpu();

	atomic_or_uint(&ci->ci_data.cpu_softints, machdep);
}

#endif /* __HAVE_FAST_SOFTINTS */
