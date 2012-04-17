/*	$NetBSD: vm_machdep.c,v 1.37.4.1 2012/04/17 00:06:36 yamt Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vm_machdep.c 1.21 91/04/06$
 *
 *	@(#)vm_machdep.c	8.6 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.37.4.1 2012/04/17 00:06:36 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/pcb.h>

#include <uvm/uvm_extern.h>

void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{

	p2->p_md.mdp_flags = p1->p_md.mdp_flags;
}

/*
 * Finish a fork operation, with process l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with l2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * l1 is the process being forked; if l1 == &lwp0, we are creating
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
	struct trapframe *tf;
	struct switchframe *sf;

	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	l2->l_md.md_flags = l1->l_md.md_flags;

	/* Copy pcb from lwp l1 to l2. */
	if (l1 == curlwp) {
		/* Sync the PCB before we copy it. */
		savectx(curpcb);
	} else {
		KASSERT(l1 == &lwp0);
	}

	*pcb2 = *pcb1;

	/*
	 * Copy the trap frame.
	 */
	tf = (struct trapframe *)(uvm_lwp_getuarea(l2) + USPACE) - 1;
	l2->l_md.md_regs = (int *)tf;
	*tf = *(struct trapframe *)l1->l_md.md_regs;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_regs[15] = (u_int)stack + stacksize;

	sf = (struct switchframe *)tf - 1;
	sf->sf_pc = (u_int)lwp_trampoline;
	pcb2->pcb_regs[6] = (int)func;		/* A2 */
	pcb2->pcb_regs[7] = (int)arg;		/* A3 */
	pcb2->pcb_regs[8] = (int)l2;		/* A4 */
	pcb2->pcb_regs[11] = (int)sf;		/* SSP */
	pcb2->pcb_ps = PSL_LOWIPL;		/* start kthreads at IPL 0 */
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* Nothing to do */
}

void
cpu_lwp_free2(struct lwp *l)
{

	/* Nothing to do */
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *upmap, *kpmap;
	vaddr_t uva;		/* User VA (map from) */
	vaddr_t kva;		/* Kernel VA (new to) */
	paddr_t pa; 		/* physical address */
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	uva = m68k_trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	len = m68k_round_page(off + len);
	kva = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(kva + off);

	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	kpmap = vm_map_pmap(phys_map);
	do {
		if (pmap_extract(upmap, uva, &pa) == false)
			panic("vmapbuf: null page frame");
#ifdef M68K_VAC
		pmap_enter(kpmap, kva, pa, VM_PROT_READ | VM_PROT_WRITE,
		    PMAP_WIRED);
#else
		pmap_kenter_pa(kva, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
#endif
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(kpmap);

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t kva;
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");

	kva = m68k_trunc_page(bp->b_data);
	off = (vaddr_t)bp->b_data - kva;
	len = m68k_round_page(off + len);

#ifdef M68K_VAC
	pmap_remove(vm_map_pmap(phys_map), kva, kva + len);
#else
	pmap_kremove(kva, len);
#endif
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}


#if defined(M68K_MMU_MOTOROLA) || defined(M68K_MMU_HP)

#include <m68k/cacheops.h>

/*
 * Map `size' bytes of physical memory starting at `paddr' into
 * kernel VA space at `vaddr'.  Read/write and cache-inhibit status
 * are specified by `prot'.
 */
void
physaccess(void *vaddr, void *paddr, int size, int prot)
{
	pt_entry_t *pte;
	u_int page;

	pte = kvtopte(vaddr);
	page = (u_int)paddr & PG_FRAME;
	for (size = btoc(size); size; size--) {
		*pte++ = PG_V | prot | page;
		page += PAGE_SIZE;
	}
	TBIAS();
}

void
physunaccess(void *vaddr, int size)
{
	pt_entry_t *pte;

	pte = kvtopte(vaddr);
	for (size = btoc(size); size; size--)
		*pte++ = PG_NV;
	TBIAS();
}

/*
 * Convert kernel VA to physical address
 */
int
kvtop(void *addr)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t)addr, &pa) == false)
		panic("kvtop: zero page frame");
	return (int)pa;
}

#endif

