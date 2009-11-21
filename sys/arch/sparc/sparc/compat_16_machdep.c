/*	$NetBSD: compat_16_machdep.c,v 1.4 2009/11/21 04:16:51 rmind Exp $ */

/*-
 * Copyright (c) 1996, 1997, 1998, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.4 2009/11/21 04:16:51 rmind Exp $");

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syscallargs.h>

#ifdef COMPAT_13
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#endif
#include <machine/frame.h>

#ifdef COMPAT_13
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#endif /* COMPAT_13 */

struct sigframe_sigcontext {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	struct	sigcontext *sf_scp;	/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sigcontext sf_sc;	/* actual sigcontext */
};

#ifdef DEBUG
extern int sigdebug;
extern int sigpid;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct sigframe_sigcontext *fp;
	struct trapframe *tf;
	int addr, onstack, oldsp, newsp, error;
	struct sigframe_sigcontext sf;
	int sig = ksi->ksi_signo;
	u_long code = KSI_TRAPCODE(ksi);
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = l->l_md.md_tf;
	oldsp = tf->tf_out[6];

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sigframe_sigcontext *)
			((char *)l->l_sigstk.ss_sp +
			 l->l_sigstk.ss_size);
	else
		fp = (struct sigframe_sigcontext *)oldsp;

	fp = (struct sigframe_sigcontext *)((int)(fp - 1) & ~7);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig_sigcontext: %s[%d] sig %d newusp %p scp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc);
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = code;
	sf.sf_scp = 0;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;
	sf.sf_sc.sc_mask = *mask;
#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &sf.sf_sc.__sc_mask13);
#endif
	sf.sf_sc.sc_sp = oldsp;
	sf.sf_sc.sc_pc = tf->tf_pc;
	sf.sf_sc.sc_npc = tf->tf_npc;
	sf.sf_sc.sc_psr = tf->tf_psr;
	sf.sf_sc.sc_g1 = tf->tf_global[1];
	sf.sf_sc.sc_o0 = tf->tf_out[0];

	/*
	 * Put the stack in a consistent state before we whack away
	 * at it.  Note that write_user_windows may just dump the
	 * registers into the pcb; we need them in the process's memory.
	 * We also need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 */
	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);
	newsp = (int)fp - sizeof(struct rwindow);
	write_user_windows();
	error = (rwindow_save(l) || copyout((void *)&sf, (void *)fp, sizeof sf) ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp));
	mutex_enter(p->p_lock);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig_sigcontext: window save or copyout error\n");
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig_siginfo: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		addr = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		addr = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		addr = 0;
		sigexit(l, SIGILL);
	}

	tf->tf_global[1] = (int)catcher;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: about to return to catcher\n");
#endif
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above),
 * and return to the given trap frame (if there is one).
 * Check carefully to make sure that the user has not
 * modified the state to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
compat_16_sys___sigreturn14(struct lwp *l, const struct compat_16_sys___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct sigcontext sc, *scp;
	struct trapframe *tf;
	struct proc *p;
	int error;

	p = l->l_proc;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
#endif
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0)
		return (EINVAL);

	/* take only psr ICC field */
	tf->tf_psr = (tf->tf_psr & ~PSR_ICC) | (scp->sc_psr & PSR_ICC);
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;

	mutex_enter(p->p_lock);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask */
	(void) sigprocmask1(l, SIG_SETMASK, &scp->sc_mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
