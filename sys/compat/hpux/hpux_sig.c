/*	$NetBSD: hpux_sig.c,v 1.20.2.5 2002/07/03 23:08:22 nathanw Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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
__KERNEL_RCSID(0, "$NetBSD: hpux_sig.c,v 1.20.2.5 2002/07/03 23:08:22 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

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
hpux_sys_sigvec(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigvec_args *uap = v;
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int sig, error;

	/* XXX */
	extern void compat_43_sigvec_to_sigaction
	    __P((const struct sigvec *, struct sigaction *));
	extern void compat_43_sigaction_to_sigvec
	    __P((const struct sigaction *, struct sigvec *));

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

	error = sigaction1(l->l_proc, sig,
	    SCARG(uap, nsv) ? &nsa : NULL,
	    SCARG(uap, osv) ? &osa : NULL);

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
hpux_sys_sigblock(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigblock_args *uap = v;
	struct proc *p = l->l_proc;
	sigset_t nmask;

	(void) splsched();

	bsdtohpuxmask(&p->p_sigctx.ps_sigmask, (int *)retval);
	hpuxtobsdmask(SCARG(uap, mask), &nmask);

	sigplusset(&nmask, &p->p_sigctx.ps_sigmask);
	sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);

	(void) spl0();
	return (0);
}

int
hpux_sys_sigsetmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigsetmask_args *uap = v;
	struct proc *p = l->l_proc;

	(void) splsched();

	bsdtohpuxmask(&p->p_sigctx.ps_sigmask, (int *)retval);
	hpuxtobsdmask(SCARG(uap, mask), &p->p_sigctx.ps_sigmask);

	sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);

	(void) spl0();
	return (0);
}

int
hpux_sys_sigpause(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigpause_args *uap = v;
	sigset_t mask;

	hpuxtobsdmask(SCARG(uap, mask), &mask);
	return (sigsuspend1(l->l_proc, &mask));
}

/* not totally correct, but close enuf' */
int
hpux_sys_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_kill_args *uap = v;

	if (SCARG(uap, signo)) {
		SCARG(uap, signo) = hpuxtobsdsig(SCARG(uap, signo));
		if (SCARG(uap, signo) == 0)
			SCARG(uap, signo) = NSIG;
	}
	return (sys_kill(l, uap, retval));
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
hpux_sys_sigprocmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigprocmask_args *uap = v;
	struct proc *p = l->l_proc;
	int error = 0;
	hpux_sigset_t sigset;
	sigset_t mask;

	/*
	 * Copy out old mask first to ensure no errors.
	 * (proc sigmask should not be changed if call fails for any reason)
	 */
	if (SCARG(uap, oset)) {
		memset((caddr_t)&sigset, 0, sizeof(sigset));
		bsdtohpuxmask(&p->p_sigctx.ps_sigmask, &sigset.sigset[0]);
		error = copyout(&sigset, SCARG(uap, oset), sizeof(sigset));
		if (error)
			return (error);
	}
	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &sigset, sizeof(sigset));
		if (error)
			return (error);
		hpuxtobsdmask(sigset.sigset[0], &mask);
		(void) splsched();
		switch (SCARG(uap, how)) {
		case HPUXSIG_BLOCK:
			sigplusset(&mask, &p->p_sigctx.ps_sigmask);
			sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);
			break;
		case HPUXSIG_UNBLOCK:
			sigminusset(&mask, &p->p_sigctx.ps_sigmask);
			break;
		case HPUXSIG_SETMASK:
			p->p_sigctx.ps_sigmask = mask;
			sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);
			break;
		default:
			error = EINVAL;
			break;
		}
		(void) spl0();
	}
	return (error);
}

int
hpux_sys_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigpending_args *uap = v;
	hpux_sigset_t sigset;

	bsdtohpuxmask(&l->l_proc->p_sigctx.ps_siglist, &sigset.sigset[0]);
	return (copyout(&sigset, SCARG(uap, set), sizeof(sigset)));
}

int
hpux_sys_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigsuspend_args *uap = v;
	hpux_sigset_t sigset;
	sigset_t mask;
	int error;

	error = copyin(SCARG(uap, set), &sigset, sizeof(sigset));
	if (error)
		return (error);

	hpuxtobsdmask(sigset.sigset[0], &mask);
	return (sigsuspend1(l->l_proc, &mask));
}

int
hpux_sys_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigaction_args *uap = v;
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
		sa->sa_handler = bsa->sa_handler;
		memset((caddr_t)&sa->sa_mask, 0, sizeof(sa->sa_mask));
		bsdtohpuxmask(&bsa->sa_mask, &sa->sa_mask.sigset[0]);
		sa->sa_flags = 0;
		if (bsa->sa_flags & SA_ONSTACK)
			sa->sa_flags |= HPUXSA_ONSTACK;
		if (bsa->sa_flags & SA_RESETHAND)
			sa->sa_flags |= HPUXSA_RESETHAND;
		if (bsa->sa_flags & SA_NOCLDSTOP)
			sa->sa_flags |= HPUXSA_NOCLDSTOP;
		error = copyout(sa, SCARG(uap, osa), sizeof (action));
		if (error)
			return (error);
	}
	if (SCARG(uap, nsa)) {
		struct sigaction act;

		error = copyin(SCARG(uap, nsa), sa, sizeof(action));
		if (error)
			return (error);
		if (sig == SIGCONT && sa->sa_handler == SIG_IGN)
			return (EINVAL);

		act.sa_handler = sa->sa_handler;
		hpuxtobsdmask(sa->sa_mask.sigset[0], &act.sa_mask);
		act.sa_flags = SA_RESTART;
		if (sa->sa_flags & HPUXSA_ONSTACK)
			act.sa_flags |= SA_ONSTACK;
		if (sa->sa_flags & HPUXSA_RESETHAND)
			act.sa_flags |= SA_RESETHAND;
		if (sa->sa_flags & HPUXSA_NOCLDSTOP)
			act.sa_flags |= SA_NOCLDSTOP;

		error = sigaction1(l->l_proc, sig, &act, NULL);
		if (error)
			return (error);
	}
	return (0);
}

int
hpux_sys_ssig_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_ssig_6x_args /* {
		syscallarg(int) signo;
		syscallarg(sig_t) fun;
	} */ *uap = v;
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
		psignal(p, SIGSYS);
		return (0);
	}
	if (a <= 0 || a >= NSIG || a == SIGKILL || a == SIGSTOP ||
	    (a == SIGCONT && sa->sa_handler == SIG_IGN))
		return (EINVAL);
	sigemptyset(&sa->sa_mask);
	sa->sa_flags = 0;
	*retval = (register_t)SIGACTION(p, a).sa_handler;
	sigaction1(p, a, sa, NULL);
#if 0
	p->p_flag |= SOUSIG;		/* mark as simulating old stuff */
#endif
	return (0);
}

/* signal numbers: convert from HPUX to BSD */
int
hpuxtobsdsig(sig)
	int sig;
{
	if (sig < 0 || sig >= NSIG)
		return(0);
	return hpux_to_native_signo[sig];
}

/* signal numbers: convert from BSD to HPUX */
int
bsdtohpuxsig(sig)
	int sig;
{
	if (sig < 0 || sig >= NSIG)
		return(0);
	return native_to_hpux_signo[sig];
}

/* signal masks: convert from HPUX to BSD (not pretty or fast) */
void
hpuxtobsdmask(hpuxmask, bsdmask)
	int hpuxmask;
	sigset_t *bsdmask;
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
bsdtohpuxmask(bsdmask, hpuxmask)
	const sigset_t *bsdmask;
	int *hpuxmask;
{
	int sig, nsig;

	*hpuxmask = 0;

	for (sig = 1; sig < NSIG; sig++) {
		if (sigismember(bsdmask, sig) &&
		    (nsig = bsdtohpuxsig(sig)) != 0)
			*hpuxmask |= (1 << sig);
	}
}
