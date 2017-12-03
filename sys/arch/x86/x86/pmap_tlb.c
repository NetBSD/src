/*	$NetBSD: pmap_tlb.c,v 1.6.2.1 2017/12/03 11:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Mindaugas Rasiukevicius.
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
 * x86 pmap(9) module: TLB shootdowns.
 *
 * TLB shootdowns are hard interrupts that operate outside the SPL framework.
 * They do not need to be blocked, provided that the pmap module gets the
 * order of events correct.  The calls are made by poking the LAPIC directly.
 * The interrupt handler is short and does one of the following: invalidate
 * a set of pages, all user TLB entries or the entire TLB.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_tlb.c,v 1.6.2.1 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <uvm/uvm.h>

#include <machine/cpuvar.h>
#ifdef XEN
#include <xen/xenpmap.h>
#endif /* XEN */
#include <x86/i82489reg.h>
#include <x86/i82489var.h>

/*
 * TLB shootdown structures.
 */

typedef struct {
#ifdef _LP64
	uintptr_t		tp_va[14];	/* whole struct: 128 bytes */
#else
	uintptr_t		tp_va[13];	/* whole struct: 64 bytes */
#endif
	uint16_t		tp_count;
	uint16_t		tp_pte;
	int			tp_userpmap;
	kcpuset_t *		tp_cpumask;
} pmap_tlb_packet_t;

/*
 * No more than N separate invlpg.
 *
 * Statistically, a value of six is big enough to cover the requested number
 * of pages in ~ 95% of the TLB shootdowns we are getting. We therefore rarely
 * reach the limit, and increasing it can actually reduce the performance due
 * to the high cost of invlpg.
 */
#define	TP_MAXVA		6

/*
 * TLB shootdown state.
 */
static pmap_tlb_packet_t	pmap_tlb_packet		__cacheline_aligned;
static volatile u_int		pmap_tlb_pendcount	__cacheline_aligned;
static volatile u_int		pmap_tlb_gen		__cacheline_aligned;
static struct evcnt		pmap_tlb_evcnt		__cacheline_aligned;

/*
 * TLB shootdown statistics.
 */
#ifdef TLBSTATS
static struct evcnt		tlbstat_local[TLBSHOOT__MAX];
static struct evcnt		tlbstat_remote[TLBSHOOT__MAX];
static struct evcnt		tlbstat_kernel[TLBSHOOT__MAX];
static struct evcnt		tlbstat_single_req;
static struct evcnt		tlbstat_single_issue;
static const char *		tlbstat_name[ ] = {
	"APTE",
	"KENTER",
	"KREMOVE",
	"FREE_PTP1",
	"FREE_PTP2",
	"REMOVE_PTE",
	"REMOVE_PTES",
	"SYNC_PV1",
	"SYNC_PV2",
	"WRITE_PROTECT",
	"ENTER",
	"UPDATE",
	"BUS_DMA",
	"BUS_SPACE"
};
#endif

void
pmap_tlb_init(void)
{

	memset(&pmap_tlb_packet, 0, sizeof(pmap_tlb_packet_t));
	pmap_tlb_pendcount = 0;
	pmap_tlb_gen = 0;

	evcnt_attach_dynamic(&pmap_tlb_evcnt, EVCNT_TYPE_INTR,
	    NULL, "TLB", "shootdown");

#ifdef TLBSTATS
	int i;

	for (i = 0; i < TLBSHOOT__MAX; i++) {
		evcnt_attach_dynamic(&tlbstat_local[i], EVCNT_TYPE_MISC,
		    NULL, "tlbshoot local", tlbstat_name[i]);
	}
	for (i = 0; i < TLBSHOOT__MAX; i++) {
		evcnt_attach_dynamic(&tlbstat_remote[i], EVCNT_TYPE_MISC,
		    NULL, "tlbshoot remote", tlbstat_name[i]);
	}
	for (i = 0; i < TLBSHOOT__MAX; i++) {
		evcnt_attach_dynamic(&tlbstat_kernel[i], EVCNT_TYPE_MISC,
		    NULL, "tlbshoot kernel", tlbstat_name[i]);
	}
	evcnt_attach_dynamic(&tlbstat_single_req, EVCNT_TYPE_MISC,
	    NULL, "tlbshoot single page", "requests");
	evcnt_attach_dynamic(&tlbstat_single_issue, EVCNT_TYPE_MISC,
	    NULL, "tlbshoot single page", "issues");
#endif
}

void
pmap_tlb_cpu_init(struct cpu_info *ci)
{
	pmap_tlb_packet_t *tp = (pmap_tlb_packet_t *)ci->ci_pmap_data;

	memset(tp, 0, sizeof(pmap_tlb_packet_t));
	kcpuset_create(&tp->tp_cpumask, true);
}

static inline void
pmap_tlbstat_count(struct pmap *pm, vaddr_t va, tlbwhy_t why)
{
#ifdef TLBSTATS
	const cpuid_t cid = cpu_index(curcpu());
	bool local = false, remote = false;

	if (va != (vaddr_t)-1LL) {
		atomic_inc_64(&tlbstat_single_req.ev_count);
	}
	if (pm == pmap_kernel()) {
		atomic_inc_64(&tlbstat_kernel[why].ev_count);
		return;
	}

	if (va >= VM_MAXUSER_ADDRESS) {
		remote = kcpuset_isotherset(pm->pm_kernel_cpus, cid);
		local = kcpuset_isset(pm->pm_kernel_cpus, cid);
	}
	remote |= kcpuset_isotherset(pm->pm_cpus, cid);
	local |= kcpuset_isset(pm->pm_cpus, cid);

	if (local) {
		atomic_inc_64(&tlbstat_local[why].ev_count);
	}
	if (remote) {
		atomic_inc_64(&tlbstat_remote[why].ev_count);
	}
#endif
}

static inline void
pmap_tlb_invalidate(const pmap_tlb_packet_t *tp)
{
	int i;

	/* Find out what we need to invalidate. */
	if (tp->tp_count == (uint16_t)-1) {
		u_int egen = uvm_emap_gen_return();
		if (tp->tp_pte & PG_G) {
			/* Invalidating user and kernel TLB entries. */
			tlbflushg();
		} else {
			/* Invalidating user TLB entries only. */
			tlbflush();
		}
		uvm_emap_update(egen);
	} else {
		/* Invalidating a single page or a range of pages. */
		for (i = tp->tp_count - 1; i >= 0; i--) {
			pmap_update_pg(tp->tp_va[i]);
		}
	}
}

/*
 * pmap_tlb_shootdown: invalidate a page on all CPUs using pmap 'pm'.
 */
void
pmap_tlb_shootdown(struct pmap *pm, vaddr_t va, pt_entry_t pte, tlbwhy_t why)
{
	pmap_tlb_packet_t *tp;
	int s;

#ifndef XEN
	KASSERT((pte & PG_G) == 0 || pm == pmap_kernel());
#endif

	/*
	 * If tearing down the pmap, do nothing.  We will flush later
	 * when we are ready to recycle/destroy it.
	 */
	if (__predict_false(curlwp->l_md.md_gc_pmap == pm)) {
		return;
	}

	if ((pte & PG_PS) != 0) {
		va &= PG_LGFRAME;
	}

	/*
	 * Add the shootdown operation to our pending set.
	 */
	s = splvm();
	tp = (pmap_tlb_packet_t *)curcpu()->ci_pmap_data;

	/* Whole address flush will be needed if PG_G is set. */
	CTASSERT(PG_G == (uint16_t)PG_G);
	tp->tp_pte |= (uint16_t)pte;

	if (tp->tp_count == (uint16_t)-1) {
		/*
		 * Already flushing everything.
		 */
	} else if (tp->tp_count < TP_MAXVA && va != (vaddr_t)-1LL) {
		/* Flush a single page. */
		tp->tp_va[tp->tp_count++] = va;
		KASSERT(tp->tp_count > 0);
	} else {
		/* Flush everything. */
		tp->tp_count = (uint16_t)-1;
	}

	if (pm != pmap_kernel()) {
		kcpuset_merge(tp->tp_cpumask, pm->pm_cpus);
		if (va >= VM_MAXUSER_ADDRESS) {
			kcpuset_merge(tp->tp_cpumask, pm->pm_kernel_cpus);
		}
		tp->tp_userpmap = 1;
	} else {
		kcpuset_copy(tp->tp_cpumask, kcpuset_running);
	}
	pmap_tlbstat_count(pm, va, why);
	splx(s);
}

#ifdef MULTIPROCESSOR
#ifdef XEN

static inline void
pmap_tlb_processpacket(pmap_tlb_packet_t *tp, kcpuset_t *target)
{

	if (tp->tp_count != (uint16_t)-1) {
		/* Invalidating a single page or a range of pages. */
		for (int i = tp->tp_count - 1; i >= 0; i--) {
			xen_mcast_invlpg(tp->tp_va[i], target);
		}
	} else {
		xen_mcast_tlbflush(target);
	}

	/* Remote CPUs have been synchronously flushed. */
	pmap_tlb_pendcount = 0;
}

#else

static inline void
pmap_tlb_processpacket(pmap_tlb_packet_t *tp, kcpuset_t *target)
{
	int err = 0;

	if (!kcpuset_match(target, kcpuset_attached)) {
		const struct cpu_info * const self = curcpu();
		CPU_INFO_ITERATOR cii;
		struct cpu_info *lci;

		for (CPU_INFO_FOREACH(cii, lci)) {
			const cpuid_t lcid = cpu_index(lci);

			if (__predict_false(lci == self) ||
			    !kcpuset_isset(target, lcid)) {
				continue;
			}
			err |= x86_ipi(LAPIC_TLB_VECTOR,
			    lci->ci_cpuid, LAPIC_DLMODE_FIXED);
		}
	} else {
		err = x86_ipi(LAPIC_TLB_VECTOR, LAPIC_DEST_ALLEXCL,
		    LAPIC_DLMODE_FIXED);
	}
	KASSERT(err == 0);
}

#endif /* XEN */
#endif /* MULTIPROCESSOR */

/*
 * pmap_tlb_shootnow: process pending TLB shootdowns queued on current CPU.
 *
 * => Must be called with preemption disabled.
 */
void
pmap_tlb_shootnow(void)
{
	pmap_tlb_packet_t *tp;
	struct cpu_info *ci;
	kcpuset_t *target;
	u_int local, gen, rcpucount;
	cpuid_t cid;
	int s;

	KASSERT(kpreempt_disabled());

	ci = curcpu();
	tp = (pmap_tlb_packet_t *)ci->ci_pmap_data;

	/* Pre-check first. */
	if (tp->tp_count == 0) {
		return;
	}

	s = splvm();
	if (tp->tp_count == 0) {
		splx(s);
		return;
	}
	cid = cpu_index(ci);

	target = tp->tp_cpumask;
	local = kcpuset_isset(target, cid) ? 1 : 0;
	rcpucount = kcpuset_countset(target) - local;
	gen = 0;

#ifdef MULTIPROCESSOR
	if (rcpucount) {
		int count;

		/*
		 * Gain ownership of the shootdown mailbox.  We must stay
		 * at IPL_VM once we own it or could deadlock against an
		 * interrupt on this CPU trying to do the same.
		 */
		KASSERT(rcpucount < ncpu);

		while (atomic_cas_uint(&pmap_tlb_pendcount, 0, rcpucount)) {
			splx(s);
			count = SPINLOCK_BACKOFF_MIN;
			while (pmap_tlb_pendcount) {
				KASSERT(pmap_tlb_pendcount < ncpu);
				SPINLOCK_BACKOFF(count);
			}
			s = splvm();
			/* An interrupt might have done it for us. */
			if (tp->tp_count == 0) {
				splx(s);
				return;
			}
		}

		/*
		 * Start a new generation of updates.  Copy our shootdown
		 * requests into the global buffer.  Note that tp_cpumask
		 * will not be used by remote CPUs (it would be unsafe).
		 */
		gen = ++pmap_tlb_gen;
		memcpy(&pmap_tlb_packet, tp, sizeof(*tp));
		pmap_tlb_evcnt.ev_count++;

		/*
		 * Initiate shootdowns on remote CPUs.
		 */
		pmap_tlb_processpacket(tp, target);
	}
#endif

	/*
	 * Shootdowns on remote CPUs are now in flight.  In the meantime,
	 * perform local shootdown if needed.
	 */
	if (local) {
		pmap_tlb_invalidate(tp);
	}

	/*
	 * Clear out our local buffer.
	 */
#ifdef TLBSTATS
	if (tp->tp_count != (uint16_t)-1) {
		atomic_add_64(&tlbstat_single_issue.ev_count, tp->tp_count);
	}
#endif
	kcpuset_zero(tp->tp_cpumask);
	tp->tp_userpmap = 0;
	tp->tp_count = 0;
	tp->tp_pte = 0;
	splx(s);

	/*
	 * Now wait for the current generation of updates to be
	 * processed by remote CPUs.
	 */
	if (rcpucount && pmap_tlb_pendcount) {
		int count = SPINLOCK_BACKOFF_MIN;

		while (pmap_tlb_pendcount && pmap_tlb_gen == gen) {
			KASSERT(pmap_tlb_pendcount < ncpu);
			SPINLOCK_BACKOFF(count);
		}
	}
}

/*
 * pmap_tlb_intr: pmap shootdown interrupt handler to invalidate TLB entries.
 *
 * => Called from IPI only.
 */
void
pmap_tlb_intr(void)
{
	const pmap_tlb_packet_t *tp = &pmap_tlb_packet;
	struct cpu_info *ci = curcpu();

	KASSERT(pmap_tlb_pendcount > 0);

	/* First, TLB flush. */
	pmap_tlb_invalidate(tp);

	/*
	 * Check the current TLB state.  If we do not want further
	 * invalidations for this pmap, then take the CPU out of
	 * the pmap's bitmask.
	 */
	if (ci->ci_tlbstate == TLBSTATE_LAZY && tp->tp_userpmap) {
		struct pmap *pm = ci->ci_pmap;
		cpuid_t cid = cpu_index(ci);

		kcpuset_atomic_clear(pm->pm_cpus, cid);
		ci->ci_tlbstate = TLBSTATE_STALE;
	}

	/* Finally, ack the request. */
	atomic_dec_uint(&pmap_tlb_pendcount);
}
