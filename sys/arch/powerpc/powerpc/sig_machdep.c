/*	$NetBSD: sig_machdep.c,v 1.5.8.5 2002/05/29 21:31:55 nathanw Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_compat_netbsd.h"
#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/ucontext.h>
#include <sys/user.h>

#include <machine/fpu.h>

/*
 * Send a signal to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	int onstack;

	tf = trapframe(l);

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->fixreg[1];
	fp = (struct sigframe *)((int)(fp - 1) & ~0xf);

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;

	/* Save register context. */
	frame.sf_sc.sc_frame = *tf;

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

	if (copyout(&frame, fp, sizeof frame) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instructoin to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->fixreg[1] = (int)fp;
	tf->lr = (int)catcher;
	tf->fixreg[3] = (int)sig;
	tf->fixreg[4] = (int)code;
	tf->fixreg[5] = (int)&fp->sf_sc;
	tf->srr0 = (int)p->p_sigctx.ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal handler returns.
 */
int
sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext sc;
	struct trapframe *tf;
	int error;

	/*
	 * The trampoline hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal hander.
	 */
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);

	/* Restore the register context. */
	tf = trapframe(l);
	if ((sc.sc_frame.srr1 & PSL_USERSTATIC) != (tf->srr1 & PSL_USERSTATIC))
		return (EINVAL);
	*tf = sc.sc_frame;

	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &sc.sc_mask, 0);

	return (EJUSTRETURN);
}

void
cpu_getmcontext(l, mcp, flagp)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flagp;
{
	const struct trapframe *tf = trapframe(l);
	struct __gregs *gr = (struct __gregs *)mcp->__gregs;
#ifndef PPC_IBM4XX
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Save GPR context. */
	(void)memcpy(gr, &tf->fixreg, 32 * sizeof (gr->__r_r0)); /* GR0-31 */
	gr->__r_cr  = tf->cr;
	gr->__r_lr  = tf->lr;
	gr->__r_pc  = tf->srr0;
	gr->__r_msr = tf->srr1;
	gr->__r_ctr = tf->ctr;
	gr->__r_xer = tf->xer;
	gr->__r_mq  = 0;				/* For now. */
	*flagp |= _UC_CPU;

#ifndef PPC_IBM4XX
	/* Save FPR context, if any. */
	if ((pcb->pcb_flags & PCB_FPU) != 0) {
		/* If we're the FPU owner, dump its context to the PCB first. */
		if (l == fpuproc)
			save_fpu(l);
		(void)memcpy(mcp->__fpregs.__fpu_regs, pcb->pcb_fpu.fpr,
		    sizeof (mcp->__fpregs.__fpu_regs));
		mcp->__fpregs.__fpu_fpscr =
		    ((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD];
		mcp->__fpregs.__fpu_valid = 1;
		*flagp |= _UC_FPU;
	} else
#endif
		mcp->__fpregs.__fpu_valid = 0;

	/* No AltiVec support, for now. */
	memset(&mcp->__vrf, 0, sizeof (mcp->__vrf));
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf = trapframe(l);
	struct __gregs *gr = (struct __gregs *)mcp->__gregs;
#ifndef PPC_IBM4XX
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Restore GPR context, if any. */
	if (flags & _UC_CPU) {
		if ((gr->__r_msr & PSL_USERSTATIC) !=
		    (tf->srr1 & PSL_USERSTATIC))
			return (EINVAL);

		(void)memcpy(&tf->fixreg, gr, 32 * sizeof (gr->__r_r0));
		tf->cr   = gr->__r_cr;
		tf->lr   = gr->__r_lr;
		tf->srr0 = gr->__r_pc;
		tf->srr1 = gr->__r_msr;
		tf->ctr  = gr->__r_ctr;
		tf->xer  = gr->__r_xer;
		/* unused = gr->__r_mq; */
	}

#ifndef PPC_IBM4XX
	/* Restore FPR context, if any. */
	if ((flags & _UC_FPU) && mcp->__fpregs.__fpu_valid != 0) {
		(void)memcpy(&pcb->pcb_fpu.fpr, &mcp->__fpregs.__fpu_regs,
		    sizeof (pcb->pcb_fpu.fpr));
		pcb->pcb_fpu.fpscr = *(double *)&mcp->__fpregs.__fpu_fpscr;

		/* If we're the FPU owner, force a reload from the PCB. */
		if (l == fpuproc)
			enable_fpu(l);
	}
#endif

	return (0);
}
