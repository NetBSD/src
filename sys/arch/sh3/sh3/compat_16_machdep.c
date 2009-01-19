/* $NetBSD: compat_16_machdep.c,v 1.1.8.2 2009/01/19 13:16:43 skrll Exp $ */

/*-
 * Copyright (c) 1996, 1997, 1998, 2002 The NetBSD Foundation, Inc.
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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.1.8.2 2009/01/19 13:16:43 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>


#ifdef COMPAT_16
/*
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf = l->l_md.md_regs;
	int sig = ksi->ksi_info._signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigframe_sigcontext *fp, frame;
	int onstack, error;

	fp = getframe(l, sig, &onstack);
	--fp;

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
	frame.sf_sc.sc_expevt = tf->tf_expevt;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.  We invoke the handler
	 * directly, only returning via the trampoline.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		tf->tf_pr = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		tf->tf_pr = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		printf("sendsig_sigcontext: bad version %d\n",
		       ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	}

	tf->tf_r4 = sig;
	tf->tf_r5 = ksi->ksi_code;
	tf->tf_r6 = (int)&fp->sf_sc;
 	tf->tf_spc = (int)catcher;
	tf->tf_r15 = (int)fp;

	/* Remember if we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
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
compat_16_sys___sigreturn14(struct lwp *l, const struct compat_16_sys___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct sigcontext *scp, context;
	struct trapframe *tf;
	struct proc *p = l->l_proc;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((void *)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore signal context. */
	tf = l->l_md.md_regs;

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

	mutex_enter(p->p_lock);
	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &context.sc_mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */
