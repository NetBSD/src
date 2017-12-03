/*	$NetBSD: sunos_machdep.c,v 1.31.22.2 2017/12/03 11:36:45 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sunos_machdep.c,v 1.31.22.2 2017/12/03 11:36:45 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <sys/syscallargs.h>
#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscallargs.h>

#include <machine/frame.h>
#include <machine/cpu.h>

#ifdef DEBUG
#include <sparc64/sparc64/sigdebug.h>
#endif

struct sunos_sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	int	sc_sp;			/* %sp to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_npc;			/* npc to restore */
	int	sc_psr;			/* pstate to restore */
	int	sc_g1;			/* %g1 to restore */
	int	sc_o0;			/* %o0 to restore */
};

struct sunos_sigframe {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	uint32_t	sf_scp;			/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sunos_sigcontext sf_sc;	/* actual sigcontext */
};

void
sunos_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	register struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	register struct sunos_sigframe *fp;
	register struct trapframe64 *tf;
	register int addr, onstack;
	struct rwindow32 *oldsp, *newsp;
	register32_t sp;
	int sig = ksi->ksi_signo, error;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sunos_sigframe sf;

	tf = (struct trapframe64 *)l->l_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sunos_sigframe *)((char *)l->l_sigstk.ss_sp +
					l->l_sigstk.ss_size);
	else
		fp = (struct sunos_sigframe *)oldsp;
	fp = (struct sunos_sigframe *)((long)(fp - 1) & ~7);

#ifdef DEBUG
	sigpid = p->p_pid;
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sunos_sendsig: %s[%d] sig %d newusp %p scp %p oldsp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc, oldsp);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = ksi->ksi_trap;
	sf.sf_scp = (u_long)&fp->sf_sc;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;
	native_sigset_to_sigset13(mask, &sf.sf_sc.sc_mask);
	sf.sf_sc.sc_sp = (long)oldsp;
	sf.sf_sc.sc_pc = tf->tf_pc;
	sf.sf_sc.sc_npc = tf->tf_npc;
	sf.sf_sc.sc_psr = TSTATECCR_TO_PSR(tf->tf_tstate); /* XXX */
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
	newsp = (struct rwindow32 *)((long)fp - sizeof(struct rwindow32));
	write_user_windows();
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK))
	    printf("sunos_sendsig: saving sf to %p, setting stack pointer %p to %p\n",
		   fp, &(((struct rwindow32 *)newsp)->rw_in[6]), oldsp);
#endif
	sp = (register32_t)(uintptr_t)oldsp;
	error = (rwindow_save(l) || 
	    copyout((void *)&sf, (void *)fp, sizeof sf) || 
	    copyout(&sp, &(((struct rwindow32 *)newsp)->rw_in[6]),
	        sizeof(sp)));
	mutex_enter(p->p_lock);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sunos_sendsig: window save or copyout error\n");
		printf("sunos_sendsig: stack was trashed trying to send sig %d, sending SIGILL\n", sig);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sunos_sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
	}
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	addr = (long)catcher;	/* user does his own trampolining */
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (uint64_t)(u_int)(u_long)newsp;
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sunos_sendsig: about to return to catcher %p thru %p\n", 
		       catcher, (void *)(u_long)addr);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif
}

int
sunos_sys_sigreturn(register struct lwp *l, const struct sunos_sys_sigreturn_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sunos_sigcontext sc, *scp;
	sigset_t mask;
	struct trapframe64 *tf;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sunos_sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	scp = (struct sunos_sigcontext *)SCARG(uap, sigcntxp);
	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
		return (EFAULT);
	scp = &sc;

	tf = (struct trapframe64 *)l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0 || scp->sc_pc == 0 || scp->sc_npc == 0)
#ifdef DEBUG
	{
		printf("sunos_sigreturn: pc %p or npc %p invalid\n", (void *)(u_long)scp->sc_pc, (void *)(u_long)scp->sc_npc);
#ifdef DDB
		Debugger();
#endif
		return (EINVAL);
	}
#endif
		return (EINVAL);
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(scp->sc_psr);
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sunos_sigreturn: return trapframe pc=%p sp=%p tstate=%llx\n",
		       (void *)(u_long)tf->tf_pc, (void *)(u_long)tf->tf_out[6], (unsigned long long)tf->tf_tstate);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	mutex_enter(p->p_lock);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask */
	native_sigset13_to_sigset(&scp->sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
