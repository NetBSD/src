/*	$NetBSD: sig_machdep.c,v 1.10 2003/01/20 05:26:47 matt Exp $	*/

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
sendsig(sig, mask, code)
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	int onstack;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

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
	fp = (struct sigframe *)((uintptr_t)(fp - 1) & ~0xf);

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
	 * Build context to run handler in.  Note the trampoline version
	 * numbers are coordinated with machine-dependent code in libc.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
#if 1 /* COMPAT_16 */
	case 0:		/* legacy on-stack sigtramp */
		tf->fixreg[1] = (register_t)fp;
		tf->lr = (register_t)catcher;
		tf->fixreg[3] = (register_t)sig;
		tf->fixreg[4] = (register_t)code;
		tf->fixreg[5] = (register_t)&fp->sf_sc;
		tf->srr0 = (register_t)p->p_sigctx.ps_sigcode;
		break;
#endif /* COMPAT_16 */

	case 1:
		tf->fixreg[1] = (register_t)fp;
		tf->lr = (register_t)catcher;
		tf->fixreg[3] = (register_t)sig;
		tf->fixreg[4] = (register_t)code;
		tf->fixreg[5] = (register_t)&fp->sf_sc;
		tf->srr0 = (register_t)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

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
	__greg_t *gr = mcp->__gregs;
#ifdef PPC_HAVE_FPU
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Save GPR context. */
	(void)memcpy(gr, &tf->fixreg, 32 * sizeof (gr[0])); /* GR0-31 */
	gr[_REG_CR]  = tf->cr;
	gr[_REG_LR]  = tf->lr;
	gr[_REG_PC]  = tf->srr0;
	gr[_REG_MSR] = tf->srr1;
	gr[_REG_CTR] = tf->ctr;
	gr[_REG_XER] = tf->xer;
	gr[_REG_MQ]  = 0;				/* For now. */
	*flagp |= _UC_CPU;

#ifdef PPC_HAVE_FPU
	/* Save FPR context, if any. */
	if ((pcb->pcb_flags & PCB_FPU) != 0) {
		/* If we're the FPU owner, dump its context to the PCB first. */
		if (pcb->pcb_fpcpu)
			save_fpu_lwp(l);
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
	__greg_t *gr = mcp->__gregs;
#ifdef PPC_HAVE_FPU
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Restore GPR context, if any. */
	if (flags & _UC_CPU) {
		if ((gr[_REG_MSR] & PSL_USERSTATIC) !=
		    (tf->srr1 & PSL_USERSTATIC))
			return (EINVAL);

		(void)memcpy(&tf->fixreg, gr, 32 * sizeof (gr[0]));
		tf->cr   = gr[_REG_CR];
		tf->lr   = gr[_REG_LR];
		tf->srr0 = gr[_REG_PC];
		tf->srr1 = gr[_REG_MSR];
		tf->ctr  = gr[_REG_CTR];
		tf->xer  = gr[_REG_XER];
		/* unused = gr[_REG_MQ]; */
	}

#ifdef PPC_HAVE_FPU
	/* Restore FPR context, if any. */
	if ((flags & _UC_FPU) && mcp->__fpregs.__fpu_valid != 0) {
		/* XXX we don't need to save the state, just to drop it */
		save_fpu_lwp(l);
		(void)memcpy(&pcb->pcb_fpu.fpr, &mcp->__fpregs.__fpu_regs,
		    sizeof (pcb->pcb_fpu.fpr));
		pcb->pcb_fpu.fpscr = *(double *)&mcp->__fpregs.__fpu_fpscr;
	}
#endif

	return (0);
}
