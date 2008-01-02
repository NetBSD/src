/*	$NetBSD: sunos_machdep.c,v 1.19.32.1 2008/01/02 21:50:26 bouyer Exp $	*/

/*
 * Copyright (c) 1995 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos_machdep.c,v 1.19.32.1 2008/01/02 21:50:26 bouyer Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <sys/syscallargs.h>
#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscallargs.h>

#include <machine/frame.h>
#include <machine/cpu.h>

#ifdef DEBUG
int sunos_sigdebug = 0;
int sunos_sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

struct sunos_sigframe {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	struct	sigcontext13 *sf_scp;	/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sigcontext13 sf_sc;	/* actual sigcontext */
};

void sunos_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sunos_sigframe *fp;
	struct trapframe *tf;
	int addr, onstack, oldsp, newsp, error;
	int sig = ksi->ksi_signo;
	u_long code = ksi->ksi_code;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sunos_sigframe sf;

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
		fp = (struct sunos_sigframe *)
		     ((char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size);
	else
		fp = (struct sunos_sigframe *)oldsp;

	fp = (struct sunos_sigframe *)((int)(fp - 1) & ~7);

#ifdef DEBUG
	if ((sunos_sigdebug & SDB_KSTACK) && p->p_pid == sunos_sigpid)
		printf("sendsig: %s[%d] sig %d newusp %p scp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc);
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = code;
	sf.sf_scp = &fp->sf_sc;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;
	native_sigset_to_sigset13(mask, &sf.sf_sc.sc_mask);
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
	mutex_exit(&p->p_smutex);
	newsp = (int)fp - sizeof(struct rwindow);
	write_user_windows();
	error = (rwindow_save(l) || copyout((void *)&sf, (void *)fp, sizeof sf) ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp));
	mutex_enter(&p->p_smutex);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sunos_sigdebug & SDB_KSTACK) && p->p_pid == sunos_sigpid)
			printf("sendsig: window save or copyout error\n");
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sunos_sigdebug & SDB_FOLLOW)
		printf("sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	addr = (int)catcher;	/* user does his own trampolining */
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sunos_sigdebug & SDB_KSTACK) && p->p_pid == sunos_sigpid)
		printf("sendsig: about to return to catcher\n");
#endif
}

int
sunos_sys_sigreturn(struct lwp *l, const struct sunos_sys_sigreturn_args *uap, register_t *retval)
{

	return (compat_13_sys_sigreturn(l,
			(const struct compat_13_sys_sigreturn_args *)uap, retval));
}
