/*	$NetBSD: vm_machdep.c,v 1.1 2002/07/05 13:32:07 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>

/*
 * SH5-specific core dump
 *
 *
 */
int
cpu_coredump(struct proc *p, struct vnode *vp, struct ucred *cred,
    struct core *chdr)
{
#ifdef notyet
	struct md_coredump md_core;
	struct coreseg cseg;
	int error;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = ALIGN(sizeof(md_core));

	md_core.md_regs = *p->p_md.md_regs;

	if ((md_core.md_regs.tf_state.sf_flags & SF_FLAGS_CALLEE_SAVED) == 0) {
		memset(&md_core.md_regs.tf_state.tf_callee, 0,
		    sizeof(md_core.md_regs.tf_state.tf_callee));
		md_core.md_regs.tf_state.sf_flags |= SF_FLAGS_CALLEE_SAVED;
	}

	if (p->p_md.md_flags & MDP_FPUSED) {
		if ((p->p_md.md_flags & MDP_FPSAVED) == 0)
			sh5_fpsave(p->p_md.md_regs.tf_state.sf_usr,
			    &p->p_addr->u_pcb);
		md_core.md_fpstate = p->p_addr->u_pcb.pcb_ctx.sf_fpregs;
	}

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_FPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred,
	    NULL, p);
	if (error)
		return (error);

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return (error);

	chdr->c_nseg++;
	return (0);
#else
	return (0);
#endif
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * Block context switches and then call switch_exit() which will
 * switch to another process thus we never return.
 */
void
cpu_exit(struct proc *p)
{
	extern volatile void switch_exit(struct proc *);

	pmap_deactivate(p);

	(void) splhigh();
	uvmexp.swtch++;
	switch_exit(p);
	/* NOTREACHED */
}

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * proc_trampoline() and call child_return() with p2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_fork(struct proc *p1, struct proc *p2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	extern void proc_trampoline(void);
	struct pcb *pcb = &p2->p_addr->u_pcb;
	struct trapframe *tf;

#ifdef DIAGNOSTIC
	if (p1 != curproc && p1 != &proc0)
		panic("cpu_fork: curproc");
#endif

	/*
	 * Save parent's FPU state, if necessary, so we have something
	 * to copy to the child
	 */
	if (p1->p_md.md_flags & MDP_FPUSED) {
		sh5_fpsave(p1->p_md.md_regs->tf_state.sf_usr,
		    &p1->p_addr->u_pcb);
		p1->p_md.md_flags |= MDP_FPSAVED;
	}

	/* Child inherits parent's md_flags and pcb */
	p2->p_md.md_flags = p1->p_md.md_flags & ~MDP_FPSAVED;
	memcpy(pcb, &p1->p_addr->u_pcb, sizeof(*pcb));

	/* Setup the child's initial kernel stack.  */
	p2->p_md.md_regs = tf = (struct trapframe *)
	    ((char *)p2->p_addr + (USPACE - (sizeof(*tf) + (sizeof(void*)*2))));

	/* Child inherits parent's trapframe */
	memcpy(tf, p1->p_md.md_regs, sizeof(*tf));

	/*
	 * If the child is to have a different user-mode stack, fix it up now.
	 */
	if (stack != NULL)
		tf->tf_caller.r15 = (register_t)(uintptr_t)stack + stacksize;

	/*
	 * Set the child's syscall return parameters to the values
	 * expected by libc's fork() stub.
	 */
	tf->tf_caller.r0 = 0;		/* No error */
	tf->tf_caller.r2 = p1->p_pid;	/* Parent's pid */
	tf->tf_caller.r3 = 1;		/* "child" flag */

	/*
	 * Set up a switchframe which will vector through proc_trampoline
	 */
	pcb->pcb_ctx.sf_pc = (register_t)(uintptr_t)proc_trampoline;
	pcb->pcb_ctx.sf_sp = (register_t)(uintptr_t)tf;
	pcb->pcb_ctx.sf_regs.r10 = (register_t)(uintptr_t)func;
	pcb->pcb_ctx.sf_regs.r11 = (register_t)(uintptr_t)arg;
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in KSEG1,
 * and size must be a multiple of NBPG.
 */
void
pagemove(caddr_t from, caddr_t to, size_t size)
{
	paddr_t pa;
	boolean_t rv;

#ifdef DEBUG
	if (size & PGOFSET ||
	    (uintptr_t)from < SH5_KSEG1_BASE || (uintptr_t)to < SH5_KSEG1_BASE)
		panic("pagemove");
#endif

	while (size > 0) {
		rv = pmap_extract(pmap_kernel(), (vaddr_t)from, &pa);
#ifdef DEBUG
		if (rv == FALSE)
			panic("pagemove 2");
		if (pmap_extract(pmap_kernel(), (vaddr_t)to, NULL) == TRUE)
			panic("pagemove 3");
#endif
		pmap_kremove((vaddr_t)from, (vsize_t)PAGE_SIZE);
		pmap_kenter_pa((vaddr_t)to, pa, VM_PROT_READ|VM_PROT_WRITE);
		from += PAGE_SIZE;
		to += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().   
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *upmap;
	vaddr_t uva;	/* User VA (map from) */
	vaddr_t kva;	/* Kernel VA (new to) */
	paddr_t pa; 	/* physical address */
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	uva = sh5_trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	len = sh5_round_page(off + len);
	kva = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(kva + off);

	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	do {
		if (pmap_extract(upmap, uva, &pa) == FALSE)
			panic("vmapbuf: null page frame");
		pmap_kenter_pa(kva, pa, VM_PROT_READ | VM_PROT_WRITE);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(pmap_kernel());
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

	kva = sh5_trunc_page(bp->b_data);
	off = (vaddr_t)bp->b_data - kva;
	len = sh5_round_page(off + len);
	pmap_kremove(kva, len);
	pmap_update(pmap_kernel());
	uvm_km_free_wakeup(phys_map, kva, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
