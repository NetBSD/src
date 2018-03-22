/* $NetBSD: vm_machdep.c,v 1.113.26.1 2018/03/22 01:44:41 pgoyette Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.113.26.1 2018/03/22 01:44:41 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
#include <machine/alpha.h>
#include <machine/pmap.h>
#include <machine/reg.h>

void
cpu_lwp_free(struct lwp *l, int proc)
{
	(void) l;
}

void
cpu_lwp_free2(struct lwp *l)
{
	(void) l;
}

/*
 * Finish a fork operation, with thread l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with l2 as an
 * argument. This causes the newly-created child thread to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * l1 is the thread being forked; if l1 == &lwp0, we are creating
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
	struct pcb *pcb1, *pcb2;
	extern void lwp_trampoline(void);

	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	l2->l_md.md_tf = l1->l_md.md_tf;
	l2->l_md.md_flags = l1->l_md.md_flags & MDLWP_FP_C;
	l2->l_md.md_astpending = 0;

	/*
	 * Cache the physical address of the pcb, so we can
	 * swap to it easily.
	 */
	l2->l_md.md_pcbpaddr = (void *)vtophys((vaddr_t)pcb2);

	/*
	 * Copy pcb and user stack pointer from proc p1 to p2.
	 * If specificed, give the child a different stack.
	 * Floating point state from the FP chip has already been saved.
	 */
	*pcb2 = *pcb1;
	if (stack != NULL)
		pcb2->pcb_hw.apcb_usp = (u_long)stack + stacksize;
	else
		pcb2->pcb_hw.apcb_usp = alpha_pal_rdusp();

	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from setjmp.
	 */
#ifdef DIAGNOSTIC
	/*
	 * If l1 != curlwp && l1 == &lwp0, we are creating a kernel
	 * thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

	/*
	 * create the child's kernel stack, from scratch.
	 */
	{
		struct trapframe *l2tf;

		/*
		 * Pick a stack pointer, leaving room for a trapframe;
		 * copy trapframe from parent so return to user mode
		 * will be to right address, with correct registers.
		 */
		l2tf = l2->l_md.md_tf = (struct trapframe *)
		    (uvm_lwp_getuarea(l2) + USPACE - sizeof(struct trapframe));
		memcpy(l2->l_md.md_tf, l1->l_md.md_tf,
		    sizeof(struct trapframe));

		/*
		 * Set up return-value registers as fork() libc stub expects.
		 */
		l2tf->tf_regs[FRAME_V0] = l1->l_proc->p_pid; /* parent's pid */
		l2tf->tf_regs[FRAME_A3] = 0;		/* no error */
		l2tf->tf_regs[FRAME_A4] = 1;		/* is child */

		pcb2->pcb_hw.apcb_ksp =
		    (uint64_t)l2->l_md.md_tf;
		pcb2->pcb_context[0] =
		    (uint64_t)func;			/* s0: pc */
		pcb2->pcb_context[1] =
		    (uint64_t)exception_return;		/* s1: ra */
		pcb2->pcb_context[2] =
		    (uint64_t)arg;			/* s2: arg */
		pcb2->pcb_context[3] =
		    (uint64_t)l2;			/* s3: lwp */
		pcb2->pcb_context[7] =
		    (uint64_t)lwp_trampoline;		/* ra: assembly magic */
	}
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY|UVM_KMF_WAITVA);
	bp->b_data = (void *)(taddr + off);
	len = atop(len);
	while (len--) {
		if (pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map), faddr,
		    &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), taddr, trunc_page(pa),
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(vm_map_pmap(phys_map));

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t addr, off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	pmap_remove(vm_map_pmap(phys_map), addr, addr + len);
	pmap_update(vm_map_pmap(phys_map));
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}

#ifdef __HAVE_CPU_UAREA_ROUTINES
void *
cpu_uarea_alloc(bool system)
{
	struct pglist pglist;
	int error;

	/*
	 * Allocate a new physically contiguous uarea which can be
	 * direct-mapped.
	 */
	error = uvm_pglistalloc(USPACE, 0, ptoa(physmem), 0, 0, &pglist, 1, 1);
	if (error)
		return NULL;

	/*
	 * Get the physical address from the first page.
	 */
	const struct vm_page * const pg = TAILQ_FIRST(&pglist);
	KASSERT(pg != NULL);
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);

	/*
	 * We need to return a direct-mapped VA for the pa.
	 */

	return (void *)PMAP_MAP_POOLPAGE(pa);
}

/*
 * Return true if we freed it, false if we didn't.
 */
bool
cpu_uarea_free(void *vva)
{
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
}
#endif /* __HAVE_CPU_UAREA_ROUTINES */
