/*	$NetBSD: vm_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#define _PMAP_PRIVATE

#include "opt_ddb.h"
#include "opt_coredump.h"

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

#include <dev/mm.h>

#include <riscv/locore.h>

/*
 * cpu_lwp_fork: Finish a fork operation, with lwp l2 nearly set up.
 * Copy and update the pcb and trapframe, making the child ready to run.
 *
 * First LWP (l1) is the lwp being forked.  If it is &lwp0, then we are
 * creating a kthread, where return path and argument are specified
 * with `func' and `arg'.
 *
 * Rig the child's kernel stack so that it starts out in cpu_lwp_trampoline()
 * and calls child_return() with l2 as an argument. This causes the
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
	tf->tf_sr &= ~SR_EF;	/* floating point must be disabled */

	/* If specified, set a different user stack for a child. */
	if (stack != NULL) {
		KASSERT(stacksize != 0);
		tf->tf_sp = stack_align((intptr_t)stack + stacksize);
	}

	l2->l_md.md_utf = tf;

	/*
	 * Rig kernel stack so that it would start out in cpu_lwp_trampoline()
	 * and call child_return() with l as an argument.  This causes the
	 * newly-created child process to go directly to user level with a
	 * parent return value of 0 from fork(), while the parent process
	 * returns normally.
	 */
	--tf;	/* cpu_switchto uses trapframes */

	tf->tf_sr = riscvreg_status_read();
	tf->tf_s0 = (intptr_t)func;			/* S0 */
	tf->tf_s1 = (intptr_t)arg;			/* S1 */
	tf->tf_ra = (intptr_t)cpu_lwp_trampoline;	/* RA */
	l2->l_md.md_ktf = tf;					/* SP */
	KASSERT(tf->tf_sr & SR_S);
#ifdef _LP64
	KASSERT(tf->tf_sr & SR_S64);
#endif
	KASSERT(tf->tf_sr & SR_EI);
}

/*
 * Routine to copy MD stuff from proc to proc on a fork.
 */
void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{
}

#ifdef _LP64
void *
cpu_uarea_alloc(bool system)
{
	struct pglist pglist;
	int error;

	/*
	 * Allocate a new physically contiguous uarea which can be
	 * direct-mapped.
	 */
	error = uvm_pglistalloc(USPACE, pmap_limits.avail_start,
	    pmap_limits.avail_end, USPACE_ALIGN, 0, &pglist, 1, 1);
	if (error) {
		return NULL;
	}

	/*
	 * Get the physical address from the first page.
	 */
	const struct vm_page * const pg = TAILQ_FIRST(&pglist);
	KASSERT(pg != NULL);
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);
	KASSERTMSG(pa >= pmap_limits.avail_start,
	    "pa (%#"PRIxPADDR") < avail_start (%#"PRIxPADDR")",
	     pa, pmap_limits.avail_start);
	KASSERTMSG(pa + USPACE <= pmap_limits.avail_end,
	    "pa (%#"PRIxPADDR") >= avail_end (%#"PRIxPADDR")",
	     pa, pmap_limits.avail_end);

	/*
	 * we need to return a direct-mapped VA for the pa.
	 */
	return (void *)pmap_md_direct_map_paddr(pa);
}

/*
 * Return true if we freed it, false if we didn't.
 */
bool
cpu_uarea_free(void *va)
{
	if (!pmap_md_direct_mapped_vaddr_p((vaddr_t)va))
		return false;

	paddr_t pa = pmap_md_direct_mapped_vaddr_to_paddr((vaddr_t)va);

	for (const paddr_t epa = pa + USPACE; pa < epa; pa += PAGE_SIZE) {
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg != NULL);
		uvm_pagefree(pg);
	}
	return true;
}
#endif /* _LP64 */

void
cpu_lwp_free(struct lwp *l, int proc)
{

	(void)l;
}

vaddr_t
cpu_lwp_pc(struct lwp *l)
{
	return l->l_md.md_utf->tf_pc;
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

	vaddr_t uva = trunc_page((vaddr_t)bp->b_data);
	const vaddr_t off = (vaddr_t)bp->b_data - uva;
        len = round_page(off + len);

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

	KASSERT(bp->b_flags & B_PHYS);

	kva = trunc_page((vaddr_t)bp->b_data);
	len = round_page((vaddr_t)bp->b_data - kva + len);
	pmap_kremove(kva, len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot) 
{
        return (atop(pa) < physmem) ? 0 : EFAULT;
}

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	if (atop(pa) < physmem) {
		if (*vap)
			*vap = pmap_md_direct_map_paddr(pa);
		return true;
	}

	return false;
}
#endif
