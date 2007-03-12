/*	$NetBSD: compat_16_machdep.c,v 1.7.2.1 2007/03/12 05:50:07 rmind Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.7.2.1 2007/03/12 05:50:07 rmind Exp $");

#include "opt_compat_netbsd.h"
#include "opt_altivec.h"
#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/ucontext.h>
#include <sys/user.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <machine/fpu.h>

/*
 * Send a signal to process.
 */
void
sendsig_sigcontext(int sig, const sigset_t *mask, u_long code)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct sigcontext *fp, frame;
	struct trapframe *tf;
	struct utrapframe *utf = &frame.sc_frame;
	int onstack, error;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = trapframe(l);

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigcontext *)((char *)l->l_sigstk.ss_sp +
						l->l_sigstk.ss_size);
	else
		fp = (struct sigcontext *)tf->fixreg[1];
	fp = (struct sigcontext *)((uintptr_t)(fp - 1) & ~0xf);

	/* Save register context. */
	memcpy(utf->fixreg, tf->fixreg, sizeof(utf->fixreg));
	utf->lr   = tf->lr;
	utf->cr   = tf->cr;
	utf->xer  = tf->xer;
	utf->ctr  = tf->ctr;
	utf->srr0 = tf->srr0;
	utf->srr1 = tf->srr1 & PSL_USERSRR1;
#ifdef PPC_HAVE_FPU
	utf->srr1 |= l->l_addr->u_pcb.pcb_flags & (PCB_FE0|PCB_FE1);
#endif
#ifdef ALTIVEC
	utf->srr1 |= l->l_addr->u_pcb.pcb_flags & PCB_ALTIVEC ? PSL_VEC : 0;
#endif
#ifdef PPC_OEA
	utf->vrsave = tf->tf_xtra[TF_VRSAVE];
	utf->mq = tf->tf_xtra[TF_MQ];
#endif

	/* Save signal stack. */
	frame.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.__sc_mask13);
#endif
	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = copyout(&frame, fp, sizeof frame);
	mutex_enter(&p->p_smutex);

	if (error != 0) {
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
		tf->fixreg[5] = (register_t)fp;
		tf->srr0 = (register_t)p->p_sigctx.ps_sigcode;
		break;
#endif /* COMPAT_16 */

	case 1:
		tf->fixreg[1] = (register_t)fp;
		tf->lr = (register_t)catcher;
		tf->fixreg[3] = (register_t)sig;
		tf->fixreg[4] = (register_t)code;
		tf->fixreg[5] = (register_t)fp;
		tf->srr0 = (register_t)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal handler returns.
 */
int
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext sc;
	struct trapframe *tf;
	struct utrapframe * const utf = &sc.sc_frame;
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

	/*
	 * Make sure SRR1 hasn't been maliciously tampered with.
	 */
	if (!PSL_USEROK_P(sc.sc_frame.srr1))
		return (EINVAL);

	/* Restore register context. */
	memcpy(tf->fixreg, utf->fixreg, sizeof(tf->fixreg));
	tf->lr   = utf->lr;
	tf->cr   = utf->cr;
	tf->xer  = utf->xer;
	tf->ctr  = utf->ctr;
	tf->srr0 = utf->srr0;
	tf->srr1 = utf->srr1;
#ifdef PPC_HAVE_FPU
	l->l_addr->u_pcb.pcb_flags &= ~(PCB_FE0|PCB_FE1);
	l->l_addr->u_pcb.pcb_flags |= utf->srr1 & (PCB_FE0|PCB_FE1);
#endif
#ifdef PPC_OEA
	tf->tf_xtra[TF_VRSAVE] = utf->vrsave;
	tf->tf_xtra[TF_MQ] = utf->mq;
#endif

	mutex_enter(&p->p_smutex);
	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &sc.sc_mask, 0);
	mutex_exit(&p->p_smutex);

	return (EJUSTRETURN);
}
