/*	$NetBSD: vm_machdep.c,v 1.43 1999/05/26 00:40:20 thorpej Exp $	*/

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

#include "opt_armfpe.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/syslog.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/reg.h>
#include <machine/vmparam.h>

#ifdef ARMFPE
#include <machine/cpus.h>
#include <arm32/fpe-arm/armfpe.h>
#endif

extern pv_addr_t systempage;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

int process_read_regs	__P((struct proc *p, struct reg *regs));
int process_read_fpregs	__P((struct proc *p, struct fpreg *regs));

void	switch_exit	__P((struct proc *p, struct proc *proc0));
extern void proc_trampoline	__P((void));

pt_entry_t *pmap_pte	__P((pmap_t, vm_offset_t));

/*
 * Special compilation symbols:
 *
 * STACKCHECKS - Fill undefined and supervisor stacks with a known pattern
 *		 on forking and check the pattern on exit, reporting
 *		 the amount of stack used.
 */

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the kernel stack and pcb, making the child
 * ready to run, and marking it so that it can return differently
 * than the parent.  Returns 1 in the child process, 0 in the parent.
 * We currently double-map the user area so that the stack is at the same
 * address in each process; in the future we will probably relocate
 * the frame pointers on the stack after copying.
 */

void
cpu_fork(p1, p2, stack, stacksize)
	struct proc *p1;
	struct proc *p2;
	void *stack;
	size_t stacksize;
{
	struct pcb *pcb = (struct pcb *)&p2->p_addr->u_pcb;
	struct trapframe *tf;
	struct switchframe *sf;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("cpu_fork: %p %p %p %p\n", p1, p2, curproc, &proc0);
#endif	/* PMAP_DEBUG */

#if 0 /* XXX */
	if (p1 == curproc) {
		/* Sync the PCB before we copy it. */
		savectx(curpcb);
	}
#endif

	/* Copy the pcb */
	*pcb = p1->p_addr->u_pcb;

	/* 
	 * Set up the undefined stack for the process.
	 * Note: this stack is not in use if we are forking from p1
	 */
	pcb->pcb_und_sp = (u_int)p2->p_addr + USPACE_UNDEF_STACK_TOP;
	pcb->pcb_sp = (u_int)p2->p_addr + USPACE_SVC_STACK_TOP;

#ifdef STACKCHECKS
	/* Fill the undefined stack with a known pattern */
	memset(((u_char *)p2->p_addr) + USPACE_UNDEF_STACK_BOTTOM, 0xdd,
	    (USPACE_UNDEF_STACK_TOP - USPACE_UNDEF_STACK_BOTTOM));
	/* Fill the kernel stack with a known pattern */
	memset(((u_char *)p2->p_addr) + USPACE_SVC_STACK_BOTTOM, 0xdd,
	    (USPACE_SVC_STACK_TOP - USPACE_SVC_STACK_BOTTOM));
#endif	/* STACKCHECKS */

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0) {
		printf("p1->procaddr=%p p1->procaddr->u_pcb=%p pid=%d pmap=%p\n",
		    p1->p_addr, &p1->p_addr->u_pcb, p1->p_pid,
		    p1->p_vmspace->vm_map.pmap);
		printf("p2->procaddr=%p p2->procaddr->u_pcb=%p pid=%d pmap=%p\n",
		    p2->p_addr, &p2->p_addr->u_pcb, p2->p_pid,
		    p2->p_vmspace->vm_map.pmap);
	}
#endif	/* PMAP_DEBUG */

	pmap_activate(p2);

#ifdef ARMFPE
	/* Initialise a new FP context for p2 and copy the context from p1 */
	arm_fpe_core_initcontext(FP_CONTEXT(p2));
	arm_fpe_copycontext(FP_CONTEXT(p1), FP_CONTEXT(p2));
#endif	/* ARMFPE */

	p2->p_md.md_regs = tf = (struct trapframe *)pcb->pcb_sp - 1;
	*tf = *p1->p_md.md_regs;

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_usr_sp = (u_int)stack + stacksize;

	sf = (struct switchframe *)tf - 1;
	sf->sf_spl = _SPL_0;
	sf->sf_r4 = (u_int)child_return;
	sf->sf_r5 = (u_int)p2;
	sf->sf_pc = (u_int)proc_trampoline;
	pcb->pcb_sp = (u_int)sf;
}


void
cpu_set_kpc(p, pc, arg)
	struct proc *p;
	void (*pc) __P((void *));
	void *arg;
{
	struct switchframe *sf = (struct switchframe *)p->p_addr->u_pcb.pcb_sp;

	sf->sf_r4 = (u_int)pc;
	sf->sf_r5 = (u_int)arg;
}


/*
 * cpu_exit is called as the last action during exit.
 *
 * We clean up a little and then call switch_exit() with the old proc as an
 * argument.  switch_exit() first switches to proc0's context, and finally
 * jumps into switch() to wait for another process to wake up.
 */

void
cpu_exit(p)
	register struct proc *p;
{
#ifdef ARMFPE
	/* Abort any active FP operation and deactivate the context */
	arm_fpe_core_abort(FP_CONTEXT(p), NULL, NULL);
	arm_fpe_core_changecontext(0);
#endif	/* ARMFPE */

#ifdef STACKCHECKS
	/* Report how much stack has been used - debugging */
	if (p) {
		u_char *ptr;
		int loop;

		ptr = ((u_char *)p2->p_addr) + USPACE_UNDEF_STACK_BOTTOM;
		for (loop = 0; loop < (USPACE_UNDEF_STACK_TOP - USPACE_UNDEF_STACK_BOTTOM)
		    && *ptr == 0xdd; ++loop, ++ptr) ;
		log(LOG_INFO, "%d bytes of undefined stack fill pattern\n", loop);
		ptr = ((u_char *)p2->p_addr) + USPACE_SVC_STACK_BOTTOM;
		for (loop = 0; loop < (USPACE_SVC_STACK_TOP - USPACE_SVC_STACK_BOTTOM)
		    && *ptr == 0xdd; ++loop, ++ptr) ;
		log(LOG_INFO, "%d bytes of svc stack fill pattern\n", loop);
	}
#endif	/* STACKCHECKS */
	uvmexp.swtch++;
	switch_exit(p, &proc0);
}


void
cpu_swapin(p)
	struct proc *p;
{

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("cpu_swapin(%p, %d, %s, %p)\n", p, p->p_pid,
		    p->p_comm, p->p_vmspace->vm_map.pmap);
#endif	/* PMAP_DEBUG */

	/* Map the system page */
	pmap_enter(p->p_vmspace->vm_map.pmap, 0x00000000, systempage.pv_pa,
	    VM_PROT_READ, TRUE, VM_PROT_READ);
}


void
cpu_swapout(p)
	struct proc *p;
{

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("cpu_swapout(%p, %d, %s, %p)\n", p, p->p_pid,
		    p->p_comm, &p->p_vmspace->vm_map.pmap);
#endif	/* PMAP_DEBUG */

	/* Free the system page mapping */
	pmap_remove(p->p_vmspace->vm_map.pmap, 0x00000000, 0x00000000 + NBPG);
}


/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */

void
pagemove(from, to, size)
	caddr_t from, to;
	size_t size;
{
	register pt_entry_t *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove: size=%08x", size);

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("pagemove: V%p to %p size %08x\n", from, to, size);
#endif	/* PMAP_DEBUG */

	fpte = vtopte(from);
	tpte = vtopte(to);

	/*
	 * Make sure the cache does not have dirty data for the
	 * pages we are moving. Pages in the buffers are only
	 * ever moved with pagemove, so we only need to clean
	 * the 'from' area.
	 */

	cpu_cache_purgeD_rng((u_int)from, size);

	while (size > 0) {
		*tpte++ = *fpte;
		*fpte++ = 0;
		size -= NBPG;
	}
	cpu_tlb_flushD();
}

extern vm_map_t phys_map;

/*
 * Map an IO request into kernel virtual address space.  Requests fall into
 * one of five catagories:
 *
 *	B_PHYS|B_UAREA:	User u-area swap.
 *			Address is relative to start of u-area (p_addr).
 *	B_PHYS|B_PAGET:	User page table swap.
 *			Address is a kernel VA in usrpt (Usrptmap).
 *	B_PHYS|B_DIRTY:	Dirty page push.
 *			Address is a VA in proc2's address space.
 *	B_PHYS|B_PGIN:	Kernel pagein of user pages.
 *			Address is VA in user's address space.
 *	B_PHYS:		User "raw" IO request.
 *			Address is VA in user's address space.
 *
 * All requests are (re)mapped into kernel VA space via the useriomap
 * (a name with only slightly more meaning than "kernelmap")
 */

void
vmapbuf(bp, len)
	struct buf *bp;
	vm_size_t len;
{
	vm_offset_t faddr, taddr, off;
	pt_entry_t *fpte, *tpte;
	pt_entry_t *pmap_pte __P((pmap_t, vm_offset_t));
	int pages;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("vmapbuf: bp=%08x buf=%08x len=%08x\n", (u_int)bp,
		    (u_int)bp->b_data, (u_int)len);
#endif	/* PMAP_DEBUG */
    
	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	taddr = uvm_km_valloc_wait(phys_map, len);

	faddr = trunc_page(bp->b_data);
	off = (vm_offset_t)bp->b_data - faddr;
	len = round_page(off + len);
	bp->b_saveaddr = bp->b_data;
	bp->b_data = (caddr_t)(taddr + off);

	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	fpte = pmap_pte(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map), faddr);
	tpte = pmap_pte(vm_map_pmap(phys_map), taddr);

	/*
	 * Make sure the cache does not have dirty data for the
	 * pages we are replacing
	 */
	if (len <= 0x1000) {
		cpu_cache_purgeID_rng(faddr, len);
		cpu_cache_purgeID_rng(taddr, len);
	} else 
		cpu_cache_purgeID();

	for (pages = len >> PAGE_SHIFT; pages; pages--)
		*tpte++ = *fpte++;

	if (len <= 0x1000)
		cpu_tlb_flushID_SE(taddr);
	else
		cpu_tlb_flushID();
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */

void
vunmapbuf(bp, len)
	struct buf *bp;
	vm_size_t len;
{
	vm_offset_t addr, off;
	pt_entry_t *pte;
	int pages;

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
	addr = trunc_page(bp->b_data);
	off = (vm_offset_t)bp->b_data - addr;
	len = round_page(off + len);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;

	pte = pmap_pte(vm_map_pmap(phys_map), addr);

	if (len <= 0x2000)
		cpu_cache_purgeID_rng(addr, len);
	else
		cpu_cache_purgeID();

	for (pages = len >> PAGE_SHIFT; pages; pages--)
		*pte++ = 0;

	if (len <= 0x1000)
		cpu_tlb_flushID_SE(addr);
	else
		cpu_tlb_flushID();

	uvm_km_free_wakeup(phys_map, addr, len);
}

/*
 * Dump the machine specific segment at the start of a core dump.
 */

int
cpu_coredump(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core *chdr;
{
	int error;
	struct {
	  struct reg regs;
	  struct fpreg fpregs;
	} cpustate;
	struct coreseg cseg;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = sizeof(cpustate);

	/* Save integer registers. */
	error = process_read_regs(p, &cpustate.regs);
	if (error)
		return error;
	/* Save floating point registers. */
	error = process_read_fpregs(p, &cpustate.fpregs);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cpustate, sizeof(cpustate),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	chdr->c_nseg++;

	return error;
}

/* End of vm_machdep.c */
