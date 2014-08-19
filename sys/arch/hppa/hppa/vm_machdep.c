/*	$NetBSD: vm_machdep.c,v 1.52.2.1 2014/08/20 00:03:04 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.52.2.1 2014/08/20 00:03:04 tls Exp $");

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
#include <sys/pool.h>
#include <sys/cpu.h>

#include <machine/cpufunc.h>
#include <machine/pmap.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

extern struct pool hppa_fppl;

#include <hppa/hppa/machdep.h>

static inline void
cpu_activate_pcb(struct lwp *l)
{
	struct trapframe *tf = l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);
#ifdef DIAGNOSTIC
	vaddr_t uarea = (vaddr_t)pcb;
	vaddr_t maxsp = uarea + USPACE;
#endif
	KASSERT(tf == (void *)(uarea + PAGE_SIZE));

	/*
	 * Stash the physical address of FP regs for later perusal
	 */
	tf->tf_cr30 = (u_int)pcb->pcb_fpregs;

#ifdef DIAGNOSTIC
	/* Create the kernel stack red zone. */
	pmap_remove(pmap_kernel(), maxsp - PAGE_SIZE, maxsp);
	pmap_update(pmap_kernel());
#endif
}

void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{

	p2->p_md.md_flags = p1->p_md.md_flags;
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb1, *pcb2;
	struct trapframe *tf;
	register_t sp, osp;
	vaddr_t uv;

	KASSERT(round_page(sizeof(struct pcb)) <= PAGE_SIZE);

	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	l2->l_md.md_astpending = 0;
	l2->l_md.md_flags = 0;

	/* Flush the parent LWP out of the FPU. */
	hppa_fpu_flush(l1);

	/* Now copy the parent PCB into the child. */
	memcpy(pcb2, pcb1, sizeof(struct pcb));

	pcb2->pcb_fpregs = pool_get(&hppa_fppl, PR_WAITOK);
	*pcb2->pcb_fpregs = *pcb1->pcb_fpregs;

	/* reset any of the pending FPU exceptions from parent */
	pcb2->pcb_fpregs->fpr_regs[0] =
	    HPPA_FPU_FORK(pcb2->pcb_fpregs->fpr_regs[0]);
	pcb2->pcb_fpregs->fpr_regs[1] = 0;
	pcb2->pcb_fpregs->fpr_regs[2] = 0;
	pcb2->pcb_fpregs->fpr_regs[3] = 0;

	l2->l_md.md_bpva = l1->l_md.md_bpva;
	l2->l_md.md_bpsave[0] = l1->l_md.md_bpsave[0];
	l2->l_md.md_bpsave[1] = l1->l_md.md_bpsave[1];

	uv = uvm_lwp_getuarea(l2);
	sp = (register_t)uv + PAGE_SIZE;
	tf = l2->l_md.md_regs = (struct trapframe *)sp;
	sp += sizeof(struct trapframe);

	/* copy the l1's trapframe to l2 */
	memcpy(tf, l1->l_md.md_regs, sizeof(*tf));

	/* Fill out all the PAs we are going to need in locore. */
	cpu_activate_pcb(l2);

	if (__predict_true(l2->l_proc->p_vmspace != NULL)) {
		hppa_setvmspace(l2);
		/*
		 * theoretically these could be inherited from the father,
		 * but just in case.
		 */
		mfctl(CR_EIEM, tf->tf_eiem);
		tf->tf_ipsw = PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I /* | PSW_L */ |
		    (curcpu()->ci_psw & PSW_O);
	}

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

	*(register_t *)(sp) = 0;	/* previous frame pointer */
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
	fdcache(HPPA_SID_KERNEL, uv, sp - uv);
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
	struct pcb *pcb = lwp_getpcb(l);
	
	/*
	 * If this thread was using the FPU, disable the FPU and record
	 * that it's unused.
	 */

	hppa_fpu_flush(l);
	pool_put(&hppa_fppl, pcb->pcb_fpregs);
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

/*
 * Map an IO request into kernel virtual address space.
 */
int
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

	return 0;
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

int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{

	l->l_md.md_regs->tf_cr27 = (u_int)addr;
	if (l == curlwp)
		mtctl(addr, CR_TLS);
	return 0;
}

