/*	$NetBSD: pmap_tlb.c,v 1.1.2.1 2010/05/26 04:55:24 rmind Exp $	*/

/*-
 * Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * TLB shootdowns are hard interrupts that operate outside the SPL framework:
 * They do not need to be blocked, provided that the pmap module gets the
 * order of events correct.  The calls are made by poking the LAPIC directly.
 * The stub to handle the interrupts is short and does one of the following:
 * invalidate a set of pages, all user TLB entries or the entire TLB.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_tlb.c,v 1.1.2.1 2010/05/26 04:55:24 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <uvm/uvm.h>

#include <machine/cpuvar.h>
#include <x86/pmap.h>
#include <x86/i82489reg.h>
#include <x86/i82489var.h>

/*
 * TLB shootdown state.
 */
static struct evcnt		pmap_tlb_evcnt __aligned(64);
struct pmap_tlb_packet		pmap_tlb_packet __aligned(64);
struct pmap_tlb_mailbox		pmap_tlb_mailbox __aligned(64);

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

	evcnt_attach_dynamic(&pmap_tlb_evcnt, EVCNT_TYPE_INTR,
	    NULL, "TLB", "shootdown");

#ifdef TLBSTATS
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

static inline void
pmap_tlbstat_count(struct pmap *pm, vaddr_t va, tlbwhy_t why)
{
#ifdef TLBSTATS
	uint32_t mask;

	if (va != (vaddr_t)-1LL) {
		atomic_inc_64(&tlbstat_single_req.ev_count);
	}
	if (pm == pmap_kernel()) {
		atomic_inc_64(&tlbstat_kernel[why].ev_count);
		return;
	}
	if (va >= VM_MAXUSER_ADDRESS) {
		mask = pm->pm_cpus | pm->pm_kernel_cpus;
	} else {
		mask = pm->pm_cpus;
	}
	if ((mask & curcpu()->ci_cpumask) != 0) {
		atomic_inc_64(&tlbstat_local[why].ev_count);
	}
	if ((mask & ~curcpu()->ci_cpumask) != 0) {
		atomic_inc_64(&tlbstat_remote[why].ev_count);
	}
#endif
}

/*
 * pmap_tlb_shootdown: invalidate a page on all CPUs using pmap 'pm'
 */
void
pmap_tlb_shootdown(struct pmap *pm, vaddr_t va, pt_entry_t pte, tlbwhy_t why)
{
	struct pmap_tlb_packet *tp;
	int s;

	KASSERT((pte & PG_G) == 0 || pm == pmap_kernel());

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
	tp = (struct pmap_tlb_packet *)curcpu()->ci_pmap_data;
	tp->tp_pte |= (uint16_t)pte;
	if (tp->tp_count < TP_MAXVA && va != (vaddr_t)-1LL) {
		/* Flush a single page. */
		tp->tp_va[tp->tp_count++] = va;
	} else {
		/* Flush everything. */
		tp->tp_count = (uint16_t)-1;
	}

	if (pm == pmap_kernel()) {
		tp->tp_cpumask = cpus_running;
	} else if (va >= VM_MAXUSER_ADDRESS) {
		tp->tp_cpumask |= (pm->pm_cpus | pm->pm_kernel_cpus);
		tp->tp_usermask |= (pm->pm_cpus | pm->pm_kernel_cpus);
	} else {
		tp->tp_cpumask |= pm->pm_cpus;
		tp->tp_usermask |= pm->pm_cpus;
	}
	pmap_tlbstat_count(pm, va, why);
	splx(s);
}

/*
 * pmap_tlb_shootnow: process pending TLB shootdowns queued on current CPU.
 *
 * => Must be called with preemption disabled.
 */
void
pmap_tlb_shootnow(void)
{
	struct pmap_tlb_packet *tp;
	struct pmap_tlb_mailbox *tm;
	struct cpu_info *ci, *lci;
	CPU_INFO_ITERATOR cii;
	uint32_t remote;
	uintptr_t gen;
	int s, err, i, count;

	KASSERT(kpreempt_disabled());

	s = splvm();
	ci = curcpu();
	tp = (struct pmap_tlb_packet *)ci->ci_pmap_data;
	if (tp->tp_count == 0) {
		splx(s);
		return;
	}
	gen = 0; /* XXXgcc */
	tm = &pmap_tlb_mailbox;
	remote = tp->tp_cpumask & ~ci->ci_cpumask;
	if (remote != 0) {
		/*
		 * Gain ownership of the shootdown mailbox.  We must stay
		 * at IPL_VM once we own it or could deadlock against an
		 * interrupt on this CPU trying to do the same.
		 */
		while (atomic_cas_32(&tm->tm_pending, 0, remote) != 0) {
			splx(s);
			count = SPINLOCK_BACKOFF_MIN;
			while (tm->tm_pending != 0) {
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
		 * requests into the global buffer.
		 */
		gen = ++tm->tm_gen;
		memcpy(&pmap_tlb_packet, tp, sizeof(*tp));
		pmap_tlb_evcnt.ev_count++;

		/*
		 * Initiate shootdowns on remote CPUs.
		 */
		if (tp->tp_cpumask == cpus_running) {
			err = x86_ipi(LAPIC_TLB_VECTOR, LAPIC_DEST_ALLEXCL,
			    LAPIC_DLMODE_FIXED);
		} else {
			err = 0;
			for (CPU_INFO_FOREACH(cii, lci)) {
				if ((lci->ci_cpumask & remote) == 0) {
					continue;
				}
				if ((lci->ci_flags & CPUF_RUNNING) == 0) {
					remote &= ~lci->ci_cpumask;
					atomic_and_32(&tm->tm_pending, remote);
					continue;
				}
				err |= x86_ipi(LAPIC_TLB_VECTOR,
				lci->ci_cpuid, LAPIC_DLMODE_FIXED);
			}
		}
		if (__predict_false(err != 0)) {
			panic("pmap_tlb_shootdown: IPI failed");
		}
	}

	/*
	 * Shootdowns on remote CPUs are now in flight.  In the meantime,
	 * perform local shootdowns and do not forget to update emap gen.
	 */
	if ((tp->tp_cpumask & ci->ci_cpumask) != 0) {
		if (tp->tp_count == (uint16_t)-1) {
			u_int egen = uvm_emap_gen_return();
			if ((tp->tp_pte & PG_G) != 0) {
				tlbflushg();
			} else {
				tlbflush();
			}
			uvm_emap_update(egen);
		} else {
			for (i = tp->tp_count - 1; i >= 0; i--) {
				pmap_update_pg(tp->tp_va[i]);
			}
		}
	}

	/*
	 * Clear out our local buffer.
	 */
#ifdef TLBSTATS
	if (tp->tp_count != (uint16_t)-1) {
		atomic_add_64(&tlbstat_single_issue.ev_count, tp->tp_count);
	}
#endif
	tp->tp_count = 0;
	tp->tp_pte = 0;
	tp->tp_cpumask = 0;
	tp->tp_usermask = 0;
	splx(s);

	/*
	 * Now wait for the current generation of updates to be
	 * processed by remote CPUs.
	 */
	if (remote != 0 && tm->tm_pending != 0) {
		count = SPINLOCK_BACKOFF_MIN;
		while (tm->tm_pending != 0 && tm->tm_gen == gen) {
			SPINLOCK_BACKOFF(count);
		}
	}
}
