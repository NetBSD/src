/*	$NetBSD: vm_machdep.c,v 1.34 1999/05/26 22:19:37 thorpej Exp $	*/

/*-
 * Copyright (c) 1996 Matthias Pfaller.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Philip A. Nelson.
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * Copyright (c) 1989, 1990 William Jolitz
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 */

#include "opt_pmap_new.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/ptrace.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>

extern struct proc *fpu_proc;

void	setredzone __P((u_short *, caddr_t));

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy the pcb and setup the kernel stack for the child.
 * Setup the child's stackframe to return to child_return
 * via proc_trampoline from cpu_switch.
 */
void
cpu_fork(p1, p2, stack, stacksize)
	register struct proc *p1, *p2;
	void *stack;
	size_t stacksize;
{
	register struct pcb *pcb = &p2->p_addr->u_pcb;
	register struct syscframe *tf;
	register struct switchframe *sf;

#ifdef DIAGNOSTIC
	/*
	 * if p1 != curproc && p1 == &proc0, we're creating a kernel thread.
	 */
	if (p1 != curproc && p1 != &proc0)
		panic("cpu_fork: curproc");
#endif

	/* Copy pcb from proc p1 to p2. */
	*pcb = p1->p_addr->u_pcb;
	pcb->pcb_onstack = (struct reg *)((u_int)p2->p_addr + USPACE) - 1;
	*pcb->pcb_onstack = *p1->p_addr->u_pcb.pcb_onstack;
	/* If p1 is holding the FPU, update the FPU context of p2. */
	if (fpu_proc == p1)
		save_fpu_context(pcb);
	pmap_activate(p2);

	/*
	 * Copy the syscframe, and arrange for the child to return directly
	 * through rei().  Note the in-line cpu_set_kpc().
	 */
	tf = (struct syscframe *)((u_int)p2->p_addr + USPACE) - 1;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->sf_regs.r_sp = (u_int)stack + stacksize;

	p2->p_md.md_regs = &tf->sf_regs;
	sf = (struct switchframe *)tf - 1;
	sf->sf_p  = p2;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_fp = (long) &tf->sf_regs.r_fp;
	sf->sf_r3 = (long) child_return;
	sf->sf_r4 = (long) p2;
	sf->sf_pl = imask[IPL_ZERO];
	pcb->pcb_ksp = (long) sf;
	pcb->pcb_kfp = (long) &sf->sf_fp;
}

/*
 * cpu_set_kpc:
 *
 * Arrange for in-kernel execution of a process to continue at the
 * named pc, as if the code at that address were called as a function
 * with the supplied argument.
 *
 * Note that it's assumed that when the named process returns, rei()
 * should be invoked, to return to user mode.
 */
void
cpu_set_kpc(p, pc, arg)
	struct proc *p;
	void (*pc) __P((void *));
	void *arg;
{
	struct pcb *pcbp;
	struct switchframe *sf;

	pcbp = &p->p_addr->u_pcb;
	sf = (struct switchframe *) pcbp->pcb_ksp;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_r3 = (long) pc;
	sf->sf_r4 = (long) arg;
}

/*
 * cpu_swapout is called immediately before a process's 'struct user'
 * and kernel stack are unwired (which are in turn done immediately
 * before it's P_INMEM flag is cleared).  If the process is the
 * current owner of the floating point unit, the FP state has to be
 * saved, so that it goes out with the pcb, which is in the user area.
 */
void
cpu_swapout(p)
	struct proc *p;
{
	/*
	 * Make sure we save the FP state before the user area vanishes.
	 */
	if (fpu_proc != p)
		return;
	save_fpu_context(&p->p_addr->u_pcb);
	fpu_proc = 0;
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to proc0's context, and finally
 * jumps into switch() to wait for another process to wake up.
 */
void
cpu_exit(arg)
	struct proc *arg;
{
	extern struct user *proc0paddr;
	register struct proc *p __asm("r3");
	uvmexp.swtch++;

	/* Copy arg into a register. */
	movd(arg, p);

	/* If we were using the FPU, forget about it. */
	if (fpu_proc == p)
		fpu_proc = 0;

	/* Switch to temporary stack and address space. */
	lprd(sp, INTSTACK);
	load_ptb(proc0paddr->u_pcb.pcb_ptb);

	/* Schedule the vmspace and stack to be freed. */
	(void) splhigh();
	exit2(p);

	/* Don't update pcb in cpu_switch. */
	curproc = NULL;
	cpu_switch(NULL);
	/* NOTREACHED */
}

/*
 * Dump the machine specific segment at the start of a core dump.
 */     
struct md_core {
	struct reg intreg;
	struct fpreg freg;
};

int
cpu_coredump(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core *chdr;
{
	struct md_core md_core;
	struct coreseg cseg;
	int error;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = sizeof(md_core);

	/* Save integer registers. */
	error = process_read_regs(p, &md_core.intreg);
	if (error)
		return error;

	/* Save floating point registers. */
	error = process_read_fpregs(p, &md_core.freg);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred,
	    NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	chdr->c_nseg++;
	return 0;
}

#if 0
/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte, vaddr)
	u_short *pte;
	caddr_t vaddr;
{
/* eventually do this by setting up an expand-down stack segment
   for ss0: selector, allowing stack access down to top of u.
   this means though that protection violations need to be handled
   thru a double fault exception that must do an integral task
   switch to a known good context, within which a dump can be
   taken. a sensible scheme might be to save the initial context
   used by sched (that has physical memory mapped 1:1 at bottom)
   and take the dump while still in mapped mode */
}
#endif

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
void
pagemove(from, to, size)
	register caddr_t from, to;
	size_t size;
{
	register pt_entry_t *fpte, *tpte, ofpte, otpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = kvtopte(from);
	tpte = kvtopte(to);

	if (size <= NBPG * 16) {
		while (size > 0) {
			otpte = *tpte;
			ofpte = *fpte;
			*tpte++ = *fpte;
			*fpte++ = 0;
			if (otpte & PG_V)
				tlbflush_entry((vaddr_t) to);
			if (ofpte & PG_V)
				tlbflush_entry((vaddr_t) from);
			from += NBPG;
			to += NBPG;
			size -= NBPG;
		}
	} else {
		while (size > 0) {
			*tpte++ = *fpte;
			*fpte++ = 0;
			from += NBPG;
			to += NBPG;
			size -= NBPG;
		}
		pmap_update();
	}
}

/*
 * Convert kernel VA to physical address
 */
int
kvtop(addr)
	register caddr_t addr;
{
	vaddr_t va;

	va = pmap_extract(pmap_kernel(), (vaddr_t)addr);
	if (va == 0)
		panic("kvtop: zero page frame");
	return((int)va);
}

extern vm_map_t phys_map;

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().   
 */
#if defined(PMAP_NEW)
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr, off;
	paddr_t fpa;
	pt_entry_t *tpte;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	faddr = trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr= uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 * XXX: unwise to expect this in a multithreaded environment.
	 * anything can happen to a pmap between the time we lock a 
	 * region, release the pmap lock, and then relock it for
	 * the pmap_extract().
	 *
	 * no need to flush TLB since we expect nothing to be mapped
	 * where we we just allocated (TLB will be flushed when our
	 * mapping is removed).
	 */
	tpte = PTE_BASE + ns532_btop(taddr);
	while (len) {
		fpa = pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
				   faddr);
		*tpte = fpa | PG_RW | PG_V;
		tpte++;
		faddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
}
#else /* PMAP_NEW */
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr, off;
	pt_entry_t *fpte, *tpte;
	pt_entry_t *pmap_pte __P((pmap_t, vaddr_t));

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	faddr = trunc_page(bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	fpte = pmap_pte(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map), faddr);
	tpte = pmap_pte(vm_map_pmap(phys_map), taddr);
	do {
		*tpte++ = *fpte++;
		len -= PAGE_SIZE;
	} while (len);
}
#endif /* PMAP_NEW */

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t addr, off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	addr = trunc_page(bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	uvm_km_free_wakeup(phys_map, addr, len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
