/*	$NetBSD: compat_16_machdep.c,v 1.4.2.1 2007/02/27 16:49:11 yamt Exp $	*/

/*
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
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
 *
 * Machine dependant functions for kernel setup
 *
 * Created      : 17/09/94
 */

#include "opt_compat_netbsd.h"
#include "opt_armfpe.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.4.2.1 2007/02/27 16:49:11 yamt Exp $");

#include <sys/mount.h>		/* XXX only needed by syscallargs.h */
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ras.h>
#include <sys/ucontext.h>

#include <arm/armreg.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#ifndef acorn26
#include <arm/cpufunc.h>
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user specified pc.
 */
#if defined(COMPAT_16) || defined(COMPAT_13)

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf;
	struct sigframe_sigcontext *fp, frame;
	int onstack, error;
	int sig = ksi->ksi_signo;
	u_long code = KSI_TRAPCODE(ksi);
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = process_frame(l);

	fp = getframe(l, sig, &onstack);

	/* make room on the stack */
	fp--;

	/* make the stack aligned */
	fp = (void *)STACKALIGN(fp);

	/* Save register context. */
	frame.sf_sc.sc_r0     = tf->tf_r0;
	frame.sf_sc.sc_r1     = tf->tf_r1;
	frame.sf_sc.sc_r2     = tf->tf_r2;
	frame.sf_sc.sc_r3     = tf->tf_r3;
	frame.sf_sc.sc_r4     = tf->tf_r4;
	frame.sf_sc.sc_r5     = tf->tf_r5;
	frame.sf_sc.sc_r6     = tf->tf_r6;
	frame.sf_sc.sc_r7     = tf->tf_r7;
	frame.sf_sc.sc_r8     = tf->tf_r8;
	frame.sf_sc.sc_r9     = tf->tf_r9;
	frame.sf_sc.sc_r10    = tf->tf_r10;
	frame.sf_sc.sc_r11    = tf->tf_r11;
	frame.sf_sc.sc_r12    = tf->tf_r12;
	frame.sf_sc.sc_usr_sp = tf->tf_usr_sp;
	frame.sf_sc.sc_usr_lr = tf->tf_usr_lr;
	frame.sf_sc.sc_svc_lr = tf->tf_svc_lr;
	frame.sf_sc.sc_pc     = tf->tf_pc;
	frame.sf_sc.sc_spsr   = tf->tf_spsr;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;

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

	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

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
	 * directly, only returning via the trampoline.  Note the
	 * trampoline version numbers are coordinated with machine-
	 * dependent code in libc.
	 */

	/*
	 * this was all in the switch below, seemed daft to duplicate it, if
	 * we do a new trampoline version it might change then
	 */
	tf->tf_r0 = sig;
	tf->tf_r1 = code;
	tf->tf_r2 = (int)&fp->sf_sc;
	tf->tf_pc = (int)catcher;
#ifdef THUMB_CODE
	if (((int) catcher) & 1)
		tf->tf_spsr |= PSR_T_bit;
	else
		tf->tf_spsr &= ~PSR_T_bit;
#endif
	tf->tf_usr_sp = (int)fp;
	
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		tf->tf_usr_lr = (int)p->p_sigctx.ps_sigcode;
#ifndef acorn26
		/* XXX This should not be needed. */
		cpu_icache_sync_all();
#endif
		break;
	case 1:
		tf->tf_usr_lr = (int)ps->sa_sigdesc[sig].sd_tramp;
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
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psr to gain improper privileges or to cause
 * a machine fault.
 */

int
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct trapframe *tf;
	struct proc *p = l->l_proc;

	/*
	 * we do a rather scary test in userland
	 */
	if (v == NULL)
		return (EFAULT);
	
	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/*
	 * Make sure the processor mode has not been tampered with and
	 * interrupts have not been disabled.
	 */
	if (!VALID_R15_PSR(context.sc_pc, context.sc_spsr))
		return EINVAL;

	/* Restore register context. */
	tf = process_frame(l);
	tf->tf_r0    = context.sc_r0;
	tf->tf_r1    = context.sc_r1;
	tf->tf_r2    = context.sc_r2;
	tf->tf_r3    = context.sc_r3;
	tf->tf_r4    = context.sc_r4;
	tf->tf_r5    = context.sc_r5;
	tf->tf_r6    = context.sc_r6;
	tf->tf_r7    = context.sc_r7;
	tf->tf_r8    = context.sc_r8;
	tf->tf_r9    = context.sc_r9;
	tf->tf_r10   = context.sc_r10;
	tf->tf_r11   = context.sc_r11;
	tf->tf_r12   = context.sc_r12;
	tf->tf_usr_sp = context.sc_usr_sp;
	tf->tf_usr_lr = context.sc_usr_lr;
	tf->tf_svc_lr = context.sc_svc_lr;
	tf->tf_pc    = context.sc_pc;
	tf->tf_spsr  = context.sc_spsr;

	mutex_enter(&p->p_smutex);

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &context.sc_mask, 0);

	mutex_exit(&p->p_smutex);

	return (EJUSTRETURN);
}
#endif
