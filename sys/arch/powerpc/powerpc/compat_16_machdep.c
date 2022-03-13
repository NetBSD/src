/*	$NetBSD: compat_16_machdep.c,v 1.23 2022/03/13 17:50:55 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.23 2022/03/13 17:50:55 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_compat_netbsd.h"
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/ucontext.h>

#include <uvm/uvm_extern.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <powerpc/pcb.h>
#include <powerpc/psl.h>
#include <powerpc/fpu.h>
#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
#include <powerpc/altivec.h>
#endif

/*
 * Send a signal to process.
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct sigacts * const ps = p->p_sigacts;
	struct sigcontext *fp, frame;
	struct trapframe * const tf = l->l_md.md_utf;
	struct utrapframe * const utf = &frame.sc_frame;
	int onstack, error;
	int sig = ksi->ksi_signo;
	u_long code = KSI_TRAPCODE(ksi);
	sig_t catcher = SIGACTION(p, sig).sa_handler;


	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigcontext *)((char *)l->l_sigstk.ss_sp +
						l->l_sigstk.ss_size);
	else
		fp = (struct sigcontext *)tf->tf_fixreg[1];
	fp = (struct sigcontext *)((uintptr_t)(fp - 1) & -CALLFRAMELEN);

	/* Save register context. */
	memcpy(utf->fixreg, tf->tf_fixreg, sizeof(utf->fixreg));
	utf->lr   = tf->tf_lr;
	utf->cr   = tf->tf_cr;
	utf->xer  = tf->tf_xer;
	utf->ctr  = tf->tf_ctr;
	utf->srr0 = tf->tf_srr0;
	utf->srr1 = tf->tf_srr1 & PSL_USERSRR1;

#ifdef PPC_HAVE_FPU
	const struct pcb * const pcb = lwp_getpcb(l);
	utf->srr1 |= pcb->pcb_flags & (PCB_FE0|PCB_FE1);
#endif
#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	/*
	 * We can't round-trip the vector unit registers with a
	 * sigcontext, so at least get them saved into the PCB.
	 * XXX vec_save_to_mcontext() has a special hack for this.
	 */
	vec_save_to_mcontext(l, NULL, NULL);
#endif
#ifdef PPC_OEA
	utf->vrsave = tf->tf_vrsave;
	utf->mq = tf->tf_mq;
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
	mutex_exit(p->p_lock);
	error = copyout(&frame, fp, sizeof frame);
	mutex_enter(p->p_lock);

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
	case __SIGTRAMP_SIGCODE_VERSION:	/* legacy on-stack sigtramp */
		tf->tf_fixreg[1] = (register_t)fp;
		tf->tf_lr = (register_t)catcher;
		tf->tf_fixreg[3] = (register_t)sig;
		tf->tf_fixreg[4] = (register_t)code;
		tf->tf_fixreg[5] = (register_t)fp;
		tf->tf_srr0 = (register_t)p->p_sigctx.ps_sigcode;
		break;
#endif /* COMPAT_16 */

	case __SIGTRAMP_SIGCONTEXT_VERSION:
		tf->tf_fixreg[1] = (register_t)fp;
		tf->tf_lr = (register_t)catcher;
		tf->tf_fixreg[3] = (register_t)sig;
		tf->tf_fixreg[4] = (register_t)code;
		tf->tf_fixreg[5] = (register_t)fp;
		tf->tf_srr0 = (register_t)ps->sa_sigdesc[sig].sd_tramp;
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
compat_16_sys___sigreturn14(struct lwp *l,
    const struct compat_16_sys___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct proc * const p = l->l_proc;
	struct sigcontext sc;
	struct utrapframe * const utf = &sc.sc_frame;
	int error;

	/*
	 * The trampoline hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);

	/* Restore the register context. */
	struct trapframe * const tf = l->l_md.md_utf;

	/*
	 * Make sure SRR1 hasn't been maliciously tampered with.
	 */
	if (!PSL_USEROK_P(sc.sc_frame.srr1))
		return (EINVAL);

	/* Restore register context. */
	memcpy(tf->tf_fixreg, utf->fixreg, sizeof(tf->tf_fixreg));
	tf->tf_lr   = utf->lr;
	tf->tf_cr   = utf->cr;
	tf->tf_xer  = utf->xer;
	tf->tf_ctr  = utf->ctr;
	tf->tf_srr0 = utf->srr0;
	tf->tf_srr1 = utf->srr1;

#ifdef PPC_HAVE_FPU
	struct pcb * const pcb = lwp_getpcb(l);
	pcb->pcb_flags &= ~(PCB_FE0|PCB_FE1);
	pcb->pcb_flags |= utf->srr1 & (PCB_FE0|PCB_FE1);
#endif
#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	/*
	 * We can't round-trip the vector unit registers with a
	 * sigcontext, so at least force them to get reloaded from
	 * the PCB (we saved them into the PCB in sendsig_sigcontext()).
	 * XXX vec_restore_from_mcontext() has a special hack for this.
	 */
	vec_restore_from_mcontext(l, NULL);
#endif
#ifdef PPC_OEA
	tf->tf_vrsave = utf->vrsave;
	tf->tf_mq = utf->mq;
#endif

	mutex_enter(p->p_lock);
	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &sc.sc_mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
