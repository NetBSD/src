/* $NetBSD: vm_machdep.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/cpu.h>
#include <sys/buf.h>
#include <sys/pmc.h>
#include <sys/exec.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <aarch64/pcb.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>

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
	const struct pcb * const pcb1 = lwp_getpcb(l1);
	struct pcb * const pcb2 = lwp_getpcb(l2);

#if 0
	printf("cpu_lwp_fork: lwp1=%p, lwp2=%p, curlwp=%p, lwp0=%p\n",
	    l1, l2, curlwp, &lwp0);
#endif

	/* Copy the pcb */
	*pcb2 = *pcb1;

	/*
	 * Disable FP for a newly created LWP but remember if the
	 * FP state is valid.
	 */
	l2->l_md.md_cpacr = CPACR_FPEN_NONE;

	/*
	 * Set up the kernel stack for the process.
	 * Note: this stack is not in use if we are forking from p1
	 */
	vaddr_t uv = uvm_lwp_getuarea(l2);

#ifdef STACKCHECKS
#define PCB_END(l)	((char *)lwp_getpcb((l)) + sizeof(struct pcb))
#define UAREA_END(l)	((char *)uvm_lwp_getuarea((l)) + USPACE)
	/* fill 0xdd for STACKCHECKS */
	memset(PCB_END(l2), 0xdd, UAREA_END(l2) - PCB_END(l2));
	printf("lwp %p: pcb=%p, stack=%p-%p\n", l2, lwp_getpcb(l2),
	    PCB_END(l2), UAREA_END(l2));
#endif

	struct trapframe * const utf = (struct trapframe *)(uv + USPACE) - 1;
	l2->l_md.md_utf = utf;

	*utf = *l1->l_md.md_utf;

	/*
	 * If specified, give the child a different stack
	 * (make sure it's 16-byte aligned).
	 */
	if (stack != NULL)
		utf->tf_sp = ((vaddr_t)(stack) + stacksize) & -16;

	/* build a new switchframe */
	struct trapframe * const ktf = utf - 1;
	ktf->tf_reg[27] = func;
	ktf->tf_reg[28] = arg;
	ktf->tf_reg[29] = 0;
	KASSERT(reg_daif_read() == 0);
	ktf->tf_lr = (uintptr_t)lwp_trampoline;

	l2->l_md.md_ktf = ktf;
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
#ifdef STACKCHECKS
	/* Report how much stack has been used - debugging */
	u_char *stop, *sbottom, *ptr;
	u_int cnt;

	stop = PCB_END(l);
	sbottom = UAREA_END(l);
	for (cnt = 0, ptr = stop; *ptr == 0xdd && ptr <= sbottom; cnt++, ptr++)
		;
	log(LOG_INFO, "lwp %p: %u/%ld bytes are used for EL1 stack\n",
	    l, cnt, sbottom - stop);
#endif
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


	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	bp->b_saveaddr = bp->b_data;
	faddr = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_alloc(phys_map, len, atop(faddr) & uvmexp.colormask,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA | UVM_KMF_COLORMATCH);
	bp->b_data = (void *)(taddr + off);

	/*
	 * The region is locked, so we expect that pmap_pte() will return
	 * non-NULL.
	 */
	while (len) {
		(void)pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    faddr, &fpa);
		pmap_enter(pmap_kernel(), taddr, fpa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
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
