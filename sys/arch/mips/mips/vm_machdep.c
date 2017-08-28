/*	$NetBSD: vm_machdep.c,v 1.143.4.3 2017/08/28 17:51:45 skrll Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: vm_machdep.c 1.21 91/04/06
 *
 *	@(#)vm_machdep.c	8.3 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.143.4.3 2017/08/28 17:51:45 skrll Exp $");

#include "opt_ddb.h"
#include "opt_coredump.h"
#include "opt_cputype.h"

#define __PMAP_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/pcb.h>
#include <mips/regnum.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <mips/psl.h>

paddr_t kvtophys(vaddr_t);	/* XXX */

/*
 * cpu_lwp_fork: Finish a fork operation, with lwp l2 nearly set up.
 * Copy and update the pcb and trapframe, making the child ready to run.
 *
 * First LWP (l1) is the lwp being forked.  If it is &lwp0, then we are
 * creating a kthread, where return path and argument are specified
 * with `func' and `arg'.
 *
 * Rig the child's kernel stack so that it will start out in lwp_trampoline()
 * and call child_return() with l2 as an argument. This causes the
 * newly-created child process to go directly to user level with an apparent
 * return value of 0 from fork(), while the parent process returns normally.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize arguments), then set up the user stack
 * pointer accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb * const pcb1 = lwp_getpcb(l1);
	struct pcb * const pcb2 = lwp_getpcb(l2);
	struct trapframe *tf;

	KASSERT(l1 == curlwp || l1 == &lwp0);

	l2->l_md.md_ss_addr = 0;
	l2->l_md.md_ss_instr = 0;
	l2->l_md.md_astpending = 0;

	/* Copy the PCB from parent. */
	*pcb2 = *pcb1;

	/*
	 * Copy the trapframe from parent, so that return to userspace
	 * will be to right address, with correct registers.
	 */
	vaddr_t ua2 = uvm_lwp_getuarea(l2);
	tf = (struct trapframe *)(ua2 + USPACE) - 1;
	*tf = *l1->l_md.md_utf;

	/* If specified, set a different user stack for a child. */
	if (stack != NULL)
		tf->tf_regs[_R_SP] = (intptr_t)stack + stacksize;

	l2->l_md.md_utf = tf;
#if (USPACE > PAGE_SIZE) || !defined(_LP64)
	CTASSERT(__arraycount(l2->l_md.md_upte) >= UPAGES);
	for (u_int i = 0; i < __arraycount(l2->l_md.md_upte); i++) {
		l2->l_md.md_upte[i] = 0;
	}
	if (!pmap_md_direct_mapped_vaddr_p(ua2)) {
		CTASSERT((PGSHIFT == 12) == (UPAGES == 2));
		pt_entry_t * const pte = pmap_pte_lookup(pmap_kernel(), ua2);
		const uint32_t x = MIPS_HAS_R4K_MMU
		    ? (MIPS3_PG_RO | MIPS3_PG_WIRED)
		    : 0;

		for (u_int i = 0; i < UPAGES; i++) {
			KASSERT(pte_valid_p(pte[i]));
			KASSERT(pte_global_p(pte[i]));
			l2->l_md.md_upte[i] = pte[i] & ~x;
		}
	}
#else
	KASSERT(pmap_md_direct_mapped_vaddr_p(ua2));
#endif
	/*
	 * Rig kernel stack so that it would start out in lwp_trampoline()
	 * and call child_return() with l as an argument.  This causes the
	 * newly-created child process to go directly to user level with a
	 * parent return value of 0 from fork(), while the parent process
	 * returns normally.
	 */

	pcb2->pcb_context.val[_L_S0] = (intptr_t)func;			/* S0 */
	pcb2->pcb_context.val[_L_S1] = (intptr_t)arg;			/* S1 */
	pcb2->pcb_context.val[MIPS_CURLWP_LABEL] = (intptr_t)l2;	/* T8 */
	pcb2->pcb_context.val[_L_SP] = (intptr_t)tf;			/* SP */
	pcb2->pcb_context.val[_L_RA] =
	   mips_locore_jumpvec.ljv_lwp_trampoline;			/* RA */
#if defined(_LP64) || defined(__mips_n32)
	KASSERT(tf->tf_regs[_R_SR] & MIPS_SR_KX);
	KASSERT(pcb2->pcb_context.val[_L_SR] & MIPS_SR_KX);
#endif
#ifndef MIPS1	/* XXX: broken */
	KASSERTMSG(pcb2->pcb_context.val[_L_SR] & MIPS_SR_INT_IE,
	    "%d.%d %#"PRIxREGISTER,
	    l1->l_proc->p_pid, l1->l_lid,
	    pcb2->pcb_context.val[_L_SR]);
#endif
}

/*
 * Routine to copy MD stuff from proc to proc on a fork.
 * For mips, this is the ABI and "32 bit process on a 64 bit kernel" flag.
 */
void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{
	p2->p_md.md_abi = p1->p_md.md_abi;
}

void *
cpu_uarea_alloc(bool system)
{
#ifdef PMAP_MAP_POOLPAGE

	struct pglist pglist;
#ifdef _LP64
	const paddr_t high = pmap_limits.avail_end;
#else
	const paddr_t high = MIPS_KSEG1_START - MIPS_KSEG0_START;
	/*
	 * Don't allocate a direct mapped uarea if we aren't allocating for a
	 * system lwp and we have memory that can't be mapped via KSEG0.
	 */
	if (!system && high < pmap_limits.avail_end)
		return NULL;
#endif
	int error;

	/*
	 * Allocate a new physically contiguous uarea which can be
	 * direct-mapped.
	 */
	error = uvm_pglistalloc(USPACE, pmap_limits.avail_start, high,
	    USPACE_ALIGN, 0, &pglist, 1, 1);
	if (error) {
		if (!system)
			return NULL;
		panic("%s: uvm_pglistalloc failed: %d", __func__, error);
	}

	/*
	 * Get the physical address from the first page.
	 */
	const struct vm_page * const pg = TAILQ_FIRST(&pglist);
	KASSERT(pg != NULL);
	const paddr_t __diagused pa = VM_PAGE_TO_PHYS(pg);
	KASSERTMSG(pa >= pmap_limits.avail_start,
	    "pa (%#"PRIxPADDR") < pmap_limits.avail_start (%#"PRIxPADDR")",
	     pa, pmap_limits.avail_start);
	KASSERTMSG(pa < pmap_limits.avail_end,
	    "pa (%#"PRIxPADDR") >= pmap_limits.avail_end (%#"PRIxPADDR")",
	     pa, pmap_limits.avail_end);

	/*
	 * we need to return a direct-mapped VA for the pa of the first (maybe
	 * only) page and call PMAP_MAP_POOLPAGE on all pages in the list, so
	 * that cache aliases are handled correctly.
	 */

	/* Initialise to unexpected result */
	vaddr_t va = MIPS_KSEG2_START;
	const struct vm_page *pglv;
	TAILQ_FOREACH_REVERSE(pglv, &pglist, pglist, pageq.queue) {
		const paddr_t palv = VM_PAGE_TO_PHYS(pglv);
		va = PMAP_MAP_POOLPAGE(palv);
	}

	KASSERT(va != MIPS_KSEG2_START);

	return (void *)va;
#else
	return NULL;
#endif
}

/*
 * Return true if we freed it, false if we didn't.
 */
bool
cpu_uarea_free(void *va)
{
#ifdef PMAP_UNMAP_POOLPAGE
#ifdef _LP64
	if (!MIPS_XKPHYS_P(va))
		return false;
#else
	if (!MIPS_KSEG0_P(va))
		return false;
#endif

	vaddr_t valv = (vaddr_t)va;
	for (size_t i = 0; i < UPAGES; i++, valv += NBPG) {
		const paddr_t pa = PMAP_UNMAP_POOLPAGE(valv);
	    	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg != NULL);
		uvm_pagefree(pg);
	}

	return true;
#else
	return false;
#endif

}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	(void)l;
}

vaddr_t
cpu_lwp_pc(struct lwp *l)
{
	return l->l_md.md_utf->tf_regs[_R_PC];
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

/*
 * Map a user I/O request into kernel virtual address space.
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t kva;	/* Kernel VA (new to) */

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	vaddr_t uva = mips_trunc_page(bp->b_data);
	const vaddr_t off = (vaddr_t)bp->b_data - uva;
        len = mips_round_page(off + len);

	kva = uvm_km_alloc(phys_map, len, atop(uva) & uvmexp.colormask,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA | UVM_KMF_COLORMATCH);
	KASSERT((atop(kva ^ uva) & uvmexp.colormask) == 0);
	bp->b_saveaddr = bp->b_data;
	bp->b_data = (void *)(kva + off);
	struct pmap * const upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	do {
		paddr_t pa;	/* physical address */
		if (pmap_extract(upmap, uva, &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_kenter_pa(kva, pa, VM_PROT_READ | VM_PROT_WRITE,
		    PMAP_WIRED);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(pmap_kernel());

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t kva;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");

	kva = mips_trunc_page(bp->b_data);
	len = mips_round_page((vaddr_t)bp->b_data - kva + len);
	pmap_kremove(kva, len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}

/*
 * Map a (kernel) virtual address to a physical address.
 *
 * MIPS processor has 3 distinct kernel address ranges:
 *
 * - kseg0 kernel "virtual address" for the   cached physical address space.
 * - kseg1 kernel "virtual address" for the uncached physical address space.
 * - kseg2 normal kernel "virtual address" mapped via the TLB.
 */
paddr_t
kvtophys(vaddr_t kva)
{
	paddr_t phys;

	if (MIPS_KSEG1_P(kva))
		return MIPS_KSEG1_TO_PHYS(kva);

	if (MIPS_KSEG0_P(kva))
		return MIPS_KSEG0_TO_PHYS(kva);

	if (kva >= VM_MIN_KERNEL_ADDRESS) {
		if (kva >= VM_MAX_KERNEL_ADDRESS)
			goto overrun;

		pt_entry_t * const ptep = pmap_pte_lookup(pmap_kernel(), kva);
		if (ptep == NULL)
			goto overrun;
		if (!pte_valid_p(*ptep)) {
			printf("kvtophys: pte not valid for %#"PRIxVADDR"\n",
			    kva);
		}
		phys = pte_to_paddr(*ptep) | (kva & PGOFSET);
		return phys;
	}
#ifdef _LP64
	if (MIPS_XKPHYS_P(kva))
		return MIPS_XKPHYS_TO_PHYS(kva);
#endif
overrun:
	printf("Virtual address %#"PRIxVADDR": cannot map to physical\n", kva);
#ifdef DDB
	Debugger();
	return 0;	/* XXX */
#endif
	panic("kvtophys");
}

/*
 * Make a kernel mapping valid for I/O, e.g. non-cachable.
 * Alignment and length constraints are as-if NBPG==PAGE_SIZE.
 */
int
ioaccess(vaddr_t vaddr, paddr_t paddr, vsize_t len)
{

	while (len > PAGE_SIZE) {
		pmap_kenter_pa(vaddr, paddr, VM_PROT_WRITE, 0);
		len -= PAGE_SIZE;
		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}

	if (len) {
		/* We could warn.. */
		pmap_kenter_pa(vaddr, paddr, VM_PROT_WRITE, 0);
	}

	/* BUGBUG should use pmap_enter() instead and check results! */
	return 0;
}

/*
 * Opposite to the above: just forget the mapping.
 */
int
iounaccess(vaddr_t vaddr, vsize_t len)
{

	pmap_kremove(vaddr, len);
	return 0;
}
