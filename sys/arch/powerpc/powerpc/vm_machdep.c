/*	$NetBSD: vm_machdep.c,v 1.98.14.1 2015/09/22 12:05:50 skrll Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.98.14.1 2015/09/22 12:05:50 skrll Exp $");

#include "opt_altivec.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "opt_ppccache.h"

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/buf.h>

#include <uvm/uvm.h>

#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
#include <powerpc/altivec.h>
#endif
#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/psl.h>

#ifdef PPC_IBM4XX
vaddr_t vmaprange(struct proc *, vaddr_t, vsize_t, int);
void vunmaprange(vaddr_t, vsize_t);
#endif

void cpu_lwp_bootstrap(void);

/*
 * Finish a fork operation, with execution context l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will have a switch frame which
 * returns to cpu_lwp_bootstrap() which will call child_return() with l2
 * as its argument.  This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * l1 is the execution context being forked; if l1 == &lwp0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
	void (*func)(void *), void *arg)
{

	/*
	 * If l1 != curlwp && l1 == &lwp0, we're creating a kernel thread.
	 */
	KASSERT(l1 == curlwp || l1 == &lwp0);

	struct pcb * const pcb1 = lwp_getpcb(l1);
	struct pcb * const pcb2 = lwp_getpcb(l2);

	/* Copy MD part of lwp and set up user trapframe pointer.  */
	l2->l_md = l1->l_md;
	l2->l_md.md_utf = trapframe(l2);

	/* Copy PCB. */
	*pcb2 = *pcb1;

	pcb2->pcb_pm = l2->l_proc->p_vmspace->vm_map.pmap;

	/*
	 * Setup the trap frame for the new process
	 */
	*l2->l_md.md_utf = *l1->l_md.md_utf;

	/*
	 * If specified, give the child a different stack.  Make sure to
	 * reserve enough at the top to store the previous LR.
	 */
	if (stack != NULL) {
		l2->l_md.md_utf->tf_fixreg[1] =	
		    ((register_t)stack + stacksize - STACK_ALIGNBYTES)
			& ~STACK_ALIGNBYTES;
	}

	/*
	 * Now deal setting up the initial function and its argument.
	 */
	struct ktrapframe * const ktf = ktrapframe(l2);
	struct callframe * const cf = ((struct callframe *)ktf) - 1;
	struct switchframe * const sf = ((struct switchframe *)cf) - 1;

	/*
	 * Align stack pointer
	 * struct ktrapframe has a partial callframe (sp & lr)
	 * followed by a real trapframe.  The partial callframe
	 * is for the callee to store LR.  The SP isn't really used
	 * since trap/syscall will use the SP in the trapframe.
	 * There happens to be a partial callframe in front of the
	 * trapframe, too.
	 */
	ktf->ktf_lr = (register_t) cpu_lwp_bootstrap;
	ktf->ktf_sp = (register_t) (ktf + 1);		/* just in case */

	cf->cf_sp = (register_t) ktf;
	cf->cf_r31 = (register_t) func;
	cf->cf_r30 = (register_t) arg;

	memset((void *)sf, 0, sizeof *sf);		/* just in case */
	sf->sf_sp = (register_t) cf;
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	sf->sf_user_sr = pmap_kernel()->pm_sr[USER_SR]; /* again, just in case */
#endif
	pcb2->pcb_sp = (register_t)sf;
	pcb2->pcb_kmapsr = 0;
	pcb2->pcb_umapsr = 0;
#ifdef PPC_HAVE_FPU
	pcb2->pcb_flags = PSL_FE_DFLT;
#endif
#ifdef CACHE_PROTO_MEI
	{
		paddr_t pa;
		int dcache_line_size, i;

		/* Flush on cache values for other cpu. */

		dcache_line_size = curcpu()->ci_ci.dcache_line_size;
		pa = vtophys((vaddr_t)sf);
		for (i = 0; i < SFRAMELEN + CALLFRAMELEN + FRAMELEN;
		    i += dcache_line_size) {
			__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");
			pa += dcache_line_size;
		}
		__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");
		pa = vtophys((vaddr_t)pcb2->pcb_pm);
		for (i = 0; i < sizeof(*pcb2->pcb_pm); i += dcache_line_size) {
			__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");
			pa += dcache_line_size;
		}
		__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");
		pa = vtophys((vaddr_t)pcb2);
		for (i = 0; i < sizeof(*pcb2); i += dcache_line_size) {
			__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");
			pa += dcache_line_size;
		}
		__asm volatile ("dcbf 0,%0"::"r"(pa):"memory");

		/* Need more flush? */
	}
#endif
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	(void)l;
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

#ifdef PPC_IBM4XX
/*
 * Map a range of user addresses into the kernel.
 */
vaddr_t
vmaprange(struct proc *p, vaddr_t uaddr, vsize_t len, int prot)
{
	vaddr_t faddr, taddr, kaddr;
	vsize_t off;
	paddr_t pa;

	faddr = trunc_page(uaddr);
	off = uaddr - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	kaddr = taddr + off;
	for (; len > 0; len -= PAGE_SIZE) {
		(void) pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map),
		    faddr, &pa);
		pmap_kenter_pa(taddr, pa, prot, 0);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	return (kaddr);
}

/*
 * Undo vmaprange.
 */
void
vunmaprange(vaddr_t kaddr, vsize_t len)
{
	vaddr_t addr;
	vsize_t off;

	addr = trunc_page(kaddr);
	off = kaddr - addr;
	len = round_page(off + len);
	pmap_kremove(addr, len);
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
}
#endif /* PPC_IBM4XX */

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: these pages have already been locked by uvm_vslock.
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr;
	vsize_t off;
	paddr_t pa;
	int prot = VM_PROT_READ | ((bp->b_flags & B_READ) ? VM_PROT_WRITE : 0);

#ifdef	DIAGNOSTIC
	if (!(bp->b_flags & B_PHYS))
		panic("vmapbuf");
#endif
	/*
	 * XXX Reimplement this with vmaprange (on at least PPC_IBM4XX CPUs).
	 */
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_saveaddr);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(taddr + off);
	for (; len > 0; len -= PAGE_SIZE) {
		(void) pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    faddr, &pa);
		/*
		 * Use pmap_enter so the referenced and modified bits are
		 * appropriately set.
		 */
		pmap_kenter_pa(taddr, pa, prot, 0);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t addr;
	vsize_t off;

#ifdef	DIAGNOSTIC
	if (!(bp->b_flags & B_PHYS))
		panic("vunmapbuf");
#endif
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	/*
	 * Since the pages were entered by pmap_enter, use pmap_remove
	 * to remove them.
	 */
	pmap_kremove(addr, len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}

#ifdef __HAVE_CPU_UAREA_ROUTINES
void *
cpu_uarea_alloc(bool system)
{
#ifdef PMAP_MAP_POOLPAGE
	struct pglist pglist;
	int error;

	/*
	 * Allocate a new physically contiguous uarea which can be
	 * direct-mapped.
	 */
	error = uvm_pglistalloc(USPACE, 0, ~0UL, 0, 0, &pglist, 1, 1);
	if (error) {
		return NULL;
	}

	/*
	 * Get the physical address from the first page.
	 */
	const struct vm_page * const pg = TAILQ_FIRST(&pglist);
	KASSERT(pg != NULL);
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);

	/*
	 * We need to return a direct-mapped VA for the pa.
	 */

	return (void *)(uintptr_t)PMAP_MAP_POOLPAGE(pa);
#else
	return NULL;
#endif
}

/*
 * Return true if we freed it, false if we didn't.
 */
bool
cpu_uarea_free(void *vva)
{
#ifdef PMAP_UNMAP_POOLPAGE
	vaddr_t va = (vaddr_t) vva;
	if (va >= VM_MIN_KERNEL_ADDRESS && va < VM_MAX_KERNEL_ADDRESS)
		return false;

	/*
	 * Since the pages are physically contiguous, the vm_page structure
	 * will be as well.
	 */
	struct vm_page *pg = PHYS_TO_VM_PAGE(PMAP_UNMAP_POOLPAGE(va));
	KASSERT(pg != NULL);
	for (size_t i = 0; i < UPAGES; i++, pg++) {
		uvm_pagefree(pg);
	}
	return true;
#else
	return false;
#endif
}
#endif /* __HAVE_CPU_UAREA_ROUTINES */
