/*	$NetBSD: vm_machdep.c,v 1.13.12.1 2018/04/16 01:59:55 pgoyette Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author: 
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

/* Comments on functions from alpha/vm_machdep.c */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <machine/frame.h>
#include <machine/md_var.h>
#include <machine/pcb.h>

#include <uvm/uvm_extern.h>

void lwp_trampoline(void);

void
cpu_lwp_free(struct lwp *l, int proc)
{

	/* XXX: Not yet. */
	(void)l;
	(void)proc;
}

void
cpu_lwp_free2(struct lwp *l)
{

	(void)l;
}

/*
 * The cpu_switchto() function saves the context of the LWP which is
 * currently running on the processor, and restores the context of the LWP
 * specified by newlwp.  man cpu_switchto(9)
 */
lwp_t *
cpu_switchto(lwp_t *oldlwp, lwp_t *newlwp, bool returning)
{
	const struct lwp *l = curlwp;
	struct pcb *oldpcb = oldlwp ? lwp_getpcb(oldlwp) : NULL;
	struct pcb *newpcb = lwp_getpcb(newlwp);
	struct cpu_info *ci = curcpu();
	register uint64_t reg9 __asm("r9");

	KASSERT(newlwp != NULL);
	
	ci->ci_curlwp = newlwp;
	
	/* required for lwp_startup, copy oldlwp into r9, "mov r9=in0" */
	__asm __volatile("mov %0=%1" : "=r"(reg9) : "r"(oldlwp));
	
	/* XXX handle RAS eventually */
	
	if (oldlwp == NULL) {
		restorectx(newpcb);
	} else {
		KASSERT(oldlwp == l);
		swapctx(oldpcb, newpcb);
	}
	
	return (oldlwp);
}

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with p2 as an
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
	struct pcb *pcb1, *pcb2;
	struct trapframe *tf;
	
	pcb1 = lwp_getpcb(l1);
	pcb2 = lwp_getpcb(l2);

	/* Copy pcb from lwp l1 to l2. */
	if (l1 == curlwp) {
		/* Sync the PCB before we copy it. */
		if (savectx(pcb1) != 0)
			panic("unexpected return from savectx");
		/* ia64_highfp_save(td1); XXX */
	} else {
		KASSERT(l1 == &lwp0);
	}

	/*
	 * XXX this seems incomplete, each thread apparently needs its
	 * own stack and bspstore, and to re-adjust the RSE "ndirty"
	 * registers.  See
	 * http://fxr.watson.org/fxr/source/ia64/ia64/vm_machdep.c?v=FREEBSD10#L262
	 * Also should verify u-area usage is consistent, which may be
	 * different than freebsd.
	 */
	*pcb2 = *pcb1;

	l2->l_md.md_flags = l1->l_md.md_flags;
	l2->l_md.md_tf = (struct trapframe *)(uvm_lwp_getuarea(l2) + USPACE) - 1;
	l2->l_md.md_astpending = 0;

        /*
	 * Copy the trapframe.
	 */
	tf = l2->l_md.md_tf;
	*tf = *l1->l_md.md_tf;

        /*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		tf->tf_special.sp = (unsigned long)stack + stacksize;

	/* Set-up the return values as expected by the fork() libc stub. */
	if (tf->tf_special.psr & IA64_PSR_IS) {
		tf->tf_scratch.gr8 = 0;
		tf->tf_scratch.gr10 = 1;
	} else {
		tf->tf_scratch.gr8 = 0;
		tf->tf_scratch.gr9 = 1;
		tf->tf_scratch.gr10 = 0;
	}

	tf->tf_scratch.gr2 = (unsigned long)FDESC_FUNC(func);
	tf->tf_scratch.gr3 = (unsigned long)arg;
	pcb2->pcb_special.sp = (unsigned long)tf - 16;
	pcb2->pcb_special.rp = (unsigned long)FDESC_FUNC(lwp_trampoline);
	pcb2->pcb_special.pfs = 0;

	return;
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
int
vmapbuf(struct buf *bp, vsize_t len)
{
printf("%s: not yet\n", __func__);
	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
printf("%s: not yet\n", __func__);
	return;
}
