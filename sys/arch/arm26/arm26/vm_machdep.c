/* $NetBSD: vm_machdep.c,v 1.3 2000/05/28 05:49:00 thorpej Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Following is for vmapbuf/vunmapbuf */
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <sys/param.h>

__RCSID("$NetBSD: vm_machdep.c,v 1.3 2000/05/28 05:49:00 thorpej Exp $");

#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/mount.h> /* XXX syscallargs.h uses fhandle_t and fsid_t */
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/armreg.h>
#include <machine/frame.h>
#include <machine/machdep.h>

extern vm_map_t phys_map; /* XXX where? */

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */

/*
 * Note:
 * 
 * p->p_addr points to a page containing the user structure
 * (see <sys/user.h>) and the kernel stack.  The user structure has to be
 * at the start of the area -- we start the kernel stack from the end.
 */

void
cpu_fork(struct proc *p1, struct proc *p2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct trapframe *tf;
	struct switchframe *sf;
	char *stacktop;

#if 0
	printf("cpu_fork: %p -> %p\n", p1, p2);
#endif
	pcb = &p2->p_addr->u_pcb;
	/* Copy the pcb */
	*pcb = p1->p_addr->u_pcb;

	/* pmap_activate(p2); XXX Other ports do.  Why?  */

	/* Set up the kernel stack */
	stacktop = (char *)p2->p_addr + USPACE;
	tf = (struct trapframe *)stacktop - 1;
	sf = (struct switchframe *)tf - 1;
	/* Duplicate old process's trapframe (if it had one) */
	if (p1->p_addr->u_pcb.pcb_tf == NULL)
		bzero(tf, sizeof(*tf));
	else
		*tf = *p1->p_addr->u_pcb.pcb_tf;
	p2->p_addr->u_pcb.pcb_tf = tf;
	/* Fabricate a new switchframe */
	bzero(sf, sizeof(*sf));
	sf->sf_r13 = (register_t)tf; /* Initial stack pointer */
	sf->sf_r14 = (register_t)proc_trampoline | R15_MODE_SVC;

	pcb->pcb_tf = tf;
	pcb->pcb_sf = sf;
	pcb->pcb_onfault = NULL;
	sf->sf_r4 = (register_t)func;
	sf->sf_r5 = (register_t)arg;
}

/*
 * Set up the registers for a newly-execked image.
 */
void
setregs(struct proc *p, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf;

	tf = p->p_addr->u_pcb.pcb_tf;
	bzero(tf, sizeof(*tf));
	tf->tf_r0 = (register_t)PS_STRINGS;
	tf->tf_r13 = stack; /* sp */
	tf->tf_r15 = pack->ep_entry;
}

void
cpu_exit(struct proc *p)
{

	/* Nothing to do here? */
	exit2(p); /* I think this is safe on a uniprocessor machine */
	cpu_switch(p);
}

/* ARGSUSED */
void
cpu_wait(struct proc *p)
{

	/* Nothing left for us to free... */
}


/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored in u. to call routine,
 * followed by kcall to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the frame pointer, it
 * returns to the user specified pc.
 */
void
sendsig(sig_t catcher, int sig, sigset_t *mask, u_long code)
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int onstack;

	tf = p->p_addr->u_pcb.pcb_tf;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)psp->ps_sigstk.ss_sp +
						  psp->ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_r13;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
	frame.sf_sc.sc_r0  = tf->tf_r0;
	frame.sf_sc.sc_r1  = tf->tf_r1;
	frame.sf_sc.sc_r2  = tf->tf_r2;
	frame.sf_sc.sc_r3  = tf->tf_r3;
	frame.sf_sc.sc_r4  = tf->tf_r4;
	frame.sf_sc.sc_r5  = tf->tf_r5;
	frame.sf_sc.sc_r6  = tf->tf_r6;
	frame.sf_sc.sc_r7  = tf->tf_r7;
	frame.sf_sc.sc_r8  = tf->tf_r8;
	frame.sf_sc.sc_r9  = tf->tf_r9;
	frame.sf_sc.sc_r10 = tf->tf_r10;
	frame.sf_sc.sc_r11 = tf->tf_r11;
	frame.sf_sc.sc_r12 = tf->tf_r12;
	frame.sf_sc.sc_r13 = tf->tf_r13;
	frame.sf_sc.sc_r14 = tf->tf_r14;
	frame.sf_sc.sc_r15 = tf->tf_r15;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_r0 = frame.sf_signum;
	tf->tf_r1 = frame.sf_code;
	tf->tf_r2 = (int)frame.sf_scp;
	tf->tf_r3 = (int)frame.sf_handler;
	tf->tf_r13 = (int)fp;
	tf->tf_r15 = (int)psp->ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal has been taken.  Reset
 * signal mask and stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by context left by
 * sendsig. Check carefully to make sure that the user has not
 * modified the psr to gain improper privileges or to cause a machine
 * fault.
 */
int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return EFAULT;

	/* Make sure the processor mode has not been tampered with. */
	if ((context.sc_r15 & R15_MODE) != R15_MODE_USR ||
	    (context.sc_r15 & (R15_IRQ_DISABLE | R15_FIQ_DISABLE)) != 0)
		return EINVAL;

	/* Restore register context. */
	tf = p->p_addr->u_pcb.pcb_tf;
	tf->tf_r0  = context.sc_r0;
	tf->tf_r1  = context.sc_r1;
	tf->tf_r2  = context.sc_r2;
	tf->tf_r3  = context.sc_r3;
	tf->tf_r4  = context.sc_r4;
	tf->tf_r5  = context.sc_r5;
	tf->tf_r6  = context.sc_r6;
	tf->tf_r7  = context.sc_r7;
	tf->tf_r8  = context.sc_r8;
	tf->tf_r9  = context.sc_r9;
	tf->tf_r10 = context.sc_r10;
	tf->tf_r11 = context.sc_r11;
	tf->tf_r12 = context.sc_r12;
	tf->tf_r13 = context.sc_r13;
	tf->tf_r14 = context.sc_r14;
	tf->tf_r15 = context.sc_r15;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return EJUSTRETURN;
}

void
cpu_swapin(struct proc *p)
{

	/* Can anyone think of anything I should do here? */
}

void
cpu_swapout(struct proc *p)
{

	/* ... or here, for that matter. */
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
/* This code was originally stolen from the alpha port. */
/*
 * This needs some care, since the user mapping of the buffer is (sometimes?)
 * wired, so we have to unwire it, and put it all back when we've finished.
 */
void
vmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vmapbuf");
	p = bp->b_proc;
	faddr = trunc_page((vaddr_t)bp->b_saveaddr = bp->b_data);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	taddr = uvm_km_valloc_wait(phys_map, len);
	bp->b_data = (caddr_t)(taddr + off);
	len = atop(len);
	while (len--) {
		if (pmap_extract(vm_map_pmap(&p->p_vmspace->vm_map), faddr,
		    &pa) == FALSE)
			panic("vmapbuf: null page frame");
		/* XXX is this allowed? */
		pmap_unwire(vm_map_pmap(&p->p_vmspace->vm_map), faddr);
		pmap_enter(vm_map_pmap(phys_map), taddr, trunc_page(pa),
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
}

/*
 * Unmap a previously-mapped user I/O request.  Put it back in user memory.
 */
void
vunmapbuf(bp, len)
	struct buf *bp;
	vsize_t len;
{
	vaddr_t faddr, taddr, off;
	paddr_t pa;
	struct proc *p;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	p = bp->b_proc;
	faddr = trunc_page((vaddr_t)bp->b_data);
	taddr = trunc_page((vaddr_t)bp->b_saveaddr);
	off = (vaddr_t)bp->b_data - faddr;
	len = round_page(off + len);
	/* XXX Re-map buffer into user space? */
	len = atop(len);
	while (len--) {
		if (pmap_extract(vm_map_pmap(phys_map), faddr, &pa) == FALSE)
			panic("vmapbuf: null page frame");
		pmap_unwire(vm_map_pmap(phys_map), faddr);
		pmap_enter(vm_map_pmap(&p->p_vmspace->vm_map), taddr,
		    trunc_page(pa), VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		faddr += PAGE_SIZE;
		taddr += PAGE_SIZE;
	}
	uvm_km_free_wakeup(phys_map, faddr, ptoa(len));
	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
