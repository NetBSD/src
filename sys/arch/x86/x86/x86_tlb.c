/*	$NetBSD: x86_tlb.c,v 1.20 2022/08/20 23:48:51 riastradh Exp $	*/

/*-
 * Copyright (c) 2008-2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: x86_tlb.c,v 1.20 2022/08/20 23:48:51 riastradh Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <uvm/uvm.h>

#include <machine/cpuvar.h>
#include <machine/pmap_private.h>

#ifdef XENPV
#include <xen/xenpmap.h>
#endif /* XENPV */
#include <x86/i82489reg.h>
#include <x86/i82489var.h>

/*
 * TLB shootdown packet.  Each CPU has a copy of this packet, where we build
 * sets of TLB shootdowns.  If shootdowns need to occur on remote CPUs, the
 * packet is copied into a shared mailbox kept on the initiator's kernel
 * stack.  Once the copy is made, no further updates to the mailbox are made
 * until the request is completed.  This keeps the cache line in the shared
 * state, and bus traffic to a minimum.
 *
 * In order to make maximal use of the available space, control fields are
 * overlaid into the lower 12 bits of the first 4 virtual addresses.  This
 * is very ugly, but it counts.
 *
 * On i386 the packet is 64 bytes in size.  On amd64 it's 128 bytes.  This
 * is sized in concert with UBC_WINSIZE, otherwise excessive shootdown
 * interrupts could be isssued.
 */

#define	TP_MAXVA	16		/* for individual mappings */
#define	TP_ALLVA	PAGE_MASK	/* special: shoot all mappings */

typedef struct {
	uintptr_t		tp_store[TP_MAXVA];
} pmap_tlb_packet_t;

#define	TP_COUNT	0
#define	TP_USERPMAP	1
#define	TP_GLOBAL	2
#define	TP_DONE		3

#define	TP_GET_COUNT(tp)	((tp)->tp_store[TP_COUNT] & PAGE_MASK)
#define	TP_GET_USERPMAP(tp)	((tp)->tp_store[TP_USERPMAP] & 1)
#define	TP_GET_GLOBAL(tp)	((tp)->tp_store[TP_GLOBAL] & 1)
#define	TP_GET_DONE(tp)		(atomic_load_relaxed(&(tp)->tp_store[TP_DONE]) & 1)
#define	TP_GET_VA(tp, i)	((tp)->tp_store[(i)] & ~PAGE_MASK)

#define	TP_INC_COUNT(tp)	((tp)->tp_store[TP_COUNT]++)
#define	TP_SET_ALLVA(tp)	((tp)->tp_store[TP_COUNT] |= TP_ALLVA)
#define	TP_SET_VA(tp, c, va)	((tp)->tp_store[(c)] |= ((va) & ~PAGE_MASK))

#define	TP_SET_USERPMAP(tp)	((tp)->tp_store[TP_USERPMAP] |= 1)
#define	TP_SET_GLOBAL(tp)	((tp)->tp_store[TP_GLOBAL] |= 1)
#define	TP_SET_DONE(tp)							     \
	do {								     \
		uintptr_t v = atomic_load_relaxed(&(tp)->tp_store[TP_DONE]); \
		atomic_store_relaxed(&(tp)->tp_store[TP_DONE], v | 1);	     \
	} while (/* CONSTCOND */ 0);

#define	TP_CLEAR(tp)		memset(__UNVOLATILE(tp), 0, sizeof(*(tp)));

/*
 * TLB shootdown state.
 */
static volatile pmap_tlb_packet_t *volatile pmap_tlb_packet __cacheline_aligned;
static volatile u_int		pmap_tlb_pendcount	__cacheline_aligned;
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
	"REMOVE_ALL",
	"KENTER",
	"KREMOVE",
	"FREE_PTP",
	"REMOVE_PTE",
	"SYNC_PV",
	"WRITE_PROTECT",
	"ENTER",
	"NVMM",
	"BUS_DMA",
	"BUS_SPACE",
};
#endif

void
pmap_tlb_init(void)
{

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
	kcpuset_create(&ci->ci_tlb_cpuset, true);
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
pmap_tlb_invalidate(volatile pmap_tlb_packet_t *tp)
{
	int i = TP_GET_COUNT(tp);

	/* Find out what we need to invalidate. */
	if (i == TP_ALLVA) {
		if (TP_GET_GLOBAL(tp) != 0) {
			/* Invalidating all TLB entries. */
			tlbflushg();
		} else {
			/* Invalidating non-global TLB entries only. */
			tlbflush();
		}
	} else {
		/* Invalidating a single page or a range of pages. */
		KASSERT(i != 0);
		do {
			--i;
			pmap_update_pg(TP_GET_VA(tp, i));
		} while (i > 0);
	}
}

/*
 * pmap_tlb_shootdown: invalidate a page on all CPUs using pmap 'pm'.
 */
void
pmap_tlb_shootdown(struct pmap *pm, vaddr_t va, pt_entry_t pte, tlbwhy_t why)
{
	pmap_tlb_packet_t *tp;
	struct cpu_info *ci;
	uint8_t count;
	int s;

#ifndef XENPV
	KASSERT((pte & PTE_G) == 0 || pm == pmap_kernel());
#endif

	if (__predict_false(pm->pm_tlb_flush != NULL)) {
		(*pm->pm_tlb_flush)(pm);
		return;
	}

	if ((pte & PTE_PS) != 0) {
		va &= PTE_LGFRAME;
	}

	/*
	 * Add the shootdown operation to our pending set.
	 */
	s = splvm();
	ci = curcpu();
	tp = (pmap_tlb_packet_t *)ci->ci_pmap_data;

	/* Whole address flush will be needed if PTE_G is set. */
	if ((pte & PTE_G) != 0) {
		TP_SET_GLOBAL(tp);
	}
	count = TP_GET_COUNT(tp);

	if (count < TP_MAXVA && va != (vaddr_t)-1LL) {
		/* Flush a single page. */
		TP_SET_VA(tp, count, va);
		TP_INC_COUNT(tp);
	} else {
		/* Flush everything - may already be set. */
		TP_SET_ALLVA(tp);
	}

	if (pm != pmap_kernel()) {
		kcpuset_merge(ci->ci_tlb_cpuset, pm->pm_cpus);
		if (va >= VM_MAXUSER_ADDRESS) {
			kcpuset_merge(ci->ci_tlb_cpuset, pm->pm_kernel_cpus);
		}
		TP_SET_USERPMAP(tp);
	} else {
		kcpuset_copy(ci->ci_tlb_cpuset, kcpuset_running);
	}
	pmap_tlbstat_count(pm, va, why);
	splx(s);
}

#ifdef XENPV

static inline void
pmap_tlb_processpacket(volatile pmap_tlb_packet_t *tp, kcpuset_t *target)
{
#ifdef MULTIPROCESSOR
	int i = TP_GET_COUNT(tp);

	if (i != TP_ALLVA) {
		/* Invalidating a single page or a range of pages. */
		KASSERT(i != 0);
		do {
			--i;
			xen_mcast_invlpg(TP_GET_VA(tp, i), target);
		} while (i > 0);
	} else {
		xen_mcast_tlbflush(target);
	}

	/* Remote CPUs have been synchronously flushed. */
	pmap_tlb_pendcount = 0;
	pmap_tlb_packet = NULL;
	TP_SET_DONE(tp);
#endif /* MULTIPROCESSOR */
}

#else

static inline void
pmap_tlb_processpacket(volatile pmap_tlb_packet_t *tp, kcpuset_t *target)
{
#ifdef MULTIPROCESSOR
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
#endif /* MULTIPROCESSOR */
}

#endif /* XENPV */

/*
 * pmap_tlb_shootnow: process pending TLB shootdowns queued on current CPU.
 *
 * => Must be called with preemption disabled.
 */
void
pmap_tlb_shootnow(void)
{
	volatile pmap_tlb_packet_t *tp, *ts;
	volatile uint8_t stackbuf[sizeof(*tp) + COHERENCY_UNIT];
	struct cpu_info *ci;
	kcpuset_t *target;
	u_int local, rcpucount;
	cpuid_t cid;
	int s;

	KASSERT(kpreempt_disabled());

	/* Pre-check first. */
	ci = curcpu();
	tp = (pmap_tlb_packet_t *)ci->ci_pmap_data;
	if (TP_GET_COUNT(tp) == 0) {
		return;
	}

	/* An interrupt may have flushed our updates, so check again. */
	s = splvm();
	if (TP_GET_COUNT(tp) == 0) {
		splx(s);
		return;
	}

	cid = cpu_index(ci);
	target = ci->ci_tlb_cpuset;
	local = kcpuset_isset(target, cid) ? 1 : 0;
	rcpucount = kcpuset_countset(target) - local;

	/*
	 * Fast path for local shootdowns only.  Do the shootdowns, and
	 * clear out the buffer for the next user.
	 */
	if (rcpucount == 0) {
		pmap_tlb_invalidate(tp);
		kcpuset_zero(ci->ci_tlb_cpuset);
		TP_CLEAR(tp);
		splx(s);
		return;
	}

	/*
	 * Copy the packet into the stack buffer, and gain ownership of the
	 * global pointer.  We must keep interrupts blocked once we own the
	 * pointer and until the IPIs are triggered, or we could deadlock
	 * against an interrupt on the current CPU trying the same.
	 */
	KASSERT(rcpucount < ncpu);
	ts = (void *)roundup2((uintptr_t)stackbuf, COHERENCY_UNIT);
	*ts = *tp;
	KASSERT(TP_GET_DONE(ts) == 0);
	while (atomic_cas_ptr(&pmap_tlb_packet, NULL,
	    __UNVOLATILE(ts)) != NULL) {
		KASSERT(atomic_load_relaxed(&pmap_tlb_packet) != ts);
		/*
		 * Don't bother with exponentional backoff, as the pointer
		 * is in a dedicated cache line and only updated twice per
		 * IPI (in contrast to the pending counter).  The cache
		 * line will spend most of its time in the SHARED state.
		 */
		splx(s);
		do {
			x86_pause();
		} while (atomic_load_relaxed(&pmap_tlb_packet) != NULL);
		s = splvm();

		/*
		 * An interrupt might have done the shootdowns for
		 * us while we spun.
		 */
		if (TP_GET_COUNT(tp) == 0) {
			splx(s);
			return;
		}
	}

	/*
	 * Ownership of the global pointer provides serialization of the
	 * update to the count and the event counter.  With those values
	 * updated, start shootdowns on remote CPUs.
	 */
	pmap_tlb_pendcount = rcpucount;
	pmap_tlb_evcnt.ev_count++;
	pmap_tlb_processpacket(ts, target);

	/*
	 * Clear out the local CPU's buffer for the next user.  Once done,
	 * we can drop the IPL.
	 */
#ifdef TLBSTATS
	if (TP_GET_COUNT(tp) != TP_ALLVA) {
		atomic_add_64(&tlbstat_single_issue.ev_count,
		    TP_GET_COUNT(tp));
	}
#endif
	kcpuset_zero(ci->ci_tlb_cpuset);
	TP_CLEAR(tp);
	splx(s);

	/*
	 * Shootdowns on remote CPUs are now in flight.  In the meantime,
	 * perform local shootdown if needed, using our copy of the packet.
	 */
	if (local) {
		pmap_tlb_invalidate(ts);
	}

	/*
	 * Wait for the updates to be processed by remote CPUs.  Poll the
	 * flag in the packet in order to limit bus traffic (only the last
	 * CPU out will update it and only we are reading it).  No memory
	 * barrier required due to prior stores - yay x86.
	 */
	while (TP_GET_DONE(ts) == 0) {
		x86_pause();
	}
}

/*
 * pmap_tlb_intr: pmap shootdown interrupt handler to invalidate TLB entries.
 *
 * Called from IPI only.  We are outside the SPL framework, with interrupts
 * disabled on the CPU: be careful.
 *
 * TLB flush and the interrupt that brought us here are serializing
 * operations (they defeat speculative execution).  Any speculative load
 * producing a TLB fill between receipt of the interrupt and the TLB flush
 * will load "current" PTEs.  None of the mappings relied on by this ISR for
 * its execution will be changing.  So it's safe to acknowledge the request
 * and allow the initiator to proceed before performing the flush.
 */
void
pmap_tlb_intr(void)
{
	pmap_tlb_packet_t copy;
	volatile pmap_tlb_packet_t *source;
	struct cpu_info *ci;

	/* Make a private copy of the packet. */
	source = pmap_tlb_packet;
	copy = *source;

	/*
	 * If we are the last CPU out, clear the active pointer and mark the
	 * packet as done.  Both can be done without using an atomic, and
	 * the one atomic we do use serves as our memory barrier.
	 *
	 * It's important to clear the active pointer before setting
	 * TP_DONE, to ensure a remote CPU does not exit & re-enter
	 * pmap_tlb_shootnow() only to find its current pointer still
	 * seemingly active.
	 */
	if (atomic_dec_uint_nv(&pmap_tlb_pendcount) == 0) {
		atomic_store_relaxed(&pmap_tlb_packet, NULL);
		__insn_barrier();
		TP_SET_DONE(source);
	}
	pmap_tlb_invalidate(&copy);

	/*
	 * Check the current TLB state.  If we don't want further flushes
	 * for this pmap, then take the CPU out of the pmap's set.  The
	 * order of updates to the set and TLB state must closely align with
	 * the pmap code, as we can interrupt code running in the pmap
	 * module.
	 */
	ci = curcpu();
	if (ci->ci_tlbstate == TLBSTATE_LAZY && TP_GET_USERPMAP(&copy) != 0) {
		kcpuset_atomic_clear(ci->ci_pmap->pm_cpus, cpu_index(ci));
		ci->ci_tlbstate = TLBSTATE_STALE;
	}
}
