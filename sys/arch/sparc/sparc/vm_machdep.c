/*	$NetBSD: vm_machdep.c,v 1.83.2.5 2008/01/21 09:39:28 yamt Exp $ */

/*
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
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.83.2.5 2008/01/21 09:39:28 yamt Exp $");

#include "opt_multiprocessor.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/simplelock.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/trap.h>

#include <sparc/sparc/cpuvar.h>

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	struct pmap *upmap, *kpmap;
	vaddr_t uva;	/* User VA (map from) */
	vaddr_t kva;	/* Kernel VA (new to) */
	paddr_t pa; 	/* physical address */
	vsize_t off;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");

	/*
	 * XXX:  It might be better to round/trunc to a
	 * segment boundary to avoid VAC problems!
	 */
	bp->b_saveaddr = bp->b_data;
	uva = trunc_page((vaddr_t)bp->b_data);
	off = (vaddr_t)bp->b_data - uva;
	len = round_page(off + len);
	kva = uvm_km_alloc(kernel_map, len, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	bp->b_data = (void *)(kva + off);

	/*
	 * We have to flush any write-back cache on the
	 * user-space mappings so our new mappings will
	 * have the correct contents.
	 */
	if (CACHEINFO.c_vactype != VAC_NONE)
		cache_flush((void *)uva, len);

	upmap = vm_map_pmap(&bp->b_proc->p_vmspace->vm_map);
	kpmap = vm_map_pmap(kernel_map);
	do {
		if (pmap_extract(upmap, uva, &pa) == false)
			panic("vmapbuf: null page frame");
		/* Now map the page into kernel space. */
		pmap_enter(kpmap, kva, pa,
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		uva += PAGE_SIZE;
		kva += PAGE_SIZE;
		len -= PAGE_SIZE;
	} while (len);
	pmap_update(kpmap);
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
	pmap_remove(vm_map_pmap(kernel_map), kva, kva + len);
	pmap_update(vm_map_pmap(kernel_map));
	uvm_km_free(kernel_map, kva, len, UVM_KMF_VAONLY);
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;

#if 0	/* XXX: The flush above is sufficient, right? */
	if (CACHEINFO.c_vactype != VAC_NONE)
		cpuinfo.cache_flush(bp->b_data, len);
#endif
}


void
cpu_proc_fork(struct proc *p1, struct proc *p2)
{

	p2->p_md.md_flags = p1->p_md.md_flags;
}


/*
 * The offset of the topmost frame in the kernel stack.
 */
#define	TOPFRAMEOFF (USPACE-sizeof(struct trapframe)-sizeof(struct frame))

/*
 * Finish a fork operation, with process l2 nearly set up.
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
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2,
	     void *stack, size_t stacksize,
	     void (*func)(void *), void *arg)
{
	struct pcb *opcb = &l1->l_addr->u_pcb;
	struct pcb *npcb = &l2->l_addr->u_pcb;
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

	if (l1 == curlwp) {
		write_user_windows();
		opcb->pcb_psr = getpsr();
	}

	bcopy((void *)opcb, (void *)npcb, sizeof(struct pcb));
	if (l1->l_md.md_fpstate != NULL) {
		struct cpu_info *cpi;
		int s;

		l2->l_md.md_fpstate = malloc(sizeof(struct fpstate),
		    M_SUBPROC, M_WAITOK);

		FPU_LOCK(s);
		if ((cpi = l1->l_md.md_fpu) != NULL) {
			if (cpi->fplwp != l1)
				panic("FPU(%d): fplwp %p",
					cpi->ci_cpuid, cpi->fplwp);
			if (l1 == cpuinfo.fplwp)
				savefpstate(l1->l_md.md_fpstate);
#if defined(MULTIPROCESSOR)
			else
				XCALL1(savefpstate, l1->l_md.md_fpstate,
					1 << cpi->ci_cpuid);
#endif
		}
		bcopy(l1->l_md.md_fpstate, l2->l_md.md_fpstate,
		    sizeof(struct fpstate));
		FPU_UNLOCK(s);
	} else
		l2->l_md.md_fpstate = NULL;

	l2->l_md.md_fpu = NULL;

	/*
	 * Setup (kernel) stack frame that will by-pass the child
	 * out of the kernel. (The trap frame invariably resides at
	 * the tippity-top of the u. area.)
	 */
	tf2 = l2->l_md.md_tf = (struct trapframe *)
			((int)npcb + USPACE - sizeof(*tf2));

	/* Copy parent's trapframe */
	*tf2 = *(struct trapframe *)((int)opcb + USPACE - sizeof(*tf2));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf2->tf_out[6] = (u_int)stack + stacksize;

	/*
	 * The fork system call always uses the old system call
	 * convention; clear carry and skip trap instruction as
	 * in syscall().
	 * note: lwp_trampoline() sets a fresh psr when returning
	 * to user mode.
	 */
	/*tf2->tf_psr &= ~PSR_C;   -* success */
	tf2->tf_pc = tf2->tf_npc;
	tf2->tf_npc = tf2->tf_pc + 4;

	/* Set return values in child mode */
	tf2->tf_out[0] = 0;
	tf2->tf_out[1] = 1;

	/* Construct kernel frame to return to in cpu_switch() */
	rp = (struct rwindow *)((u_int)npcb + TOPFRAMEOFF);
	rp->rw_local[0] = (int)func;		/* Function to call */
	rp->rw_local[1] = (int)arg;		/* and its argument */
	rp->rw_local[2] = (int)l2;		/* the new LWP */

	npcb->pcb_pc = (int)lwp_trampoline - 8;
	npcb->pcb_sp = (int)rp;
	npcb->pcb_psr &= ~PSR_CWP;	/* Run in window #0 */
	npcb->pcb_wim = 1;		/* Fence at window #1 */
}

/*
 * Cleanup FPU state.
 */
void
cpu_lwp_free(struct lwp *l, int proc)
{
	struct fpstate *fs;

	if ((fs = l->l_md.md_fpstate) != NULL) {
		struct cpu_info *cpi;
		int s;

		FPU_LOCK(s);
		if ((cpi = l->l_md.md_fpu) != NULL) {
			if (cpi->fplwp != l)
				panic("FPU(%d): fplwp %p",
					cpi->ci_cpuid, cpi->fplwp);
			if (l == cpuinfo.fplwp)
				savefpstate(fs);
#if defined(MULTIPROCESSOR)
			else
				XCALL1(savefpstate, fs, 1 << cpi->ci_cpuid);
#endif
			cpi->fplwp = NULL;
		}
		l->l_md.md_fpu = NULL;
		FPU_UNLOCK(s);
	}
}

void
cpu_lwp_free2(struct lwp *l)
{
	struct fpstate *fs;

	if ((fs = l->l_md.md_fpstate) != NULL)
		free((void *)fs, M_SUBPROC);
}

#ifdef COREDUMP
/*
 * cpu_coredump is called to write a core dump header.
 * (should this be defined elsewhere?  machdep.c?)
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	int error;
	struct md_coredump md_core;
	struct coreseg cseg;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core);
		chdr->c_nseg++;
		return 0;
	}

	md_core.md_tf = *l->l_md.md_tf;
	if (l->l_md.md_fpstate) {
		if (l == cpuinfo.fplwp)
			savefpstate(l->l_md.md_fpstate);
		md_core.md_fpstate = *l->l_md.md_fpstate;
	} else
		bzero((void *)&md_core.md_fpstate, sizeof(struct fpstate));

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
