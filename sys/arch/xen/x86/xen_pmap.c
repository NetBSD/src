/*	$NetBSD: xen_pmap.c,v 1.15 2012/01/22 18:16:34 cherry Exp $	*/

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
 *
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
__KERNEL_RCSID(0, "$NetBSD: xen_pmap.c,v 1.15 2012/01/22 18:16:34 cherry Exp $");

#include "opt_user_ldt.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#if !defined(__x86_64__)
#include "opt_kstack_dr0.h"
#endif /* !defined(__x86_64__) */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/xcall.h>

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

static pd_entry_t * const alternate_pdes[] = APDES_INITIALIZER;
extern pd_entry_t * const normal_pdes[];

extern paddr_t pmap_pa_start; /* PA of first physical page for this domain */
extern paddr_t pmap_pa_end;   /* PA of last physical page for this domain */

void
pmap_apte_flush(struct pmap *pmap)
{

	KASSERT(kpreempt_disabled());

	/*
	 * Flush the APTE mapping from all other CPUs that
	 * are using the pmap we are using (who's APTE space
	 * is the one we've just modified).
	 *
	 * XXXthorpej -- find a way to defer the IPI.
	 */
	pmap_tlb_shootdown(pmap, (vaddr_t)-1LL, 0, TLBSHOOT_APTE);
	pmap_tlb_shootnow();
}

/*
 * Unmap the content of APDP PDEs
 */
void
pmap_unmap_apdp(void)
{
	int i;

	for (i = 0; i < PDP_SIZE; i++) {
		pmap_pte_set(APDP_PDE+i, 0);
#if defined (PAE)
		/*
		 * For PAE, there are two places where alternative recursive
		 * mappings could be found with Xen:
		 * - in the L2 shadow pages
		 * - the "real" L2 kernel page (pmap_kl2pd), which is unique
		 * and static.
		 * We first clear the APDP for the current pmap. As L2 kernel
		 * page is unique, we only need to do it once for all pmaps.
		 */
		pmap_pte_set(APDP_PDE_SHADOW+i, 0);
#endif
	}
}

/*
 * pmap_map_ptes: map a pmap's PTEs into KVM and lock them in
 *
 * => we lock enough pmaps to keep things locked in
 * => must be undone with pmap_unmap_ptes before returning
 */

void
pmap_map_ptes(struct pmap *pmap, struct pmap **pmap2,
	      pd_entry_t **ptepp, pd_entry_t * const **pdeppp)
{
	pd_entry_t opde, npde;
	struct pmap *ourpmap;
	struct cpu_info *ci;
	struct lwp *l;
	bool iscurrent;
	uint64_t ncsw;
	int s;

	/* the kernel's pmap is always accessible */
	if (pmap == pmap_kernel()) {
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		return;
	}
	KASSERT(kpreempt_disabled());

 retry:
	l = curlwp;
	ncsw = l->l_ncsw;
 	ourpmap = NULL;
	ci = curcpu();
#if defined(__x86_64__)
	/*
	 * curmap can only be pmap_kernel so at this point
	 * pmap_is_curpmap is always false
	 */
	iscurrent = 0;
	ourpmap = pmap_kernel();
#else /* __x86_64__*/
	if (ci->ci_want_pmapload &&
	    vm_map_pmap(&l->l_proc->p_vmspace->vm_map) == pmap) {
		pmap_load();
		if (l->l_ncsw != ncsw)
			goto retry;
	}
	iscurrent = pmap_is_curpmap(pmap);
	/* if curpmap then we are always mapped */
	if (iscurrent) {
		mutex_enter(pmap->pm_lock);
		*pmap2 = NULL;
		*ptepp = PTE_BASE;
		*pdeppp = normal_pdes;
		goto out;
	}
	ourpmap = ci->ci_pmap;
#endif /* __x86_64__ */

	/* need to lock both curpmap and pmap: use ordered locking */
	pmap_reference(ourpmap);
	if ((uintptr_t) pmap < (uintptr_t) ourpmap) {
		mutex_enter(pmap->pm_lock);
		mutex_enter(ourpmap->pm_lock);
	} else {
		mutex_enter(ourpmap->pm_lock);
		mutex_enter(pmap->pm_lock);
	}

	if (l->l_ncsw != ncsw)
		goto unlock_and_retry;

	/* need to load a new alternate pt space into curpmap? */
	COUNT(apdp_pde_map);
	opde = *APDP_PDE;
	if (!pmap_valid_entry(opde) ||
	    pmap_pte2pa(opde) != pmap_pdirpa(pmap, 0)) {
		int i;
		s = splvm();
		/* Make recursive entry usable in user PGD */
		for (i = 0; i < PDP_SIZE; i++) {
			npde = pmap_pa2pte(
			    pmap_pdirpa(pmap, i * NPDPG)) | PG_k | PG_V;
			xpq_queue_pte_update(
			    xpmap_ptom(pmap_pdirpa(pmap, PDIR_SLOT_PTE + i)),
			    npde);
			xpq_queue_pte_update(xpmap_ptetomach(&APDP_PDE[i]),
			    npde);
#ifdef PAE
			/* update shadow entry too */
			xpq_queue_pte_update(
			    xpmap_ptetomach(&APDP_PDE_SHADOW[i]), npde);
#endif /* PAE */
			xpq_queue_invlpg(
			    (vaddr_t)&pmap->pm_pdir[PDIR_SLOT_PTE + i]);
		}
		if (pmap_valid_entry(opde))
			pmap_apte_flush(ourpmap);
		splx(s);
	}
	*pmap2 = ourpmap;
	*ptepp = APTE_BASE;
	*pdeppp = alternate_pdes;
	KASSERT(l->l_ncsw == ncsw);
#if !defined(__x86_64__)
 out:
#endif
 	/*
 	 * might have blocked, need to retry?
 	 */
	if (l->l_ncsw != ncsw) {
 unlock_and_retry:
	    	if (ourpmap != NULL) {
			mutex_exit(ourpmap->pm_lock);
			pmap_destroy(ourpmap);
		}
		mutex_exit(pmap->pm_lock);
		goto retry;
	}
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 */

void
pmap_unmap_ptes(struct pmap *pmap, struct pmap *pmap2)
{

	if (pmap == pmap_kernel()) {
		return;
	}
	KASSERT(kpreempt_disabled());
	if (pmap2 == NULL) {
		mutex_exit(pmap->pm_lock);
	} else {
#if defined(__x86_64__)
		KASSERT(pmap2 == pmap_kernel());
#else
		KASSERT(curcpu()->ci_pmap == pmap2);
#endif
#if defined(MULTIPROCESSOR)
		pmap_unmap_apdp();
		pmap_pte_flush();
		pmap_apte_flush(pmap2);
#endif /* MULTIPROCESSOR */
		COUNT(apdp_pde_unmap);
		mutex_exit(pmap->pm_lock);
		mutex_exit(pmap2->pm_lock);
		pmap_destroy(pmap2);
	}
}

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

	npte = ma | ((prot & VM_PROT_WRITE) ? PG_RW : PG_RO) |
	     PG_V | PG_k;
	if (flags & PMAP_NOCACHE)
		npte |= PG_N;

	if ((cpu_feature[2] & CPUID_NOX) && !(prot & VM_PROT_EXECUTE))
		npte |= PG_NX;

	opte = pmap_pte_testset (pte, npte); /* zap! */

	if (pmap_valid_entry(opte)) {
#if defined(MULTIPROCESSOR)
		kpreempt_disable();
		pmap_tlb_shootdown(pmap_kernel(), va, opte, TLBSHOOT_KENTER);
		kpreempt_enable();
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
 * Flush all APDP entries found in pmaps
 * Required during Xen save/restore operations, as Xen does not
 * handle alternative recursive mappings properly
 */
void
pmap_xen_suspend(void)
{
	int i;
	int s;
	struct pmap *pm;

	s = splvm();

	pmap_unmap_apdp();

	mutex_enter(&pmaps_lock);
	/*
	 * Set APDP entries to 0 in all pmaps.
	 * Note that for PAE kernels, this only clears the APDP entries
	 * found in the L2 shadow pages, as pmap_pdirpa() is used to obtain
	 * the PA of the pmap->pm_pdir[] pages (forming the 4 contiguous
	 * pages of PAE PD: 3 for user space, 1 for the L2 kernel shadow page)
	 */
	LIST_FOREACH(pm, &pmaps, pm_list) {
		for (i = 0; i < PDP_SIZE; i++) {
			xpq_queue_pte_update(
			    xpmap_ptom(pmap_pdirpa(pm, PDIR_SLOT_APTE + i)),
			    0);
		}
	}
	mutex_exit(&pmaps_lock);

	xpq_flush_queue();

	splx(s);

#ifdef PAE
	pmap_unmap_recursive_entries();
#endif
}

void
pmap_xen_resume(void)
{
#ifdef PAE
	pmap_map_recursive_entries();
#endif
}

#ifdef PAE
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

	xpq_flush_queue();
}

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
	for (i = 0; i < PDP_SIZE; i++)
		xpq_queue_pte_update(
		    xpmap_ptom(pmap_pdirpa(pmap_kernel(), PDIR_SLOT_PTE + i)),
		    0);

	xpq_flush_queue();

}
#endif /* PAE */

#if defined(PAE) || defined(__x86_64__)

extern struct cpu_info	* (*xpq_cpu)(void);
static __inline void
pmap_kpm_setpte(struct cpu_info *ci, struct pmap *pmap, int index)
{
	if (pmap == pmap_kernel()) {
		KASSERT(index >= PDIR_SLOT_KERN);
	}
#ifdef PAE
	xpq_queue_pte_update(
		xpmap_ptetomach(&ci->ci_kpm_pdir[l2tol2(index)]),
		pmap->pm_pdir[index]);
#elif defined(__x86_64__)
	xpq_queue_pte_update(
		xpmap_ptetomach(&ci->ci_kpm_pdir[index]),
		pmap->pm_pdir[index]);
#endif /* PAE */
}

static void
pmap_kpm_sync_xcall(void *arg1, void *arg2)
{
	KASSERT(arg1 != NULL);
	KASSERT(arg2 != NULL);

	struct pmap *pmap = arg1;
	int index = *(int *)arg2;
	KASSERT(pmap == pmap_kernel() || index < PDIR_SLOT_PTE);
	
	struct cpu_info *ci = xpq_cpu();

#ifdef PAE
	KASSERTMSG(pmap == pmap_kernel(), "%s not allowed for PAE user pmaps", __func__);
#endif /* PAE */

	if (__predict_true(pmap != pmap_kernel()) &&
	    pmap != ci->ci_pmap) {
		/* User pmap changed. Nothing to do. */
		return;
	}

	/* Update per-cpu kpm */
	pmap_kpm_setpte(ci, pmap, index);
	pmap_pte_flush();
	return;
}

/*
 * Synchronise shadow pdir with the pmap on all cpus on which it is
 * loaded.
 */
void
xen_kpm_sync(struct pmap *pmap, int index)
{
	uint64_t where;
	
	KASSERT(pmap != NULL);

	pmap_pte_flush();

	if (__predict_false(xpq_cpu != &x86_curcpu)) { /* Too early to xcall */
		CPU_INFO_ITERATOR cii;
		struct cpu_info *ci;
		int s = splvm();
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (ci == NULL) {
				continue;
			}
			if (pmap == pmap_kernel() ||
			    ci->ci_cpumask & pmap->pm_cpus) {
				pmap_kpm_setpte(ci, pmap, index);
			}
		}
		pmap_pte_flush();
		splx(s);
		return;
	}

	if (pmap == pmap_kernel()) {
		where = xc_broadcast(XC_HIGHPRI,
		    pmap_kpm_sync_xcall, pmap, &index);
		xc_wait(where);
	} else {
		KASSERT(mutex_owned(pmap->pm_lock));
		KASSERT(kpreempt_disabled());

		CPU_INFO_ITERATOR cii;
		struct cpu_info *ci;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (ci == NULL) {
				continue;
			}
			while (ci->ci_cpumask & pmap->pm_cpus) {
#ifdef MULTIPROCESSOR
#define CPU_IS_CURCPU(ci) __predict_false((ci) == curcpu())
#else /* MULTIPROCESSOR */
#define CPU_IS_CURCPU(ci) __predict_true((ci) == curcpu())
#endif /* MULTIPROCESSOR */
#if 0 /* XXX: Race with remote pmap_load() */
				if (ci->ci_want_pmapload &&
				    !CPU_IS_CURCPU(ci)) {
					/*
					 * XXX: make this more cpu
					 *  cycle friendly/co-operate
					 *  with pmap_load()
					 */
					continue;
				    }
#endif /* 0 */
				where = xc_unicast(XC_HIGHPRI, pmap_kpm_sync_xcall,
				    pmap, &index, ci);
				xc_wait(where);
				break;
			}
		}
	}
}

#endif /* PAE || __x86_64__ */
