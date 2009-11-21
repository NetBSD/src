/*	$NetBSD: vm_machdep.c,v 1.38 2009/11/21 15:36:34 rmind Exp $	*/

/*	$OpenBSD: vm_machdep.c,v 1.64 2008/09/30 18:54:26 miod Exp $	*/

/*
 * Copyright (c) 1999-2004 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.38 2009/11/21 15:36:34 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>
#include <sys/exec.h>
#include <sys/core.h>

#include <machine/cpufunc.h>
#include <machine/pmap.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

#include <hppa/hppa/machdep.h>

static inline void
cpu_activate_pcb(struct lwp *l)
{
	struct trapframe *tf = l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);
	vaddr_t uarea = (vaddr_t)pcb;
#ifdef DIAGNOSTIC
	vaddr_t maxsp = (vaddr_t)uarea + USPACE;
#endif
	KASSERT(tf == (void *)(uarea + PAGE_SIZE));
	/*
	 * Stash the physical for the pcb of U for later perusal
	 */
	pcb->pcb_uva = uarea;
	tf->tf_cr30 = kvtop((void *)uarea);
	fdcache(HPPA_SID_KERNEL, (vaddr_t)pcb, sizeof(struct pcb));

#ifdef DIAGNOSTIC
	/* Create the kernel stack red zone. */
	pmap_remove(pmap_kernel(), maxsp - PAGE_SIZE, maxsp);
	pmap_update(pmap_kernel());
#endif
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct proc *p = l2->l_proc;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pm_space;
	struct pcb *pcb1, *pcb2;
	struct trapframe *tf;
	register_t sp, osp;

	KASSERT(round_page(sizeof(struct pcb)) <= PAGE_SIZE);

	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	l2->l_md.md_flags = 0;

	/* Flush the parent LWP out of the FPU. */
	hppa_fpu_flush(l1);

	/* Now copy the parent PCB into the child. */
	memcpy(pcb2, pcb1, sizeof(struct pcb));
	fdcache(HPPA_SID_KERNEL, (vaddr_t)pcb1, sizeof(pcb1->pcb_fpregs));

	/* reset any of the pending FPU exceptions from parent */
	pcb2->pcb_fpregs[0] = HPPA_FPU_FORK(pcb2->pcb_fpregs[0]);
	pcb2->pcb_fpregs[1] = 0;
	pcb2->pcb_fpregs[2] = 0;
	pcb2->pcb_fpregs[3] = 0;

	sp = (register_t)l2->l_addr + PAGE_SIZE;
	l2->l_md.md_regs = tf = (struct trapframe *)sp;
	sp += sizeof(struct trapframe);

	/* copy the l1's trapframe to l2 */
	memcpy(tf, l1->l_md.md_regs, sizeof(*tf));

	/* Fill out all the PAs we are going to need in locore. */
	cpu_activate_pcb(l2);

	/* Load all of the user's space registers. */
	tf->tf_sr0 = tf->tf_sr1 = tf->tf_sr3 = tf->tf_sr2 = 
	tf->tf_sr4 = tf->tf_sr5 = tf->tf_sr6 = space;
	tf->tf_iisq_head = tf->tf_iisq_tail = space;

	/* Load the protection registers */
	tf->tf_pidr1 = tf->tf_pidr2 = pmap->pm_pid;

	/*
	 * theoretically these could be inherited from the father,
	 * but just in case.
	 */
	tf->tf_sr7 = HPPA_SID_KERNEL;
	mfctl(CR_EIEM, tf->tf_eiem);
	tf->tf_ipsw = PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I /* | PSW_L */ |
	    (kpsw & PSW_O);
	pcb2->pcb_fpregs[HPPA_NFPREGS] = 0;

	/*
	 * Set up return value registers as libc:fork() expects
	 */
	tf->tf_ret0 = l1->l_proc->p_pid;
	tf->tf_ret1 = 1;	/* ischild */
	tf->tf_t1 = 0;		/* errno */

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_sp = (register_t)stack;

	/*
	 * Build stack frames for the cpu_switchto & co.
	 */
	osp = sp;

	/* lwp_trampoline's frame */
	sp += HPPA_FRAME_SIZE;

	*(register_t *)(sp + HPPA_FRAME_PSP) = osp;
	*(register_t *)(sp + HPPA_FRAME_CRP) = (register_t)lwp_trampoline;

	*HPPA_FRAME_CARG(2, sp) = KERNMODE(func);
	*HPPA_FRAME_CARG(3, sp) = (register_t)arg;

	/*
	 * cpu_switchto's frame
	 * 	stack usage is std frame + callee-save registers
	 */
	sp += HPPA_FRAME_SIZE + 16*4;
	pcb2->pcb_ksp = sp;
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l2->l_addr, sp - (vaddr_t)l2->l_addr);
}

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb *pcb = lwp_getpcb(l);
	struct trapframe *tf;
	register_t sp, osp;

	sp = (register_t)pcb + PAGE_SIZE;
	l->l_md.md_regs = tf = (struct trapframe *)sp;
	sp += sizeof(struct trapframe);

	cpu_activate_pcb(l);

	/*
	 * Build stack frames for the cpu_switchto & co.
	 */
	osp = sp;

	/* setfunc_trampoline's frame */
	sp += HPPA_FRAME_SIZE;

	*(register_t *)(sp + HPPA_FRAME_PSP) = osp;
	*(register_t *)(sp + HPPA_FRAME_CRP) = (register_t)setfunc_trampoline;

	*HPPA_FRAME_CARG(2, sp) = KERNMODE(func);
	*HPPA_FRAME_CARG(3, sp) = (register_t)arg;

	/*
	 * cpu_switchto's frame
	 * 	stack usage is std frame + callee-save registers
	 */
	sp += HPPA_FRAME_SIZE + 16*4;
	pcb->pcb_ksp = sp;
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l->l_addr, sp - (vaddr_t)l->l_addr);
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/*
	 * If this thread was using the FPU, disable the FPU and record
	 * that it's unused.
	 */

	hppa_fpu_flush(l);
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

/*
 * Map an IO request into kernel virtual address space.
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t uva, kva;
	paddr_t pa;
	vsize_t size, off;
	int npf;
	struct pmap *upmap, *kpmap;

#ifdef DIAGNOSTIC
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
#endif
	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	kpmap = vm_map_pmap(phys_map);
	bp->b_saveaddr = bp->b_data;
	uva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	size = round_page(off + len);
	kva = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(kva + off);
	npf = btoc(size);
	while (npf--) {
		if (pmap_extract(upmap, uva, &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_enter(kpmap, kva, pa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
	}
	pmap_update(kpmap);
}

/*
 * Unmap IO request from the kernel virtual address space.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *pmap;
	vaddr_t kva;
	vsize_t off;

#ifdef DIAGNOSTIC
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
#endif
	kva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - kva;
	len = round_page(off + len);
	pmap = vm_map_pmap(phys_map);
	pmap_remove(pmap, kva, kva + len);
	pmap_update(pmap);
	uvm_km_free(phys_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
