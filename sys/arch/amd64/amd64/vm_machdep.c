/*	$NetBSD: vm_machdep.c,v 1.13.10.1 2006/05/11 23:26:18 elad Exp $	*/

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
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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

/*
 *	Utah $Hdr: vm_machdep.c 1.16.1.1 89/06/23$
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.13.10.1 2006/05/11 23:26:18 elad Exp $");

#include "opt_user_ldt.h"
#include "opt_largepages.h"

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
#include <machine/gdt.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/fpu.h>
#include <machine/mtrr.h>

extern char x86_64_doubleflt_stack[];

static void setredzone __P((struct lwp *));

void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{
	p2->p_md.md_flags = p1->p_md.md_flags;
	if (p1->p_flag & P_32)
		p2->p_flag |= P_32;
}

/*
 * Finish a new thread operation, with lwp l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * proc_trampoline() and call child_return() with l2 as an
 * argument. This causes the newly-created child process to go
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
cpu_lwp_fork(l1, l2, stack, stacksize, func, arg)
	struct lwp *l1, *l2;
	void *stack;
	size_t stacksize;
	void (*func) __P((void *));
	void *arg;
{
	struct pcb *pcb = &l2->l_addr->u_pcb;
	struct trapframe *tf;
	struct switchframe *sf;

	/*
	 * If fpuproc != p1, then the fpu h/w state is irrelevant and the
	 * state had better already be in the pcb.  This is true for forks
	 * but not for dumps.
	 *
	 * If fpuproc == p1, then we have to save the fpu h/w state to
	 * p1's pcb so that we can copy it.
	 */
	if (l1->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_lwp(l1, 1);

	l2->l_md.md_flags = l1->l_md.md_flags;

	/* Copy pcb from proc p1 to p2. */
	if (l1 == curlwp) {
		/* Sync the PCB before we copy it. */
		savectx(curpcb);
	}
#ifdef DIAGNOSTIC
	else if (l1 != &lwp0)
		panic("cpu_fork: curproc");
#endif
	*pcb = l1->l_addr->u_pcb;

	/*
	 * Preset these so that gdt_compact() doesn't get confused if called
	 * during the allocations below.
	 *
	 * Note: pcb_ldt_sel is handled in the pmap_activate() call when
	 * we run the new process.
	 */
	l2->l_md.md_tss_sel = GSEL(GNULL_SEL, SEL_KPL);

	/* Fix up the TSS. */
	pcb->pcb_tss.tss_rsp0 = (u_int64_t)l2->l_addr + USPACE - 16;
	pcb->pcb_tss.tss_ist[0] = (u_int64_t)l2->l_addr + PAGE_SIZE - 16;
	pcb->pcb_tss.tss_ist[1] = (uint64_t)x86_64_doubleflt_stack
	    + PAGE_SIZE - 16;

	l2->l_md.md_tss_sel = tss_alloc(pcb);

	/*
	 * Copy the trapframe.
	 */
	l2->l_md.md_regs = tf = (struct trapframe *)pcb->pcb_tss.tss_rsp0 - 1;
	*tf = *l1->l_md.md_regs;

	setredzone(l2);

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_rsp = (u_int64_t)stack + stacksize;

	sf = (struct switchframe *)tf - 1;
	sf->sf_r12 = (u_int64_t)func;
	sf->sf_r13 = (u_int64_t)arg;
	if (func == child_return && !(l2->l_proc->p_flag & P_32))
		sf->sf_rip = (u_int64_t)child_trampoline;
	else
		sf->sf_rip = (u_int64_t)proc_trampoline;
	pcb->pcb_rsp = (u_int64_t)sf;
	pcb->pcb_rbp = 0;

	pcb->pcb_fs = l1->l_addr->u_pcb.pcb_fs;
	pcb->pcb_gs = l1->l_addr->u_pcb.pcb_fs;
}

void
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf = l->l_md.md_regs;
	struct switchframe *sf = (struct switchframe *)tf - 1;

	sf->sf_r12 = (u_int64_t)func;
	sf->sf_r13 = (u_int64_t)arg;
	if (func == child_return && !(l->l_proc->p_flag & P_32))
		sf->sf_rip = (u_int64_t)child_trampoline;
	else
		sf->sf_rip = (u_int64_t)proc_trampoline;
	pcb->pcb_rsp = (u_int64_t)sf;
	pcb->pcb_rbp = 0;
}

void
cpu_swapin(l)
	struct lwp *l;
{
	setredzone(l);
}

void
cpu_swapout(l)
	struct lwp *l;
{

	/*
	 * Make sure we save the FP state before the user area vanishes.
	 */
	fpusave_lwp(l, 1);
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
	/* If we were using the FPU, forget about it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_lwp(l, 0);

	if (proc && l->l_md.md_flags & MDP_USEDMTRR)
		mtrr_clean(l->l_proc);

	/* Nuke the TSS. */
	tss_free(l->l_md.md_tss_sel);
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to proc0's context, and finally
 * jumps into switch() to wait for another process to wake up.
 */
void
cpu_exit(struct lwp *l)
{

	switch_exit(l, lwp_exit2);
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

	return coredump_write(iocookie, UIO_USERSPACE, &md_core,
	    sizeof(md_core));
}

/*
 * Set a red zone in the kernel stack after the u. area.
 */
static void
setredzone(struct lwp *l)
{
	pmap_remove(pmap_kernel(), (vaddr_t)l->l_addr + PAGE_SIZE,
	    (vaddr_t)l->l_addr + 2 * PAGE_SIZE);
	pmap_update(pmap_kernel());
}

/*
 * Convert kernel VA to physical address
 */
int
kvtop(addr)
	register caddr_t addr;
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), (vaddr_t)addr, &pa) == FALSE)
		panic("kvtop: zero page frame");
	return((int)pa);
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().   
 */
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr, off;
	paddr_t fpa;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
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
		pmap_kenter_pa(taddr, fpa, VM_PROT_READ|VM_PROT_WRITE);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
}

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
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	pmap_kremove(addr, len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}
