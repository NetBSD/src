/*	$NetBSD: linux_signal.c,v 1.12 1998/09/11 12:50:09 mycroft Exp $	*/

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
#include <compat/linux/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>

#define	linux_sigmask(n)	(1 << ((n) - 1))
#define	linux_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define	linux_sigismember(s, n)	(*(s) & linux_sigmask(n))
#define	linux_sigaddset(s, n)	(*(s) |= linux_sigmask(n))

int native_to_linux_sig[NSIG] = {
	0,
	LINUX_SIGHUP,
	LINUX_SIGINT,
	LINUX_SIGQUIT,
	LINUX_SIGILL,
	LINUX_SIGTRAP,
	LINUX_SIGABRT,
	0,			/* SIGEMT */
	LINUX_SIGFPE,
	LINUX_SIGKILL,
	LINUX_SIGBUS,
	LINUX_SIGSEGV,
	0,			/* SIGSEGV */
	LINUX_SIGPIPE,
	LINUX_SIGALRM,
	LINUX_SIGTERM,
	LINUX_SIGURG,
	LINUX_SIGSTOP,
	LINUX_SIGTSTP,
	LINUX_SIGCONT,
	LINUX_SIGCHLD,
	LINUX_SIGTTIN,
	LINUX_SIGTTOU,
	LINUX_SIGIO,
	LINUX_SIGXCPU,
	LINUX_SIGXFSZ,
	LINUX_SIGVTALRM,
	LINUX_SIGPROF,
	LINUX_SIGWINCH,
	0,			/* SIGINFO */
	LINUX_SIGUSR1,
	LINUX_SIGUSR2,
	LINUX_SIGPWR,
};

int linux_to_native_sig[LINUX_NSIG] = {
	0,
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGBUS,
	SIGFPE,
	SIGKILL,
	SIGUSR1,
	SIGSEGV,
	SIGUSR2,
	SIGPIPE,
	SIGALRM,
	SIGTERM,
	0,			/* SIGSTKFLT */
	SIGCHLD,
	SIGCONT,
	SIGSTOP,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	SIGURG,
	SIGXCPU,
	SIGXFSZ,
	SIGVTALRM,
	SIGPROF,
	SIGWINCH,
	SIGIO,
	SIGPWR,
	0,			/* SIGUNUSED */
};

/* linux_signal.c */
void linux_to_native_sigaction __P((struct linux_sigaction *, struct sigaction *));
void native_to_linux_sigaction __P((struct sigaction *, struct linux_sigaction *));

void
linux_to_native_sigset(lss, bss)
	const linux_sigset_t *lss;
	sigset_t *bss;
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < LINUX_NSIG; i++) {
		if (linux_sigismember(lss, i)) {
			newsig = linux_to_native_sig[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}

void
native_to_linux_sigset(bss, lss)
	const sigset_t *bss;
	linux_sigset_t *lss;
{
	int i, newsig;
	
	linux_sigemptyset(lss);
	for (i = 1; i < NSIG; i++) {
		if (sigismember(bss, i)) {
			newsig = native_to_linux_sig[i];
			if (newsig)
				linux_sigaddset(lss, newsig);
		}
	}
}

/*
 * Convert between Linux and BSD sigaction structures. Linux has
 * one extra field (sa_restorer) which we don't support.
 */
void
linux_to_native_sigaction(lsa, bsa)
	struct linux_sigaction *lsa;
	struct sigaction *bsa;
{

	bsa->sa_handler = lsa->sa_handler;
	linux_to_native_sigset(&lsa->sa_mask, &bsa->sa_mask);
	bsa->sa_flags = 0;
	if ((lsa->sa_flags & LINUX_SA_NOCLDSTOP) != 0)
		bsa->sa_flags |= SA_NOCLDSTOP;
	if ((lsa->sa_flags & LINUX_SA_ONSTACK) != 0)
		bsa->sa_flags |= SA_ONSTACK;
	if ((lsa->sa_flags & LINUX_SA_RESTART) != 0)
		bsa->sa_flags |= SA_RESTART;
	if ((lsa->sa_flags & LINUX_SA_ONESHOT) != 0)
		bsa->sa_flags |= SA_RESETHAND;
	if ((lsa->sa_flags & LINUX_SA_NOMASK) != 0)
		bsa->sa_flags |= SA_NODEFER;
	if ((lsa->sa_flags & ~LINUX_SA_ALLBITS) != 0)
/*XXX*/		printf("linux_to_native_sigaction: extra bits ignored\n");
	if (lsa->sa_restorer != 0)
/*XXX*/		printf("linux_to_native_sigaction: sa_restorer ignored\n");
}

void
native_to_linux_sigaction(bsa, lsa)
	struct sigaction *bsa;
	struct linux_sigaction *lsa;
{

	lsa->sa_handler = bsa->sa_handler;
	native_to_linux_sigset(&bsa->sa_mask, &lsa->sa_mask);
	lsa->sa_flags = 0;
	if ((bsa->sa_flags & SA_NOCLDSTOP) != 0)
		lsa->sa_flags |= LINUX_SA_NOCLDSTOP;
	if ((bsa->sa_flags & SA_ONSTACK) != 0)
		lsa->sa_flags |= LINUX_SA_ONSTACK;
	if ((bsa->sa_flags & SA_RESTART) != 0)
		lsa->sa_flags |= LINUX_SA_RESTART;
	if ((bsa->sa_flags & SA_NODEFER) != 0)
		lsa->sa_flags |= LINUX_SA_NOMASK;
	if ((bsa->sa_flags & SA_RESETHAND) != 0)
		lsa->sa_flags |= LINUX_SA_ONESHOT;
	lsa->sa_restorer = NULL;
}


/*
 * The Linux sigaction() system call. Do the usual conversions,
 * and just call sigaction(). Some flags and values are silently
 * ignored (see above).
 */
int
linux_sys_sigaction(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct linux_sigaction *) nsa;
		syscallarg(struct linux_sigaction *) osa;
	} */ *uap = v;
	struct linux_sigaction nlsa, olsa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nlsa, sizeof(nlsa));
		if (error)
			return (error);
		linux_to_native_sigaction(&nlsa, &nbsa);
	}
	error = sigaction1(p, linux_to_native_sig[SCARG(uap, signum)],
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		native_to_linux_sigaction(&obsa, &olsa);
		error = copyout(&olsa, SCARG(uap, osa), sizeof(olsa));
		if (error)
			return (error);
	}
	return (0);
}

/*
 * The Linux signal() system call. I think that the signal() in the C
 * library actually calls sigaction, so I doubt this one is ever used.
 * But hey, it can't hurt having it here. The same restrictions as for
 * sigaction() apply.
 */
int
linux_sys_signal(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_signal_args /* {
		syscallarg(int) sig;
		syscallarg(linux_handler_t) handler;
	} */ *uap = v;
	struct sigaction nbsa, obsa;
	int error;

	nbsa.sa_handler = SCARG(uap, handler);
	sigemptyset(&nbsa.sa_mask);
	nbsa.sa_flags = SA_RESETHAND | SA_NODEFER;
	error = sigaction1(p, linux_to_native_sig[SCARG(uap, sig)],
	    &nbsa, &obsa);
	if (error)
		return (error);
	*retval = (int)obsa.sa_handler;
	return (0);
}

int
linux_sys_sigprocmask(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(const linux_sigset_t *) set;
		syscallarg(linux_sigset_t *) oset;
	} */ *uap = v;
	linux_sigset_t nlss, olss;
	sigset_t nbss, obss;
	int how;
	int error;

	switch (SCARG(uap, how)) {
	case LINUX_SIG_BLOCK:
		how = SIG_BLOCK;
		break;
	case LINUX_SIG_UNBLOCK:
		how = SIG_UNBLOCK;
		break;
	case LINUX_SIG_SETMASK:
		how = SIG_SETMASK;
		break;
	default:
		return (EINVAL);
	}

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &nlss, sizeof(nlss));
		if (error)
			return (error);
		linux_to_native_sigset(&nlss, &nbss);
	}
	error = sigprocmask1(p, how,
	    SCARG(uap, set) ? &nbss : 0, SCARG(uap, oset) ? &obss : 0);
	if (error)
		return (error); 
	if (SCARG(uap, oset)) {
		native_to_linux_sigset(&obss, &olss);
		error = copyout(&olss, SCARG(uap, oset), sizeof(olss));
		if (error)
			return (error);
	}       
	return (error);
}

/* ARGSUSED */
int
linux_sys_siggetmask(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	sigset_t bss;
	linux_sigset_t lss;
	int error;

	error = sigprocmask1(p, SIG_SETMASK, 0, &bss);
	if (error)
		return (error);
	native_to_linux_sigset(&bss, &lss);
	*retval = lss;
	return (0);
}

/*
 * The following three functions fiddle with a process' signal mask.
 * Convert the signal masks because of the different signal
 * values for Linux. The need for this is the reason why
 * they are here, and have not been mapped directly.
 */
int
linux_sys_sigsetmask(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigsetmask_args /* {
		syscallarg(linux_sigset_t) mask;
	} */ *uap = v;
	sigset_t nbss, obss;
	linux_sigset_t nlss, olss;
	int error;

	nlss = SCARG(uap, mask);
	linux_to_native_sigset(&nlss, &nbss);
	error = sigprocmask1(p, SIG_SETMASK, &nbss, &obss);
	if (error)
		return (error);
	native_to_linux_sigset(&obss, &olss);
	*retval = olss;
	return (0);
}

int
linux_sys_sigpending(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigpending_args /* {
		syscallarg(linux_sigset_t *) set;
	} */ *uap = v;
	sigset_t bss;
	linux_sigset_t lss;

	sigpending1(p, &bss);
	native_to_linux_sigset(&bss, &lss);
	return copyout(&lss, SCARG(uap, set), sizeof(lss));
}

int
linux_sys_sigsuspend(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigsuspend_args /* {
		syscallarg(caddr_t) restart;
		syscallarg(int) oldmask;
		syscallarg(int) mask;
	} */ *uap = v;
	linux_sigset_t lss;
	sigset_t bss;
	
	lss = SCARG(uap, mask);
	linux_to_native_sigset(&lss, &bss);
	return (sigsuspend1(p, &bss));
}

/*
 * The deprecated pause(2), which is really just an instance
 * of sigsuspend(2).
 */
int
linux_sys_pause(p, v, retval)
	register struct proc *p;
	void *v;	
	register_t *retval;
{	

	return (sigsuspend1(p, 0));
}

/*
 * Once more: only a signal conversion is needed.
 */
int
linux_sys_kill(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap = v;
	struct sys_kill_args ka;

	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = linux_to_native_sig[SCARG(uap, signum)];
	return sys_kill(p, &ka, retval);
}
