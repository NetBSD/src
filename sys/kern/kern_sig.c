/*	$NetBSD: kern_sig.c,v 1.112.2.17 2002/06/24 22:10:50 nathanw Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_sig.c	8.14 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sig.c,v 1.112.2.17 2002/06/24 22:10:50 nathanw Exp $");

#include "opt_ktrace.h"
#include "opt_compat_sunos.h"
#include "opt_compat_netbsd32.h"	

#define	SIGPROP		/* include signal properties table */
#include <sys/param.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/core.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/ucontext.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/exec.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>

#include <sys/user.h>		/* for coredump */

#include <uvm/uvm_extern.h>

static void	proc_stop(struct proc *p);
void		killproc(struct proc *, char *);
static int	build_corename(struct proc *, char [MAXPATHLEN]);
sigset_t	contsigmask, stopsigmask, sigcantmask;

struct pool	sigacts_pool;	/* memory pool for sigacts structures */
struct pool	siginfo_pool;	/* memory pool for siginfo structures */

/*
 * Can process p, with pcred pc, send the signal signum to process q?
 */
#define	CANSIGNAL(p, pc, q, signum) \
	((pc)->pc_ucred->cr_uid == 0 || \
	    (pc)->p_ruid == (q)->p_cred->p_ruid || \
	    (pc)->pc_ucred->cr_uid == (q)->p_cred->p_ruid || \
	    (pc)->p_ruid == (q)->p_ucred->cr_uid || \
	    (pc)->pc_ucred->cr_uid == (q)->p_ucred->cr_uid || \
	    ((signum) == SIGCONT && (q)->p_session == (p)->p_session))

/*
 * Initialize signal-related data structures.
 */
void
signal_init(void)
{

	pool_init(&sigacts_pool, sizeof(struct sigacts), 0, 0, 0, "sigapl",
	    &pool_allocator_nointr);
	pool_init(&siginfo_pool, sizeof(siginfo_t), 0, 0, 0, "siginfo",
	    &pool_allocator_nointr);
}

/*
 * Create an initial sigctx structure, using the same signal state
 * as p. If 'share' is set, share the sigctx_proc part, otherwise just
 * copy it from parent.
 */
void
sigactsinit(struct proc *np, struct proc *pp, int share)
{
	struct sigacts *ps;

	if (share) {
		np->p_sigacts = pp->p_sigacts;
		pp->p_sigacts->sa_refcnt++;
	} else {
		ps = pool_get(&sigacts_pool, PR_WAITOK);
		if (pp)
			memcpy(ps, pp->p_sigacts, sizeof(struct sigacts));
		else
			memset(ps, '\0', sizeof(struct sigacts));
		ps->sa_refcnt = 1;
		np->p_sigacts = ps;
	}
}

/*
 * Make this process not share its sigctx, maintaining all
 * signal state.
 */
void
sigactsunshare(struct proc *p)
{
	struct sigacts *oldps;

	if (p->p_sigacts->sa_refcnt == 1)
		return;

	oldps = p->p_sigacts;
	sigactsinit(p, NULL, 0);

	if (--oldps->sa_refcnt == 0)
		pool_put(&sigacts_pool, oldps);
}

/*
 * Release a sigctx structure.
 */
void
sigactsfree(struct proc *p)
{
	struct sigacts *ps;

	ps = p->p_sigacts;
	if (--ps->sa_refcnt > 0)
		return;

	pool_put(&sigacts_pool, ps);
}

int
sigaction1(struct proc *p, int signum, const struct sigaction *nsa,
	struct sigaction *osa)
{
	struct sigacts	*ps;
	int		prop;

	ps = p->p_sigacts;
	if (signum <= 0 || signum >= NSIG)
		return (EINVAL);

	if (osa)
		*osa = SIGACTION_PS(ps, signum);

	if (nsa) {
		if (nsa->sa_flags & ~SA_ALLBITS)
			return (EINVAL);

		prop = sigprop[signum];
		if (prop & SA_CANTMASK)
			return (EINVAL);

		(void) splsched();	/* XXXSMP */
		SIGACTION_PS(ps, signum) = *nsa;
		sigminusset(&sigcantmask, &SIGACTION_PS(ps, signum).sa_mask);
		if ((prop & SA_NORESET) != 0)
			SIGACTION_PS(ps, signum).sa_flags &= ~SA_RESETHAND;
		if (signum == SIGCHLD) {
			if (nsa->sa_flags & SA_NOCLDSTOP)
				p->p_flag |= P_NOCLDSTOP;
			else
				p->p_flag &= ~P_NOCLDSTOP;
			if (nsa->sa_flags & SA_NOCLDWAIT) {
				/*
				 * Paranoia: since SA_NOCLDWAIT is implemented
				 * by reparenting the dying child to PID 1 (and
				 * trust it to reap the zombie), PID 1 itself
				 * is forbidden to set SA_NOCLDWAIT.
				 */
				if (p->p_pid == 1)
					p->p_flag &= ~P_NOCLDWAIT;
				else
					p->p_flag |= P_NOCLDWAIT;
			} else
				p->p_flag &= ~P_NOCLDWAIT;
		}
		if ((nsa->sa_flags & SA_NODEFER) == 0)
			sigaddset(&SIGACTION_PS(ps, signum).sa_mask, signum);
		else
			sigdelset(&SIGACTION_PS(ps, signum).sa_mask, signum);
		/*
	 	 * Set bit in p_sigctx.ps_sigignore for signals that are set to
		 * SIG_IGN, and for signals set to SIG_DFL where the default is
		 * to ignore. However, don't put SIGCONT in
		 * p_sigctx.ps_sigignore, as we have to restart the process.
	 	 */
		if (nsa->sa_handler == SIG_IGN ||
		    (nsa->sa_handler == SIG_DFL && (prop & SA_IGNORE) != 0)) {
						/* never to be seen again */
			sigdelset(&p->p_sigctx.ps_siglist, signum);
			if (signum != SIGCONT) {
						/* easier in psignal */
				sigaddset(&p->p_sigctx.ps_sigignore, signum);
			}
			sigdelset(&p->p_sigctx.ps_sigcatch, signum);
		} else {
			sigdelset(&p->p_sigctx.ps_sigignore, signum);
			if (nsa->sa_handler == SIG_DFL)
				sigdelset(&p->p_sigctx.ps_sigcatch, signum);
			else
				sigaddset(&p->p_sigctx.ps_sigcatch, signum);
		}
		(void) spl0();
	}

	return (0);
}

/* ARGSUSED */
int
sys___sigaction14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigaction14_args /* {
		syscallarg(int)				signum;
		syscallarg(const struct sigaction *)	nsa;
		syscallarg(struct sigaction *)		osa;
	} */ *uap = v;
	struct proc		*p;
	struct sigaction	nsa, osa;
	int			error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nsa, sizeof(nsa));
		if (error)
			return (error);
	}
	p = l->l_proc;
	error = sigaction1(p, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nsa : 0, SCARG(uap, osa) ? &osa : 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		error = copyout(&osa, SCARG(uap, osa), sizeof(osa));
		if (error)
			return (error);
	}
	return (0);
}

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default and disable the signal
 * stack.
 */
void
siginit(struct proc *p)
{
	struct sigacts	*ps;
	int		signum, prop;

	ps = p->p_sigacts;
	sigemptyset(&contsigmask);
	sigemptyset(&stopsigmask);
	sigemptyset(&sigcantmask);
	for (signum = 1; signum < NSIG; signum++) {
		prop = sigprop[signum];
		if (prop & SA_CONT)
			sigaddset(&contsigmask, signum);
		if (prop & SA_STOP)
			sigaddset(&stopsigmask, signum);
		if (prop & SA_CANTMASK)
			sigaddset(&sigcantmask, signum);
		if (prop & SA_IGNORE && signum != SIGCONT)
			sigaddset(&p->p_sigctx.ps_sigignore, signum);
		sigemptyset(&SIGACTION_PS(ps, signum).sa_mask);
		SIGACTION_PS(ps, signum).sa_flags = SA_RESTART;
	}
	sigemptyset(&p->p_sigctx.ps_sigcatch);
	p->p_flag &= ~P_NOCLDSTOP;

	/*
	 * Reset stack state to the user stack.
	 */
	p->p_sigctx.ps_sigstk.ss_flags = SS_DISABLE;
	p->p_sigctx.ps_sigstk.ss_size = 0;
	p->p_sigctx.ps_sigstk.ss_sp = 0;

	/* One reference. */
	ps->sa_refcnt = 1;
}

/*
 * Reset signals for an exec of the specified process.
 */
void
execsigs(struct proc *p)
{
	struct sigacts	*ps;
	int		signum, prop;

	sigactsunshare(p);

	ps = p->p_sigacts;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigctx.ps_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	for (signum = 1; signum < NSIG; signum++) {
		if (sigismember(&p->p_sigctx.ps_sigcatch, signum)) {
			prop = sigprop[signum];
			if (prop & SA_IGNORE) {
				if ((prop & SA_CONT) == 0)
					sigaddset(&p->p_sigctx.ps_sigignore,
					    signum);
				sigdelset(&p->p_sigctx.ps_siglist, signum);
			}
			SIGACTION_PS(ps, signum).sa_handler = SIG_DFL;
		}
		sigemptyset(&SIGACTION_PS(ps, signum).sa_mask);
		SIGACTION_PS(ps, signum).sa_flags = SA_RESTART;
	}
	sigemptyset(&p->p_sigctx.ps_sigcatch);
	p->p_flag &= ~P_NOCLDSTOP;

	/*
	 * Reset stack state to the user stack.
	 */
	p->p_sigctx.ps_sigstk.ss_flags = SS_DISABLE;
	p->p_sigctx.ps_sigstk.ss_size = 0;
	p->p_sigctx.ps_sigstk.ss_sp = 0;
}

int
sigprocmask1(struct proc *p, int how, const sigset_t *nss, sigset_t *oss)
{

	if (oss)
		*oss = p->p_sigctx.ps_sigmask;

	if (nss) {
		(void)splsched();	/* XXXSMP */
		switch (how) {
		case SIG_BLOCK:
			sigplusset(nss, &p->p_sigctx.ps_sigmask);
			break;
		case SIG_UNBLOCK:
			sigminusset(nss, &p->p_sigctx.ps_sigmask);
			CHECKSIGS(p);
			break;
		case SIG_SETMASK:
			p->p_sigctx.ps_sigmask = *nss;
			CHECKSIGS(p);
			break;
		default:
			(void)spl0();	/* XXXSMP */
			return (EINVAL);
		}
		sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);
		(void)spl0();		/* XXXSMP */
	}

	return (0);
}
	
/*
 * Manipulate signal mask.
 * Note that we receive new mask, not pointer,
 * and return old mask as return value;
 * the library stub does the rest.
 */
int
sys___sigprocmask14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigprocmask14_args /* {
		syscallarg(int)			how;
		syscallarg(const sigset_t *)	set;
		syscallarg(sigset_t *)		oset;
	} */ *uap = v;
	struct proc	*p;
	sigset_t	nss, oss;
	int		error;

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &nss, sizeof(nss));
		if (error)
			return (error);
	}
	p = l->l_proc;
	error = sigprocmask1(p, SCARG(uap, how),
	    SCARG(uap, set) ? &nss : 0, SCARG(uap, oset) ? &oss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oset)) {
		error = copyout(&oss, SCARG(uap, oset), sizeof(oss));
		if (error)
			return (error);
	}
	return (0);
}

void
sigpending1(struct proc *p, sigset_t *ss)
{

	*ss = p->p_sigctx.ps_siglist;
	sigminusset(&p->p_sigctx.ps_sigmask, ss);
}

/* ARGSUSED */
int
sys___sigpending14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigpending14_args /* {
		syscallarg(sigset_t *)	set;
	} */ *uap = v;
	struct proc	*p;
	sigset_t	ss;

	p = l->l_proc;
	sigpending1(p, &ss);
	return (copyout(&ss, SCARG(uap, set), sizeof(ss)));
}

int
sigsuspend1(struct proc *p, const sigset_t *ss)
{
	struct sigacts *ps;

	ps = p->p_sigacts;
	if (ss) {
		/*
		 * When returning from sigpause, we want
		 * the old mask to be restored after the
		 * signal handler has finished.  Thus, we
		 * save it here and mark the sigctx structure
		 * to indicate this.
		 */
		p->p_sigctx.ps_oldmask = p->p_sigctx.ps_sigmask;
		p->p_sigctx.ps_flags |= SAS_OLDMASK;
		(void) splsched();	/* XXXSMP */
		p->p_sigctx.ps_sigmask = *ss;
		CHECKSIGS(p);
		sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);
		(void) spl0();		/* XXXSMP */
	}

	while (tsleep((caddr_t) ps, PPAUSE|PCATCH, "pause", 0) == 0)
		/* void */;
	/* always return EINTR rather than ERESTART... */
	return (EINTR);
}

/*
 * Suspend process until signal, providing mask to be set
 * in the meantime.  Note nonstandard calling convention:
 * libc stub passes mask, not pointer, to save a copyin.
 */
/* ARGSUSED */
int
sys___sigsuspend14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigsuspend14_args /* {
		syscallarg(const sigset_t *)	set;
	} */ *uap = v;
	struct proc	*p;
	sigset_t	ss;
	int		error;

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &ss, sizeof(ss));
		if (error)
			return (error);
	}

	p = l->l_proc;
	return (sigsuspend1(p, SCARG(uap, set) ? &ss : 0));
}

int
sigaltstack1(struct proc *p, const struct sigaltstack *nss,
	struct sigaltstack *oss)
{

	if (oss)
		*oss = p->p_sigctx.ps_sigstk;

	if (nss) {
		if (nss->ss_flags & ~SS_ALLBITS)
			return (EINVAL);

		if (nss->ss_flags & SS_DISABLE) {
			if (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
				return (EINVAL);
		} else {
			if (nss->ss_size < MINSIGSTKSZ)
				return (ENOMEM);
		}
		p->p_sigctx.ps_sigstk = *nss;
	}

	return (0);
}

/* ARGSUSED */
int
sys___sigaltstack14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigaltstack14_args /* {
		syscallarg(const struct sigaltstack *)	nss;
		syscallarg(struct sigaltstack *)	oss;
	} */ *uap = v;
	struct proc		*p;
	struct sigaltstack	nss, oss;
	int			error;

	if (SCARG(uap, nss)) {
		error = copyin(SCARG(uap, nss), &nss, sizeof(nss));
		if (error)
			return (error);
	}
	p = l->l_proc;
	error = sigaltstack1(p,
	    SCARG(uap, nss) ? &nss : 0, SCARG(uap, oss) ? &oss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		error = copyout(&oss, SCARG(uap, oss), sizeof(oss));
		if (error)
			return (error);
	}
	return (0);
}

/* ARGSUSED */
int
sys_kill(struct lwp *l, void *v, register_t *retval)
{
	struct sys_kill_args /* {
		syscallarg(int)	pid;
		syscallarg(int)	signum;
	} */ *uap = v;
	struct proc	*cp, *p;
	struct pcred	*pc;

	cp = l->l_proc;
	pc = cp->p_cred;
	if ((u_int)SCARG(uap, signum) >= NSIG)
		return (EINVAL);
	if (SCARG(uap, pid) > 0) {
		/* kill single process */
		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return (ESRCH);
		if (!CANSIGNAL(cp, pc, p, SCARG(uap, signum)))
			return (EPERM);
		if (SCARG(uap, signum))
			psignal(p, SCARG(uap, signum));
		return (0);
	}
	switch (SCARG(uap, pid)) {
	case -1:		/* broadcast signal */
		return (killpg1(cp, SCARG(uap, signum), 0, 1));
	case 0:			/* signal own process group */
		return (killpg1(cp, SCARG(uap, signum), 0, 0));
	default:		/* negative explicit process group */
		return (killpg1(cp, SCARG(uap, signum), -SCARG(uap, pid), 0));
	}
	/* NOTREACHED */
}

/*
 * Common code for kill process group/broadcast kill.
 * cp is calling process.
 */
int
killpg1(struct proc *cp, int signum, int pgid, int all)
{
	struct proc	*p;
	struct pcred	*pc;
	struct pgrp	*pgrp;
	int		nfound;
	
	pc = cp->p_cred;
	nfound = 0;
	if (all) {
		/* 
		 * broadcast 
		 */
		proclist_lock_read();
		for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM || 
			    p == cp || !CANSIGNAL(cp, pc, p, signum))
				continue;
			nfound++;
			if (signum)
				psignal(p, signum);
		}
		proclist_unlock_read();
	} else {
		if (pgid == 0)		
			/* 
			 * zero pgid means send to my process group.
			 */
			pgrp = cp->p_pgrp;
		else {
			pgrp = pgfind(pgid);
			if (pgrp == NULL)
				return (ESRCH);
		}
		for (p = pgrp->pg_members.lh_first;
		    p != 0;
		    p = p->p_pglist.le_next) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM ||
			    !CANSIGNAL(cp, pc, p, signum))
				continue;
			nfound++;
			if (signum && P_ZOMBIE(p) == 0)
				psignal(p, signum);
		}
	}
	return (nfound ? 0 : ESRCH);
}

/*
 * Send a signal to a process group.
 */
void
gsignal(int pgid, int signum)
{
	struct pgrp *pgrp;

	if (pgid && (pgrp = pgfind(pgid)))
		pgsignal(pgrp, signum, 0);
}

/*
 * Send a signal to a process group. If checktty is 1,
 * limit to members which have a controlling terminal.
 */
void
pgsignal(struct pgrp *pgrp, int signum, int checkctty)
{
	struct proc *p;

	if (pgrp)
		for (p = pgrp->pg_members.lh_first; p != 0;
		    p = p->p_pglist.le_next)
			if (checkctty == 0 || p->p_flag & P_CONTROLT)
				psignal(p, signum);
}

/*
 * Send a signal caused by a trap to the current process.
 * If it will be caught immediately, deliver it with correct code.
 * Otherwise, post it normally.
 */
void
trapsignal(struct lwp *l, int signum, u_long code)
{
	struct proc	*p;
	struct sigacts	*ps;

	p = l->l_proc;
	ps = p->p_sigacts;
	if ((p->p_flag & P_TRACED) == 0 &&
	    sigismember(&p->p_sigctx.ps_sigcatch, signum) &&
	    !sigismember(&p->p_sigctx.ps_sigmask, signum)) {
		p->p_stats->p_ru.ru_nsignals++;
#ifdef KTRACE
		if (KTRPOINT(p, KTR_PSIG))
			ktrpsig(p, signum,
			    SIGACTION_PS(ps, signum).sa_handler,
			    &p->p_sigctx.ps_sigmask, code);
#endif
		psendsig(l, SIGACTION_PS(ps, signum).sa_handler,
		    signum, &p->p_sigctx.ps_sigmask, code);
		(void) splsched();	/* XXXSMP */
		sigplusset(&SIGACTION_PS(ps, signum).sa_mask,
		    &p->p_sigctx.ps_sigmask);
		if (SIGACTION_PS(ps, signum).sa_flags & SA_RESETHAND) {
			sigdelset(&p->p_sigctx.ps_sigcatch, signum);
			if (signum != SIGCONT && sigprop[signum] & SA_IGNORE)
				sigaddset(&p->p_sigctx.ps_sigignore, signum);
			SIGACTION_PS(ps, signum).sa_handler = SIG_DFL;
		}
		(void) spl0();		/* XXXSMP */
	} else {
		p->p_sigctx.ps_code = code;	/* XXX for core dump/debugger */
		p->p_sigctx.ps_sig = signum;	/* XXX to verify code */
		psignal(p, signum);
	}
}

/*
 * Send the signal to the process.  If the signal has an action, the action
 * is usually performed by the target process rather than the caller; we add
 * the signal to the set of pending signals for the process.
 *
 * Exceptions:
 *   o When a stop signal is sent to a sleeping process that takes the
 *     default action, the process is stopped without awakening it.
 *   o SIGCONT restarts stopped processes (or puts them back to sleep)
 *     regardless of the signal action (eg, blocked or ignored).
 *
 * Other ignored signals are discarded immediately.
 *
 * XXXSMP: Invoked as psignal() or sched_psignal().
 */
void
psignal1(struct proc *p, int signum,
	int dolock)		/* XXXSMP: works, but icky */
{
	struct lwp	*l;

	int		s, prop;
	sig_t		action;

#ifdef DIAGNOSTIC
	if (signum <= 0 || signum >= NSIG)
		panic("psignal signal number");

	/* XXXSMP: works, but icky */
	if (dolock)
		SCHED_ASSERT_UNLOCKED();
	else
		SCHED_ASSERT_LOCKED();
#endif
	prop = sigprop[signum];

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & P_TRACED)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 * (Note: we don't set SIGCONT in p_sigctx.ps_sigignore,
		 * and if it is set to SIG_IGN,
		 * action will be SIG_DFL here.)
		 */
		if (sigismember(&p->p_sigctx.ps_sigignore, signum))
			return;
		if (sigismember(&p->p_sigctx.ps_sigmask, signum))
			action = SIG_HOLD;
		else if (sigismember(&p->p_sigctx.ps_sigcatch, signum))
			action = SIG_CATCH;
		else {
			action = SIG_DFL;

			if (prop & SA_KILL && p->p_nice > NZERO)
				p->p_nice = NZERO;

			/*
			 * If sending a tty stop signal to a member of an
			 * orphaned process group, discard the signal here if
			 * the action is default; don't stop the process below
			 * if sleeping, and don't clear any pending SIGCONT.
			 */
			if (prop & SA_TTYSTOP && p->p_pgrp->pg_jobc == 0)
				return;
		}
	}

	if (prop & SA_CONT)
		sigminusset(&stopsigmask, &p->p_sigctx.ps_siglist);

	if (prop & SA_STOP)
		sigminusset(&contsigmask, &p->p_sigctx.ps_siglist);

	sigaddset(&p->p_sigctx.ps_siglist, signum);

	/* CHECKSIGS() is "inlined" here. */
	p->p_sigctx.ps_sigcheck = 1;

	/*
	 * Defer further processing for signals which are held,
	 * except that stopped processes must be continued by SIGCONT.
	 */
	if (action == SIG_HOLD && ((prop & SA_CONT) == 0 || p->p_stat != SSTOP))
		return;
	/* XXXSMP: works, but icky */
	if (dolock)
		SCHED_LOCK(s);
	
	if (p->p_nrlwps > 0) {
		/*
		 * At least one LWP is running or on a run queue. 
		 * The signal will be noticed when one of them returns 
		 * to userspace.
		 */
		signotify(p);
		/* 
		 * The signal will be noticed very soon.
		 */
		goto out;
	} else {
		/* Process is sleeping or stopped */
		/* Find out if any of the sleeps are interruptable */
		for (l = LIST_FIRST(&p->p_lwps); 
		     l != NULL; 
		     l = LIST_NEXT(l, l_sibling))
			if (l->l_stat == LSSLEEP && 
			    l->l_flag & L_SINTR)
				break;
		if (p->p_stat == SACTIVE) {
			/* All LWPs must be sleeping */
			
			if (l != NULL && (p->p_flag & P_TRACED))
				goto run;
			     
			/*
			 * If SIGCONT is default (or ignored) and process is
			 * asleep, we are finished; the process should not
			 * be awakened.
			 */
			if ((prop & SA_CONT) && action == SIG_DFL) {
				sigdelset(&p->p_sigctx.ps_siglist, signum);
				goto out;
			}

			/*
			 * When a sleeping process receives a stop
			 * signal, process immediately if possible.
			 */
			if ((prop & SA_STOP) && action == SIG_DFL) {
				/*
				 * If a child holding parent blocked,
				 * stopping could cause deadlock.
				 */
				if (p->p_flag & P_PPWAIT)
					goto out;
				sigdelset(&p->p_sigctx.ps_siglist, signum);
				p->p_xstat = signum;
				if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0) {
					/*
					 * XXXSMP: recursive call; don't lock
					 * the second time around.
					 */
					sched_psignal(p->p_pptr, SIGCHLD);
				}
				proc_stop(p);	/* XXXSMP: recurse? */
				goto out;
			}

			if (l == NULL)
				goto out;

			/*
			 * All other (caught or default) signals
			 * cause the process to run.
			 */
			goto runfast;
			/*NOTREACHED*/
		} else if (p->p_stat == SSTOP) {
			/* Process is stopped */
			/*
			 * If traced process is already stopped,
			 * then no further action is necessary.
			 */
			if (p->p_flag & P_TRACED)
				goto out;

			/*
			 * Kill signal always sets processes running,
			 * if possible.
			 */
			if (signum == SIGKILL) {
				l = proc_unstop(p);
				if (l)
					goto runfast;
				/* XXX should this be possible? */
				goto out;
			}
			
			if (prop & SA_CONT) {
				/*
				 * If SIGCONT is default (or ignored),
				 * we continue the process but don't
				 * leave the signal in ps_siglist, as
				 * it has no further action.  If
				 * SIGCONT is held, we continue the
				 * process and leave the signal in
				 * ps_siglist.  If the process catches
				 * SIGCONT, let it handle the signal
				 * itself.  If it isn't waiting on an
				 * event, then it goes back to run
				 * state.  Otherwise, process goes
				 * back to sleep state.  
				 */
				if (action == SIG_DFL)
					sigdelset(&p->p_sigctx.ps_siglist, 
					signum);
				l = proc_unstop(p);
				if (l && (action == SIG_CATCH))
					goto runfast;
				if (l)
					goto run;
				goto out;
			}

			if (prop & SA_STOP) {
				/*
				 * Already stopped, don't need to stop again.
				 * (If we did the shell could get confused.)
				 */
				sigdelset(&p->p_sigctx.ps_siglist, signum);
				goto out;
			}

			/*
			 * If process is sleeping interruptibly, then
			 * simulate a wakeup so that when it is
			 * continued, it will be made runnable and can
			 * look at the signal.  But don't make the
			 * process runnable, leave it stopped.  
			 */
			if (l)
				unsleep(l);
			goto out;
		} else {
			/* Else what? */
			panic("psignal: Invalid process state.");
		}
	}
	/*NOTREACHED*/

 runfast:
	/*
	 * Raise priority to at least PUSER.
	 */
	if (l->l_priority > PUSER)
		l->l_priority = PUSER;
 run:
	setrunnable(l);		/* XXXSMP: recurse? */
 out:
	/* XXXSMP: works, but icky */
	if (dolock)
		SCHED_UNLOCK(s);
}

void
psendsig(struct lwp *l, sig_t catcher, int sig, sigset_t *mask, u_long code)
{
	struct proc *p = l->l_proc;
	struct lwp *le, *li;
	siginfo_t *si;	

	if (p->p_flag & P_SA) {
		si = pool_get(&siginfo_pool, PR_WAITOK);
		si->si_signo = sig;
		si->si_errno = 0;
		si->si_code = code;
		le = li = NULL;
		if (code)
			le = l;
		else
			li = l;

		sa_upcall(l, SA_UPCALL_SIGNAL, le, li, 
			    sizeof(siginfo_t), si);
		return;
	}

	(*p->p_emul->e_sendsig)(catcher, sig, mask, code);
}

static __inline int firstsig(const sigset_t *);

static __inline int
firstsig(const sigset_t *ss)
{
	int sig;

	sig = ffs(ss->__bits[0]);
	if (sig != 0)
		return (sig);
#if NSIG > 33
	sig = ffs(ss->__bits[1]);
	if (sig != 0)
		return (sig + 32);
#endif
#if NSIG > 65
	sig = ffs(ss->__bits[2]);
	if (sig != 0)
		return (sig + 64);
#endif
#if NSIG > 97
	sig = ffs(ss->__bits[3]);
	if (sig != 0)
		return (sig + 96);
#endif
	return (0);
}

/*
 * If the current process has received a signal (should be caught or cause
 * termination, should interrupt current syscall), return the signal number.
 * Stop signals with default action are processed immediately, then cleared;
 * they aren't returned.  This is checked after each entry to the system for
 * a syscall or trap (though this can usually be done without calling issignal
 * by checking the pending signal masks in the CURSIG macro.) The normal call
 * sequence is
 *
 *	while (signum = CURSIG(curlwp))
 *		postsig(signum);
 */
int
issignal(struct lwp *l)
{
	struct proc	*p;
	int		s, signum, prop;
	int		dolock, locked;
	sigset_t	ss;

	dolock = (l->l_flag & L_SINTR) == 0;
	locked = !dolock;
	p = l->l_proc;
	for (;;) {
		sigpending1(p, &ss);
		if (p->p_flag & P_PPWAIT)
			sigminusset(&stopsigmask, &ss);
		signum = firstsig(&ss);
		if (signum == 0) {		 	/* no signal to send */
			p->p_sigctx.ps_sigcheck = 0;
			if (locked && dolock)
				SCHED_LOCK(s);
			return (0);
		}
							/* take the signal! */
		sigdelset(&p->p_sigctx.ps_siglist, signum);

		/*
		 * We should see pending but ignored signals
		 * only if P_TRACED was on when they were posted.
		 */
		if (sigismember(&p->p_sigctx.ps_sigignore, signum) &&
		    (p->p_flag & P_TRACED) == 0)
			continue;

		if (p->p_flag & P_TRACED && (p->p_flag & P_PPWAIT) == 0) {
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the debugger.
			 */
			p->p_xstat = signum;
			if ((p->p_flag & P_FSTRACE) == 0)
				psignal1(p->p_pptr, SIGCHLD, dolock);
			if (dolock)
				SCHED_LOCK(s);
			proc_stop(p);
			mi_switch(l, NULL);
			SCHED_ASSERT_UNLOCKED();
			if (dolock)
				splx(s);
			else
				dolock = 1;

			/*
			 * If we are no longer being traced, or the parent
			 * didn't give us a signal, look for more signals.
			 */
			if ((p->p_flag & P_TRACED) == 0 || p->p_xstat == 0)
				continue;

			/*
			 * If the new signal is being masked, look for other
			 * signals.
			 */
			signum = p->p_xstat;
			/*
			 * `p->p_sigctx.ps_siglist |= mask' is done
			 * in setrunnable().
			 */
			if (sigismember(&p->p_sigctx.ps_sigmask, signum))
				continue;
							/* take the signal! */
			sigdelset(&p->p_sigctx.ps_siglist, signum);
		}

		prop = sigprop[signum];

		/*
		 * Decide whether the signal should be returned.
		 * Return the signal's number, or fall through
		 * to clear it from the pending mask.
		 */
		switch ((long)SIGACTION(p, signum).sa_handler) {

		case (long)SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_pid <= 1) {
#ifdef DIAGNOSTIC
				/*
				 * Are you sure you want to ignore SIGSEGV
				 * in init? XXX
				 */
				printf("Process (pid %d) got signal %d\n",
				    p->p_pid, signum);
#endif
				break;		/* == ignore */
			}
			/*
			 * If there is a pending stop signal to process
			 * with default action, stop here,
			 * then clear the signal.  However,
			 * if process is member of an orphaned
			 * process group, ignore tty stop signals.
			 */
			if (prop & SA_STOP) {
				if (p->p_flag & P_TRACED ||
		    		    (p->p_pgrp->pg_jobc == 0 &&
				    prop & SA_TTYSTOP))
					break;	/* == ignore */
				p->p_xstat = signum;
				if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
					psignal1(p->p_pptr, SIGCHLD, dolock);
				if (dolock)
					SCHED_LOCK(s);
				proc_stop(p);
				mi_switch(l, NULL);
				SCHED_ASSERT_UNLOCKED();
				if (dolock)
					splx(s);
				else
					dolock = 1;
				break;
			} else if (prop & SA_IGNORE) {
				/*
				 * Except for SIGCONT, shouldn't get here.
				 * Default action is to ignore; drop it.
				 */
				break;		/* == ignore */
			} else
				goto keep;
			/*NOTREACHED*/

		case (long)SIG_IGN:
			/*
			 * Masking above should prevent us ever trying
			 * to take action on an ignored signal other
			 * than SIGCONT, unless process is traced.
			 */
			if ((prop & SA_CONT) == 0 &&
			    (p->p_flag & P_TRACED) == 0)
				printf("issignal\n");
			break;		/* == ignore */

		default:
			/*
			 * This signal has an action, let
			 * postsig() process it.
			 */
			goto keep;
		}
	}
	/* NOTREACHED */

 keep:
						/* leave the signal for later */
	sigaddset(&p->p_sigctx.ps_siglist, signum);
	CHECKSIGS(p);
	if (locked && dolock)
		SCHED_LOCK(s);
	return (signum);
}

/*
 * Put the argument process into the stopped state and notify the parent
 * via wakeup.  Signals are handled elsewhere.  The process must not be
 * on the run queue.
 */
static void
proc_stop(struct proc *p)
{
	struct lwp *l;

	SCHED_ASSERT_LOCKED();

	/* XXX lock process LWP state */
	p->p_stat = SSTOP;
	p->p_flag &= ~P_WAITED;


	/* 
	 * Put as many LWP's as possible in stopped state.
	 * Any in uninterruptable sleep will notice the stopped state as 
	 * they try to return.
	 */
	   
	for (l = LIST_FIRST(&p->p_lwps); l != NULL; 
	     l = LIST_NEXT(l, l_sibling)) {
		if (l->l_stat == LSONPROC) {
			/* XXX SMP this assumes that a LWP that is LSONPROC
			 * is curlwp and hence is about to be mi_switched 
			 * away; the only callers of proc_stop() are:
			 * - psignal
			 * - issignal()
			 * For the former, proc_stop() is only called when
			 * no processes are running, so we don't worry.
			 * For the latter, proc_stop() is called right
			 * before mi_switch().
			 */
			l->l_stat = LSSTOP;
			p->p_nrlwps--;
		} else if (l->l_stat == LSRUN) {
			/* Remove LWP from the run queue */
			remrunqueue(l);
			l->l_stat = LSSTOP;
			p->p_nrlwps--;
		} else if (l->l_stat == LSSLEEP) {
			/* Note: this puts all sleeping LWPs into LSSTOP.
			 * Formerly, uninterruptably sleeping procs were
			 * left to discover the STOP signal on their
			 * way back to userspace, but that's harder 
			 * with multiple LWPs. 
			 * XXX This should be okay, but.....
			 */
			l->l_stat = LSSTOP;
		} else if ((l->l_stat == LSSUSPENDED) || 
		    (l->l_stat == LSZOMB) ||
			(l->l_stat == LSDEAD)) {
			/* Don't do anything. These guys aren't going anywhere.
			 */
		}
#ifdef DIAGNOSTIC
		else {
			panic("proc_stop: process %d lwp %d "
			      "in unstoppable state %d.\n", 
			    p->p_pid, l->l_lid, l->l_stat);
		}
#endif
	}
	/* XXX unlock process LWP state */
		    
	sched_wakeup((caddr_t)p->p_pptr);
}

struct lwp *
proc_unstop(p)
	struct proc *p;
{
	struct lwp *l, *lr = NULL;

	SCHED_ASSERT_LOCKED();

	/* Our caller will want to invoke setrunnable() on whatever we return.
	 * XXX what if everyone was asleep when the process was stopped? 
	 * Yuck.
	 */

	p->p_stat = SACTIVE;
	for (l = LIST_FIRST(&p->p_lwps); l != NULL; 
	     l = LIST_NEXT(l, l_sibling))
		if (l->l_stat == LSSTOP) {
			if (l->l_wchan == 0) {
				if (lr == NULL)
					lr = l;
				else
					setrunnable(l);
			} else
				l->l_stat = LSSLEEP;
		}

	return lr;
}

/*
 * Take the action for the specified signal
 * from the current set of pending signals.
 */
void
postsig(int signum)
{
	struct lwp *l;
	struct proc	*p;
	struct sigacts	*ps;
	sig_t		action;
	u_long		code;
	sigset_t	*returnmask;

	l = curlwp;
	p = l->l_proc;
	ps = p->p_sigacts;
#ifdef DIAGNOSTIC
	if (signum == 0)
		panic("postsig");
#endif

	KERNEL_PROC_LOCK(l);

	sigdelset(&p->p_sigctx.ps_siglist, signum);
	action = SIGACTION_PS(ps, signum).sa_handler;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_PSIG))
		ktrpsig(p,
		    signum, action, p->p_sigctx.ps_flags & SAS_OLDMASK ?
		    &p->p_sigctx.ps_oldmask : &p->p_sigctx.ps_sigmask, 0);
#endif
	if (action == SIG_DFL) {
		/*
		 * Default action, where the default is to kill
		 * the process.  (Other cases were ignored above.)
		 */
		sigexit(l, signum);
		/* NOTREACHED */
	} else {
		/*
		 * If we get here, the signal must be caught.
		 */
#ifdef DIAGNOSTIC
		if (action == SIG_IGN ||
		    sigismember(&p->p_sigctx.ps_sigmask, signum))
			panic("postsig action");
#endif
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal.
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want
		 * restored after the signal processing is completed.
		 */
		if (p->p_sigctx.ps_flags & SAS_OLDMASK) {
			returnmask = &p->p_sigctx.ps_oldmask;
			p->p_sigctx.ps_flags &= ~SAS_OLDMASK;
		} else
			returnmask = &p->p_sigctx.ps_sigmask;
		p->p_stats->p_ru.ru_nsignals++;
		if (p->p_sigctx.ps_sig != signum) {
			code = 0;
		} else {
			code = p->p_sigctx.ps_code;
			p->p_sigctx.ps_code = 0;
			p->p_sigctx.ps_sig = 0;
		}
		psendsig(l, action, signum, returnmask, code);
		(void) splsched();	/* XXXSMP */
		sigplusset(&SIGACTION_PS(ps, signum).sa_mask,
		    &p->p_sigctx.ps_sigmask);
		if (SIGACTION_PS(ps, signum).sa_flags & SA_RESETHAND) {
			sigdelset(&p->p_sigctx.ps_sigcatch, signum);
			if (signum != SIGCONT && sigprop[signum] & SA_IGNORE)
				sigaddset(&p->p_sigctx.ps_sigignore, signum);
			SIGACTION_PS(ps, signum).sa_handler = SIG_DFL;
		}
		(void) spl0();		/* XXXSMP */
	}

	KERNEL_PROC_UNLOCK(l);
}

/*
 * Kill the current process for stated reason.
 */
void
killproc(struct proc *p, char *why)
{

	log(LOG_ERR, "pid %d was killed: %s\n", p->p_pid, why);
	uprintf("sorry, pid %d was killed: %s\n", p->p_pid, why);
	psignal(p, SIGKILL);
}

/*
 * Force the current process to exit with the specified signal, dumping core
 * if appropriate.  We bypass the normal tests for masked and caught signals,
 * allowing unrecoverable failures to terminate the process without changing
 * signal state.  Mark the accounting record with the signal termination.
 * If dumping core, save the signal number for the debugger.  Calls exit and
 * does not return.
 */

#if defined(DEBUG)
int	kern_logsigexit = 1;	/* not static to make public for sysctl */
#else
int	kern_logsigexit = 0;	/* not static to make public for sysctl */
#endif

static	const char logcoredump[] =
	"pid %d (%s), uid %d: exited on signal %d (core dumped)\n";
static	const char lognocoredump[] =
	"pid %d (%s), uid %d: exited on signal %d (core not dumped, err = %d)\n";

/* Wrapper function for use in p_userret */
static void
lwp_coredump_hook(struct lwp *l, void *arg)
{
	int s;

	/*
	 * Suspend ourselves, so that the kernel stack and therefore
	 * the userland registers saved in the trapframe are around
	 * for coredump() to write them out.
	 */
	l->l_flag &= ~L_DETACHED;
	SCHED_LOCK(s);
	l->l_stat = LSSUSPENDED;
	l->l_proc->p_nrlwps--;
	/* XXX NJWLWP check if this makes sense here: */
	l->l_proc->p_stats->p_ru.ru_nvcsw++; 
	mi_switch(l, NULL);
	SCHED_ASSERT_UNLOCKED();

	lwp_exit(l);
}

void
sigexit(struct lwp *l, int signum)
{
	struct proc	*p;
	int		error, exitsig;

	p = l->l_proc;

	/*
	 * Don't permit coredump() or exit1() multiple times 
	 * in the same process.
	 */
	if (p->p_flag & P_WEXIT)
		lwp_exit(l);
	p->p_flag |= P_WEXIT;
	/* Make other LWPs stick around long enough to be dumped */
	p->p_userret = lwp_coredump_hook;
	p->p_userret_arg = NULL;

	exitsig = signum;
	p->p_acflag |= AXSIG;
	if (sigprop[signum] & SA_CORE) {
		p->p_sigctx.ps_sig = signum;
		if ((error = coredump(l)) == 0)
			exitsig |= WCOREFLAG;

		if (kern_logsigexit) {
			int uid = p->p_cred && p->p_ucred ? 
				p->p_ucred->cr_uid : -1;

			if (error) 
				log(LOG_INFO, lognocoredump, p->p_pid,
				    p->p_comm, uid, signum, error);
			else
				log(LOG_INFO, logcoredump, p->p_pid,
				    p->p_comm, uid, signum);
		}

	}

	exit1(l, W_EXITCODE(0, exitsig));
	/* NOTREACHED */
}

/*
 * Dump core, into a file named "progname.core" or "core" (depending on the 
 * value of shortcorename), unless the process was setuid/setgid.
 */
int
coredump(struct lwp *l)
{
	struct vnode		*vp;
	struct proc		*p;
	struct vmspace		*vm;
	struct ucred		*cred;
	struct nameidata	nd;
	struct vattr		vattr;
	int			error, error1;
	char			name[MAXPATHLEN];

	p = l->l_proc;
	vm = p->p_vmspace;
	cred = p->p_cred->pc_ucred;

	/*
	 * Make sure the process has not set-id, to prevent data leaks.
	 */
	if (p->p_flag & P_SUGID)
		return (EPERM);

	/*
	 * Refuse to core if the data + stack + user size is larger than
	 * the core dump limit.  XXX THIS IS WRONG, because of mapped
	 * data.
	 */
	if (USPACE + ctob(vm->vm_dsize + vm->vm_ssize) >=
	    p->p_rlimit[RLIMIT_CORE].rlim_cur)
		return (EFBIG);		/* better error code? */

	/*
	 * The core dump will go in the current working directory.  Make
	 * sure that the directory is still there and that the mount flags
	 * allow us to write core dumps there.
	 */
	vp = p->p_cwdi->cwdi_cdir;
	if (vp->v_mount == NULL ||
	    (vp->v_mount->mnt_flag & MNT_NOCOREDUMP) != 0)
		return (EPERM);

	error = build_corename(p, name);
	if (error)
		return error;

	/* We don't want to switch away from crashing. */
	/* XXX multiprocessor: stop LWPs on other processors. */
	if (l->l_flag & L_SA) 
		l->l_flag &= ~L_SA;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_SYSSPACE, name, p);
	error = vn_open(&nd, O_CREAT | FWRITE | FNOSYMLINK, S_IRUSR | S_IWUSR);
	if (error)
		return (error);
	vp = nd.ni_vp;

	/* Don't dump to non-regular files or files with links. */
	if (vp->v_type != VREG ||
	    VOP_GETATTR(vp, &vattr, cred, p) || vattr.va_nlink != 1) {
		error = EINVAL;
		goto out;
	}
	VATTR_NULL(&vattr);
	vattr.va_size = 0;
	VOP_LEASE(vp, p, cred, LEASE_WRITE);
	VOP_SETATTR(vp, &vattr, cred, p);
	p->p_acflag |= ACORE;

	/* Now dump the actual core file. */
	error = (*p->p_execsw->es_coredump)(l, vp, cred);
 out:
	VOP_UNLOCK(vp, 0);
	error1 = vn_close(vp, FWRITE, cred, p);
	if (error == 0)
		error = error1;
	return (error);
}

/*
 * Nonexistent system call-- signal process (may want to handle it).
 * Flag error in case process won't see signal immediately (blocked or ignored).
 */
/* ARGSUSED */
int
sys_nosys(struct lwp *l, void *v, register_t *retval)
{
	struct proc 	*p;

	p = l->l_proc;
	psignal(p, SIGSYS);
	return (ENOSYS);
}

static int
build_corename(struct proc *p, char dst[MAXPATHLEN])
{
	const char	*s;
	char		*d, *end;
	int		i;
	
	for (s = p->p_limit->pl_corename, d = dst, end = d + MAXPATHLEN;
	    *s != '\0'; s++) {
		if (*s == '%') {
			switch (*(s + 1)) {
			case 'n':
				i = snprintf(d, end - d, "%s", p->p_comm);
				break;
			case 'p':
				i = snprintf(d, end - d, "%d", p->p_pid);
				break;
			case 'u':
				i = snprintf(d, end - d, "%s",
				    p->p_pgrp->pg_session->s_login);
				break;
			case 't':
				i = snprintf(d, end - d, "%ld",
				    p->p_stats->p_start.tv_sec);
				break;
			default:
				goto copy;
			}
			d += i;
			s++;
		} else {
 copy:			*d = *s;
			d++;
		}
		if (d >= end)
			return (ENAMETOOLONG);
	}
	*d = '\0';
	return 0;
}

void
getucontext(struct lwp *l, ucontext_t *ucp)
{
	struct proc	*p;

	p = l->l_proc;

	ucp->uc_flags = 0;
	ucp->uc_link = l->l_ctxlink;

	(void)sigprocmask1(p, 0, NULL, &ucp->uc_sigmask);
	ucp->uc_flags |= _UC_SIGMASK;

	/*
	 * The (unsupplied) definition of the `current execution stack'
	 * in the System V Interface Definition appears to allow returning
	 * the main context stack.
	 */
	if ((p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK) == 0) {
		ucp->uc_stack.ss_sp = (void *)USRSTACK;
		ucp->uc_stack.ss_size = ctob(p->p_vmspace->vm_ssize);
		ucp->uc_stack.ss_flags = 0;	/* XXX, def. is Very Fishy */
	} else {
		/* Simply copy alternate signal execution stack. */
		ucp->uc_stack = p->p_sigctx.ps_sigstk;
	}
	ucp->uc_flags |= _UC_STACK;

	cpu_getmcontext(l, &ucp->uc_mcontext, &ucp->uc_flags);
}

/* ARGSUSED */
int
sys_getcontext(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getcontext_args /* {
		syscallarg(struct __ucontext *) ucp;
	} */ *uap = v;
	ucontext_t uc;

	getucontext(l, &uc);

	return (copyout(&uc, SCARG(uap, ucp), sizeof (*SCARG(uap, ucp))));
}

int
setucontext(struct lwp *l, const ucontext_t *ucp)
{
	struct proc	*p;
	int		error;

	p = l->l_proc;
	if ((error = cpu_setmcontext(l, &ucp->uc_mcontext, ucp->uc_flags)) != 0)
		return (error);
	l->l_ctxlink = ucp->uc_link;
	/*
	 * We might want to take care of the stack portion here but currently
	 * don't; see the comment in getucontext().
	 */
	if ((ucp->uc_flags & _UC_SIGMASK) != 0)
		sigprocmask1(p, SIG_SETMASK, &ucp->uc_sigmask, NULL);

	return 0;
}

/* ARGSUSED */
int
sys_setcontext(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setcontext_args /* {
		syscallarg(const ucontext_t *) ucp;
	} */ *uap = v;
	ucontext_t uc;
	int error;

	if (SCARG(uap, ucp) == NULL)	/* i.e. end of uc_link chain */
		exit1(l, W_EXITCODE(0, 0));
	else if ((error = copyin(SCARG(uap, ucp), &uc, sizeof (uc))) != 0 ||
	    (error = setucontext(l, &uc)) != 0)
		return (error);

	return (EJUSTRETURN);
}


/*
 * Returns true if signal is ignored or masked for passed process.
 */
int
sigismasked(struct proc *p, int sig)
{

	return (sigismember(&p->p_sigctx.ps_sigignore, sig) ||
	    sigismember(&p->p_sigctx.ps_sigmask, sig));
}

