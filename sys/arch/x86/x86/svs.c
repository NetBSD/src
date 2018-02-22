/*	$NetBSD: svs.c,v 1.5 2018/02/22 09:41:06 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: svs.c,v 1.5 2018/02/22 09:41:06 maxv Exp $");

#include "opt_svs.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <x86/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/frameasm.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

/*
 * Separate Virtual Space
 *
 * A per-cpu L4 page is maintained in ci_svs_updirpa. During each context
 * switch to a user pmap, updirpa is populated with the entries of the new
 * pmap, minus what we don't want to have mapped in userland.
 *
 * Note on locking/synchronization here:
 *
 * (a) Touching ci_svs_updir without holding ci_svs_mtx first is *not*
 *     allowed.
 *
 * (b) pm_kernel_cpus contains the set of CPUs that have the pmap loaded
 *     in their CR3 register. It must *not* be replaced by pm_cpus.
 *
 * (c) When a context switch on the current CPU is made from a user LWP
 *     towards a kernel LWP, CR3 is not updated. Therefore, the pmap's
 *     pm_kernel_cpus still contains the current CPU. It implies that the
 *     remote CPUs that execute other threads of the user process we just
 *     left will keep synchronizing us against their changes.
 *
 * List of areas that are removed from userland:
 *     PTE Space         [OK]
 *     Direct Map        [OK]
 *     Remote PCPU Areas [OK]
 *     Kernel Heap       [OK]
 *     Kernel Image      [OK]
 *
 * TODO:
 *
 * (a) The NMI stack is not double-entered. Therefore if we ever receive
 *     an NMI and leave it, the content of the stack will be visible to
 *     userland (via Meltdown). Normally we never leave NMIs, unless a
 *     privileged user launched PMCs. That's unlikely to happen, our PMC
 *     support is pretty minimal.
 *
 * (b) Enable SVS depending on the CPU model, and add a sysctl to disable
 *     it dynamically.
 *
 * (c) Narrow down the entry points: hide the 'jmp handler' instructions.
 *     This makes sense on GENERIC_KASLR kernels.
 *
 * (d) Right now there is only one global LDT, and that's not compatible
 *     with USER_LDT.
 */

bool svs_enabled __read_mostly = false;

struct svs_utls {
	paddr_t kpdirpa;
	uint64_t scratch;
	vaddr_t rsp0;
};

static pd_entry_t *
svs_tree_add(struct cpu_info *ci, vaddr_t va)
{
	extern const vaddr_t ptp_masks[];
	extern const int ptp_shifts[];
	extern const long nbpd[];
	pd_entry_t *dstpde;
	size_t i, pidx, mod;
	struct vm_page *pg;
	paddr_t pa;

	dstpde = ci->ci_svs_updir;
	mod = (size_t)-1;

	for (i = PTP_LEVELS; i > 1; i--) {
		pidx = pl_i(va % mod, i);

		if (!pmap_valid_entry(dstpde[pidx])) {
			pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
			if (pg == 0)
				panic("%s: failed to allocate PA for CPU %d\n",
					__func__, cpu_index(ci));
			pa = VM_PAGE_TO_PHYS(pg);

			dstpde[pidx] = PG_V | PG_RW | pa;
		}

		pa = (paddr_t)(dstpde[pidx] & PG_FRAME);
		dstpde = (pd_entry_t *)PMAP_DIRECT_MAP(pa);
		mod = nbpd[i-1];
	}

	return dstpde;
}

static void
svs_page_add(struct cpu_info *ci, vaddr_t va)
{
	pd_entry_t *srcpde, *dstpde, pde;
	size_t idx, pidx;
	paddr_t pa;

	/* Create levels L4, L3 and L2. */
	dstpde = svs_tree_add(ci, va);

	pidx = pl1_i(va % NBPD_L2);

	/*
	 * If 'va' is in a large page, we need to compute its physical
	 * address manually.
	 */
	idx = pl2_i(va);
	srcpde = L2_BASE;
	if (!pmap_valid_entry(srcpde[idx])) {
		panic("%s: L2 page not mapped", __func__);
	}
	if (srcpde[idx] & PG_PS) {
		pa = srcpde[idx] & PG_2MFRAME;
		pa += (paddr_t)(va % NBPD_L2);
		pde = (srcpde[idx] & ~(PG_G|PG_PS|PG_2MFRAME)) | pa;

		if (pmap_valid_entry(dstpde[pidx])) {
			panic("%s: L1 page already mapped", __func__);
		}
		dstpde[pidx] = pde;
		return;
	}

	/*
	 * Normal page, just copy the PDE.
	 */
	idx = pl1_i(va);
	srcpde = L1_BASE;
	if (!pmap_valid_entry(srcpde[idx])) {
		panic("%s: L1 page not mapped", __func__);
	}
	if (pmap_valid_entry(dstpde[pidx])) {
		panic("%s: L1 page already mapped", __func__);
	}
	dstpde[pidx] = srcpde[idx] & ~(PG_G);
}

static void
svs_rsp0_init(struct cpu_info *ci)
{
	const cpuid_t cid = cpu_index(ci);
	vaddr_t va, rsp0;
	pd_entry_t *pd;
	size_t pidx;

	rsp0 = (vaddr_t)&pcpuarea->ent[cid].rsp0;

	/* The first page is a redzone. */
	va = rsp0 + PAGE_SIZE;

	/* Create levels L4, L3 and L2. */
	pd = svs_tree_add(ci, va);

	/* Get the info for L1. */
	pidx = pl1_i(va % NBPD_L2);
	if (pmap_valid_entry(pd[pidx])) {
		panic("%s: rsp0 page already mapped", __func__);
	}

	ci->ci_svs_rsp0_pte = (pt_entry_t *)&pd[pidx];
	ci->ci_svs_rsp0 = rsp0 + PAGE_SIZE + sizeof(struct trapframe);
	ci->ci_svs_ursp0 = ci->ci_svs_rsp0 - sizeof(struct trapframe);
	ci->ci_svs_krsp0 = 0;
}

static void
svs_utls_init(struct cpu_info *ci)
{
	const vaddr_t utlsva = (vaddr_t)&pcpuarea->utls;
	struct svs_utls *utls;
	struct vm_page *pg;
	pd_entry_t *pd;
	size_t pidx;
	paddr_t pa;
	vaddr_t va;

	/* Create levels L4, L3 and L2. */
	pd = svs_tree_add(ci, utlsva);

	/* Allocate L1. */
	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
	if (pg == 0)
		panic("%s: failed to allocate PA for CPU %d\n", __func__,
		    cpu_index(ci));
	pa = VM_PAGE_TO_PHYS(pg);

	/* Enter L1. */
	if (pmap_valid_entry(L1_BASE[pl1_i(utlsva)])) {
		panic("%s: local page already mapped", __func__);
	}
	pidx = pl1_i(utlsva % NBPD_L2);
	if (pmap_valid_entry(pd[pidx])) {
		panic("%s: L1 page already mapped", __func__);
	}
	pd[pidx] = PG_V | PG_RW | pmap_pg_nx | pa;

	/*
	 * Now, allocate a VA in the kernel map, that points to the UTLS
	 * page.
	 */
	va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY|UVM_KMF_NOWAIT);
	if (va == 0) {
		panic("%s: unable to allocate VA\n", __func__);
	}
	pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	ci->ci_svs_utls = va;

	/* Initialize the constant fields of the UTLS page */
	utls = (struct svs_utls *)ci->ci_svs_utls;
	utls->rsp0 = ci->ci_svs_rsp0;
}

static void
svs_range_add(struct cpu_info *ci, vaddr_t va, size_t size)
{
	size_t i, n;

	KASSERT(size % PAGE_SIZE == 0);
	n = size / PAGE_SIZE;
	for (i = 0; i < n; i++) {
		svs_page_add(ci, va + i * PAGE_SIZE);
	}
}

void
cpu_svs_init(struct cpu_info *ci)
{
	extern char __text_user_start;
	extern char __text_user_end;
	const cpuid_t cid = cpu_index(ci);
	struct vm_page *pg;

	KASSERT(ci != NULL);

	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
	if (pg == 0)
		panic("%s: failed to allocate L4 PA for CPU %d\n",
			__func__, cpu_index(ci));
	ci->ci_svs_updirpa = VM_PAGE_TO_PHYS(pg);

	ci->ci_svs_updir = (pt_entry_t *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (ci->ci_svs_updir == NULL)
		panic("%s: failed to allocate L4 VA for CPU %d\n",
			__func__, cpu_index(ci));

	pmap_kenter_pa((vaddr_t)ci->ci_svs_updir, ci->ci_svs_updirpa,
		VM_PROT_READ | VM_PROT_WRITE, 0);

	pmap_update(pmap_kernel());

	ci->ci_svs_kpdirpa = pmap_pdirpa(pmap_kernel(), 0);

	mutex_init(&ci->ci_svs_mtx, MUTEX_DEFAULT, IPL_VM);

	svs_page_add(ci, (vaddr_t)&pcpuarea->idt);
	svs_page_add(ci, (vaddr_t)&pcpuarea->ldt);
	svs_range_add(ci, (vaddr_t)&pcpuarea->ent[cid],
	    offsetof(struct pcpu_entry, rsp0));
	svs_range_add(ci, (vaddr_t)&__text_user_start,
	    (vaddr_t)&__text_user_end - (vaddr_t)&__text_user_start);

	svs_rsp0_init(ci);
	svs_utls_init(ci);
}

void
svs_pmap_sync(struct pmap *pmap, int index)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	cpuid_t cid;

	KASSERT(svs_enabled);
	KASSERT(pmap != NULL);
	KASSERT(pmap != pmap_kernel());
	KASSERT(mutex_owned(pmap->pm_lock));
	KASSERT(kpreempt_disabled());
	KASSERT(index < 255);

	for (CPU_INFO_FOREACH(cii, ci)) {
		cid = cpu_index(ci);

		if (!kcpuset_isset(pmap->pm_kernel_cpus, cid)) {
			continue;
		}

		/* take the lock and check again */
		mutex_enter(&ci->ci_svs_mtx);
		if (kcpuset_isset(pmap->pm_kernel_cpus, cid)) {
			ci->ci_svs_updir[index] = pmap->pm_pdir[index];
		}
		mutex_exit(&ci->ci_svs_mtx);
	}
}

void
svs_lwp_switch(struct lwp *oldlwp, struct lwp *newlwp)
{
	struct cpu_info *ci = curcpu();
	struct svs_utls *utls;
	struct pcb *pcb;
	pt_entry_t *pte;
	uintptr_t rsp0;
	vaddr_t va;

	KASSERT(svs_enabled);

	if (newlwp->l_flag & LW_SYSTEM) {
		return;
	}

#ifdef DIAGNOSTIC
	if (oldlwp != NULL && !(oldlwp->l_flag & LW_SYSTEM)) {
		pcb = lwp_getpcb(oldlwp);
		rsp0 = pcb->pcb_rsp0;
		va = rounddown(rsp0, PAGE_SIZE);
		KASSERT(ci->ci_svs_krsp0 == rsp0 - sizeof(struct trapframe));
		pte = ci->ci_svs_rsp0_pte;
		KASSERT(*pte == L1_BASE[pl1_i(va)]);
	}
#endif

	pcb = lwp_getpcb(newlwp);
	rsp0 = pcb->pcb_rsp0;
	va = rounddown(rsp0, PAGE_SIZE);

	/* Update the kernel rsp0 in cpu_info */
	ci->ci_svs_krsp0 = rsp0 - sizeof(struct trapframe);
	KASSERT((ci->ci_svs_krsp0 % PAGE_SIZE) ==
	    (ci->ci_svs_ursp0 % PAGE_SIZE));

	utls = (struct svs_utls *)ci->ci_svs_utls;
	utls->scratch = 0;

	/*
	 * Enter the user rsp0. We don't need to flush the TLB here, since
	 * the user page tables are not loaded.
	 */
	pte = ci->ci_svs_rsp0_pte;
	*pte = L1_BASE[pl1_i(va)];
}

static inline pt_entry_t
svs_pte_atomic_read(struct pmap *pmap, size_t idx)
{
	/*
	 * XXX: We don't have a basic atomic_fetch_64 function?
	 */
	return atomic_cas_64(&pmap->pm_pdir[idx], 666, 666);
}

/*
 * We may come here with the pmap unlocked. So read its PTEs atomically. If
 * a remote CPU is updating them at the same time, it's not a problem: the
 * remote CPU will call svs_pmap_sync afterwards, and our updirpa will be
 * synchronized properly.
 */
void
svs_pdir_switch(struct pmap *pmap)
{
	struct cpu_info *ci = curcpu();
	struct svs_utls *utls;
	pt_entry_t pte;
	size_t i;

	KASSERT(kpreempt_disabled());
	KASSERT(pmap != pmap_kernel());

	ci->ci_svs_kpdirpa = pmap_pdirpa(pmap, 0);

	/* Update the info in the UTLS page */
	utls = (struct svs_utls *)ci->ci_svs_utls;
	utls->kpdirpa = ci->ci_svs_kpdirpa;

	mutex_enter(&ci->ci_svs_mtx);

	/* User slots. */
	for (i = 0; i < 255; i++) {
		pte = svs_pte_atomic_read(pmap, i);
		ci->ci_svs_updir[i] = pte;
	}

	mutex_exit(&ci->ci_svs_mtx);
}

static void
svs_pgg_scanlvl(bool enable, int lvl, vaddr_t *levels)
{
	pd_entry_t *pde = (pd_entry_t *)(levels[lvl-1]);
	pt_entry_t set, rem;
	size_t i, start;
	paddr_t pa;
	int nlvl;

	set = enable ? PG_G : 0;
	rem = enable ? 0 : PG_G;

	start = (lvl == 4) ? 256 : 0;

	for (i = start; i < 512; i++) {
		if (!pmap_valid_entry(pde[i])) {
			continue;
		}
		if (lvl == 1) {
			pde[i] = (pde[i] & ~rem) | set;
		} else if (pde[i] & PG_PS) {
			pde[i] = (pde[i] & ~rem) | set;
		} else {
			pa = (paddr_t)(pde[i] & PG_FRAME);
			nlvl = lvl - 1;

			/* remove the previous mapping */
			pmap_kremove_local(levels[nlvl-1], PAGE_SIZE);

			/* kenter the lower level */
			pmap_kenter_pa(levels[nlvl-1], pa,
			    VM_PROT_READ|VM_PROT_WRITE, 0);
			pmap_update(pmap_kernel());

			/* go to the lower level */
			svs_pgg_scanlvl(enable, nlvl, levels);
		}
	}
}

static void
svs_pgg_update(bool enable)
{
	const paddr_t pa = pmap_pdirpa(pmap_kernel(), 0);
	vaddr_t levels[4];
	size_t i;

	if (!(cpu_feature[0] & CPUID_PGE)) {
		return;
	}

	pmap_pg_g = enable ? PG_G : 0;

	for (i = 0; i < 4; i++) {
		levels[i] = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_VAONLY);
	}

	pmap_kenter_pa(levels[3], pa, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	svs_pgg_scanlvl(enable, 4, levels);

	for (i = 0; i < 4; i++) {
		pmap_kremove_local(levels[i], PAGE_SIZE);
		uvm_km_free(kernel_map, levels[i], PAGE_SIZE, UVM_KMF_VAONLY);
	}

	tlbflushg();
}

static void
svs_enable(void)
{
	extern uint8_t svs_enter, svs_enter_end;
	extern uint8_t svs_enter_altstack, svs_enter_altstack_end;
	extern uint8_t svs_leave, svs_leave_end;
	extern uint8_t svs_leave_altstack, svs_leave_altstack_end;
	u_long psl, cr0;
	uint8_t *bytes;
	size_t size;

	svs_enabled = true;

	x86_patch_window_open(&psl, &cr0);

	bytes = &svs_enter;
	size = (size_t)&svs_enter_end - (size_t)&svs_enter;
	x86_hotpatch(HP_NAME_SVS_ENTER, bytes, size);

	bytes = &svs_enter_altstack;
	size = (size_t)&svs_enter_altstack_end -
	    (size_t)&svs_enter_altstack;
	x86_hotpatch(HP_NAME_SVS_ENTER_ALT, bytes, size);

	bytes = &svs_leave;
	size = (size_t)&svs_leave_end - (size_t)&svs_leave;
	x86_hotpatch(HP_NAME_SVS_LEAVE, bytes, size);

	bytes = &svs_leave_altstack;
	size = (size_t)&svs_leave_altstack_end -
	    (size_t)&svs_leave_altstack;
	x86_hotpatch(HP_NAME_SVS_LEAVE_ALT, bytes, size);

	x86_patch_window_close(psl, cr0);
}

void
svs_init(bool early)
{
	/*
	 * When early, declare that we want to use SVS, and hotpatch the
	 * entry points. When late, remove PG_G from the page tables.
	 */
	if (early) {
		if (cpu_vendor != CPUVENDOR_INTEL) {
			return;
		}
		svs_enable();
	} else if (svs_enabled) {
		svs_pgg_update(false);
	}
}

