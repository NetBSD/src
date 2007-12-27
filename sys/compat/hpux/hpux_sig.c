/*	$NetBSD: hpux_sig.c,v 1.35.14.2 2007/12/27 00:43:43 mjf Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: hpux_sig.c 1.4 92/01/20$
 *
 *	@(#)hpux_sig.c	8.2 (Berkeley) 9/23/93
 */

/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: hpux_sig.c 1.4 92/01/20$
 *
 *	@(#)hpux_sig.c	8.2 (Berkeley) 9/23/93
 */

/*
 * Signal related HPUX compatibility routines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_sig.c,v 1.35.14.2 2007/12/27 00:43:43 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_sig.h>
#include <compat/hpux/hpux_syscallargs.h>

extern const unsigned char native_to_hpux_signo[];
extern const unsigned char hpux_to_native_signo[];

/*
 * XXX: In addition to mapping the signal number we also have
 * to see if the "old" style signal mechinism is needed.
 * If so, we set the OUSIG flag.  This is not really correct
 * as under HP-UX "old" style handling can be set on a per
 * signal basis and we are setting it for all signals in one
 * swell foop.  I suspect we can get away with this since I
 * doubt any program of interest mixes the two semantics.
 */
int
hpux_sys_sigvec(struct lwp *l, const struct hpux_sys_sigvec_args *uap, register_t *retval)
{
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int sig, error;

	/* XXX */
	extern void compat_43_sigvec_to_sigaction
(const struct sigvec *, struct sigaction *);
	extern void compat_43_sigaction_to_sigvec
(const struct sigaction *, struct sigvec *);

	/*
	 * XXX We don't handle HPUXSV_RESET!
	 */

	sig = hpuxtobsdsig(SCARG(uap, signo));
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
		return (EINVAL);

	if (SCARG(uap, nsv)) {
		error = copyin(SCARG(uap, nsv), &nsv, sizeof(nsv));
		if (error)
			return (error);

		compat_43_sigvec_to_sigaction(&nsv, &nsa);
	}

	error = sigaction1(l, sig,
	    SCARG(uap, nsv) ? &nsa : NULL,
	    SCARG(uap, osv) ? &osa : NULL,
	    NULL, 0);

	if (error)
		return (error);

	if (SCARG(uap, osv)) {
		compat_43_sigaction_to_sigvec(&osa, &osv);
		error = copyout(&osv, SCARG(uap, osv), sizeof(osv));
		if (error)
			return (error);
	}

	return (0);
}

int
hpux_sys_sigblock(struct lwp *l, const struct hpux_sys_sigblock_args *uap, register_t *retval)
{
	sigset_t nmask;

	bsdtohpuxmask(&l->l_sigmask, (int *)retval);
	hpuxtobsdmask(SCARG(uap, mask), &nmask);

	mutex_enter(&l->l_proc->p_smutex);
	sigplusset(&nmask, &l->l_sigmask);
	sigminusset(&sigcantmask, &l->l_sigmask);
	mutex_exit(&l->l_proc->p_smutex);

	return (0);
}

int
hpux_sys_sigsetmask(struct lwp *l, const struct hpux_sys_sigsetmask_args *uap, register_t *retval)
{

	bsdtohpuxmask(&l->l_sigmask, (int *)retval);
	hpuxtobsdmask(SCARG(uap, mask), &l->l_sigmask);

	mutex_enter(&l->l_proc->p_smutex);
	sigminusset(&sigcantmask, &l->l_sigmask);
	mutex_exit(&l->l_proc->p_smutex);

	return (0);
}

int
hpux_sys_sigpause(struct lwp *l, const struct hpux_sys_sigpause_args *uap, register_t *retval)
{
	sigset_t mask;

	hpuxtobsdmask(SCARG(uap, mask), &mask);
	return (sigsuspend1(l, &mask));
}

/* not totally correct, but close enuf' */
int
hpux_sys_kill(struct lwp *l, const struct hpux_sys_kill_args *uap, register_t *retval)
{
	struct sys_kill_args bsd_ua;
	int signo = SCARG(uap, signo);

	SCARG(&bsd_ua, pid) = SCARG(uap, pid);

	if (signo) {
		signo = hpuxtobsdsig(signo);
		if (signo == 0)
			signo = NSIG;
	}
	SCARG(&bsd_ua, signum) = signo;
	return sys_kill(l, &bsd_ua, retval);
}

/*
 * The following (sigprocmask, sigpending, sigsuspend, sigaction are
 * POSIX calls.  Under BSD, the library routine dereferences the sigset_t
 * pointers before traping.  Not so under HP-UX.
 */

/*
 * Manipulate signal mask.
 * Note that we receive new mask, not pointer,
 * and return old mask as return value;
 * the library stub does the rest.
 *
 * XXX We don't handle all HP-UX signals!
 */
int
hpux_sys_sigprocmask(struct lwp *l, const struct hpux_sys_sigprocmask_args *uap, register_t *retval)
{
	int error = 0;
	hpux_sigset_t sigset;
	sigset_t mask;

	/*
	 * Copy out old mask first to ensure no errors.
	 * (proc sigmask should not be changed if call fails for any reason)
	 */
	if (SCARG(uap, oset)) {
		memset((void *)&sigset, 0, sizeof(sigset));
		mutex_enter(&l->l_proc->p_smutex);
		bsdtohpuxmask(&l->l_sigmask, &sigset.sigset[0]);
		mutex_exit(&l->l_proc->p_smutex);
		error = copyout(&sigset, SCARG(uap, oset), sizeof(sigset));
		if (error)
			return (error);
	}
	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &sigset, sizeof(sigset));
		if (error)
			return (error);
		hpuxtobsdmask(sigset.sigset[0], &mask);
		mutex_enter(&l->l_proc->p_smutex);
		switch (SCARG(uap, how)) {
		case HPUXSIG_BLOCK:
			sigplusset(&mask, &l->l_sigmask);
			sigminusset(&sigcantmask, &l->l_sigmask);
			break;
		case HPUXSIG_UNBLOCK:
			sigminusset(&mask, &l->l_sigmask);
			break;
		case HPUXSIG_SETMASK:
			l->l_sigmask = mask;
			sigminusset(&sigcantmask, &l->l_sigmask);
			break;
		default:
			error = EINVAL;
			break;
		}
		l->l_flag |= LW_PENDSIG;
		mutex_exit(&l->l_proc->p_smutex);
	}
	return (error);
}

int
hpux_sys_sigpending(struct lwp *l, const struct hpux_sys_sigpending_args *uap, register_t *retval)
{
	hpux_sigset_t sigset;

	bsdtohpuxmask(&l->l_sigpendset->sp_set, &sigset.sigset[0]);
	return (copyout(&sigset, SCARG(uap, set), sizeof(sigset)));
}

int
hpux_sys_sigsuspend(struct lwp *l, const struct hpux_sys_sigsuspend_args *uap, register_t *retval)
{
	hpux_sigset_t sigset;
	sigset_t mask;
	int error;

	error = copyin(SCARG(uap, set), &sigset, sizeof(sigset));
	if (error)
		return (error);

	hpuxtobsdmask(sigset.sigset[0], &mask);
	return (sigsuspend1(l, &mask));
}

int
hpux_sys_sigaction(struct lwp *l, const struct hpux_sys_sigaction_args *uap, register_t *retval)
{
	struct hpux_sigaction action;
	struct hpux_sigaction *sa;
	struct sigaction *bsa;
	int sig, error;

	sig = hpuxtobsdsig(SCARG(uap, signo));
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
		return (EINVAL);

	bsa = &SIGACTION(l->l_proc, sig);

	sa = &action;
	if (SCARG(uap, osa)) {
		mutex_enter(&l->l_proc->p_smutex);
		sa->hpux_sa_handler = bsa->sa_handler;
		memset((void *)&sa->hpux_sa_mask, 0, sizeof(sa->hpux_sa_mask));
		bsdtohpuxmask(&bsa->sa_mask, &sa->hpux_sa_mask.sigset[0]);
		sa->hpux_sa_flags = 0;
		if (bsa->sa_flags & SA_ONSTACK)
			sa->hpux_sa_flags |= HPUXSA_ONSTACK;
		if (bsa->sa_flags & SA_RESETHAND)
			sa->hpux_sa_flags |= HPUXSA_RESETHAND;
		if (bsa->sa_flags & SA_NOCLDSTOP)
			sa->hpux_sa_flags |= HPUXSA_NOCLDSTOP;
		mutex_exit(&l->l_proc->p_smutex);
		error = copyout(sa, SCARG(uap, osa), sizeof (action));
		if (error)
			return (error);
	}
	if (SCARG(uap, nsa)) {
		struct sigaction act;

		error = copyin(SCARG(uap, nsa), sa, sizeof(action));
		if (error)
			return (error);
		if (sig == SIGCONT && sa->hpux_sa_handler == SIG_IGN)
			return (EINVAL);

		act.sa_handler = sa->hpux_sa_handler;
		hpuxtobsdmask(sa->hpux_sa_mask.sigset[0], &act.sa_mask);
		act.sa_flags = SA_RESTART;
		if (sa->hpux_sa_flags & HPUXSA_ONSTACK)
			act.sa_flags |= SA_ONSTACK;
		if (sa->hpux_sa_flags & HPUXSA_RESETHAND)
			act.sa_flags |= SA_RESETHAND;
		if (sa->hpux_sa_flags & HPUXSA_NOCLDSTOP)
			act.sa_flags |= SA_NOCLDSTOP;

		error = sigaction1(l, sig, &act, NULL, NULL, 0);
		if (error)
			return (error);
	}
	return (0);
}

int
hpux_sys_ssig_6x(struct lwp *l, const struct hpux_sys_ssig_6x_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signo;
		syscallarg(sig_t) fun;
	} */
	struct proc *p = l->l_proc;
	int a;
	struct sigaction vec;
	struct sigaction *sa = &vec;

	memset(sa, 0, sizeof(*sa));
	a = hpuxtobsdsig(SCARG(uap, signo));
	sa->sa_handler = SCARG(uap, fun);
	/*
	 * Kill processes trying to use job control facilities
	 * (this'll help us find any vestiges of the old stuff).
	 */
	if ((a &~ 0377) ||
	    (sa->sa_handler != SIG_DFL && sa->sa_handler != SIG_IGN &&
	     ((int)sa->sa_handler) & 1)) {
	     	mutex_enter(&proclist_mutex);
		psignal(p, SIGSYS);
		mutex_exit(&proclist_mutex);
		return (0);
	}
	if (a <= 0 || a >= NSIG || a == SIGKILL || a == SIGSTOP ||
	    (a == SIGCONT && sa->sa_handler == SIG_IGN))
		return (EINVAL);
	sigemptyset(&sa->sa_mask);
	sa->sa_flags = 0;
	*retval = (register_t)SIGACTION(p, a).sa_handler;
	sigaction1(l, a, sa, NULL, NULL, 0);
#if 0
	p->p_flag |= SOUSIG;		/* mark as simulating old stuff */
#endif
	return (0);
}

/* signal numbers: convert from HPUX to BSD */
int
hpuxtobsdsig(int sig)
{
	if (sig < 0 || sig >= NSIG)
		return(0);
	return hpux_to_native_signo[sig];
}

/* signal numbers: convert from BSD to HPUX */
int
bsdtohpuxsig(int sig)
{
	if (sig < 0 || sig >= NSIG)
		return(0);
	return native_to_hpux_signo[sig];
}

/* signal masks: convert from HPUX to BSD (not pretty or fast) */
void
hpuxtobsdmask(int hpuxmask, sigset_t *bsdmask)
{
	int sig, nsig;

	sigemptyset(bsdmask);

	for (sig = 1; sig < NSIG; sig++) {
		if ((hpuxmask & (1 << sig)) != 0 &&
		    (nsig = hpuxtobsdsig(sig)) != 0)
			sigaddset(bsdmask, sig);
	}
}

void
bsdtohpuxmask(const sigset_t *bsdmask, int *hpuxmask)
{
	int sig, nsig;

	*hpuxmask = 0;

	for (sig = 1; sig < NSIG; sig++) {
		if (sigismember(bsdmask, sig) &&
		    (nsig = bsdtohpuxsig(sig)) != 0)
			*hpuxmask |= (1 << sig);
	}
}
