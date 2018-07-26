/*	$NetBSD: xen_pmap.c,v 1.27 2018/07/26 17:20:09 maxv Exp $	*/

/*
 * Copyright (c) 2007 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2006 Mathieu Ropert <mro@adviseo.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2001 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_pmap.c,v 1.27 2018/07/26 17:20:09 maxv Exp $");

#include "opt_user_ldt.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm.h>

#include <dev/isa/isareg.h>

#include <machine/specialreg.h>
#include <machine/gdt.h>
#include <machine/isa_machdep.h>
#include <machine/cpuvar.h>

#include <x86/pmap.h>
#include <x86/pmap_pv.h>

#include <x86/i82489reg.h>
#include <x86/i82489var.h>

#include <xen/xen-public/xen.h>
#include <xen/hypervisor.h>
#include <xen/xenpmap.h>

#define COUNT(x)	/* nothing */

extern pd_entry_t * const normal_pdes[];

extern paddr_t pmap_pa_start; /* PA of first physical page for this domain */
extern paddr_t pmap_pa_end;   /* PA of last physical page for this domain */

int
pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	paddr_t ma;

	if (__predict_false(pa < pmap_pa_start || pmap_pa_end <= pa)) {
		ma = pa; /* XXX hack */
	} else {
		ma = xpmap_ptom(pa);
	}

	return pmap_enter_ma(pmap, va, ma, pa, prot, flags, DOMID_SELF);
}

/*
 * pmap_kenter_ma: enter a kernel mapping without R/M (pv_entry) tracking
 *
 * => no need to lock anything, assume va is already allocated
 * => should be faster than normal pmap enter function
 * => we expect a MACHINE address
 */

void
pmap_kenter_ma(vaddr_t va, paddr_t ma, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte, opte, npte;

	if (va < VM_MIN_KERNEL_ADDRESS)
		pte = vtopte(va);
	else
		pte = kvtopte(va);

	npte = ma | ((prot & VM_PROT_WRITE) ? PG_RW : PG_RO) | PG_V;
	if (flags & PMAP_NOCACHE)
		npte |= PG_N;

	if ((cpu_feature[2] & CPUID_NOX) && !(prot & VM_PROT_EXECUTE))
		npte |= PG_NX;

	opte = pmap_pte_testset(pte, npte); /* zap! */

	if (pmap_valid_entry(opte)) {
#if defined(MULTIPROCESSOR)
		if (__predict_false(x86_mp_online == false)) {
			pmap_update_pg(va);
		} else {
			kpreempt_disable();
			pmap_tlb_shootdown(pmap_kernel(), va, opte,
			    TLBSHOOT_KENTER);
			kpreempt_enable();
		}
#else
		/* Don't bother deferring in the single CPU case. */
		pmap_update_pg(va);
#endif
	}
}

/*
 * pmap_extract_ma: extract a MA for the given VA
 */

bool
pmap_extract_ma(struct pmap *pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *ptes, pte;
	pd_entry_t pde;
	pd_entry_t * const *pdes;
	struct pmap *pmap2;

	kpreempt_disable();
	pmap_map_ptes(pmap, &pmap2, &ptes, &pdes);
	if (!pmap_pdes_valid(va, pdes, &pde)) {
		pmap_unmap_ptes(pmap, pmap2);
		kpreempt_enable();
		return false;
	}

	pte = ptes[pl1_i(va)];
	pmap_unmap_ptes(pmap, pmap2);
	kpreempt_enable();

	if (__predict_true((pte & PG_V) != 0)) {
		if (pap != NULL)
			*pap = (pte & PG_FRAME) | (va & (NBPD_L1 - 1));
		return true;
	}

	return false;
}

/*
 * Xen pmap's handlers for save/restore
 */
void
pmap_xen_suspend(void)
{
	pmap_unmap_recursive_entries();

	xpq_flush_queue();
}

void
pmap_xen_resume(void)
{
	pmap_map_recursive_entries();

	xpq_flush_queue();
}

/*
 * NetBSD uses L2 shadow pages to support PAE with Xen. However, Xen does not
 * handle them correctly during save/restore, leading to incorrect page
 * tracking and pinning during restore.
 * For save/restore to succeed, two functions are introduced:
 * - pmap_map_recursive_entries(), used by resume code to set the recursive
 *   mapping entries to their correct value
 * - pmap_unmap_recursive_entries(), used by suspend code to clear all
 *   PDIR_SLOT_PTE entries
 */
void
pmap_map_recursive_entries(void)
{
	int i;
	struct pmap *pm;

	mutex_enter(&pmaps_lock);
	LIST_FOREACH(pm, &pmaps, pm_list) {
		for (i = 0; i < PDP_SIZE; i++) {
			xpq_queue_pte_update(
			    xpmap_ptom(pmap_pdirpa(pm, PDIR_SLOT_PTE + i)),
			    xpmap_ptom((pm)->pm_pdirpa[i]) | PG_V);
		}
	}
	mutex_exit(&pmaps_lock);

	for (i = 0; i < PDP_SIZE; i++) {
		xpq_queue_pte_update(
		    xpmap_ptom(pmap_pdirpa(pmap_kernel(), PDIR_SLOT_PTE + i)),
		    xpmap_ptom(pmap_kernel()->pm_pdirpa[i]) | PG_V);
	}
}

/*
 * Unmap recursive entries found in pmaps. Required during Xen
 * save/restore operations, as Xen does not handle recursive mappings
 * properly.
 */
void
pmap_unmap_recursive_entries(void)
{
	int i;
	struct pmap *pm;

	/*
	 * Invalidate pmap_pdp_cache as it contains L2-pinned objects with
	 * recursive entries.
	 * XXX jym@ : find a way to drain per-CPU caches to. pool_cache_inv
	 * does not do that.
	 */
	pool_cache_invalidate(&pmap_pdp_cache);

	mutex_enter(&pmaps_lock);
	LIST_FOREACH(pm, &pmaps, pm_list) {
		for (i = 0; i < PDP_SIZE; i++) {
			xpq_queue_pte_update(
			    xpmap_ptom(pmap_pdirpa(pm, PDIR_SLOT_PTE + i)), 0);
		}
	}
	mutex_exit(&pmaps_lock);

	/* do it for pmap_kernel() too! */
	for (i = 0; i < PDP_SIZE; i++) {
		xpq_queue_pte_update(
		    xpmap_ptom(pmap_pdirpa(pmap_kernel(), PDIR_SLOT_PTE + i)),
		    0);
	}
}

static __inline void
pmap_kpm_setpte(struct cpu_info *ci, struct pmap *pmap, int index)
{
	KASSERT(mutex_owned(pmap->pm_lock));
	KASSERT(mutex_owned(&ci->ci_kpm_mtx));
	if (pmap == pmap_kernel()) {
		KASSERT(index >= PDIR_SLOT_KERN);
	}

#ifdef __x86_64__
	xpq_queue_pte_update(
	    xpmap_ptetomach(&ci->ci_kpm_pdir[index]),
	    pmap->pm_pdir[index]);
#else
	xpq_queue_pte_update(
	    xpmap_ptetomach(&ci->ci_kpm_pdir[l2tol2(index)]),
	    pmap->pm_pdir[index]);
#endif

	xpq_flush_queue();
}

/*
 * Synchronise shadow pdir with the pmap on all cpus on which it is
 * loaded.
 */
void
xen_kpm_sync(struct pmap *pmap, int index)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(pmap != NULL);
	KASSERT(kpreempt_disabled());

	pmap_pte_flush();

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == NULL) {
			continue;
		}
		cpuid_t cid = cpu_index(ci);
		if (pmap != pmap_kernel() &&
		    !kcpuset_isset(pmap->pm_xen_ptp_cpus, cid))
			continue;

		/* take the lock and check again */
		mutex_enter(&ci->ci_kpm_mtx);
		if (pmap == pmap_kernel() ||
		    kcpuset_isset(pmap->pm_xen_ptp_cpus, cid)) {
			pmap_kpm_setpte(ci, pmap, index);
		}
		mutex_exit(&ci->ci_kpm_mtx);
	}
}
