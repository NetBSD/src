/*	$NetBSD: sh3_machdep.c,v 1.13 2001/05/16 12:42:38 msaitoh Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/user.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

char cpu_model[120];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

extern int physmem;

void
sh3_startup()
{
	unsigned i;
	caddr_t v;
	int sz;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	struct pcb *pcb;
	char pbuf[9];

	printf(version);

	sprintf(cpu_model, "Hitachi SH3");

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffers = 0;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
					VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update();

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	curpcb = pcb = &proc0.p_addr->u_pcb;
	pcb->r15 = (int)proc0.p_addr + USPACE - 16;

	proc0.p_md.md_regs = (struct trapframe *)pcb->r15 - 1;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	int onstack;

	tf = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_r15;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
	frame.sf_sc.sc_ssr = tf->tf_ssr;
	frame.sf_sc.sc_spc = tf->tf_spc;
	frame.sf_sc.sc_pr = tf->tf_pr;
	frame.sf_sc.sc_r15 = tf->tf_r15;
	frame.sf_sc.sc_r14 = tf->tf_r14;
	frame.sf_sc.sc_r13 = tf->tf_r13;
	frame.sf_sc.sc_r12 = tf->tf_r12;
	frame.sf_sc.sc_r11 = tf->tf_r11;
	frame.sf_sc.sc_r10 = tf->tf_r10;
	frame.sf_sc.sc_r9 = tf->tf_r9;
	frame.sf_sc.sc_r8 = tf->tf_r8;
	frame.sf_sc.sc_r7 = tf->tf_r7;
	frame.sf_sc.sc_r6 = tf->tf_r6;
	frame.sf_sc.sc_r5 = tf->tf_r5;
	frame.sf_sc.sc_r4 = tf->tf_r4;
	frame.sf_sc.sc_r3 = tf->tf_r3;
	frame.sf_sc.sc_r2 = tf->tf_r2;
	frame.sf_sc.sc_r1 = tf->tf_r1;
	frame.sf_sc.sc_r0 = tf->tf_r0;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
#ifdef TODO
	frame.sf_sc.sc_err = tf->tf_err;
#endif

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

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
	tf->tf_spc = (int)p->p_sigctx.ps_sigcode;
#ifdef TODO
	tf->tf_ssr &= ~(PSL_T|PSL_VM|PSL_AC);
#endif
	tf->tf_r15 = (int)fp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
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
		return (EFAULT);

	/* Restore signal context. */
	tf = p->p_md.md_regs;

	/* Check for security violations. */
	if (((context.sc_ssr ^ tf->tf_ssr) & PSL_USERSTATIC) != 0)
		return (EINVAL);

	tf->tf_ssr = context.sc_ssr;

	tf->tf_r0 = context.sc_r0;
	tf->tf_r1 = context.sc_r1;
	tf->tf_r2 = context.sc_r2;
	tf->tf_r3 = context.sc_r3;
	tf->tf_r4 = context.sc_r4;
	tf->tf_r5 = context.sc_r5;
	tf->tf_r6 = context.sc_r6;
	tf->tf_r7 = context.sc_r7;
	tf->tf_r8 = context.sc_r8;
	tf->tf_r9 = context.sc_r9;
	tf->tf_r10 = context.sc_r10;
	tf->tf_r11 = context.sc_r11;
	tf->tf_r12 = context.sc_r12;
	tf->tf_r13 = context.sc_r13;
	tf->tf_r14 = context.sc_r14;
	tf->tf_spc = context.sc_spc;
	tf->tf_r15 = context.sc_r15;
	tf->tf_pr = context.sc_pr;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}

/*
 * Clear registers on exec
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	register struct pcb *pcb = &p->p_addr->u_pcb;
	register struct trapframe *tf;

	p->p_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;

	tf = p->p_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_r1 = 0;
	tf->tf_r2 = 0;
	tf->tf_r3 = 0;
	tf->tf_r4 = *(int *)stack;	/* argc */
	tf->tf_r5 = stack+4;		/* argv */
	tf->tf_r6 = stack+4*tf->tf_r4 + 8; /* envp */
	tf->tf_r7 = 0;
	tf->tf_r8 = 0;
	tf->tf_r9 = 0;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_spc = pack->ep_entry;
	tf->tf_ssr = PSL_USERSET;
	tf->tf_r15 = stack;
#ifdef TODO
	tf->tf_r9 = (int)PS_STRINGS;
#endif
}

struct queue {
	struct queue *q_next, *q_prev;
};

/*
 * insert an element into a queue
 */
void
_insque(v1, v2)
	void *v1;
	void *v2;
{
	struct queue *elem = v1, *head = v2;
	struct queue *next;

	next = head->q_next;
	elem->q_next = next;
	head->q_next = elem;
	elem->q_prev = head;
	next->q_prev = elem;
}

/*
 * remove an element from a queue
 */
void
_remque(v)
	void *v;
{
	struct queue *elem = v;
	struct queue *next, *prev;

	next = elem->q_next;
	prev = elem->q_prev;
	next->q_prev = prev;
	prev->q_next = next;
	elem->q_prev = 0;
}
