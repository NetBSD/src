/*	$NetBSD: vm_machdep.c,v 1.67.4.1 2007/02/27 16:52:24 yamt Exp $	*/

/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 */

/*-
 * Copyright (c) 1996 Matthias Pfaller.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Philip A. Nelson.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.67.4.1 2007/02/27 16:52:24 yamt Exp $");

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

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>

extern struct lwp *fpu_lwp;

void	setredzone(u_short *, caddr_t);

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
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb = &l2->l_addr->u_pcb;
	struct syscframe *tf;
	struct switchframe *sf;

#ifdef DIAGNOSTIC
	/*
	 * if l1 != curlwp && l1 == &lwp0, we're creating a kernel thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

	/* Copy pcb from lwp l1 to l2. */
	*pcb = l1->l_addr->u_pcb;
	pcb->pcb_onstack = (struct reg *)((u_int)l2->l_addr + USPACE) - 1;
	*pcb->pcb_onstack = *l1->l_addr->u_pcb.pcb_onstack;
	/* If l1 is holding the FPU, update the FPU context of l2. */
	if (fpu_lwp == l1)
		save_fpu_context(pcb);
	pmap_activate(l2);

	/*
	 * Copy the syscframe.
	 */
	tf = (struct syscframe *)((u_int)l2->l_addr + USPACE) - 1;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->sf_regs.r_sp = (u_int)stack + stacksize;

	l2->l_md.md_regs = &tf->sf_regs;
	sf = (struct switchframe *)tf - 1;
	sf->sf_lwp= l2;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_fp = (long) &tf->sf_regs.r_fp;
	sf->sf_r3 = (long) func;
	sf->sf_r4 = (long) arg;
	sf->sf_pl = imask[IPL_ZERO];
	pcb->pcb_ksp = (long) sf;
	pcb->pcb_kfp = (long) &sf->sf_fp;
}

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct syscframe *tf;
	struct switchframe *sf;

	tf = (struct syscframe *)((u_int)l->l_addr + USPACE) - 1;
	sf = (struct switchframe *)tf - 1;
	sf->sf_lwp = l;
	sf->sf_pc = (long) proc_trampoline;
	sf->sf_fp = (long) &tf->sf_regs.r_fp;
	sf->sf_r3 = (long) func;
	sf->sf_r4 = (long) arg;
	sf->sf_pl = imask[IPL_ZERO];
	pcb->pcb_ksp = (long) sf;
	pcb->pcb_kfp = (long) &sf->sf_fp;
}

/*
 * cpu_swapout is called immediately before a process's 'struct user'
 * and kernel stack are unwired (which are in turn done immediately
 * after its P_INMEM flag is cleared).  If the process is the
 * current owner of the floating point unit, the FP state has to be
 * saved, so that it goes out with the pcb, which is in the user area.
 */
void
cpu_swapout(struct lwp *l)
{

	/*
	 * Make sure we save the FP state before the user area vanishes.
	 */
	if (fpu_lwp != l)
		return;
	save_fpu_context(&l->l_addr->u_pcb);
	fpu_lwp = 0;
}

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* If we were using the FPU, forget about it. */
	if (fpu_lwp == l)
		fpu_lwp = 0;
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to proc0's context, and finally
 * jumps into switch() to wait for another process to wake up.
 */
void
cpu_exit(struct lwp *arg)
{
	extern struct user *proc0paddr;
	register struct lwp *l __asm("r3");

	/* Copy arg into a register. */
	movd(arg, l);

	/* Switch to temporary stack and address space. */
	lprd(sp, INTSTACK);
	load_ptb(proc0paddr->u_pcb.pcb_ptb);

	/* Schedule the vmspace and stack to be freed. */
	(void) splhigh();
	lwp_exit2(l);		/* XXXJRT FRAGILE!  USE A REGISTER! */

	/* Don't update pcb in cpu_switch. */
	curlwp = NULL;
	cpu_switch(NULL, NULL);
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
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	struct md_core md_core;
	struct coreseg cseg;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core);
		chdr->c_nseg++;
		return 0;
	}

	/* Save integer registers. */
	error = process_read_regs(l, &md_core.intreg);
	if (error)
		return error;

	/* Save floating point registers. */
	error = process_read_fpregs(l, &md_core.freg);
	if (error)
		return error;

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

#if 0
/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(u_short *pte, caddr_t vaddr)
{

/*
 * eventually do this by setting up an expand-down stack segment
 * for ss0: selector, allowing stack access down to top of u.
 * this means though that protection violations need to be handled
 * thru a double fault exception that must do an integral task
 * switch to a known good context, within which a dump can be
 * taken. a sensible scheme might be to save the initial context
 * used by sched (that has physical memory mapped 1:1 at bottom)
 * and take the dump while still in mapped mode.
 */
}
#endif

/*
 * Convert kernel VA to physical address
 */
int
kvtop(caddr_t addr)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t)addr, &pa) == false)
		panic("kvtop: zero page frame");
	return((int)pa);
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t fpa;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	faddr = trunc_page((vaddr_t)(bp->b_saveaddr = bp->b_data));
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr= uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
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
	while (len) {
		(void) pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    faddr, &fpa);
		pmap_kenter_pa(taddr, fpa, VM_PROT_READ | VM_PROT_WRITE);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
	pmap_update(vm_map_pmap(phys_map));
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
	pmap_kremove(addr, len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
