/*	$NetBSD: vm_machdep.c,v 1.100.6.1 2015/12/27 12:09:44 skrll Exp $ */

/*
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)vm_machdep.c	8.2 (Berkeley) 9/23/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.100.6.1 2015/12/27 12:09:44 skrll Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/core.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <sys/bus.h>

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().   
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *upmap;
	vaddr_t uva;	/* User VA (map from) */
	vaddr_t kva;	/* Kernel VA (new to) */
	paddr_t pa; 	/* physical address */
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	bp->b_saveaddr = bp->b_data;
	uva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	len = round_page(off + len);
	kva = uvm_km_alloc(kernel_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(kva + off);

	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	do {
		if (pmap_extract(upmap, uva, &pa) == FALSE)
			panic("vmapbuf: null page frame");
		/* Now map the page into kernel space. */
		pmap_kenter_pa(kva, pa, VM_PROT_READ | VM_PROT_WRITE, 0);

		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(pmap_kernel());

	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	vaddr_t kva;
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");

	kva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - kva;
	len = round_page(off + len);
	pmap_kremove(kva, len);
	uvm_km_free(kernel_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}

void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{

	p2->p_md.md_flags = p1->p_md.md_flags;
}


/*
 * The offset of the topmost frame in the kernel stack.
 */
#ifdef __arch64__
#define	TOPFRAMEOFF (USPACE-sizeof(struct trapframe)-CC64FSZ)
#define	STACK_OFFSET	BIAS
#else
#undef	trapframe
#define	trapframe	trapframe64
#undef	rwindow
#define	rwindow		rwindow32
#define	TOPFRAMEOFF (USPACE-sizeof(struct trapframe)-CC64FSZ)
#define	STACK_OFFSET	0
#endif

#ifdef DEBUG
char cpu_forkname[] = "cpu_lwp_fork()";
#endif

/*
 * Finish a fork operation, with lwp l2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with l2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * l1 is the process being forked; if l1 == &lwp0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void lwp_trampoline(void);
void
cpu_lwp_fork(register struct lwp *l1, register struct lwp *l2, void *stack, size_t stacksize, void (*func)(void *), void *arg)
{
	struct pcb *opcb = lwp_getpcb(l1);
	struct pcb *npcb = lwp_getpcb(l2);
	struct trapframe *tf2;
	struct rwindow *rp;

	/*
	 * Save all user registers to l1's stack or, in the case of
	 * user registers and invalid stack pointers, to opcb.
	 * We then copy the whole pcb to l2; when switch() selects l2
	 * to run, it will run at the `lwp_trampoline' stub, rather
	 * than returning at the copying code below.
	 *
	 * If process l1 has an FPU state, we must copy it.  If it is
	 * the FPU user, we must save the FPU state first.
	 */

#ifdef NOTDEF_DEBUG
	printf("cpu_lwp_fork()\n");
#endif
	if (l1 == curlwp) {
		write_user_windows();

		/*
		 * We're in the kernel, so we don't really care about
		 * %ccr or %asi.  We do want to duplicate %pstate and %cwp.
		 */
		opcb->pcb_pstate = getpstate();
		opcb->pcb_cwp = getcwp();
	}
#ifdef DIAGNOSTIC
	else if (l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif
#ifdef DEBUG
	/* prevent us from having NULL lastcall */
	opcb->lastcall = cpu_forkname;
#else
	opcb->lastcall = NULL;
#endif
	memcpy(npcb, opcb, sizeof(struct pcb));
       	if (l1->l_md.md_fpstate) {
       		fpusave_lwp(l1, true);
		l2->l_md.md_fpstate = pool_cache_get(fpstate_cache, PR_WAITOK);
		memcpy(l2->l_md.md_fpstate, l1->l_md.md_fpstate,
		    sizeof(struct fpstate64));
	} else
		l2->l_md.md_fpstate = NULL;

	/*
	 * Setup (kernel) stack frame that will by-pass the child
	 * out of the kernel. (The trap frame invariably resides at
	 * the tippity-top of the u. area.)
	 */
	tf2 = l2->l_md.md_tf = (struct trapframe *)
			((long)npcb + USPACE - sizeof(*tf2));

	/* Copy parent's trapframe */
	*tf2 = *(struct trapframe *)((long)opcb + USPACE - sizeof(*tf2));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf2->tf_out[6] = (uint64_t)(u_long)stack + stacksize;

	/*
	 * Need to create a %tstate if we are forking our first userland
	 * process - in all other cases we inherit from the parent.
	 */
	if (l2->l_proc->p_pid == 1)
		tf2->tf_tstate = (ASI_PRIMARY_NO_FAULT<<TSTATE_ASI_SHIFT) |
		    ((PSTATE_USER)<<TSTATE_PSTATE_SHIFT);

	/*
	 * Set return values in child mode and clear condition code,
	 * in case we end up running a signal handler before returning
	 * to userland.
	 */
	tf2->tf_out[0] = 0;
	tf2->tf_out[1] = 1;
	tf2->tf_tstate &= ~TSTATE_CCR;

	/* Construct kernel frame to return to in cpu_switch() */
	rp = (struct rwindow *)((u_long)npcb + TOPFRAMEOFF);
	*rp = *(struct rwindow *)((u_long)opcb + TOPFRAMEOFF);

	rp->rw_local[0] = (long)func;	/* Function to call */
	rp->rw_local[1] = (long)arg;	/* and its argument */
	rp->rw_local[2] = (long)l2;	/* new lwp */

	npcb->pcb_pc = (long)lwp_trampoline - 8;
	npcb->pcb_sp = (long)rp - STACK_OFFSET;
}

static inline void
fpusave_cpu(bool save)
{
	struct lwp *l = fplwp;

	if (l == NULL)
		return;

	if (save)
		savefpstate(l->l_md.md_fpstate);
	else
		clearfpstate();

	fplwp = NULL;
}

void
fpusave_lwp(struct lwp *l, bool save)
{
#ifdef MULTIPROCESSOR
	volatile struct cpu_info *ci;

	if (l == fplwp) {
		int s = intr_disable();
		fpusave_cpu(save);
		intr_restore(s);
		return;
	}

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		int spincount;

		if (ci == curcpu() || !CPUSET_HAS(cpus_active, ci->ci_index))
			continue;
		if (ci->ci_fplwp != l)
			continue;
		sparc64_send_ipi(ci->ci_cpuid, save ?
				 sparc64_ipi_save_fpstate :
				 sparc64_ipi_drop_fpstate, (uintptr_t)l, 0);

		spincount = 0;
		while (ci->ci_fplwp == l) {
			membar_Sync();
			spincount++;
			if (spincount > 10000000)
				panic("fpusave_lwp ipi didn't");
		}
		break;
	}
#else
	if (l == fplwp)
		fpusave_cpu(save);
#endif
}


void
cpu_lwp_free(struct lwp *l, int proc)
{

	if (l->l_md.md_fpstate != NULL)
		fpusave_lwp(l, false);
}

void
cpu_lwp_free2(struct lwp *l)
{
	struct fpstate64 *fs;

	if ((fs = l->l_md.md_fpstate) != NULL)
		pool_cache_put(fpstate_cache, fs);
}

int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{
	struct trapframe *tf = l->l_md.md_tf;

	tf->tf_global[7] = (uintptr_t)addr;

	return 0;
}
