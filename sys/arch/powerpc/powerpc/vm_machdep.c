/*	$NetBSD: vm_machdep.c,v 1.35 2001/08/19 18:09:22 chs Exp $	*/

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

#include "opt_altivec.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#include <machine/fpu.h>
#include <machine/pcb.h>

#if !defined(MULTIPROCESSOR) && defined(PPC_HAVE_FPU)
#define save_fpu_proc(p) save_fpu(p)		/* XXX */
#endif

#ifdef PPC_IBM4XX
vaddr_t vmaprange(struct proc *, vaddr_t, vsize_t, int);
void vunmaprange(vaddr_t, vsize_t);
#endif

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * fork_trampoline() and call child_return() with p2 as an
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
cpu_fork(p1, p2, stack, stacksize, func, arg)
	struct proc *p1, *p2;
	void *stack;
	size_t stacksize;
	void (*func)(void *);
	void *arg;
{
	struct trapframe *tf;
	struct callframe *cf;
	struct switchframe *sf;
	caddr_t stktop1, stktop2;
	void fork_trampoline(void);
	struct pcb *pcb = &p2->p_addr->u_pcb;

#ifdef DIAGNOSTIC
	/*
	 * if p1 != curproc && p1 == &proc0, we're creating a kernel thread.
	 */
	if (p1 != curproc && p1 != &proc0)
		panic("cpu_fork: curproc");
#endif

#ifdef PPC_HAVE_FPU
	if (p1->p_addr->u_pcb.pcb_fpcpu)
		save_fpu_proc(p1);
#endif
	*pcb = p1->p_addr->u_pcb;
#ifdef ALTIVEC
	if (p1->p1_addr->u_pcb.pcb_vr != NULL) {
		if (p1 == vecproc)
			save_vec(p1);
		pcb->pcb_vr = pool_get(vecpl, POOL_WAITOK);
		*pcb->pcb_vr = *p1->p1_addr->u_ucb.pcb_vr;
	}
#endif

	pcb->pcb_pm = p2->p_vmspace->vm_map.pmap;
#ifdef NEWPMAP
	pcb->pcb_pmreal = pcb->pcb_pm;		/* XXX */
#else
	(void) pmap_extract(pmap_kernel(), (vaddr_t)pcb->pcb_pm,
	    (paddr_t *)&pcb->pcb_pmreal);
#endif

	/*
	 * Setup the trap frame for the new process
	 */
	stktop1 = (caddr_t)trapframe(p1);
	stktop2 = (caddr_t)trapframe(p2);
	memcpy(stktop2, stktop1, sizeof(struct trapframe));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL) {
		tf = trapframe(p2);
		tf->fixreg[1] = (register_t)stack + stacksize;
	}

	stktop2 = (caddr_t)((u_long)stktop2 & ~15);	/* Align stack pointer */

	/*
	 * There happens to be a callframe, too.
	 */
	cf = (struct callframe *)stktop2;
	cf->lr = (int)fork_trampoline;

	/*
	 * Below the trap frame, there is another call frame:
	 */
	stktop2 -= 16;
	cf = (struct callframe *)stktop2;
	cf->r31 = (register_t)func;
	cf->r30 = (register_t)arg;

	/*
	 * Below that, we allocate the switch frame:
	 */
	stktop2 -= roundup(sizeof *sf, 16);	/* must match SFRAMELEN in genassym */
	sf = (struct switchframe *)stktop2;
	memset((void *)sf, 0, sizeof *sf);		/* just in case */
	sf->sp = (int)cf;
#ifndef PPC_IBM4XX
	sf->user_sr = pmap_kernel()->pm_sr[USER_SR]; /* again, just in case */
#endif
	pcb->pcb_sp = (int)stktop2;
	pcb->pcb_spl = 0;
}

void
cpu_swapin(p)
	struct proc *p;
{
	struct pcb *pcb = &p->p_addr->u_pcb;

#ifdef NEWPMAP
	pcb->pcb_pmreal = pcb->pcb_pm;		/* XXX */
#else
	(void) pmap_extract(pmap_kernel(), (vaddr_t)pcb->pcb_pm,
	    (paddr_t *)&pcb->pcb_pmreal);
#endif
}

/*
 * Move pages from one kernel virtual address to another.
 */
void
pagemove(from, to, size)
	caddr_t from, to;
	size_t size;
{
	paddr_t pa;
	vaddr_t va;

	for (va = (vaddr_t)from; size > 0; size -= NBPG) {
		(void) pmap_extract(pmap_kernel(), va, &pa);
		pmap_kremove(va, NBPG);
		pmap_kenter_pa((vaddr_t)to, pa, VM_PROT_READ|VM_PROT_WRITE);
		va += NBPG;
		to += NBPG;
	}
	pmap_update();
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switchexit() with the old proc
 * as an argument.  switchexit() switches to the idle context, schedules
 * the old vmspace and stack to be freed, then selects a new process to
 * run.
 */
void
cpu_exit(p)
	struct proc *p;
{
	void switchexit(struct proc *);		/* Defined in locore.S */
#ifdef ALTIVEC
	struct pcb *pcb = &p->p_addr->u_pcb;
#endif

#ifdef PPC_HAVE_FPU
	if (p->p_addr->u_pcb.pcb_fpcpu)		/* release the FPU */
		fpuproc = NULL;
#endif
#ifdef ALTIVEC
	if (p == vecproc)			/* release the AltiVEC */
		vecproc = NULL;
	if (pcb->pcb_vr != NULL)
		pool_put(vecpl, pcb->pcb_vr);
#endif

	splsched();
	switchexit(p);
}

/*
 * Write the machine-dependent part of a core dump.
 */
int
cpu_coredump(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core *chdr;
{
	struct coreseg cseg;
	struct md_coredump md_core;
	struct pcb *pcb = &p->p_addr->u_pcb;
	int error;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_POWERPC, 0);
	chdr->c_hdrsize = ALIGN(sizeof *chdr);
	chdr->c_seghdrsize = ALIGN(sizeof cseg);
	chdr->c_cpusize = sizeof md_core;

	md_core.frame = *trapframe(p);
	if (pcb->pcb_flags & PCB_FPU) {
#ifdef PPC_HAVE_FPU
		if (p->p_addr->u_pcb.pcb_fpcpu)
			save_fpu_proc(p);
#endif
		md_core.fpstate = pcb->pcb_fpu;
	} else
		memset(&md_core.fpstate, 0, sizeof(md_core.fpstate));

#ifdef ALTIVEC
	if (pcb->pcb_flags & PCB_ALTIVEC) {
		if (p == vecproc)
			save_vec(p);
		md_core.vstate = *pcb->pcb_vr;
	} else
#endif
		memset(&md_core.vstate, 0, sizeof(md_core.vstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	if ((error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
			    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
			    IO_NODELOCKED|IO_UNIT, cred, NULL, p)) != 0)
		return error;
	if ((error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof md_core,
			    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
			    IO_NODELOCKED|IO_UNIT, cred, NULL, p)) != 0)
		return error;

	chdr->c_nseg++;
	return 0;
}

#ifdef PPC_IBM4XX
/*
 * Map a range of user addresses into the kernel.
 */
vaddr_t
vmaprange(p, uaddr, len, prot)
	struct proc *p;
	vaddr_t uaddr;
	vsize_t len;
	int prot;
{
	vaddr_t faddr, taddr, kaddr;
	vsize_t off;
	paddr_t pa;

	faddr = trunc_page(uaddr);
	off = uaddr - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	kaddr = taddr + off;
	for (; len > 0; len -= NBPG) {
		(void) pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map),
		    faddr, &pa);
		pmap_kenter_pa(taddr, pa, prot);
		faddr += NBPG;
		taddr += NBPG;
	}
	return (kaddr);
}

/*
 * Undo vmaprange.
 */
void
vunmaprange(kaddr, len)
	vaddr_t kaddr;
	vsize_t len;
{
	vaddr_t addr;
	vsize_t off;

	addr = trunc_page(kaddr);
	off = kaddr - addr;
	len = round_page(off + len);
	pmap_kremove(addr, len);
	uvm_km_free_wakeup(phys_map, addr, len);
}
#endif /* PPC_IBM4XX */

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: these pages have already been locked by uvm_vslock.
 */
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr;
	vsize_t off;
	paddr_t pa;

#ifdef	DIAGNOSTIC
	if (!(bp->b_flags & B_PHYS))
		panic("vmapbuf");
#endif
	/*
	 * XXX Reimplement this with vmaprange (on at least PPC_IBM4XX CPUs).
	 */
	faddr = trunc_page((vaddr_t)bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	for (; len > 0; len -= NBPG) {
		(void) pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    faddr, &pa);
		pmap_kenter_pa(taddr, pa, VM_PROT_READ|VM_PROT_WRITE);
		faddr += NBPG;
		taddr += NBPG;
	}
	pmap_update();
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
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
	pmap_kremove(addr, len);
	pmap_update();
	uvm_km_free_wakeup(phys_map, addr, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
