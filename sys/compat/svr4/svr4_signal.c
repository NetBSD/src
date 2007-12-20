/*	$NetBSD: svr4_signal.c,v 1.62 2007/12/20 23:03:05 dsl Exp $	 */

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
__KERNEL_RCSID(0, "$NetBSD: svr4_signal.c,v 1.62 2007/12/20 23:03:05 dsl Exp $");

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

#include <uvm/uvm_extern.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>

#include <compat/common/compat_sigaltstack.h>

#define	svr4_sigmask(n)		(1 << (((n) - 1) & 31))
#define	svr4_sigword(n)		(((n) - 1) >> 5)
#define svr4_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define	svr4_sigismember(s, n)	((s)->bits[svr4_sigword(n)] & svr4_sigmask(n))
#define	svr4_sigaddset(s, n)	((s)->bits[svr4_sigword(n)] |= svr4_sigmask(n))

static inline void svr4_sigfillset(svr4_sigset_t *);
void svr4_to_native_sigaction(const struct svr4_sigaction *,
				struct sigaction *);
void native_to_svr4_sigaction(const struct sigaction *,
				struct svr4_sigaction *);

extern const int native_to_svr4_signo[];
extern const int svr4_to_native_signo[];

static inline void
svr4_sigfillset(svr4_sigset_t *s)
{
	int i;

	svr4_sigemptyset(s);
	for (i = 1; i < SVR4_NSIG; i++)
		if (svr4_to_native_signo[i] != 0)
			svr4_sigaddset(s, i);
}

void
svr4_to_native_sigset(const svr4_sigset_t *sss, sigset_t *bss)
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
native_to_svr4_sigset(const sigset_t *bss, svr4_sigset_t *sss)
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
svr4_to_native_sigaction(const struct svr4_sigaction *ssa, struct sigaction *bsa)
{

	bsa->sa_handler = (sig_t) ssa->svr4_sa_handler;
	svr4_to_native_sigset(&ssa->svr4_sa_mask, &bsa->sa_mask);
	bsa->sa_flags = 0;
	if ((ssa->svr4_sa_flags & SVR4_SA_ONSTACK) != 0)
		bsa->sa_flags |= SA_ONSTACK;
	if ((ssa->svr4_sa_flags & SVR4_SA_RESETHAND) != 0)
		bsa->sa_flags |= SA_RESETHAND;
	if ((ssa->svr4_sa_flags & SVR4_SA_RESTART) != 0)
		bsa->sa_flags |= SA_RESTART;
	if ((ssa->svr4_sa_flags & SVR4_SA_SIGINFO) != 0)
		bsa->sa_flags |= SA_SIGINFO;
	if ((ssa->svr4_sa_flags & SVR4_SA_NODEFER) != 0)
		bsa->sa_flags |= SA_NODEFER;
	if ((ssa->svr4_sa_flags & SVR4_SA_NOCLDWAIT) != 0)
		bsa->sa_flags |= SA_NOCLDWAIT;
	if ((ssa->svr4_sa_flags & SVR4_SA_NOCLDSTOP) != 0)
		bsa->sa_flags |= SA_NOCLDSTOP;
	if ((ssa->svr4_sa_flags & ~SVR4_SA_ALLBITS) != 0) {
		DPRINTF(("svr4_to_native_sigaction: extra bits %x ignored\n",
		    ssa->svr4_sa_flags & ~SVR4_SA_ALLBITS));
	}
}

void
native_to_svr4_sigaction(const struct sigaction *bsa, struct svr4_sigaction *ssa)
{

	ssa->svr4_sa_handler = (svr4_sig_t) bsa->sa_handler;
	native_to_svr4_sigset(&bsa->sa_mask, &ssa->svr4_sa_mask);
	ssa->svr4_sa_flags = 0;
	if ((bsa->sa_flags & SA_ONSTACK) != 0)
		ssa->svr4_sa_flags |= SVR4_SA_ONSTACK;
	if ((bsa->sa_flags & SA_RESETHAND) != 0)
		ssa->svr4_sa_flags |= SVR4_SA_RESETHAND;
	if ((bsa->sa_flags & SA_RESTART) != 0)
		ssa->svr4_sa_flags |= SVR4_SA_RESTART;
	if ((bsa->sa_flags & SA_NODEFER) != 0)
		ssa->svr4_sa_flags |= SVR4_SA_NODEFER;
	if ((bsa->sa_flags & SA_NOCLDSTOP) != 0)
		ssa->svr4_sa_flags |= SVR4_SA_NOCLDSTOP;
}

int
svr4_sys_sigaction(struct lwp *l, const struct svr4_sys_sigaction_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const struct svr4_sigaction *) nsa;
		syscallarg(struct svr4_sigaction *) osa;
	} */
	struct svr4_sigaction nssa, ossa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nssa, sizeof(nssa));
		if (error)
			return (error);
		svr4_to_native_sigaction(&nssa, &nbsa);
	}
	error = sigaction1(l, svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))],
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		native_to_svr4_sigaction(&obsa, &ossa);
		error = copyout(&ossa, SCARG(uap, osa), sizeof(ossa));
		if (error)
			return (error);
	}
	return (0);
}

int
svr4_sys_sigaltstack(struct lwp *l, const struct svr4_sys_sigaltstack_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct svr4_sigaltstack *) nss;
		syscallarg(struct svr4_sigaltstack *) oss;
	} */
	compat_sigaltstack(uap, svr4_sigaltstack,
	    SVR4_SS_ONSTACK, SVR4_SS_DISABLE);
}

/*
 * Stolen from the ibcs2 one
 */
int
svr4_sys_signal(struct lwp *l, const struct svr4_sys_signal_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(svr4_sig_t) handler;
	} */
	int signum = svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))];
	struct proc *p = l->l_proc;
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
		ss = l->l_sigmask;	/* XXXAD locking */
		sigdelset(&ss, signum);
		return (sigsuspend1(l, &ss));

	default:
		return (ENOSYS);
	}
}

int
svr4_sys_sigprocmask(struct lwp *l, const struct svr4_sys_sigprocmask_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) how;
		syscallarg(const svr4_sigset_t *) set;
		syscallarg(svr4_sigset_t *) oset;
	} */
	struct proc *p = l->l_proc;
	svr4_sigset_t nsss, osss;
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
		if (SCARG(uap, set))
			return EINVAL;
		break;
	}

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &nsss, sizeof(nsss));
		if (error)
			return error;
		svr4_to_native_sigset(&nsss, &nbss);
	}
	mutex_enter(&p->p_smutex);
	error = sigprocmask1(l, how,
	    SCARG(uap, set) ? &nbss : NULL, SCARG(uap, oset) ? &obss : NULL);
	mutex_exit(&p->p_smutex);
	if (error)
		return error;
	if (SCARG(uap, oset)) {
		native_to_svr4_sigset(&obss, &osss);
		error = copyout(&osss, SCARG(uap, oset), sizeof(osss));
		if (error)
			return error;
	}
	return 0;
}

int
svr4_sys_sigpending(struct lwp *l, const struct svr4_sys_sigpending_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(svr4_sigset_t *) set;
	} */
	sigset_t bss;
	svr4_sigset_t sss;

	switch (SCARG(uap, what)) {
	case 1:	/* sigpending */
		sigpending1(l, &bss);
		native_to_svr4_sigset(&bss, &sss);
		break;

	case 2:	/* sigfillset */
		svr4_sigfillset(&sss);
		break;

	default:
		return (EINVAL);
	}
	return (copyout(&sss, SCARG(uap, set), sizeof(sss)));
}

int
svr4_sys_sigsuspend(struct lwp *l, const struct svr4_sys_sigsuspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(const svr4_sigset_t *) set;
	} */
	svr4_sigset_t sss;
	sigset_t bss;
	int error;

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &sss, sizeof(sss));
		if (error)
			return (error);
		svr4_to_native_sigset(&sss, &bss);
	}

	return (sigsuspend1(l, SCARG(uap, set) ? &bss : 0));
}

int
svr4_sys_pause(struct lwp *l, const void *v, register_t *retval)
{

	return (sigsuspend1(l, 0));
}

int
svr4_sys_kill(struct lwp *l, const struct svr4_sys_kill_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */
	struct sys_kill_args ka;

	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = svr4_to_native_signo[SVR4_SIGNO(SCARG(uap, signum))];
	return sys_kill(l, &ka, retval);
}

void
svr4_getcontext(struct lwp *l, struct svr4_ucontext *uc)
{
	sigset_t mask;
	struct proc *p = l->l_proc;

	svr4_getmcontext(l, &uc->uc_mcontext, &uc->uc_flags);
	uc->uc_link = l->l_ctxlink;

	/*
	 * The (unsupplied) definition of the `current execution stack'
	 * in the System V Interface Definition appears to allow returning
	 * the main context stack.
	 */
	if ((l->l_sigstk.ss_flags & SS_ONSTACK) == 0) {
		uc->uc_stack.ss_sp = (void *)USRSTACK;
		uc->uc_stack.ss_size = ctob(p->p_vmspace->vm_ssize);
		uc->uc_stack.ss_flags = 0;	/* XXX, def. is Very Fishy */
	} else {
		/* Simply copy alternate signal execution stack. */
		uc->uc_stack.ss_sp = l->l_sigstk.ss_sp;
		uc->uc_stack.ss_size = l->l_sigstk.ss_size;
		uc->uc_stack.ss_flags = l->l_sigstk.ss_flags;
	}
	(void)sigprocmask1(l, 0, NULL, &mask);

	native_to_svr4_sigset(&mask, &uc->uc_sigmask);
	uc->uc_flags |= _UC_SIGMASK | _UC_STACK;
}


int
svr4_setcontext(struct lwp *l, struct svr4_ucontext *uc)
{
	struct proc *p = l->l_proc;
	sigset_t mask;

	if (uc->uc_flags & _UC_SIGMASK) {
		svr4_to_native_sigset(&uc->uc_sigmask, &mask);
		mutex_enter(&p->p_smutex);
		sigprocmask1(l, SIG_SETMASK, &mask, NULL);
		mutex_exit(&p->p_smutex);
	}

	/* Ignore the stack; see comment in svr4_getcontext. */

	l->l_ctxlink = uc->uc_link;
	svr4_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);

	return EJUSTRETURN;
}

int
svr4_sys_context(struct lwp *l, const struct svr4_sys_context_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) func;
		syscallarg(struct svr4_ucontext *) uc;
	} */
	int error;
	svr4_ucontext_t uc;
	*retval = 0;

	switch (SCARG(uap, func)) {
	case SVR4_GETCONTEXT:
		DPRINTF(("getcontext(%p)\n", SCARG(uap, uc)));
		svr4_getcontext(l, &uc);
	return (copyout(&uc, SCARG(uap, uc), sizeof (*SCARG(uap, uc))));


	case SVR4_SETCONTEXT:
		DPRINTF(("setcontext(%p)\n", SCARG(uap, uc)));
		error = copyin(SCARG(uap, uc), &uc, sizeof (uc));
		if (error)
			return (error);
		svr4_setcontext(l, &uc);
		return EJUSTRETURN;

	default:
		DPRINTF(("context(%d, %p)\n", SCARG(uap, func),
		    SCARG(uap, uc)));
		return ENOSYS;
	}
}
