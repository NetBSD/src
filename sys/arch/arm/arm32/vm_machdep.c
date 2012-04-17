/*	$NetBSD: vm_machdep.c,v 1.55.4.1 2012/04/17 00:06:04 yamt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * vm_machdep.h
 *
 * vm machine specific bits
 *
 * Created      : 08/10/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.55.4.1 2012/04/17 00:06:04 yamt Exp $");

#include "opt_armfpe.h"
#include "opt_pmap_debug.h"
#include "opt_perfctrs.h"
#include "opt_cputypes.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/pmc.h>
#include <sys/exec.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/reg.h>
#include <machine/vmparam.h>

#ifdef ARMFPE
#include <arm/fpe-arm/armfpe.h>
#endif

extern pv_addr_t systempage;

int process_read_regs(struct proc *p, struct reg *regs);
int process_read_fpregs(struct proc *p, struct fpreg *regs);

void lwp_trampoline(void);

/*
 * Special compilation symbols:
 *
 * STACKCHECKS - Fill undefined and supervisor stacks with a known pattern
 *		 on forking and check the pattern on exit, reporting
 *		 the amount of stack used.
 */

void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{

#if defined(PERFCTRS)
	if (PMC_ENABLED(p1))
		pmc_md_fork(p1, p2);
	else {
		p2->p_md.pmc_enabled = 0;
		p2->p_md.pmc_state = NULL;
	}
#endif
}

/*
 * Finish a fork operation, with LWP l2 nearly set up.
 *
 * Copy and update the pcb and trapframe, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() which will call the specified func with the argument arg.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb1, *pcb2;
	struct trapframe *tf;
	struct switchframe *sf;
	vaddr_t uv;

	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("cpu_lwp_fork: %p %p %p %p\n", l1, l2, curlwp, &lwp0);
#endif	/* PMAP_DEBUG */

#if 0 /* XXX */
	if (l1 == curlwp) {
		/* Sync the PCB before we copy it. */
		savectx(curpcb);
	}
#endif

	l2->l_md.md_flags = l1->l_md.md_flags & MDP_VFPUSED;

#ifdef FPU_VFP
	/*
	 * Copy the floating point state from the VFP to the PCB
	 * if this process has state stored there.
	 */
	if (pcb1->pcb_vfpcpu != NULL)
		vfp_saveregs_lwp(l1, 1);
#endif

	/* Copy the pcb */
	*pcb2 = *pcb1;

	/* 
	 * Set up the kernel stack for the process.
	 * Note: this stack is not in use if we are forking from p1
	 */
	uv = uvm_lwp_getuarea(l2);
	pcb2->pcb_un.un_32.pcb32_sp = uv + USPACE_SVC_STACK_TOP;

#ifdef STACKCHECKS
	/* Fill the kernel stack with a known pattern */
	memset((void *)(uv + USPACE_SVC_STACK_BOTTOM), 0xdd,
	    (USPACE_SVC_STACK_TOP - USPACE_SVC_STACK_BOTTOM));
#endif	/* STACKCHECKS */

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0) {
		printf("l1: pcb=%p pid=%d pmap=%p\n",
		    pcb1, l1->l_lid, l1->l_proc->p_vmspace->vm_map.pmap);
		printf("l2: pcb=%p pid=%d pmap=%p\n",
		    pcb2, l2->l_lid, l2->l_proc->p_vmspace->vm_map.pmap);
	}
#endif	/* PMAP_DEBUG */

#ifdef ARMFPE
	/* Initialise a new FP context for p2 and copy the context from p1 */
	arm_fpe_core_initcontext(FP_CONTEXT(l2));
	arm_fpe_copycontext(FP_CONTEXT(l1), FP_CONTEXT(l2));
#endif	/* ARMFPE */

	tf = (struct trapframe *)pcb2->pcb_un.un_32.pcb32_sp - 1;
	pcb2->pcb_tf = tf;
	*tf = *pcb1->pcb_tf;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_usr_sp = (u_int)stack + stacksize;

	sf = (struct switchframe *)tf - 1;
	sf->sf_r4 = (u_int)func;
	sf->sf_r5 = (u_int)arg;
	sf->sf_sp = (u_int)tf;
	sf->sf_pc = (u_int)lwp_trampoline;
	pcb2->pcb_un.un_32.pcb32_sp = (u_int)sf;
}

/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to lwp0's context, and finally
 * jumps into switch() to wait for another process to wake up.
 */

void
cpu_lwp_free(struct lwp *l, int proc)
{
#ifdef FPU_VFP
	struct pcb *pcb;
#endif

#ifdef ARMFPE
	/* Abort any active FP operation and deactivate the context */
	arm_fpe_core_abort(FP_CONTEXT(l), NULL, NULL);
	arm_fpe_core_changecontext(0);
#endif	/* ARMFPE */

#ifdef FPU_VFP
	pcb = lwp_getpcb(l);
	if (pcb->pcb_vfpcpu != NULL)
		vfp_saveregs_lwp(l, 0);
#endif

#ifdef STACKCHECKS
	/* Report how much stack has been used - debugging */
	if (l) {
		u_char *ptr;
		int loop;

		ptr = (u_char *)pcb + USPACE_SVC_STACK_BOTTOM;
		for (loop = 0; loop < (USPACE_SVC_STACK_TOP - USPACE_SVC_STACK_BOTTOM)
		    && *ptr == 0xdd; ++loop, ++ptr) ;
		log(LOG_INFO, "%d bytes of svc stack fill pattern\n", loop);
	}
#endif	/* STACKCHECKS */
}

void
cpu_lwp_free2(struct lwp *l)
{
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t faddr, taddr, off;
	paddr_t fpa;


#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("vmapbuf: bp=%08x buf=%08x len=%08x\n", (u_int)bp,
		    (u_int)bp->b_data, (u_int)len);
#endif	/* PMAP_DEBUG */
    
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(taddr + off);

	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	while (len) {
		(void) pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    faddr, &fpa);
		pmap_enter(pmap_kernel(), taddr, fpa,
			VM_PROT_READ|VM_PROT_WRITE, VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t addr, off;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("vunmapbuf: bp=%08x buf=%08x len=%08x\n",
		    (u_int)bp, (u_int)bp->b_data, (u_int)len);
#endif	/* PMAP_DEBUG */

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");

	/*
	 * Make sure the cache does not have dirty data for the
	 * pages we had mapped.
	 */
	addr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - addr;
	len = round_page(off + len);
	
	pmap_remove(pmap_kernel(), addr, addr + len);
	pmap_update(pmap_kernel());
	uvm_km_free(phys_map, addr, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}

/* End of vm_machdep.c */
