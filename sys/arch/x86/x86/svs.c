/*	$NetBSD: svs.c,v 1.11 2018/02/24 19:52:46 maxv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: svs.c,v 1.11 2018/02/24 19:52:46 maxv Exp $");

#include "opt_svs.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <x86/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/frameasm.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

/*
 * Separate Virtual Space
 *
 * A per-cpu L4 page is maintained in ci_svs_updirpa. During each context
 * switch to a user pmap, the lower half of updirpa is populated with the
 * entries containing the userland pages.
 *
 * ~~~~~~~~~~ The UTLS Page ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * We use a special per-cpu page that we call UTLS, for User Thread Local
 * Storage. Each CPU has one UTLS page. This page has two VAs:
 *
 *  o When the user page tables are loaded in CR3, the VA to access this
 *    page is &pcpuarea->utls, defined as SVS_UTLS in assembly. This VA is
 *    _constant_ across CPUs, but in the user page tables this VA points to
 *    the physical page of the UTLS that is _local_ to the CPU.
 *
 *  o When the kernel page tables are loaded in CR3, the VA to access this
 *    page is ci->ci_svs_utls.
 *
 * +----------------------------------------------------------------------+
 * | CPU0 Local Data                                      (Physical Page) |
 * | +------------------+                                 +-------------+ |
 * | | User Page Tables | SVS_UTLS ---------------------> | cpu0's UTLS | |
 * | +------------------+                                 +-------------+ |
 * +-------------------------------------------------------------^--------+
 *                                                               |
 *                                                               +----------+
 *                                                                          |
 * +----------------------------------------------------------------------+ |
 * | CPU1 Local Data                                      (Physical Page) | |
 * | +------------------+                                 +-------------+ | |
 * | | User Page Tables | SVS_UTLS ---------------------> | cpu1's UTLS | | |
 * | +------------------+                                 +-------------+ | |
 * +-------------------------------------------------------------^--------+ |
 *                                                               |          |
 *   +------------------+                 /----------------------+          |
 *   | Kern Page Tables | ci->ci_svs_utls                                   |
 *   +------------------+                 \---------------------------------+
 *
 * The goal of the UTLS page is to provide an area where we can store whatever
 * we want, in a way that it is accessible both when the Kernel and when the
 * User page tables are loaded in CR3.
 *
 * We store in the UTLS page three 64bit values:
 *
 *  o UTLS_KPDIRPA: the value we must put in CR3 in order to load the kernel
 *    page tables.
 *
 *  o UTLS_SCRATCH: a dummy place where we temporarily store a value during
 *    the syscall entry procedure.
 *
 *  o UTLS_RSP0: the value we must put in RSP in order to have a stack where
 *    we can push the register states. This is used only during the syscall
 *    entry procedure, because there the CPU does not automatically switch
 *    RSP (it does not use the TSS.rsp0 mechanism described below).
 *
 * ~~~~~~~~~~ The Stack Switching Mechanism Without SVS ~~~~~~~~~~~~~~~~~~~~~~
 *
 * The kernel stack is per-lwp (pcb_rsp0). When doing a context switch between
 * two user LWPs, the kernel updates TSS.rsp0 (which is per-cpu) to point to
 * the stack of the new LWP. Then the execution continues. At some point, the
 * user LWP we context-switched to will perform a syscall or will receive an
 * interrupt. There, the CPU will automatically read TSS.rsp0 and use it as a
 * stack. The kernel then pushes the register states on this stack, and
 * executes in kernel mode normally.
 *
 * TSS.rsp0 is used by the CPU only during ring3->ring0 transitions. Therefore,
 * when an interrupt is received while we were in kernel mode, the CPU does not
 * read TSS.rsp0. Instead, it just uses the current stack.
 *
 * ~~~~~~~~~~ The Stack Switching Mechanism With SVS ~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * In the pcpu_area structure, pointed to by the "pcpuarea" variable, each CPU
 * has a two-page rsp0 entry (pcpuarea->ent[cid].rsp0). These two pages do
 * _not_ have associated physical addresses. They are only two VAs.
 *
 * The first page is unmapped and acts as a redzone. The second page is
 * dynamically kentered into the highest page of the real per-lwp kernel stack;
 * but pay close attention, it is kentered _only_ in the user page tables.
 * That is to say, the VA of this second page is mapped when the user page
 * tables are loaded, but not mapped when the kernel page tables are loaded.
 *
 * During a context switch, svs_lwp_switch() gets called first. This function
 * does the kenter job described above, not in the kernel page tables (that
 * are currently loaded), but in the user page tables (that are not loaded).
 * 
 *           VIRTUAL ADDRESSES                     PHYSICAL ADDRESSES
 *
 * +-----------------------------+
 * |      KERNEL PAGE TABLES     |
 * |    +-------------------+    |                +-------------------+
 * |    | pcb_rsp0 (page 0) | ------------------> | pcb_rsp0 (page 0) |
 * |    +-------------------+    |                +-------------------+
 * |    | pcb_rsp0 (page 1) | ------------------> | pcb_rsp0 (page 1) |
 * |    +-------------------+    |                +-------------------+
 * |    | pcb_rsp0 (page 2) | ------------------> | pcb_rsp0 (page 2) |
 * |    +-------------------+    |                +-------------------+
 * |    | pcb_rsp0 (page 3) | ------------------> | pcb_rsp0 (page 3) |
 * |    +-------------------+    |            +-> +-------------------+
 * +-----------------------------+            |
 *                                            |
 * +---------------------------------------+  |
 * |           USER PAGE TABLES            |  |
 * | +----------------------------------+  |  |
 * | | pcpuarea->ent[cid].rsp0 (page 0) |  |  |
 * | +----------------------------------+  |  |
 * | | pcpuarea->ent[cid].rsp0 (page 1) | ----+
 * | +----------------------------------+  |
 * +---------------------------------------+
 *
 * After svs_lwp_switch() gets called, we set pcpuarea->ent[cid].rsp0 (page 1)
 * in TSS.rsp0. Later, when returning to userland on the lwp we context-
 * switched to, we will load the user page tables and execute in userland
 * normally.
 *
 * Next time an interrupt or syscall is received, the CPU will automatically
 * use TSS.rsp0 as a stack. Here it is executing with the user page tables
 * loaded, and therefore TSS.rsp0 is _mapped_.
 *
 * As part of the kernel entry procedure, we now switch CR3 to load the kernel
 * page tables. Here, we are still using the stack pointer we set in TSS.rsp0.
 *
 * Remember that it was only one page of stack which was mapped only in the
 * user page tables. We just switched to the kernel page tables, so we must
 * update RSP to be the real per-lwp kernel stack (pcb_rsp0). And we do so,
 * without touching the stack (since it is now unmapped, touching it would
 * fault).
 *
 * After we updated RSP, we can continue execution exactly as in the non-SVS
 * case. We don't need to copy the values the CPU pushed on TSS.rsp0: even if
 * we updated RSP to a totally different VA, this VA points to the same
 * physical page as TSS.rsp0. So in the end, the values the CPU pushed are
 * still here even with the new RSP.
 *
 * Thanks to this double-kenter optimization, we don't need to copy the
 * trapframe during each user<->kernel transition.
 *
 * ~~~~~~~~~~ Notes On Locking And Synchronization ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  o Touching ci_svs_updir without holding ci_svs_mtx first is *not*
 *    allowed.
 *
 *  o pm_kernel_cpus contains the set of CPUs that have the pmap loaded
 *    in their CR3 register. It must *not* be replaced by pm_cpus.
 *
 *  o When a context switch on the current CPU is made from a user LWP
 *    towards a kernel LWP, CR3 is not updated. Therefore, the pmap's
 *    pm_kernel_cpus still contains the current CPU. It implies that the
 *    remote CPUs that execute other threads of the user process we just
 *    left will keep synchronizing us against their changes.
 *
 * ~~~~~~~~~~ List Of Areas That Are Removed From Userland ~~~~~~~~~~~~~~~~~~~
 *
 *  o PTE Space
 *  o Direct Map
 *  o Remote PCPU Areas
 *  o Kernel Heap
 *  o Kernel Image
 *
 * ~~~~~~~~~~ Todo List ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Ordered from highest priority to lowest:
 *
 *  o Handle segment register faults properly.
 *
 *  o The NMI stack is not double-entered. Therefore if we ever receive an NMI
 *    and leave it, the content of the stack will be visible to userland (via
 *    Meltdown). Normally we never leave NMIs, unless a privileged user
 *    launched PMCs. That's unlikely to happen, our PMC support is pretty
 *    minimal, and privileged only.
 *
 *  o Narrow down the entry points: hide the 'jmp handler' instructions. This
 *    makes sense on GENERIC_KASLR kernels.
 *
 *  o Right now there is only one global LDT, and that's not compatible with
 *    USER_LDT.
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

	/* Create levels L4, L3 and L2 of the UTLS page. */
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
	 * page. After that, the UTLS page will be accessible in kernel
	 * mode via ci_svs_utls.
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

static void
svs_disable_hotpatch(void)
{
	extern uint8_t nosvs_enter, nosvs_enter_end;
	extern uint8_t nosvs_enter_altstack, nosvs_enter_altstack_end;
	extern uint8_t nosvs_leave, nosvs_leave_end;
	extern uint8_t nosvs_leave_altstack, nosvs_leave_altstack_end;
	u_long psl, cr0;
	uint8_t *bytes;
	size_t size;

	x86_patch_window_open(&psl, &cr0);

	bytes = &nosvs_enter;
	size = (size_t)&nosvs_enter_end - (size_t)&nosvs_enter;
	x86_hotpatch(HP_NAME_SVS_ENTER, bytes, size);

	bytes = &nosvs_enter_altstack;
	size = (size_t)&nosvs_enter_altstack_end -
	    (size_t)&nosvs_enter_altstack;
	x86_hotpatch(HP_NAME_SVS_ENTER_ALT, bytes, size);

	bytes = &nosvs_leave;
	size = (size_t)&nosvs_leave_end - (size_t)&nosvs_leave;
	x86_hotpatch(HP_NAME_SVS_LEAVE, bytes, size);

	bytes = &nosvs_leave_altstack;
	size = (size_t)&nosvs_leave_altstack_end -
	    (size_t)&nosvs_leave_altstack;
	x86_hotpatch(HP_NAME_SVS_LEAVE_ALT, bytes, size);

	x86_patch_window_close(psl, cr0);
}

static volatile unsigned long svs_cpu_barrier1 __cacheline_aligned;
static volatile unsigned long svs_cpu_barrier2 __cacheline_aligned;
typedef void (vector)(void);

static void
svs_disable_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	extern vector Xsyscall;
	u_long psl;

	psl = x86_read_psl();

	atomic_dec_ulong(&svs_cpu_barrier1);
	while (atomic_cas_ulong(&svs_cpu_barrier1, 0, 0) != 0) {
		x86_pause();
	}

	/* cpu0 is the one that does the hotpatch job */
	if (ci == &cpu_info_primary) {
		svs_enabled = false;
		svs_disable_hotpatch();
	}

	/* put back the non-SVS syscall entry point */
	wrmsr(MSR_LSTAR, (uint64_t)Xsyscall);

	/* enable global pages */
	if (cpu_feature[0] & CPUID_PGE)
		lcr4(rcr4() | CR4_PGE);

	atomic_dec_ulong(&svs_cpu_barrier2);
	while (atomic_cas_ulong(&svs_cpu_barrier2, 0, 0) != 0) {
		x86_pause();
	}

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();

	x86_write_psl(psl);
}

static int
svs_disable(void)
{
	struct cpu_info *ci = NULL;
	CPU_INFO_ITERATOR cii;
	uint64_t xc;

	mutex_enter(&cpu_lock);

	/*
	 * We expect all the CPUs to be online.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct schedstate_percpu *spc = &ci->ci_schedstate;
		if (spc->spc_flags & SPCF_OFFLINE) {
			printf("[!] cpu%d offline, SVS not disabled\n",
			    cpu_index(ci));
			mutex_exit(&cpu_lock);
			return EOPNOTSUPP;
		}
	}

	svs_cpu_barrier1 = ncpu;
	svs_cpu_barrier2 = ncpu;

	printf("[+] Disabling SVS...");
	xc = xc_broadcast(0, svs_disable_cpu, NULL, NULL);
	xc_wait(xc);
	printf(" done!\n");

	mutex_exit(&cpu_lock);

	return 0;
}

int sysctl_machdep_svs_enabled(SYSCTLFN_ARGS);

int
sysctl_machdep_svs_enabled(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, val;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (val == 1) {
		error = EINVAL;
	} else {
		if (svs_enabled)
			error = svs_disable();
		else
			error = 0;
	}

	return error;
}

void
svs_init(void)
{
	if (cpu_vendor != CPUVENDOR_INTEL) {
		return;
	}
	svs_enable();
}
