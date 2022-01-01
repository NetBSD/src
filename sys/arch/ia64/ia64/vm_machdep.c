/*	$NetBSD: vm_machdep.c,v 1.17 2022/01/01 21:07:14 andvar Exp $	*/

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

	/* return oldlwp for the original thread that called cpu_switchto */
	return ((lwp_t *)reg9);
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
	vaddr_t ua1 = uvm_lwp_getuarea(l1);
	vaddr_t ua2 = uvm_lwp_getuarea(l2);
	struct pcb *pcb1 = lwp_getpcb(l1);
	struct pcb *pcb2 = lwp_getpcb(l2);

	struct trapframe *tf;
	uint64_t ndirty;

	/*
	 * Save the preserved registers and the high FP registers in the
	 * PCB if we're the parent (ie l1 == curlwp) so that we have
	 * a valid PCB. This also causes a RSE flush. We don't have to
	 * do that otherwise, because there wouldn't be anything important
	 * to save.
	 *
	 * Copy pcb from lwp l1 to l2.
	 */
	if (l1 == curlwp) {
		/* Sync the PCB before we copy it. */
		if (savectx(pcb1) != 0)
			panic("unexpected return from savectx()");
		/* ia64_highfp_save(td1); XXX */
	} else {
		KASSERT(l1 == &lwp0);
	}

	/*
	 * create the child's kernel stack and backing store. We basically
	 * create an image of the parent's stack and backing store and
	 * adjust where necessary.
	 */
	*pcb2 = *pcb1;

	l2->l_md.md_flags = l1->l_md.md_flags;
	l2->l_md.md_tf = (struct trapframe *)(ua2 + UAREA_TF_OFFSET);
	l2->l_md.md_astpending = 0;
	l2->l_md.user_stack = NULL;
	l2->l_md.user_stack_size = 0;
	
        /*
	 * Copy the trapframe.
	 */
	tf = l2->l_md.md_tf;
	*tf = *l1->l_md.md_tf;

	/* XXX need something like this, but still not correct */
	ndirty = tf->tf_special.ndirty + (tf->tf_special.bspstore & 0x1ffUL);
	memcpy((void *)(ua2 + UAREA_BSPSTORE_OFFSET),
	       (void *)(ua1 + UAREA_BSPSTORE_OFFSET), ndirty);

        /*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL) {
		l2->l_md.user_stack = stack;
		l2->l_md.user_stack_size = stacksize;
		tf->tf_special.sp = (unsigned long)stack + UAREA_SP_OFFSET;
		tf->tf_special.bspstore = (unsigned long)stack + UAREA_BSPSTORE_OFFSET;

		memcpy(stack, (void *)(ua1 + UAREA_BSPSTORE_OFFSET), ndirty);
	}

	/* Set-up the return values as expected by the fork() libc stub. */
	if (tf->tf_special.psr & IA64_PSR_IS) {
		tf->tf_scratch.gr8 = 0;
		tf->tf_scratch.gr10 = 1;
	} else {
		tf->tf_scratch.gr8 = 0;
		tf->tf_scratch.gr9 = 1;
		tf->tf_scratch.gr10 = 0;
	}

	pcb2->pcb_special.bspstore = ua2 + UAREA_BSPSTORE_OFFSET + ndirty;
	pcb2->pcb_special.pfs = 0;
	pcb2->pcb_special.sp = ua2 + UAREA_SP_OFFSET;
	pcb2->pcb_special.rp = (unsigned long)FDESC_FUNC(lwp_trampoline);
	tf->tf_scratch.gr2 = (unsigned long)FDESC_FUNC(func);
	tf->tf_scratch.gr3 = (unsigned long)arg;

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
	panic("XXX %s implement", __func__);
	return 0;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	panic("XXX %s implement", __func__);
	return;
}
