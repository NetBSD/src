/*	$NetBSD: vm_machdep.c,v 1.99 2007/12/22 08:59:02 dsl Exp $	     */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.99 2007/12/22 08:59:02 dsl Exp $");

#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_coredump.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/mount.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>
#include <machine/mtpr.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/macros.h>
#include <machine/trap.h>
#include <machine/pcb.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/sid.h>

#include <sys/syscallargs.h>

#include "opt_cputype.h"

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * cpu_lwp_bootstrap() and call child_return() with p2 as an
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
 *
 * cpu_lwp_fork() copies parent process trapframe and creates a fake CALLS
 * frame on top of it, so that it can safely call child_return().
 * We also take away mapping for the fourth page after pcb, so that
 * we get something like a "red zone" for the kernel stack.
 */
void cpu_lwp_bootstrap(void);
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct callsframe *cf;
	extern int sret; /* Return address in trap routine */

#ifdef DIAGNOSTIC
	/*
	 * if p1 != curlwp && p1 == &proc0, we're creating a kernel thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

	/*
	 * Copy the trap frame.
	 */
	tf = (struct trapframe *)((u_int)l2->l_addr + USPACE) - 1;
	l2->l_addr->u_pcb.framep = tf;
	*tf = *(struct trapframe *)l1->l_addr->u_pcb.framep;

	/*
	 * Activate address space for the new process.	The PTEs have
	 * already been allocated by way of pmap_create().
	 * This writes the page table registers to the PCB.
	 */
	pmap_activate(l2);

	/* Mark guard page invalid in kernel stack */
	kvtopte((uintptr_t)l2->l_addr + REDZONEADDR)->pg_v = 0;

	/*
	 * Set up the calls frame above (below) the trapframe and populate
	 * it with something good.  This is so that we can simulate that we 
	 * called cpu_lwp_bootstrap with a CALLS insn and it can return to
	 * sret.
	 */
	cf = (struct callsframe *)tf - 1;
	cf->ca_cond = 0;
	cf->ca_maskpsw = 0x20000000;	/* CALLS stack frame, no registers */
	cf->ca_pc = (uintptr_t)&sret;	/* return PC; userspace trampoline */
	cf->ca_argno = 1;
	cf->ca_arg1 = 0;		/* unused */

	/*
	 * Set up internal defs in PCB. This matches the "fake" CALLS frame
	 * that were constructed earlier.
	 */
	pcb = &l2->l_addr->u_pcb;
	pcb->iftrap = NULL;
	pcb->AP = (uintptr_t)&cf->ca_argno;
	pcb->KSP = (uintptr_t)cf;
	pcb->FP = (uintptr_t)cf;
	pcb->PC = (uintptr_t)cpu_lwp_bootstrap + 2;
	pcb->PSL = PSL_HIGHIPL;
	/* pcb->R[0] (oldlwp) set by Swtchto */
	pcb->R[1] = (uintptr_t)l2;
	pcb->R[2] = (uintptr_t)func;
	pcb->R[3] = (uintptr_t)arg;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->sp = (uintptr_t)stack + stacksize;

	/*
	 * Set the last return information after fork().
	 * This is only interesting if the child will return to userspace,
	 * but doesn't hurt otherwise.
	 */
	tf->r0 = l1->l_proc->p_pid; /* parent pid. (shouldn't be needed) */
	tf->r1 = 1;
	tf->psl = PSL_U|PSL_PREVU;
}

int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	return ENOEXEC;
}

int
sys_sysarch(struct lwp *l, const struct sys_sysarch_args *uap, register_t *retval)
{
	return (ENOSYS);
}

#ifdef COREDUMP
/*
 * Dump the machine specific header information at the start of a core dump.
 * First put all regs in PCB for debugging purposes. This is not an good
 * way to do this, but good for my purposes so far.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	struct md_coredump md_core;
	struct coreseg cseg;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = sizeof(struct core);
		chdr->c_seghdrsize = sizeof(struct coreseg);
		chdr->c_cpusize = sizeof(struct md_coredump);
		chdr->c_nseg++;
		return 0;
	}

	md_core.md_tf = *(struct trapframe *)l->l_addr->u_pcb.framep; /*XXX*/

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

/*
 * Map in a bunch of pages read/writable for the kernel.
 */
void
ioaccess(vaddr_t vaddr, paddr_t paddr, int npgs)
{
	uint32_t *pte = (uint32_t *)kvtopte(vaddr);
	int i;

	for (i = 0; i < npgs; i++)
		pte[i] = PG_V | PG_KW | (PG_PFNUM(paddr) + i);
}

/*
 * Opposite to the above: just forget their mapping.
 */
void
iounaccess(vaddr_t vaddr, int npgs)
{
	uint32_t *pte = (uint32_t *)kvtopte(vaddr);
	int i;

	for (i = 0; i < npgs; i++)
		pte[i] = 0;
	mtpr(0, PR_TBIA);
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
#if VAX46 || VAX48 || VAX49 || VAX53 || VAXANY
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;

	if (vax_boardtype != VAX_BTYP_46
	    && vax_boardtype != VAX_BTYP_48
	    && vax_boardtype != VAX_BTYP_49
	    && vax_boardtype != VAX_BTYP_53)
		return;
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_saveaddr);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
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
#endif
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
#if VAX46 || VAX48 || VAX49 || VAX53 || VAXANY
	vaddr_t addr, off;

	if (vax_boardtype != VAX_BTYP_46
	    && vax_boardtype != VAX_BTYP_48
	    && vax_boardtype != VAX_BTYP_49
	    && vax_boardtype != VAX_BTYP_53)
		return;
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
#endif
}
