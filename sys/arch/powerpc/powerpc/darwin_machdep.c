/*	$NetBSD: darwin_machdep.c,v 1.17.12.3 2008/01/21 09:38:28 yamt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_machdep.c,v 1.17.12.3 2008/01/21 09:38:28 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/signal.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscallargs.h>

#include <machine/psl.h>
#include <machine/darwin_machdep.h>

/* 
 * First argument is in reg 3, duplicated from 
 * sys/arch/powerpc/powerpc/syscall.c 
 */
#define FIRSTARG 3

/*
 * Send a signal to a Darwin process.
 */
void
darwin_sendsig(ksi, mask)
	const ksiginfo_t *ksi;
	const sigset_t *mask;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf;
	struct darwin_sigframe *sfp, sf;
	int onstack;
	size_t stack_size;
	sig_t catcher;
	int sig;
	int error;

	tf = trapframe(l);

	sig = ksi->ksi_signo;
	catcher = SIGACTION(p, sig).sa_handler;

	/* Use an alternate signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Set the new stack pointer sfp */
	if (onstack) {
		sfp = (struct darwin_sigframe *)
		    ((char *)l->l_sigstk.ss_sp +
		    l->l_sigstk.ss_size);
		stack_size = l->l_sigstk.ss_size;
	} else {
		sfp = (struct darwin_sigframe *)tf->fixreg[1];
		stack_size = 0;
	}
	/* 16 bytes alignement */
	sfp = (struct darwin_sigframe *)((u_long)(sfp - 1) & ~0xfUL);

	/* Prepare the signal frame */
	bzero(&sf, sizeof(sf));
	sf.dmc.es.dar = tf->dar;
	sf.dmc.es.dsisr = tf->dsisr;
	sf.dmc.es.exception = tf->exc;

	sf.dmc.ss.srr0 = tf->srr0;
	sf.dmc.ss.srr1 = tf->srr1 & PSL_USERSRR1;
	memcpy(&sf.dmc.ss.gpreg[0], &tf->fixreg[0], sizeof(sf.dmc.ss.gpreg));
	sf.dmc.ss.cr = tf->cr;
	sf.dmc.ss.xer = tf->xer;
	sf.dmc.ss.lr = tf->lr;
	sf.dmc.ss.ctr = tf->ctr;
	sf.dmc.ss.mq = 0; /* XXX */

	/* XXX What should we do with th FP regs? */

	/* 
	 * Darwin only supports 32 signals. 
	 * Unsupported signals are mapped to 0 
	 */
	if (sig >= 32) {
		DPRINTF(("unsupported signal for darwin: %d\n", sig));
		sig = 0;
	}

	native_to_darwin_siginfo(ksi, &sf.duc.si);
	sf.duc.uctx.uc_onstack = onstack;
	native_sigset_to_sigset13(mask, &sf.duc.uctx.uc_sigmask);
	sf.duc.uctx.uc_stack.ss_sp = (char *)sfp;
	sf.duc.uctx.uc_stack.ss_size = stack_size;
	if (onstack)
		sf.duc.uctx.uc_stack.ss_flags |= SS_ONSTACK;
	sf.duc.uctx.uc_link = l->l_ctxlink;
	sf.duc.uctx.uc_mcsize = sizeof(sf.dmc);
	sf.duc.uctx.uc_mcontext = (struct darwin_mcontext *)&sfp->dmc;

	/* Copyout mcontext */
	if ((error = copyout(&sf.dmc, &sfp->dmc, sizeof(sf.dmc))) != 0) {
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* Copyout ucontext */
	if ((error = copyout(&sf.duc, &sfp->duc, sizeof(sf.duc))) != 0) {
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* Darwin only supports libc based trampoline */
	if (ps->sa_sigdesc[sig].sd_vers != 1) {
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* Prepare registers */
	tf->fixreg[1] = (u_long)sfp;
	tf->fixreg[3] = (u_long)catcher;
	if (SIGACTION(p, sig).sa_flags & SA_SIGINFO)
		tf->fixreg[4] = 2; /* with siginfo */
	else
		tf->fixreg[4] = 1; /* without siginfo */
	tf->fixreg[5] = (u_long)sig;
	tf->fixreg[6] = (u_long)&sfp->duc.si;
	tf->fixreg[7] = (u_long)&sfp->duc.uctx;
	tf->lr = (u_long)tf->srr0;
	tf->srr0 = (u_long)ps->sa_sigdesc[sig].sd_tramp;
	tf->srr1 = (PSL_EE | PSL_ME | PSL_IR | PSL_DR | PSL_PR);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

	return;
}

/*
 * The signal trampoline calls this system call 
 * to get the process state restored like it was
 * before the signal delivery.
 * 
 * This is the version for X.2 binaries and older
 */
int
darwin_sys_sigreturn_x2(struct lwp *l, const struct darwin_sys_sigreturn_x2_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_ucontext *) uctx;
	} */
	struct darwin_ucontext uctx;
	struct darwin_mcontext mctx;
	struct trapframe *tf;
	sigset_t mask;
	size_t mcsize;
	int error;

	/*
	 * The trampoline hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal hander.
	 */
	if ((error = copyin(SCARG(uap, uctx), &uctx, sizeof(uctx))) != 0)
		return (error);

	/* Check mcontext size, as it is handed by user code */
	mcsize = uctx.uc_mcsize;
	if (mcsize > sizeof(mctx))
		mcsize = sizeof(mctx);

	if ((error = copyin(uctx.uc_mcontext, &mctx, mcsize)) != 0)
		return (error);

	/* Check for security abuse */
	tf = trapframe(l);
	if (!PSL_USEROK_P(mctx.ss.srr1)) {
		DPRINTF(("uctx.ss.srr1 = 0x%08x, rf->srr1 = 0x%08lx\n",
		    mctx.ss.srr1, tf->srr1));
		return (EINVAL);
	}

	/* Restore the context */
	tf->dar = mctx.es.dar;
	tf->dsisr = mctx.es.dsisr;
	tf->exc = mctx.es.exception;

	tf->srr0 = mctx.ss.srr0;
	tf->srr1 = mctx.ss.srr1;
	memcpy(&tf->fixreg[0], &mctx.ss.gpreg[0], sizeof(mctx.ss.gpreg));
	tf->cr = mctx.ss.cr;
	tf->xer = mctx.ss.xer;
	tf->lr = mctx.ss.lr;
	tf->ctr = mctx.ss.ctr;

	/* Restore signal stack */
	if (uctx.uc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	native_sigset13_to_sigset(&uctx.uc_sigmask, &mask);
	(void)sigprocmask1(l, SIG_SETMASK, &mask, 0);

	return (EJUSTRETURN);
}

/*
 * This is the version used starting with X.3 binaries
 */
int
darwin_sys_sigreturn(struct lwp *l, const struct darwin_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_ucontext *) uctx;
		syscallarg(int) ucvers;
	} */

	switch (SCARG(uap, ucvers)) {
	case DARWIN_UCVERS_X2:
		return darwin_sys_sigreturn_x2(l, (const void *)uap, retval);
		break;

	default:
		printf("darwin_sys_sigreturn: ucvers = %d\n", 
		    SCARG(uap, ucvers));
		break;
	}
	return (EJUSTRETURN);
}

/*
 * Set the return value for darwin binaries after a fork(). The userland
 * libSystem stub expects the child pid to be in retval[0] for the parent
 * and the child as well. It will perform the required operation to transform 
 * it in the POSIXly correct value: zero for the child.
 * We also need to skip the next instruction because the system call
 * was successful (We also do this in the syscall handler, Darwin 
 * works that way).
 */
void
darwin_fork_child_return(void *arg)
{
	struct lwp * const l = arg;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = trapframe(l);

	child_return(arg);

	tf->fixreg[FIRSTARG] = p->p_pid;
	tf->srr0 +=4;

	return;
}
