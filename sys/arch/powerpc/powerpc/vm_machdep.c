/*	$NetBSD: vm_machdep.c,v 1.74.6.1 2009/06/09 17:54:06 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.74.6.1 2009/06/09 17:54:06 snj Exp $");

#include "opt_altivec.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif
#include <machine/fpu.h>
#include <machine/pcb.h>

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
	struct trapframe *tf;
	struct callframe *cf;
	struct switchframe *sf;
	char *stktop1, *stktop2;
	struct pcb *pcb = &l2->l_addr->u_pcb;

#ifdef DIAGNOSTIC
	/*
	 * if p1 != curlwp && p1 == &proc0, we're creating a kernel thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

#ifdef PPC_HAVE_FPU
	if (l1->l_addr->u_pcb.pcb_fpcpu)
		save_fpu_lwp(l1, FPU_SAVE);
#endif
#ifdef ALTIVEC
	if (l1->l_addr->u_pcb.pcb_veccpu)
		save_vec_lwp(l1, ALTIVEC_SAVE);
#endif
	*pcb = l1->l_addr->u_pcb;

	pcb->pcb_pm = l2->l_proc->p_vmspace->vm_map.pmap;

	l2->l_md.md_flags = 0;

	/*
	 * Setup the trap frame for the new process
	 */
	stktop1 = (void *)trapframe(l1);
	stktop2 = (void *)trapframe(l2);
	memcpy(stktop2, stktop1, sizeof(struct trapframe));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL) {
		tf = trapframe(l2);
		tf->fixreg[1] = (register_t)stack + stacksize;
	}

	/*
	 * Align stack pointer
	 * Since sizeof(struct trapframe) is 41 words, this will
	 * give us 12 bytes on the stack, which pad us somewhat
	 * for an extra call frame (or at least space for callee
	 * to store LR).
	 */
	stktop2 = (void *)((uintptr_t)stktop2 & ~(CALLFRAMELEN-1));

	/*
	 * There happens to be a callframe, too.
	 */
	cf = (struct callframe *)stktop2;
	cf->sp = (register_t)(stktop2 + CALLFRAMELEN);
	cf->lr = (register_t)cpu_lwp_bootstrap;

	/*
	 * Below the trap frame, there is another call frame:
	 */
	stktop2 -= CALLFRAMELEN;
	cf = (struct callframe *)stktop2;
	cf->sp = (register_t)(stktop2 + CALLFRAMELEN);
	cf->r31 = (register_t)func;
	cf->r30 = (register_t)arg;

	/*
	 * Below that, we allocate the switch frame:
	 */
	stktop2 -= SFRAMELEN;		/* must match SFRAMELEN in genassym */
	sf = (struct switchframe *)stktop2;
	memset((void *)sf, 0, sizeof *sf);		/* just in case */
	sf->sp = (register_t)cf;
#ifndef PPC_IBM4XX
	sf->user_sr = pmap_kernel()->pm_sr[USER_SR]; /* again, just in case */
#endif
	pcb->pcb_sp = (register_t)stktop2;
	pcb->pcb_kmapsr = 0;
	pcb->pcb_umapsr = 0;
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
#if defined(PPC_HAVE_FPU) || defined(ALTIVEC)
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

#ifdef PPC_HAVE_FPU
	if (pcb->pcb_fpcpu)			/* release the FPU */
		save_fpu_lwp(l, FPU_DISCARD);
#endif
#ifdef ALTIVEC
	if (pcb->pcb_veccpu)			/* release the AltiVEC */
		save_vec_lwp(l, ALTIVEC_DISCARD);
#endif

}

#ifdef COREDUMP
/*
 * Write the machine-dependent part of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	struct coreseg cseg;
	struct md_coredump md_core;
	struct pcb *pcb = &l->l_addr->u_pcb;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_POWERPC, 0);
		chdr->c_hdrsize = ALIGN(sizeof *chdr);
		chdr->c_seghdrsize = ALIGN(sizeof cseg);
		chdr->c_cpusize = sizeof md_core;
		chdr->c_nseg++;
		return 0;
	}

	md_core.frame = *trapframe(l);
	if (pcb->pcb_flags & PCB_FPU) {
#ifdef PPC_HAVE_FPU
		if (pcb->pcb_fpcpu)
			save_fpu_lwp(l, FPU_SAVE);
#endif
		md_core.fpstate = pcb->pcb_fpu;
	} else
		memset(&md_core.fpstate, 0, sizeof(md_core.fpstate));

#ifdef ALTIVEC
	if (pcb->pcb_flags & PCB_ALTIVEC) {
		if (pcb->pcb_veccpu)
			save_vec_lwp(l, ALTIVEC_SAVE);
		md_core.vstate = pcb->pcb_vr;
	} else
#endif
		memset(&md_core.vstate, 0, sizeof(md_core.vstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
		    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &md_core,
	    sizeof(md_core));
}
#endif

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
		pmap_kenter_pa(taddr, pa, prot);
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
void
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
		pmap_kenter_pa(taddr, pa, prot);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
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

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	extern void setfunc_trampoline(void);
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;
	struct callframe *cf;
	struct switchframe *sf;

	tf = trapframe(l);
	cf = (struct callframe *) ((uintptr_t)tf & ~(CALLFRAMELEN-1));
	cf->lr = (register_t)setfunc_trampoline;
	cf--;
	cf->sp = (register_t) (cf+1);
	cf->r31 = (register_t) func;
	cf->r30 = (register_t) arg;
	sf = (struct switchframe *) ((uintptr_t) cf - SFRAMELEN);
	memset((void *)sf, 0, sizeof *sf);		/* just in case */
	sf->sp = (register_t) cf;
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	sf->user_sr = pmap_kernel()->pm_sr[USER_SR]; /* again, just in case */
#endif
	pcb->pcb_sp = (register_t)sf;
	pcb->pcb_kmapsr = 0;
	pcb->pcb_umapsr = 0;
#ifdef PPC_HAVE_FPU
	pcb->pcb_flags = PSL_FE_DFLT;
#endif
}
