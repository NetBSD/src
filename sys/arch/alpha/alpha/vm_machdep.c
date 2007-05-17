/* $NetBSD: vm_machdep.c,v 1.93 2007/05/17 14:51:12 yamt Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.93 2007/05/17 14:51:12 yamt Exp $");
#include "opt_coredump.h"

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
#include <machine/alpha.h>
#include <machine/pmap.h>
#include <machine/reg.h>

#ifdef COREDUMP
/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	int error;
	struct md_coredump cpustate;
	struct coreseg cseg;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(cpustate);
		chdr->c_nseg++;
		return 0;
	}

	cpustate.md_tf = *l->l_md.md_tf;
	cpustate.md_tf.tf_regs[FRAME_SP] = alpha_pal_rdusp();	/* XXX */
	if (l->l_md.md_flags & MDP_FPUSED) {
		if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
			fpusave_proc(l, 1);
		cpustate.md_fpstate = l->l_addr->u_pcb.pcb_fp;
	} else
		memset(&cpustate.md_fpstate, 0, sizeof(cpustate.md_fpstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &cpustate,
	    sizeof(cpustate));
}
#endif

void
cpu_lwp_free(struct lwp *l, int proc)
{

	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 0);
}

void
cpu_lwp_free2(struct lwp *l)
{
	(void) l;
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
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct user *up = l2->l_addr;

	l2->l_md.md_tf = l1->l_md.md_tf;

	l2->l_md.md_flags = l1->l_md.md_flags & (MDP_FPUSED | MDP_FP_C);

	/*
	 * Cache the physical address of the pcb, so we can
	 * swap to it easily.
	 */
	l2->l_md.md_pcbpaddr = (void *)vtophys((vaddr_t)&up->u_pcb);

	/*
	 * Copy floating point state from the FP chip to the PCB
	 * if this process has state stored there.
	 */
	if (l1->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l1, 1);

	/*
	 * Copy pcb and user stack pointer from proc p1 to p2.
	 * If specificed, give the child a different stack.
	 */
	l2->l_addr->u_pcb = l1->l_addr->u_pcb;
	if (stack != NULL)
		l2->l_addr->u_pcb.pcb_hw.apcb_usp = (u_long)stack + stacksize;
	else
		l2->l_addr->u_pcb.pcb_hw.apcb_usp = alpha_pal_rdusp();
	simple_lock_init(&l2->l_addr->u_pcb.pcb_fpcpu_slock);

	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from setjmp.
	 */
#ifdef DIAGNOSTIC
	/*
	 * If l1 != curlwp && l1 == &lwp0, we are creating a kernel
	 * thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

	/*
	 * create the child's kernel stack, from scratch.
	 */
	{
		struct trapframe *l2tf;

		/*
		 * Pick a stack pointer, leaving room for a trapframe;
		 * copy trapframe from parent so return to user mode
		 * will be to right address, with correct registers.
		 */
		l2tf = l2->l_md.md_tf = (struct trapframe *)
		    ((char *)l2->l_addr + USPACE - sizeof(struct trapframe));
		memcpy(l2->l_md.md_tf, l1->l_md.md_tf,
		    sizeof(struct trapframe));

		/*
		 * Set up return-value registers as fork() libc stub expects.
		 */
		l2tf->tf_regs[FRAME_V0] = l1->l_proc->p_pid; /* parent's pid */
		l2tf->tf_regs[FRAME_A3] = 0;		/* no error */
		l2tf->tf_regs[FRAME_A4] = 1;		/* is child */

		cpu_setfunc(l2, func, arg);
	}
}

void
cpu_setfunc(l, func, arg)
	struct lwp *l;
	void (*func) __P((void *));
	void *arg;
{
	struct user *up = l->l_addr;

	up->u_pcb.pcb_hw.apcb_ksp =
	    (u_int64_t)l->l_md.md_tf;
	up->u_pcb.pcb_context[0] =
	    (u_int64_t)func;			/* s0: pc */
	up->u_pcb.pcb_context[1] =
	    (u_int64_t)exception_return;	/* s1: ra */
	up->u_pcb.pcb_context[2] =
	    (u_int64_t)arg;			/* s2: arg */
	up->u_pcb.pcb_context[3] =
	    (u_int64_t)l;			/* s3: lwp */
	up->u_pcb.pcb_context[7] =
	    (u_int64_t)lwp_trampoline;		/* ra: assembly magic */
}	

/*
 * Finish a swapin operation.
 *
 * We need to cache the physical address of the PCB, so we can
 * swap context to it easily.
 */
void
cpu_swapin(struct lwp *l)
{
	struct user *up = l->l_addr;

	l->l_md.md_pcbpaddr = (void *)vtophys((vaddr_t)&up->u_pcb);
}

/*
 * cpu_swapout is called immediately before a process's 'struct user'
 * and kernel stack are unwired (which are in turn done immediately
 * before it's P_INMEM flag is cleared).  If the process is the
 * current owner of the floating point unit, the FP state has to be
 * saved, so that it goes out with the pcb, which is in the user area.
 */
void
cpu_swapout(struct lwp *l)
{

	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 1);
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
	paddr_t pa;
	struct proc *p;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY|UVM_KMF_WAITVA);
	bp->b_data = (void *)(taddr + off);
	len = atop(len);
	while (len--) {
		if (pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map), faddr,
		    &pa) == false)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), taddr, trunc_page(pa),
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
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
	pmap_remove(vm_map_pmap(phys_map), addr, addr + len);
	pmap_update(vm_map_pmap(phys_map));
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
