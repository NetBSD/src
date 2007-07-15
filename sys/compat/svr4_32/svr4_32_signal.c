/*	$NetBSD: svr4_32_signal.c,v 1.19.2.2 2007/07/15 13:27:20 ad Exp $	 */

/*-
 * Copyright (c) 1994, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_signal.c,v 1.19.2.2 2007/07/15 13:27:20 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_svr4.h"
#endif

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
#include <sys/wait.h>

#include <sys/syscallargs.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4_32/svr4_32_lwp.h>
#include <compat/svr4_32/svr4_32_ucontext.h>
#include <compat/svr4_32/svr4_32_syscallargs.h>
#include <compat/svr4_32/svr4_32_util.h>

#include <compat/common/compat_sigaltstack.h>

#define	svr4_sigmask(n)		(1 << (((n) - 1) & 31))
#define	svr4_sigword(n)		(((n) - 1) >> 5)
#define svr4_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define	svr4_sigismember(s, n)	((s)->bits[svr4_sigword(n)] & svr4_sigmask(n))
#define	svr4_sigaddset(s, n)	((s)->bits[svr4_sigword(n)] |= svr4_sigmask(n))

static inline void svr4_32_sigfillset __P((svr4_32_sigset_t *));
void svr4_32_to_native_sigaction __P((const struct svr4_32_sigaction *,
				struct sigaction *));
void native_to_svr4_32_sigaction __P((const struct sigaction *,
				struct svr4_32_sigaction *));

#ifndef COMPAT_SVR4
const int native_to_svr4_signo[NSIG] = {
	0,			/* 0 */
	SVR4_SIGHUP,		/* 1 */
	SVR4_SIGINT,		/* 2 */
	SVR4_SIGQUIT,		/* 3 */
	SVR4_SIGILL,		/* 4 */
	SVR4_SIGTRAP,		/* 5 */
	SVR4_SIGABRT,		/* 6 */
	SVR4_SIGEMT,		/* 7 */
	SVR4_SIGFPE,		/* 8 */
	SVR4_SIGKILL,		/* 9 */
	SVR4_SIGBUS,		/* 10 */
	SVR4_SIGSEGV,		/* 11 */
	SVR4_SIGSYS,		/* 12 */
	SVR4_SIGPIPE,		/* 13 */
	SVR4_SIGALRM,		/* 14 */
	SVR4_SIGTERM,		/* 15 */
	SVR4_SIGURG,		/* 16 */
	SVR4_SIGSTOP,		/* 17 */
	SVR4_SIGTSTP,		/* 18 */
	SVR4_SIGCONT,		/* 19 */
	SVR4_SIGCHLD,		/* 20 */
	SVR4_SIGTTIN,		/* 21 */
	SVR4_SIGTTOU,		/* 22 */
	SVR4_SIGIO,		/* 23 */
	SVR4_SIGXCPU,		/* 24 */
	SVR4_SIGXFSZ,		/* 25 */
	SVR4_SIGVTALRM,		/* 26 */
	SVR4_SIGPROF,		/* 27 */
	SVR4_SIGWINCH,		/* 28 */
	0,			/* 29 SIGINFO */
	SVR4_SIGUSR1,		/* 30 */
	SVR4_SIGUSR2,		/* 31 */
	SVR4_SIGPWR,		/* 32 */
	SVR4_SIGRTMIN + 0,	/* 33 */
	SVR4_SIGRTMIN + 1,	/* 34 */
	SVR4_SIGRTMIN + 2,	/* 35 */
	SVR4_SIGRTMIN + 3,	/* 36 */
	SVR4_SIGRTMIN + 4,	/* 37 */
	SVR4_SIGRTMIN + 5,	/* 38 */
	SVR4_SIGRTMIN + 6,	/* 39 */
	SVR4_SIGRTMIN + 7,	/* 40 */
	SVR4_SIGRTMIN + 8,	/* 41 */
	SVR4_SIGRTMIN + 9,	/* 42 */
	SVR4_SIGRTMIN + 10,	/* 43 */
	SVR4_SIGRTMIN + 11,	/* 44 */
	SVR4_SIGRTMIN + 12,	/* 45 */
	SVR4_SIGRTMIN + 13,	/* 46 */
	SVR4_SIGRTMIN + 14,	/* 47 */
	SVR4_SIGRTMIN + 15,	/* 48 */
	SVR4_SIGRTMIN + 16,	/* 49 */
	SVR4_SIGRTMIN + 17,	/* 50 */
	SVR4_SIGRTMIN + 18,	/* 51 */
	SVR4_SIGRTMIN + 19,	/* 52 */
	SVR4_SIGRTMIN + 20,	/* 53 */
	SVR4_SIGRTMIN + 21,	/* 54 */
	SVR4_SIGRTMIN + 22,	/* 55 */
	SVR4_SIGRTMIN + 23,	/* 56 */
	SVR4_SIGRTMIN + 24,	/* 57 */
	SVR4_SIGRTMIN + 25,	/* 58 */
	SVR4_SIGRTMIN + 26,	/* 59 */
	SVR4_SIGRTMIN + 27,	/* 60 */
	SVR4_SIGRTMIN + 28,	/* 61 */
	SVR4_SIGRTMIN + 29,	/* 62 */
	SVR4_SIGRTMIN + 30,	/* 63 */
};

const int svr4_to_native_signo[SVR4_NSIG] = {
	0,			/* 0 */
	SIGHUP,			/* 1 */
	SIGINT,			/* 2 */
	SIGQUIT,		/* 3 */
	SIGILL,			/* 4 */
	SIGTRAP,		/* 5 */
	SIGABRT,		/* 6 */
	SIGEMT,			/* 7 */
	SIGFPE,			/* 8 */
	SIGKILL,		/* 9 */
	SIGBUS,			/* 10 */
	SIGSEGV,		/* 11 */
	SIGSYS,			/* 12 */
	SIGPIPE,		/* 13 */
	SIGALRM,		/* 14 */
	SIGTERM,		/* 15 */
	SIGUSR1,		/* 16 */
	SIGUSR2,		/* 17 */
	SIGCHLD,		/* 18 */
	SIGPWR,			/* 19 */
	SIGWINCH,		/* 20 */
	SIGURG,			/* 21 */
	SIGIO,			/* 22 */
	SIGSTOP,		/* 23 */
	SIGTSTP,		/* 24 */
	SIGCONT,		/* 25 */
	SIGTTIN,		/* 26 */
	SIGTTOU,		/* 27 */
	SIGVTALRM,		/* 28 */
	SIGPROF,		/* 29 */
	SIGXCPU,		/* 30 */
	SIGXFSZ,		/* 31 */
	SIGRTMIN + 0,		/* 32 */
	SIGRTMIN + 1,		/* 33 */
	SIGRTMIN + 2,		/* 34 */
	SIGRTMIN + 3,		/* 35 */
	SIGRTMIN + 4,		/* 36 */
	SIGRTMIN + 5,		/* 37 */
	SIGRTMIN + 6,		/* 38 */
	SIGRTMIN + 7,		/* 39 */
	SIGRTMIN + 8,		/* 40 */
	SIGRTMIN + 9,		/* 41 */
	SIGRTMIN + 10,		/* 42 */
	SIGRTMIN + 11,		/* 43 */
	SIGRTMIN + 12,		/* 44 */
	SIGRTMIN + 13,		/* 45 */
	SIGRTMIN + 14,		/* 46 */
	SIGRTMIN + 15,		/* 47 */
	SIGRTMIN + 16,		/* 48 */
	SIGRTMIN + 17,		/* 49 */
	SIGRTMIN + 18,		/* 50 */
	SIGRTMIN + 19,		/* 51 */
	SIGRTMIN + 20,		/* 52 */
	SIGRTMIN + 21,		/* 53 */
	SIGRTMIN + 22,		/* 54 */
	SIGRTMIN + 23,		/* 55 */
	SIGRTMIN + 24,		/* 56 */
	SIGRTMIN + 25,		/* 57 */
	SIGRTMIN + 26,		/* 58 */
	SIGRTMIN + 27,		/* 59 */
	SIGRTMIN + 28,		/* 60 */
	SIGRTMIN + 29,		/* 61 */
	SIGRTMIN + 30,		/* 62 */
	0,			/* 63 */
};
#endif

static inline void
svr4_32_sigfillset(s)
	svr4_32_sigset_t *s;
{
	int i;

	svr4_sigemptyset(s);
	for (i = 1; i < SVR4_NSIG; i++)
		if (svr4_to_native_signo[i] != 0)
			svr4_sigaddset(s, i);
}

void
svr4_32_to_native_sigset(sss, bss)
	const svr4_32_sigset_t *sss;
	sigset_t *bss;
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < SVR4_NSIG; i++) {
		if (svr4_sigismember(sss, i)) {
			newsig = svr4_to_native_signo[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}


void
native_to_svr4_32_sigset(bss, sss)
	const sigset_t *bss;
	svr4_32_sigset_t *sss;
{
	int i, newsig;

	svr4_sigemptyset(sss);
	for (i = 1; i < NSIG; i++) {
		if (sigismember(bss, i)) {
			newsig = native_to_svr4_signo[i];
			if (newsig)
				svr4_sigaddset(sss, newsig);
		}
	}
}

/*
 * XXX: Only a subset of the flags is currently implemented.
 */
void
svr4_32_to_native_sigaction(ssa, bsa)
	const struct svr4_32_sigaction *ssa;
	struct sigaction *bsa;
{

	bsa->sa_handler = NETBSD32PTR64(ssa->svr4_32_sa_handler);
	svr4_32_to_native_sigset(&ssa->svr4_32_sa_mask, &bsa->sa_mask);
	bsa->sa_flags = 0;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_ONSTACK) != 0)
		bsa->sa_flags |= SA_ONSTACK;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_RESETHAND) != 0)
		bsa->sa_flags |= SA_RESETHAND;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_RESTART) != 0)
		bsa->sa_flags |= SA_RESTART;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_SIGINFO) != 0) {
		DPRINTF(("svr4_to_native_sigaction: SA_SIGINFO ignored\n"));
	}
	if ((ssa->svr4_32_sa_flags & SVR4_SA_NODEFER) != 0)
		bsa->sa_flags |= SA_NODEFER;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_NOCLDWAIT) != 0)
		bsa->sa_flags |= SA_NOCLDWAIT;
	if ((ssa->svr4_32_sa_flags & SVR4_SA_NOCLDSTOP) != 0)
		bsa->sa_flags |= SA_NOCLDSTOP;
	if ((ssa->svr4_32_sa_flags & ~SVR4_SA_ALLBITS) != 0) {
		DPRINTF(("svr4_32_to_native_sigaction: extra bits %x ignored\n",
		    ssa->svr4_32_sa_flags & ~SVR4_SA_ALLBITS));
	}
}

void
native_to_svr4_32_sigaction(bsa, ssa)
	const struct sigaction *bsa;
	struct svr4_32_sigaction *ssa;
{

	NETBSD32PTR32(ssa->svr4_32_sa_handler, bsa->sa_handler);
	native_to_svr4_32_sigset(&bsa->sa_mask, &ssa->svr4_32_sa_mask);
	ssa->svr4_32_sa_flags = 0;
	if ((bsa->sa_flags & SA_ONSTACK) != 0)
		ssa->svr4_32_sa_flags |= SVR4_SA_ONSTACK;
	if ((bsa->sa_flags & SA_RESETHAND) != 0)
		ssa->svr4_32_sa_flags |= SVR4_SA_RESETHAND;
	if ((bsa->sa_flags & SA_RESTART) != 0)
		ssa->svr4_32_sa_flags |= SVR4_SA_RESTART;
	if ((bsa->sa_flags & SA_NODEFER) != 0)
		ssa->svr4_32_sa_flags |= SVR4_SA_NODEFER;
	if ((bsa->sa_flags & SA_NOCLDSTOP) != 0)
		ssa->svr4_32_sa_flags |= SVR4_SA_NOCLDSTOP;
}

int
svr4_32_sys_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct svr4_32_sigaction *) nsa;
		syscallarg(struct svr4_32_sigaction *) osa;
	} */ *uap = v;
	struct svr4_32_sigaction nssa, ossa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG_P32(uap, nsa)) {
		error = copyin(SCARG_P32(uap, nsa),
			       &nssa, sizeof(nssa));
		if (error)
			return (error);
		svr4_32_to_native_sigaction(&nssa, &nbsa);
	}
	error = sigaction1(l,
			   svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))],
	    SCARG_P32(uap, nsa) ? &nbsa : 0, SCARG_P32(uap, osa) ? &obsa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG_P32(uap, osa)) {
		native_to_svr4_32_sigaction(&obsa, &ossa);
		error = copyout(&ossa, SCARG_P32(uap, osa),
				sizeof(ossa));
		if (error)
			return (error);
	}
	return (0);
}

int
svr4_32_sys_sigaltstack(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sigaltstack_args /* {
		syscallarg(const struct svr4_32_sigaltstack_tp) nss;
		syscallarg(struct svr4_32_sigaltstack_tp) oss;
	} */ *uap = v;
	compat_sigaltstack(uap, svr4_32_sigaltstack,
	    SVR4_SS_ONSTACK, SVR4_SS_DISABLE);
}

/*
 * Stolen from the ibcs2 one
 */
int
svr4_32_sys_signal(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_signal_args /* {
		syscallarg(int) signum;
		syscallarg(svr4_32_sig_t) handler;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int signum = svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))];
	struct sigaction nbsa, obsa;
	sigset_t ss;
	int error;

	if (signum <= 0 || signum >= SVR4_NSIG)
		return (EINVAL);

	switch (SVR4_SIGCALL(SCARG(uap, signum))) {
	case SVR4_SIGDEFER_MASK:
		if (SCARG(uap, handler) == SVR4_SIG_HOLD)
			goto sighold;
		/* FALLTHROUGH */

	case SVR4_SIGNAL_MASK:
		nbsa.sa_handler = (sig_t)SCARG(uap, handler);
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		error = sigaction1(l, signum, &nbsa, &obsa, NULL, 0);
		if (error)
			return (error);
		*retval = (u_int)(u_long)obsa.sa_handler;
		return (0);

	case SVR4_SIGHOLD_MASK:
	sighold:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		mutex_enter(&p->p_smutex);
		error = sigprocmask1(l, SIG_BLOCK, &ss, 0);
		mutex_exit(&p->p_smutex);
		return error;

	case SVR4_SIGRELSE_MASK:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		mutex_enter(&p->p_smutex);
		error = sigprocmask1(l, SIG_UNBLOCK, &ss, 0);
		mutex_exit(&p->p_smutex);
		return error;

	case SVR4_SIGIGNORE_MASK:
		nbsa.sa_handler = SIG_IGN;
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		return (sigaction1(l, signum, &nbsa, 0, NULL, 0));

	case SVR4_SIGPAUSE_MASK:
		mutex_enter(&p->p_smutex);
		ss = l->l_sigmask;
		mutex_exit(&p->p_smutex);
		sigdelset(&ss, signum);
		return (sigsuspend1(l, &ss));

	default:
		return (ENOSYS);
	}
}

int
svr4_32_sys_sigprocmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(const svr4_32_sigset_t *) set;
		syscallarg(svr4_32_sigset_t *) oset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	svr4_32_sigset_t nsss, osss;
	sigset_t nbss, obss;
	int how;
	int error;

	/*
	 * Initialize how to 0 to avoid a compiler warning.  Note that
	 * this is safe because of the check in the default: case.
	 */
	how = 0;

	switch (SCARG(uap, how)) {
	case SVR4_SIG_BLOCK:
		how = SIG_BLOCK;
		break;
	case SVR4_SIG_UNBLOCK:
		how = SIG_UNBLOCK;
		break;
	case SVR4_SIG_SETMASK:
		how = SIG_SETMASK;
		break;
	default:
		if (SCARG_P32(uap, set))
			return EINVAL;
		break;
	}

	if (SCARG_P32(uap, set)) {
		error = copyin(SCARG_P32(uap, set),
			       &nsss, sizeof(nsss));
		if (error)
			return error;
		svr4_32_to_native_sigset(&nsss, &nbss);
	}
	mutex_enter(&p->p_smutex);
	error = sigprocmask1(l, how,
	    SCARG_P32(uap, set) ? &nbss : NULL, SCARG_P32(uap, oset) ? &obss : NULL);
	mutex_exit(&p->p_smutex);
	if (error)
		return error;
	if (SCARG_P32(uap, oset)) {
		native_to_svr4_32_sigset(&obss, &osss);
		error = copyout(&osss, SCARG_P32(uap, oset),
				sizeof(osss));
		if (error)
			return error;
	}
	return 0;
}

int
svr4_32_sys_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sigpending_args /* {
		syscallarg(int) what;
		syscallarg(svr4_sigset_t *) set;
	} */ *uap = v;
	sigset_t bss;
	svr4_32_sigset_t sss;

	switch (SCARG(uap, what)) {
	case 1:	/* sigpending */
		sigpending1(l, &bss);
		native_to_svr4_32_sigset(&bss, &sss);
		break;

	case 2:	/* sigfillset */
		svr4_32_sigfillset(&sss);
		break;

	default:
		return (EINVAL);
	}
	return (copyout(&sss, SCARG_P32(uap, set), sizeof(sss)));
}

int
svr4_32_sys_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sigsuspend_args /* {
		syscallarg(const svr4_32_sigset_t *) set;
	} */ *uap = v;
	svr4_32_sigset_t sss;
	sigset_t bss;
	int error;

	if (SCARG_P32(uap, set)) {
		error = copyin(SCARG_P32(uap, set), &sss, sizeof(sss));
		if (error)
			return (error);
		svr4_32_to_native_sigset(&sss, &bss);
	}

	return (sigsuspend1(l, SCARG_P32(uap, set) ? &bss : 0));
}

int
svr4_32_sys_pause(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (sigsuspend1(l, 0));
}

int
svr4_32_sys_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap = v;
	struct sys_kill_args ka;

	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))];
	return sys_kill(l, &ka, retval);
}

void
svr4_32_getcontext(l, uc, mask)
	struct lwp *l;
	struct svr4_32_ucontext *uc;
	const sigset_t *mask;
{
	void *sp;
	struct svr4_32_sigaltstack *ss = &uc->uc_stack;

	memset(uc, 0, sizeof(*uc));

	/* get machine context */
	sp = svr4_32_getmcontext(l, &uc->uc_mcontext, &uc->uc_flags);

	/* get link */
	NETBSD32PTR32(uc->uc_link, l->l_ctxlink);

	/* get stack state. XXX: solaris appears to do this */
#if 0
	svr4_32_to_native_sigaltstack(&uc->uc_stack, &p->p_sigacts->ps_sigstk);
#else
	NETBSD32PTR32(ss->ss_sp, (void *)(((u_long) sp) & ~(16384 - 1)));
	ss->ss_size = 16384;
	ss->ss_flags = 0;
#endif
	/* get signal mask */
	mutex_enter(&l->l_proc->p_smutex);
	native_to_svr4_32_sigset(mask, &uc->uc_sigmask);
	mutex_exit(&l->l_proc->p_smutex);

	uc->uc_flags |= SVR4_UC_STACK|SVR4_UC_SIGMASK;
}


int
svr4_32_setcontext(l, uc)
	struct lwp *l;
	struct svr4_32_ucontext *uc;
{
	int error;
	struct proc *p = l->l_proc;

	/* set machine context */
	if ((error = svr4_32_setmcontext(l, &uc->uc_mcontext, uc->uc_flags)) != 0)
		return error;

	/* set link */
	l->l_ctxlink = NETBSD32PTR64(uc->uc_link);

	mutex_enter(&p->p_smutex);

	/* set signal stack */
	if (uc->uc_flags & SVR4_UC_STACK) {
		l->l_sigstk.ss_sp = NETBSD32PTR64(uc->uc_stack.ss_sp);
		l->l_sigstk.ss_size = uc->uc_stack.ss_size;
		l->l_sigstk.ss_flags =
		    (uc->uc_stack.ss_flags & SVR4_SS_ONSTACK ? SS_ONSTACK : 0) |
		    (uc->uc_stack.ss_flags & SVR4_SS_DISABLE ? SS_DISABLE : 0);
	}

	/* set signal mask */
	if (uc->uc_flags & SVR4_UC_SIGMASK) {
		sigset_t mask;

		svr4_32_to_native_sigset(&uc->uc_sigmask, &mask);
		(void)sigprocmask1(l, SIG_SETMASK, &mask, 0);
	}

	mutex_exit(&p->p_smutex);

	return EJUSTRETURN;
}

int
svr4_32_sys_context(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_context_args /* {
		syscallarg(int) func;
		syscallarg(struct svr4_32_ucontext *) uc;
	} */ *uap = v;
	struct svr4_32_ucontext uc;
	int error;
	*retval = 0;

	switch (SCARG(uap, func)) {
	case SVR4_GETCONTEXT:
		DPRINTF(("getcontext(%p)\n", SCARG(uap, uc)));
		svr4_32_getcontext(l, &uc, &l->l_sigmask);
		return copyout(&uc, SCARG_P32(uap, uc), sizeof(uc));

	case SVR4_SETCONTEXT:
		DPRINTF(("setcontext(%p)\n", SCARG(uap, uc)));
		if (!SCARG_P32(uap, uc))
			exit1(l, W_EXITCODE(0, 0));
		else if ((error = copyin(SCARG_P32(uap, uc),
					 &uc, sizeof(uc))) != 0)
			return error;
		else
			return svr4_32_setcontext(l, &uc);

	default:
		DPRINTF(("context(%d, %p)\n", SCARG(uap, func),
		    SCARG(uap, uc)));
		return ENOSYS;
	}
	return 0;
}
