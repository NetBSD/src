/*	$NetBSD: linux_sigaction.c,v 1.1 1995/02/28 23:25:12 fvdl Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * heavily from: svr4_signal.c,v 1.7 1995/01/09 01:04:21 christos Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <sys/syscallargs.h>

#include <compat/linux/linux_types.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>
#include <compat/linux/linux_signal.h>

/*
 * Most of ths stuff in this file is taken from Christos' SVR4 emul
 * code. The things that need to be done are largely the same, so
 * re-inventing the wheel doesn't make much sense.
 */

/*
 * Some boring signal conversion functions. Just a switch() for all signals;
 * return the converted signal number, 0 if not supported.
 */

int
bsd_to_linux_sig(sig)
	int sig;
{
	switch(sig) {
	case SIGHUP:
		return LINUX_SIGHUP;
	case SIGINT:
		return LINUX_SIGINT;
	case SIGQUIT:
		return LINUX_SIGQUIT;
	case SIGILL:
		return LINUX_SIGILL;
	case SIGTRAP:
		return LINUX_SIGTRAP;
	case SIGABRT:
		return LINUX_SIGABRT;
	case SIGFPE:
		return LINUX_SIGFPE;
	case SIGKILL:
		return LINUX_SIGKILL;
	case SIGBUS:
		return LINUX_SIGBUS;
	case SIGSEGV:
		return LINUX_SIGSEGV;
	case SIGPIPE:
		return LINUX_SIGPIPE;
	case SIGALRM:
		return LINUX_SIGALRM;
	case SIGTERM:
		return LINUX_SIGTERM;
	case SIGURG:
		return LINUX_SIGURG;
	case SIGSTOP:
		return LINUX_SIGSTOP;
	case SIGTSTP:
		return LINUX_SIGTSTP;
	case SIGCONT:
		return LINUX_SIGCONT;
	case SIGCHLD:
		return LINUX_SIGCHLD;
	case SIGTTIN:
		return LINUX_SIGTTIN;
	case SIGTTOU:
		return LINUX_SIGTTOU;
	case SIGIO:
		return LINUX_SIGIO;
	case SIGXCPU:
		return LINUX_SIGXCPU;
	case SIGXFSZ:
		return LINUX_SIGXFSZ;
	case SIGVTALRM:
		return LINUX_SIGVTALRM;
	case SIGPROF:
		return LINUX_SIGPROF;
	case SIGWINCH:
		return LINUX_SIGWINCH;
	case SIGUSR1:
		return LINUX_SIGUSR1;
	case SIGUSR2:
		return LINUX_SIGUSR2;
	/* Not supported: EMT, SYS, INFO */
	}
	return 0;
}

int
linux_to_bsd_sig(sig)
	int sig;
{
	switch(sig) {
	case LINUX_SIGHUP:
		return SIGHUP;
	case LINUX_SIGINT:
		return SIGINT;
	case LINUX_SIGQUIT:
		return SIGQUIT;
	case LINUX_SIGILL:
		return SIGILL;
	case LINUX_SIGTRAP:
		return SIGTRAP;
	case LINUX_SIGABRT:
		return SIGABRT;
	case LINUX_SIGBUS:
		return SIGBUS;
	case LINUX_SIGFPE:
		return SIGFPE;
	case LINUX_SIGKILL:
		return SIGKILL;
	case LINUX_SIGUSR1:
		return SIGUSR1;
	case LINUX_SIGSEGV:
		return SIGSEGV;
	case LINUX_SIGUSR2:
		return SIGUSR2;
	case LINUX_SIGPIPE:
		return SIGPIPE;
	case LINUX_SIGALRM:
		return SIGALRM;
	case LINUX_SIGTERM:
		return SIGTERM;
	case LINUX_SIGCHLD:
		return SIGCHLD;
	case LINUX_SIGCONT:
		return SIGCONT;
	case LINUX_SIGSTOP:
		return SIGSTOP;
	case LINUX_SIGTSTP:
		return SIGTSTP;
	case LINUX_SIGTTIN:
		return SIGTTIN;
	case LINUX_SIGTTOU:
		return SIGTTOU;
	case LINUX_SIGURG:
		return SIGURG;
	case LINUX_SIGXCPU:
		return SIGXCPU;
	case LINUX_SIGXFSZ:
		return SIGXFSZ;
	case LINUX_SIGVTALRM:
		return SIGVTALRM;
	case LINUX_SIGPROF:
		return SIGPROF;
	case LINUX_SIGWINCH:
		return SIGWINCH;
	case LINUX_SIGIO:
		return SIGIO;
	/* Not supported: STKFLT, PWR */
	}
	return 0;
}

/*
 * Ok, we know that Linux and BSD signals both are just an unsigned int.
 * Don't bother to use the sigismember() stuff for now.
 */
static void
linux_to_bsd_sigset(lss, bss)
	const linux_sigset_t *lss;
	sigset_t *bss;
{
	int i, newsig;

	*bss = (sigset_t) 0;
	for (i = 1; i <= LINUX_NSIG; i++) {
		if (*lss & sigmask(i)) {
			newsig = linux_to_bsd_sig(i);
			if (newsig)
				*bss |= sigmask(newsig);
		}
	}
}

void
bsd_to_linux_sigset(bss, lss)
	const sigset_t *bss;
	linux_sigset_t *lss;
{
	int i, newsig;
	
	*lss = (linux_sigset_t) 0;
	for (i = 1; i <= NSIG; i++) {
		if (*bss & sigmask(i)) {
			newsig = bsd_to_linux_sig(i);
			if (newsig)
				*lss |= sigmask(newsig);
		}
	}
}

/*
 * Convert between Linux and BSD sigaction structures. Linux has
 * one extra field (sa_restorer) which we don't support. The Linux
 * SA_ONESHOT and SA_NOMASK flags (which together form the old
 * SysV signal behavior) are silently ignored. XXX
 */
void
linux_to_bsd_sigaction(lsa, bsa)
	struct linux_sigaction *lsa;
	struct sigaction *bsa;
{
	bsa->sa_handler = lsa->sa_handler;
	linux_to_bsd_sigset(&bsa->sa_mask, &lsa->sa_mask);
	bsa->sa_flags = 0;
	bsa->sa_flags |= cvtto_bsd_mask(lsa->sa_flags, LINUX_SA_NOCLDSTOP,
	    SA_NOCLDSTOP);
	bsa->sa_flags |= cvtto_bsd_mask(lsa->sa_flags, LINUX_SA_ONSTACK,
	    SA_ONSTACK);
	bsa->sa_flags |= cvtto_bsd_mask(lsa->sa_flags, LINUX_SA_RESTART,
	    SA_RESTART);
}

void
bsd_to_linux_sigaction(bsa, lsa)
	struct sigaction *bsa;
	struct linux_sigaction *lsa;
{
	lsa->sa_handler = bsa->sa_handler;
	bsd_to_linux_sigset(&lsa->sa_mask, &bsa->sa_mask);
	lsa->sa_flags = 0;
	lsa->sa_flags |= cvtto_linux_mask(bsa->sa_flags, SA_NOCLDSTOP,
	    LINUX_SA_NOCLDSTOP);
	lsa->sa_flags |= cvtto_linux_mask(bsa->sa_flags, SA_ONSTACK,
	    LINUX_SA_ONSTACK);
	lsa->sa_flags |= cvtto_linux_mask(bsa->sa_flags, SA_RESTART,
	    LINUX_SA_RESTART);
	lsa->sa_restorer = NULL;
}


/*
 * The Linux sigaction() system call. Do the usual conversions,
 * and just call sigaction(). Some flags and values are silently
 * ignored (see above).
 */
int
linux_sigaction(p, uap, retval)
	register struct proc *p;
	struct linux_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(struct linux_sigaction *) nsa;
		syscallarg(struct linux_sigaction *) osa;
	} */ *uap;
	register_t *retval;
{
	struct sigaction *nbsda = NULL, *obsda = NULL, tmpbsda;
	struct linux_sigaction *nlsa, *olsa, tmplsa;
	struct sigaction_args sa;
	caddr_t sg;
	int error;

	sg = stackgap_init();
	olsa = SCARG(uap, osa);
	nlsa = SCARG(uap, nsa);

	if (olsa != NULL)
		obsda = stackgap_alloc(&sg, sizeof (struct sigaction));

	if (nlsa != NULL) {
		nbsda = stackgap_alloc(&sg, sizeof (struct sigaction));
		if ((error = copyin(nlsa, &tmplsa, sizeof tmplsa)))
			return error;
		linux_to_bsd_sigaction(&tmplsa, &tmpbsda);
		if ((error = copyout(&tmpbsda, nbsda, sizeof tmpbsda)))
			return error;
	}

	SCARG(&sa, signum) = linux_to_bsd_sig(SCARG(uap, signum));
	SCARG(&sa, nsa) = nbsda;
	SCARG(&sa, osa) = obsda;

	if ((error = sigaction(p, &sa, retval)))
		return error;

	if (olsa != NULL) {
		if ((error = copyin(obsda, &tmpbsda, sizeof tmpbsda)))
			return error;
		bsd_to_linux_sigaction(&tmpbsda, &tmplsa);
		if ((error = copyout(&tmplsa, olsa, sizeof tmplsa)))
			return error;
	}
	return 0;
}

/*
 * The Linux signal() system call. I think that the signal() in the C
 * library actually calls sigaction, so I doubt this one is ever used.
 * But hey, it can't hurt having it here. The same restrictions as for
 * sigaction() apply.
 */
int
linux_signal(p, uap, retval)
	register struct proc *p;
	struct linux_signal_args /* {
		syscallarg(int) sig;
		syscallarg(linux_handler_t) handler;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	struct sigaction_args sa_args;
	struct sigaction *osa, *nsa, tmpsa;
	int error;

	sg = stackgap_init();
	nsa = stackgap_alloc(&sg, sizeof *nsa);
	osa = stackgap_alloc(&sg, sizeof *osa);

	tmpsa.sa_handler = SCARG(uap, handler);
	tmpsa.sa_mask = (sigset_t) 0;
	tmpsa.sa_flags = 0;
	if ((error = copyout(&tmpsa, nsa, sizeof tmpsa)))
		return error;

	SCARG(&sa_args, signum) = linux_to_bsd_sig(SCARG(uap, sig));
	SCARG(&sa_args, osa) = osa;
	SCARG(&sa_args, nsa) = nsa;
	if ((error = sigaction(p, &sa_args, retval)))
		return error;

	if ((error = copyin(osa, &tmpsa, sizeof *osa)))
		return error;
	retval[0] = (register_t) tmpsa.sa_handler;

	return 0;
}

/*
 * This is just a copy of the svr4 compat one. I feel so creative now.
 */
int
linux_sigprocmask(p, uap, retval)
	register struct proc *p;
	register struct linux_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(linux_sigset_t *) set;
		syscallarg(linux_sigset_t * oset;
	} */ *uap;
	register_t *retval;
{
	linux_sigset_t ss;
	sigset_t bs;
	int error = 0;

	*retval = 0;

	if (SCARG(uap, oset) != NULL) {
		/* Fix the return value first if needed */
		bsd_to_linux_sigset(&p->p_sigmask, &ss);
		if ((error = copyout(&ss, SCARG(uap, oset), sizeof(ss))) != 0)
			return error;
	}

	if (SCARG(uap, set) == NULL)
		/* Just examine */
		return 0;

	if ((error = copyin(SCARG(uap, set), &ss, sizeof(ss))) != 0)
		return error;

	linux_to_bsd_sigset(&ss, &bs);

	(void) splhigh();

	switch (SCARG(uap, how)) {
	case LINUX_SIG_BLOCK:
		p->p_sigmask |= bs & ~sigcantmask;
		break;

	case LINUX_SIG_UNBLOCK:
		p->p_sigmask &= ~bs;
		break;

	case LINUX_SIG_SETMASK:
		p->p_sigmask = bs & ~sigcantmask;
		break;

	default:
		error = EINVAL;
		break;
	}

	(void) spl0();

	return error;
}

/*
 * The functions below really make no distinction between an int
 * and [linux_]sigset_t. This is ok for now, but it might break
 * sometime. Then again, sigset_t is trusted to be an int everywhere
 * else in the kernel too.
 */
/* ARGSUSED */
int
linux_siggetmask(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{
	bsd_to_linux_sigset(&p->p_sigmask, (linux_sigset_t *) retval);
	return 0;
}

/*
 * The following three functions fiddle with a process' signal mask.
 * Convert the signal masks because of the different signal
 * values for Linux. The need for this is the reason why
 * they are here, and have not been mapped directly.
 */
int
linux_sigsetmask(p, uap, retval)
	struct proc *p;
	struct linux_sigsetmask_args /* {
		syscallarg(linux_sigset_t) mask;
	} */ *uap;
	register_t *retval;
{
	linux_sigset_t mask;
	sigset_t bsdsig;

	bsd_to_linux_sigset(&p->p_sigmask, (linux_sigset_t *) retval);

	mask = SCARG(uap, mask);
	bsd_to_linux_sigset(&mask, &bsdsig);

	splhigh();
	p->p_sigmask = bsdsig & ~sigcantmask;
	spl0();

	return 0;
}

int
linux_sigpending(p, uap, retval)
	struct proc *p;
	struct linux_sigpending_args /* {
		syscallarg(linux_sigset_t *) mask;
	} */ *uap;
	register_t *retval;
{
	sigset_t bsdsig;
	linux_sigset_t linuxsig;

	bsdsig = p->p_siglist & p->p_sigmask;

	bsd_to_linux_sigset(&bsdsig, &linuxsig);
	return copyout(&linuxsig, SCARG(uap, mask), sizeof linuxsig);
}

int
linux_sigsuspend(p, uap, retval)
	struct proc *p;
	struct linux_sigsuspend_args /* {
		syscallarg(int) mask;
	} */ *uap;
	register_t *retval;
{
	struct sigsuspend_args ssa;

	linux_to_bsd_sigset(&SCARG(uap, mask), &SCARG(&ssa, mask));
	return sigsuspend(p, &ssa, retval);
}

/*
 * Once more: only a signal conversion is needed.
 */
int
linux_kill(p, uap, retval)
	struct proc *p;
	struct linux_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap;
	register_t *retval;
{
	SCARG(uap, signum) = linux_to_bsd_sig(SCARG(uap, signum));
	return kill(p, (struct kill_args *) uap, retval);
}
