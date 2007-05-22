/*	$NetBSD: vm_machdep.c,v 1.24.4.1 2007/05/22 17:26:54 matt Exp $	*/

/*	$OpenBSD: vm_machdep.c,v 1.25 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1999-2000 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.24.4.1 2007/05/22 17:26:54 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/exec.h>
#include <sys/core.h>

#include <machine/cpufunc.h>
#include <machine/pmap.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

#include <hppa/hppa/machdep.h>

/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *core)
{
	struct md_coredump md_core;
	struct coreseg cseg;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*core, COREMAGIC, MID_ZERO, 0);
		core->c_hdrsize = ALIGN(sizeof(*core));
		core->c_seghdrsize = ALIGN(sizeof(cseg));
		core->c_cpusize = sizeof(md_core);
		core->c_nseg++;
		return 0;
	}

	process_read_regs(l, &md_core.md_reg);

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_ZERO, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = core->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    core->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &md_core,
	    sizeof(md_core));
}

void
cpu_swapin(struct lwp *l)
{
	struct trapframe *tf = l->l_md.md_regs;

	/*
	 * Stash the physical for the pcb of U for later perusal
	 */
	l->l_addr->u_pcb.pcb_uva = (vaddr_t)l->l_addr;
	tf->tf_cr30 = kvtop((void *)l->l_addr);
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l->l_addr, sizeof(l->l_addr->u_pcb));

#ifdef HPPA_REDZONE
	/* Create the kernel stack red zone. */
	pmap_redzone((vaddr_t)l->l_addr + HPPA_REDZONE,
		(vaddr_t)l->l_addr + USPACE, 1);
#endif
}

void
cpu_swapout(struct lwp *l)
{

	/* Flush this LWP out of the FPU. */
	hppa_fpu_flush(l);
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct proc *p = l2->l_proc;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pmap_space;
	struct pcb *pcbp;
	struct trapframe *tf;
	register_t sp, osp;

#ifdef DIAGNOSTIC
	if (round_page(sizeof(struct user)) > PAGE_SIZE)
		panic("USPACE too small for user");
#endif

	/* Flush the parent LWP out of the FPU. */
	hppa_fpu_flush(l1);

	/* Now copy the parent PCB into the child. */
	pcbp = &l2->l_addr->u_pcb;
	bcopy(&l1->l_addr->u_pcb, pcbp, sizeof(*pcbp));
	fdcache(HPPA_SID_KERNEL, (vaddr_t)&l1->l_addr->u_pcb,
		sizeof(pcbp->pcb_fpregs));
	/* reset any of the pending FPU exceptions from parent */
	pcbp->pcb_fpregs[0] = HPPA_FPU_FORK(pcbp->pcb_fpregs[0]);
	pcbp->pcb_fpregs[1] = 0;
	pcbp->pcb_fpregs[2] = 0;
	pcbp->pcb_fpregs[3] = 0;

	sp = (register_t)l2->l_addr + PAGE_SIZE;
	l2->l_md.md_regs = tf = (struct trapframe *)sp;
	sp += sizeof(struct trapframe);

	/* copy the l1's trapframe to l2 */
	bcopy(l1->l_md.md_regs, tf, sizeof(*tf));

	/*
	 * cpu_swapin() is supposed to fill out all the PAs
	 * we gonna need in locore
	 */
	cpu_swapin(l2);

	/* Load all of the user's space registers. */
	tf->tf_sr0 = tf->tf_sr1 = tf->tf_sr3 = tf->tf_sr2 = 
	tf->tf_sr4 = tf->tf_sr5 = tf->tf_sr6 = space;
	tf->tf_iisq_head = tf->tf_iisq_tail = space;
	tf->tf_pidr1 = tf->tf_pidr2 = pmap->pmap_pid;

	/* record the vmspace just in case it changes underneath us. */
	pmap->pmap_vmspace = p->p_vmspace;

	/*
	 * theoretically these could be inherited from the father,
	 * but just in case.
	 */
	tf->tf_sr7 = HPPA_SID_KERNEL;
	mfctl(CR_EIEM, tf->tf_eiem);
	tf->tf_ipsw = PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I /* | PSW_L */;
	pcbp->pcb_fpregs[HPPA_NFPREGS] = 0;

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
	pcbp->pcb_ksp = sp;
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l2->l_addr, sp - (vaddr_t)l2->l_addr);
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
