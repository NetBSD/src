/*	$NetBSD: kern_sig.c,v 1.228.2.5 2006/11/18 21:39:22 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 *	@(#)kern_sig.c	8.14 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sig.c,v 1.228.2.5 2006/11/18 21:39:22 ad Exp $");

#include "opt_ktrace.h"
#include "opt_ptrace.h"

#define	SIGPROP		/* include signal properties table */

#include <sys/param.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/ucontext.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/exec.h>
#include <sys/kauth.h>
#include <sys/acct.h>

#include <machine/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

static void	ksiginfo_exithook(struct proc *, void *);

int	sigunwait(struct proc *, const ksiginfo_t *);
void	sigclear(sigpend_t *, sigset_t *);
void	sigclearall(struct proc *, sigset_t *);
void	sigput(sigpend_t *, struct proc *, ksiginfo_t *);
int	sigpost(struct lwp *, sig_t *, int prop, ksiginfo_t **, int);
int	sigchecktrace(void);
void	sigswitch(int, int);
void	sigrealloc(ksiginfo_t *);

sigset_t	contsigmask, stopsigmask, sigcantmask;

struct pool	sigacts_pool;	/* memory pool for sigacts structures */
static void	sigacts_poolpage_free(struct pool *, void *);
static void	*sigacts_poolpage_alloc(struct pool *, int);

static struct pool_allocator sigactspool_allocator = {
        .pa_alloc = sigacts_poolpage_alloc,
	.pa_free = sigacts_poolpage_free,
};

#ifdef DEBUG
int	kern_logsigexit = 1;
#else
int	kern_logsigexit = 1;
#endif

static	const char logcoredump[] =
    "pid %d (%s), uid %d: exited on signal %d (core dumped)\n";
static	const char lognocoredump[] =
    "pid %d (%s), uid %d: exited on signal %d (core not dumped, err = %d)\n";

POOL_INIT(siginfo_pool, sizeof(siginfo_t), 0, 0, 0, "siginfo",
    &pool_allocator_nointr);
POOL_INIT(ksiginfo_pool, sizeof(ksiginfo_t), 0, 0, 0, "ksiginfo", NULL);

/*
 * signal_init:
 *
 * 	Initialize global signal-related data structures.
 */
void
signal_init(void)
{

	sigactspool_allocator.pa_pagesz = (PAGE_SIZE)*2;

	pool_init(&sigacts_pool, sizeof(struct sigacts), 0, 0, 0, "sigapl",
	    sizeof(struct sigacts) > PAGE_SIZE ?
	    &sigactspool_allocator : &pool_allocator_nointr);

	exithook_establish(ksiginfo_exithook, NULL);
	exechook_establish(ksiginfo_exithook, NULL);
}

/*
 * sigacts_poolpage_alloc:
 *
 *	 Allocate a page for the sigacts memory pool.
 */
static void *
sigacts_poolpage_alloc(struct pool *pp, int flags)
{

	return (void *)uvm_km_alloc(kernel_map,
	    (PAGE_SIZE)*2, (PAGE_SIZE)*2,
	    ((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK)
	    | UVM_KMF_WIRED);
}

/*
 * sigacts_poolpage_free:
 *
 *	 Free a page on behalf of the sigacts memory pool.
 */
static void
sigacts_poolpage_free(struct pool *pp, void *v)
{
        uvm_km_free(kernel_map, (vaddr_t)v, (PAGE_SIZE)*2, UVM_KMF_WIRED);
}

/*
 * sigactsinit:
 * 
 *	 Create an initial sigctx structure, using the same signal state as
 *	 p.  If 'share' is set, share the sigctx_proc part, otherwise just
 *	 copy it from parent.
 */
struct sigacts *
sigactsinit(struct proc *pp, int share)
{
	struct sigacts *ps;

	if (pp != NULL) {
		LOCK_ASSERT(mutex_owned(&pp->p_smutex));
	}

	if (share) {
		ps = pp->p_sigacts;
		mutex_enter(&ps->sa_mutex);
		ps->sa_refcnt++;
		mutex_exit(&ps->sa_mutex);
	} else {
		if (pp)
			mutex_exit(&pp->p_smutex);
		ps = pool_get(&sigacts_pool, PR_WAITOK);
		mutex_init(&ps->sa_mutex, MUTEX_SPIN, IPL_NONE);
		if (pp) {
			mutex_enter(&pp->p_smutex);
			memcpy(ps, pp->p_sigacts, sizeof(struct sigacts));
		} else
			memset(ps, 0, sizeof(struct sigacts));
		ps->sa_refcnt = 1;
	}

	return ps;
}

/*
 * sigactsunshare:
 * 
 *	Make this process not share its sigctx, maintaining all
 *	signal state.
 */
void
sigactsunshare(struct proc *p)
{
	struct sigacts *ps, *oldps;
	int refcnt;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	oldps = p->p_sigacts;

	mutex_enter(&oldps->sa_mutex);
	refcnt = oldps->sa_refcnt;
	mutex_exit(&oldps->sa_mutex);
	if (refcnt == 1)
		return;

	mutex_exit(&p->p_smutex);
	ps = sigactsinit(NULL, 0);
	mutex_enter(&p->p_smutex);
	p->p_sigacts = ps;

	mutex_enter(&oldps->sa_mutex);
	refcnt = --oldps->sa_refcnt;
	mutex_exit(&oldps->sa_mutex);

	if (refcnt == 0)
		pool_put(&sigacts_pool, oldps);
}

/*
 * sigactsfree;
 *
 *	Release a sigctx structure.
 */
void
sigactsfree(struct sigacts *ps)
{
	int refcnt;

	mutex_enter(&ps->sa_mutex);
	refcnt = --ps->sa_refcnt;
	mutex_exit(&ps->sa_mutex);

	if (refcnt == 0)
		pool_put(&sigacts_pool, ps);
}

/*
 * siginit:
 *
 *	Initialize signal state for process 0; set to ignore signals that
 *	are ignored by default and disable the signal stack.  Locking not
 *	required as the system is still cold.
 */
void
siginit(struct proc *p)
{
	struct lwp *l;
	struct sigacts *ps;
	int signo, prop;

	ps = p->p_sigacts;
	sigemptyset(&contsigmask);
	sigemptyset(&stopsigmask);
	sigemptyset(&sigcantmask);
	for (signo = 1; signo < NSIG; signo++) {
		prop = sigprop[signo];
		if (prop & SA_CONT)
			sigaddset(&contsigmask, signo);
		if (prop & SA_STOP)
			sigaddset(&stopsigmask, signo);
		if (prop & SA_CANTMASK)
			sigaddset(&sigcantmask, signo);
		if (prop & SA_IGNORE && signo != SIGCONT)
			sigaddset(&p->p_sigctx.ps_sigignore, signo);
		sigemptyset(&SIGACTION_PS(ps, signo).sa_mask);
		SIGACTION_PS(ps, signo).sa_flags = SA_RESTART;
	}
	sigemptyset(&p->p_sigctx.ps_sigcatch);
	p->p_sflag &= ~PS_NOCLDSTOP;

	CIRCLEQ_INIT(&p->p_sigpend.sp_info);
	sigemptyset(&p->p_sigpend.sp_set);

	/*
	 * Reset per LWP state.
	 */
	l = LIST_FIRST(&p->p_lwps);
	l->l_sigwaited = NULL;
	l->l_sigpend = &l->l_sigstore.ss_pend;
	l->l_sigmask = &l->l_sigstore.ss_mask;
	l->l_sigstk = &l->l_sigstore.ss_stk;
	l->l_sigstk->ss_flags = SS_DISABLE;
	l->l_sigstk->ss_size = 0;
	l->l_sigstk->ss_sp = 0;
	CIRCLEQ_INIT(&l->l_sigpend->sp_info);
	sigemptyset(&l->l_sigpend->sp_set);

	/* One reference. */
	ps->sa_refcnt = 1;
}

/*
 * execsigs:
 *
 *	Reset signals for an exec of the specified process.
 */
void
execsigs(struct proc *p)
{
	struct sigacts *ps;
	struct lwp *l;
	int signo, prop;
	sigset_t tset;

	KASSERT(p->p_nlwps == 1);

	mutex_enter(&p->p_smutex);

	sigactsunshare(p);

	ps = p->p_sigacts;

	/*
	 * Reset caught signals.  Held signals remain held through
	 * l->l_sigmask (unless they were caught, and are now ignored
	 * by default).
	 */
	sigemptyset(&tset);
	for (signo = 1; signo < NSIG; signo++) {
		if (sigismember(&p->p_sigctx.ps_sigcatch, signo)) {
			prop = sigprop[signo];
			if (prop & SA_IGNORE) {
				if ((prop & SA_CONT) == 0)
					sigaddset(&p->p_sigctx.ps_sigignore,
					    signo);
				sigaddset(&tset, signo);
			}
			SIGACTION_PS(ps, signo).sa_handler = SIG_DFL;
		}
		sigemptyset(&SIGACTION_PS(ps, signo).sa_mask);
		SIGACTION_PS(ps, signo).sa_flags = SA_RESTART;
	}
	sigclearall(p, &tset);
	sigemptyset(&p->p_sigctx.ps_sigcatch);

	/*
	 * Reset no zombies if child dies flag as Solaris does.
	 */
	p->p_flag &= ~(P_NOCLDWAIT | P_CLDSIGIGN);
	if (SIGACTION_PS(ps, SIGCHLD).sa_handler == SIG_IGN)
		SIGACTION_PS(ps, SIGCHLD).sa_handler = SIG_DFL;

	/*
	 * Reset per-LWP state.
	 */
	l = LIST_FIRST(&p->p_lwps);
	l->l_sigwaited = NULL;
	l->l_sigpend = &l->l_sigstore.ss_pend;
	l->l_sigmask = &l->l_sigstore.ss_mask;
	l->l_sigstk = &l->l_sigstore.ss_stk;
	l->l_sigstk->ss_flags = SS_DISABLE;
	l->l_sigstk->ss_size = 0;
	l->l_sigstk->ss_sp = 0;
	CIRCLEQ_INIT(&l->l_sigpend->sp_info);
	sigemptyset(&l->l_sigpend->sp_set);

	mutex_exit(&p->p_smutex);
}

/*
 * ksiginfo_exithook:
 *
 *	Free all pending ksiginfo entries from a process on exit.
 */
static void
ksiginfo_exithook(struct proc *p, void *v)
{

	mutex_enter(&p->p_smutex);
	sigclearall(p, NULL);
	mutex_exit(&p->p_smutex);
}

/*
 * ksiginfo_alloc:
 *
 *	Allocate a new ksiginfo structure from the pool, and optionally copy
 *	an existing one.  If the existing ksiginfo_t is from the pool, and
 *	has not been queued somewhere, then just return it.  Additionally,
 *	if the existing ksiginfo_t does not contain any information beyond
 *	the signal number, then just return it.
 */
ksiginfo_t *
ksiginfo_alloc(struct proc *p, ksiginfo_t *ok, int flags)
{
	ksiginfo_t *kp;
	int f;

	if (ok != NULL) {
		f = ok->ksi_flags;
		if ((f & (KSI_QUEUED | KSI_FROMPOOL)) == KSI_FROMPOOL)
		    	return ok;
		if ((f & KSI_EMPTY) != 0)
			return ok;
	}

	kp = pool_get(&ksiginfo_pool, flags);
	if (kp == NULL) {
#ifdef DIAGNOSTIC
		printf("Out of memory allocating ksiginfo for pid %d\n",
		   p->p_pid);
#endif
		return NULL;
	}

	if (ok != NULL) {
		memcpy(kp, ok, sizeof(*kp));
		kp->ksi_flags &= ~KSI_QUEUED;
	} else
		KSI_INIT_EMPTY(kp);

	kp->ksi_flags |= KSI_FROMPOOL;

	return kp;
}

/*
 * ksiginfo_free:
 *
 *	If the given ksiginfo_t is from the pool and has not been queued,
 *	then free it.
 */
void
ksiginfo_free(ksiginfo_t *kp)
{

	if ((kp->ksi_flags & (KSI_QUEUED | KSI_FROMPOOL)) == KSI_FROMPOOL)
		pool_put(&ksiginfo_pool, kp);
}

/*
 * sigget:
 *
 *	Fetch the first pending signal from a set.  Optionally, also fetch
 *	or manufacture a ksiginfo element.  Returns the number of the first
 *	pending signal, or zero.
 */ 
int
sigget(sigpend_t *sp, ksiginfo_t *out, int signo, sigset_t *mask)
{
        ksiginfo_t *ksi;
	sigset_t tset;

	/* Construct mask from signo, and 'mask'. */
	if (signo == 0) {
		if (mask != NULL) {
			tset = *mask;
			__sigandset(&sp->sp_set, &tset);
		} else
			tset = sp->sp_set;
		
		/* If there are no signals pending, that's it. */
		if ((signo = firstsig(&tset)) == 0)
			return 0;
	} else if (sigismember(&sp->sp_set, signo) == 0)
		return 0;

	sigdelset(&sp->sp_set, signo);

	/* Find siginfo and copy it out. */
	CIRCLEQ_FOREACH(ksi, &sp->sp_info, ksi_list) {
		if (ksi->ksi_signo == signo) {
			CIRCLEQ_REMOVE(&sp->sp_info, ksi, ksi_list);
			if (out != NULL) {
				memcpy(out, ksi, sizeof(*out));
				out->ksi_flags &= ~(KSI_FROMPOOL | KSI_QUEUED);
			}
			KASSERT((ksi->ksi_flags & KSI_FROMPOOL) != 0);
			KASSERT((ksi->ksi_flags & KSI_QUEUED) != 0);
			pool_put(&ksiginfo_pool, ksi);
			return signo;
		}
	}

	/* If there's no siginfo, then manufacture it. */
	if (out != NULL) {
		KSI_INIT(out);
		out->ksi_info._signo = signo;
		out->ksi_info._code = SI_USER;
	}

	return signo;
}

/*
 * sigput:
 * 
 *	Append a new ksiginfo element to the list of pending ksiginfo's, if
 *	we need to (e.g. SA_SIGINFO was requested).
 */
void
sigput(sigpend_t *sp, struct proc *p, ksiginfo_t *ksi)
{
	ksiginfo_t *kp;
	struct sigaction *sa = &SIGACTION_PS(p->p_sigacts, ksi->ksi_signo);

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	KASSERT((ksi->ksi_flags & KSI_QUEUED) == 0);

	sigaddset(&sp->sp_set, ksi->ksi_signo);

	/*
	 * If siginfo is not required, or there is none, then just mark the
	 * signal as pending.
	 */
	if (((sa->sa_flags & SA_SIGINFO) == 0 && ksi->ksi_lid == 0) ||
	    KSI_EMPTY_P(ksi))
		return;

	KASSERT((ksi->ksi_flags & KSI_FROMPOOL) != 0);

#ifdef notyet	/* XXX: QUEUING */
	if (ksi->ksi_signo < SIGRTMIN)
#endif
	{
		CIRCLEQ_FOREACH(kp, &sp->sp_info, ksi_list) {
			if (kp->ksi_signo == ksi->ksi_signo) {
				KSI_COPY(ksi, kp);
				kp->ksi_flags |= KSI_QUEUED;
				return;
			}
		}
	}

	ksi->ksi_flags |= KSI_QUEUED;
	CIRCLEQ_INSERT_TAIL(&sp->sp_info, ksi, ksi_list);
}

/*
 * sigclear:
 *
 *	Clear all pending signals in the specified set.
 */
void
sigclear(sigpend_t *sp, sigset_t *mask)
{
	ksiginfo_t *ksi, *next;

	sigminusset(mask, &sp->sp_set);

	ksi = CIRCLEQ_FIRST(&sp->sp_info);
	for (; ksi != (void *)&sp->sp_info; ksi = next) {
		next = CIRCLEQ_NEXT(ksi, ksi_list);
		if (sigismember(mask, ksi->ksi_signo)) {
			CIRCLEQ_REMOVE(&sp->sp_info, ksi, ksi_list);
			KASSERT((ksi->ksi_flags & KSI_FROMPOOL) != 0);
			KASSERT((ksi->ksi_flags & KSI_QUEUED) != 0);
			pool_put(&ksiginfo_pool, ksi);
		}
	}
}

/*
 * sigclearall:
 *
 *	Clear all pending signals in the specified set from a process and
 *	its LWPs.
 */
void
sigclearall(struct proc *p, sigset_t *mask)
{
	struct lwp *l;
	sigset_t tset;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	if (mask == NULL) {
		sigfillset(&tset);
		mask = &tset;
	}

	sigclear(&p->p_sigpend, mask);

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		sigclear(l->l_sigpend, mask);
	}
}

/*
 * sigpinch:
 *
 *	Pull non-LWP-private pending signals off a first set according to a
 *	mask, and add them to a second.
 */ 
void
sigpinch(sigpend_t *sp, sigpend_t *to, sigset_t *mask)
{
        ksiginfo_t *ksi, *next;
	int signo, global;
	sigset_t tset;

	tset = *mask;
	__sigandset(&sp->sp_set, &tset);

	while ((signo = firstsig(&tset)) != 0) {
		/*
		 * If the KSI is LWP private, then bin it.  Remember
		 * whether or not we found a process wide signal.
		 */
		global = 0;

		ksi = CIRCLEQ_FIRST(&sp->sp_info);
		for (; ksi != (void *)&sp->sp_info; ksi = next) {
			next = CIRCLEQ_NEXT(ksi, ksi_list);
			if (ksi->ksi_signo != signo)
				continue;
			CIRCLEQ_REMOVE(&sp->sp_info, ksi, ksi_list);
			if (ksi->ksi_lid != 0)
				pool_put(&ksiginfo_pool, ksi);
			else {
				global = 1;
				CIRCLEQ_INSERT_TAIL(&to->sp_info, ksi,
				    ksi_list);
			}
		}

		sigdelset(&sp->sp_set, signo);
		if (global)
			sigaddset(&to->sp_set, signo);
	}
}

/*
 * sigispending:
 *
 *	Return true if there are pending signals for the current LWP.
 */ 
int
sigispending(struct lwp *l)
{
	int signo;

	LOCK_ASSERT(mutex_owned(&l->l_proc->p_smutex));

	if ((signo = firstsig(&l->l_sigpend->sp_set)) != 0 &&
	    !sigismember(l->l_sigmask, signo)) {
	    	lwp_lock(l);
		signotify(l);	/* paranoia */
		lwp_unlock(l);
		return ERESTART;
	}

	return 0;
}

/*
 * siginfo_alloc:
 *
 *	 Allocate a new siginfo_t structure from the pool.
 */
siginfo_t *
siginfo_alloc(int flags)
{

	return pool_get(&siginfo_pool, flags);
}

/*
 * siginfo_free:
 *
 *	 Return a siginfo_t structure to the pool.
 */
void
siginfo_free(void *arg)
{

	pool_put(&siginfo_pool, arg);
}

void
getucontext(struct lwp *l, ucontext_t *ucp)
{

	/* XXXAD check locking */

	ucp->uc_flags = 0;
	ucp->uc_link = l->l_ctxlink;

	(void)sigprocmask1(l, 0, NULL, &ucp->uc_sigmask);
	ucp->uc_flags |= _UC_SIGMASK;

	/*
	 * The (unsupplied) definition of the `current execution stack'
	 * in the System V Interface Definition appears to allow returning
	 * the main context stack.
	 *
	 * XXXLWP this is borken for multiple LWPs.
	 */
	if ((l->l_sigstk->ss_flags & SS_ONSTACK) == 0) {
		ucp->uc_stack.ss_sp = (void *)USRSTACK;
		ucp->uc_stack.ss_size = ctob(l->l_proc->p_vmspace->vm_ssize);
		ucp->uc_stack.ss_flags = 0;	/* XXX, def. is Very Fishy */
	} else {
		/* Simply copy alternate signal execution stack. */
		ucp->uc_stack = *l->l_sigstk;
	}
	ucp->uc_flags |= _UC_STACK;

	cpu_getmcontext(l, &ucp->uc_mcontext, &ucp->uc_flags);
}

int
setucontext(struct lwp *l, const ucontext_t *ucp)
{
	struct proc	*p;
	int		error;

	/* XXXAD check locking */

	p = l->l_proc;
	if ((error = cpu_setmcontext(l, &ucp->uc_mcontext, ucp->uc_flags)) != 0)
		return (error);
	l->l_ctxlink = ucp->uc_link;

	if ((ucp->uc_flags & _UC_SIGMASK) != 0)
		sigprocmask1(l, SIG_SETMASK, &ucp->uc_sigmask, NULL);

	/*
	 * If there was stack information, update whether or not we are
	 * still running on an alternate signal stack.
	 */
	if ((ucp->uc_flags & _UC_STACK) != 0) {
		if (ucp->uc_stack.ss_flags & SS_ONSTACK)
			l->l_sigstk->ss_flags |= SS_ONSTACK;
		else
			l->l_sigstk->ss_flags &= ~SS_ONSTACK;
	}

	return 0;
}

/*
 * Common code for kill process group/broadcast kill.  cp is calling
 * process.
 */
int
killpg1(struct lwp *l, ksiginfo_t *ksi, int pgid, int all)
{
	struct proc	*p, *cp;
	kauth_cred_t	pc;
	struct pgrp	*pgrp;
	int		nfound;
	int		signo = ksi->ksi_signo;

	cp = l->l_proc;
	pc = l->l_cred;
	nfound = 0;

	rw_enter(&proclist_lock, RW_READER);
	if (all) {
		/*
		 * broadcast
		 */
		PROCLIST_FOREACH(p, &allproc) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM || p == cp)
				continue;
			mutex_enter(&p->p_mutex);
			if (kauth_authorize_process(pc,
			    KAUTH_PROCESS_CANSIGNAL, p,
			    (void *)(uintptr_t)signo, NULL, NULL) == 0) {
				nfound++;
				if (signo) {
					mutex_enter(&p->p_smutex);
					kpsignal2(p, ksi);
					mutex_exit(&p->p_smutex);
				}
			}
			mutex_exit(&p->p_mutex);
		}
	} else {
		if (pgid == 0)
			/*
			 * zero pgid means send to my process group.
			 */
			pgrp = cp->p_pgrp;
		else {
			pgrp = pg_find(pgid, PFIND_LOCKED);
			if (pgrp == NULL)
				goto out;
		}
		LIST_FOREACH(p, &pgrp->pg_members, p_pglist) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM)
				continue;
			mutex_enter(&p->p_mutex);
			if (kauth_authorize_process(pc, KAUTH_PROCESS_CANSIGNAL,
			    p, (void *)(uintptr_t)signo, NULL, NULL) == 0) {
				nfound++;
				if (signo) {
					mutex_enter(&p->p_smutex);
					if (P_ZOMBIE(p) == 0)
						kpsignal2(p, ksi);
					mutex_exit(&p->p_smutex);
				}
			}
			mutex_exit(&p->p_mutex);
		}
	}
  out:
	rw_exit(&proclist_lock);
	return (nfound ? 0 : ESRCH);
}

/*
 * Send a signal to a process group. If checktty is 1, limit to members
 * which have a controlling terminal.
 */
void
pgsignal(struct pgrp *pgrp, int sig, int checkctty)
{
	ksiginfo_t ksi;

	KSI_INIT_EMPTY(&ksi);
	ksi.ksi_signo = sig;
	kpgsignal(pgrp, &ksi, NULL, checkctty);
}

void
kpgsignal(struct pgrp *pgrp, ksiginfo_t *ksi, void *data, int checkctty)
{
	struct proc *p;

	LOCK_ASSERT(mutex_owned(&proclist_mutex) ||
	    rw_lock_held(&proclist_lock));

	if (pgrp)
		LIST_FOREACH(p, &pgrp->pg_members, p_pglist)
			if (checkctty == 0 || p->p_lflag & PL_CONTROLT)
				kpsignal(p, ksi, data);
}

/*
 * Send a signal caused by a trap to the current LWP.  If it will be caught
 * immediately, deliver it with correct code.  Otherwise, post it normally.
 */
void
trapsignal(struct lwp *l, ksiginfo_t *ksi)
{
	struct proc	*p;
	struct sigacts	*ps;
	int signo = ksi->ksi_signo;

	KASSERT(KSI_TRAP_P(ksi));

	ksi->ksi_lid = l->l_lid;
	p = l->l_proc;

	mutex_enter(&p->p_smutex);	
	ps = p->p_sigacts;
	if ((p->p_slflag & PSL_TRACED) == 0 &&
	    sigismember(&p->p_sigctx.ps_sigcatch, signo) &&
	    !sigismember(l->l_sigmask, signo)) {
		p->p_stats->p_ru.ru_nsignals++;
		kpsendsig(l, ksi, l->l_sigmask);
		mutex_exit(&p->p_smutex);
#ifdef KTRACE
		if (KTRPOINT(p, KTR_PSIG))
			ktrpsig(l, signo, SIGACTION_PS(ps, signo).sa_handler,
			    l->l_sigmask, ksi);
#endif
	} else {
		/* XXX for core dump/debugger */
		p->p_sigctx.ps_lwp = l->l_lid;
		p->p_sigctx.ps_signo = ksi->ksi_signo;
		p->p_sigctx.ps_code = ksi->ksi_trap;
		kpsignal2(p, ksi);

		mutex_exit(&p->p_smutex);
	}
}

/*
 * Fill in signal information and signal the parent for a child status change.
 */
void
child_psignal(struct proc *p, int mask)
{
	ksiginfo_t ksi;
	struct proc *q;
	int xstat;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	LOCK_ASSERT(mutex_owned(&proclist_mutex) ||
	    rw_lock_held(&proclist_lock));

	xstat = p->p_xstat;

	KSI_INIT(&ksi);
	ksi.ksi_signo = SIGCHLD;
	ksi.ksi_code = xstat == SIGCONT ? CLD_CONTINUED : CLD_STOPPED;
	ksi.ksi_pid = p->p_pid;
	ksi.ksi_uid = kauth_cred_geteuid(p->p_cred);
	ksi.ksi_status = p->p_xstat;
	ksi.ksi_utime = p->p_stats->p_ru.ru_utime.tv_sec;
	ksi.ksi_stime = p->p_stats->p_ru.ru_stime.tv_sec;

	q = p->p_pptr;

	mutex_exit(&p->p_smutex);
	mutex_enter(&q->p_smutex);

	if (xstat == SIGCONT) {
		mutex_enter(&proc_stop_mutex);
		q->p_nstopchild++;
		mutex_exit(&proc_stop_mutex);
		cv_broadcast(&q->p_waitcv);
	}

	if ((q->p_sflag & mask) == 0)
		kpsignal2(q, &ksi);

	mutex_exit(&q->p_smutex);
	mutex_enter(&p->p_smutex);
}

void
psignal(struct proc *p, int signo)
{
	ksiginfo_t ksi;

	KSI_INIT_EMPTY(&ksi);
	ksi.ksi_signo = signo;
	mutex_enter(&p->p_smutex);
	kpsignal2(p, &ksi);
	mutex_exit(&p->p_smutex);
}

void
kpsignal(struct proc *p, ksiginfo_t *ksi, void *data)
{

	/* XXXSMP Why is this here? */
	if ((p->p_sflag & PS_WEXIT) == 0 && data) {
		size_t fd;
		struct filedesc *fdp = p->p_fd;

		ksi->ksi_fd = -1;
		for (fd = 0; fd < fdp->fd_nfiles; fd++) {
			struct file *fp = fdp->fd_ofiles[fd];
			/* XXX: lock? */
			if (fp && fp->f_data == data) {
				ksi->ksi_fd = fd;
				break;
			}
		}
	}
	mutex_enter(&p->p_smutex);
	kpsignal2(p, ksi);
	mutex_exit(&p->p_smutex);
}

/*
 * sigismasked:
 *
 *	 Returns true if signal is ignored or masked for the specified LWP.
 */
int
sigismasked(struct lwp *l, int sig)
{
	struct proc *p = l->l_proc;

	return (sigismember(&p->p_sigctx.ps_sigignore, sig) ||
	    sigismember(l->l_sigmask, sig));
}

/*
 * sigpost:
 *
 *	 Post a pending signal to an LWP.
 *
 *	 Returns non-zero if the LWP was able to take the signal.  Must be
 *	 called with the LWP locked, and will release the lock on return.
 */
int
sigpost(struct lwp *l, sig_t *actionp, int prop, ksiginfo_t **ksi,
	int idlecheck)
{
	struct proc *p = l->l_proc;
	ksiginfo_t *kp;
	sig_t action;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	kp = ksiginfo_alloc(p, *ksi, PR_NOWAIT);
	if (kp == NULL)
		return 1;
	*ksi = kp;

	lwp_lock(l);
	action = *actionp;

	/*
	 * When sending signals to SA processes, we first try to find an
	 * idle VP to take it.
	 */
	if (idlecheck && (l->l_flag & (L_SA_IDLE | L_SA_YIELD)) == 0) {
		lwp_unlock(l);
		return 0;
	}

	/*
	 * If we have no hope of taking the signal, then just bin it.
	 * If it's LWP private, then pretend that all is well.
	 */
	if (l->l_stat == LSZOMB || l->l_stat == LSDEAD || l->l_stat == LSIDL) {
	    	lwp_unlock(l);
	    	return kp->ksi_lid != 0;
	}

	/*
	 * If stopping or starting, clear any pending signals that would do
	 * the inverse (job control signals go to all LWPs).
	 */
	if (prop & SA_CONT)
		sigclear(l->l_sigpend, &stopsigmask);
	if (prop & SA_STOP)
		sigclear(l->l_sigpend, &contsigmask);

	/*
	 * Special case for signals that can't be masked.  If sending a STOP
	 * or KILL, the LWP must take the signal (job control, and kill
	 * signals go to all LWPs).
	 */
	if (prop & SA_CANTMASK) {
		if (prop & SA_KILL) {
			sigput(l->l_sigpend, p, kp);
			signotify(l);

			switch (l->l_stat) {
			case LSSUSPENDED:
				/* lwp_continue() will release the lock. */
				lwp_continue(l);
				return 1;
			case LSSLEEP:
				if ((l->l_flag & L_SINTR) == 0)
					break;
				/* FALLTHROUGH */
			case LSSTOP:
				/* setrunnable() will release the lock. */
				setrunnable(l);
				return 1;
			case LSRUN:
			case LSONPROC:
				break;
			default:
				panic("sigpost: can't take unmaskable sig");
				break;
			}

			lwp_unlock(l);
			return 1;
		}

		KASSERT(prop & SA_STOP);

		/*
		 * If the LWP is already stopped, there's no sense in trying
		 * to stop it again.  Otherwise post the signal and have it
		 * stop itself.
		 */
		if (l->l_stat != LSSTOP) {
			sigput(l->l_sigpend, p, kp);
			signotify(l);
		}

		lwp_unlock(l);
		return 1;
	}

	if (sigismember(l->l_sigmask, kp->ksi_signo))
		action = SIG_HOLD;

	if (l->l_stat == LSSTOP) {
		/*
		 * If the LWP is stopped and we are sending a continue
		 * signal, then start it again.  If the signal is held, add
		 * it to the pending set.  If the process wants to catch the
		 * signal, then notify the LWP to take it when it resumes.
		 *
		 * In the SA case, we send continue signals to all VPs in
		 * the process; only ONE of them may catch the signal.
		 */
		if ((prop & SA_CONT) != 0) {
			if (action == SIG_HOLD || action == SIG_CATCH) {
				sigput(l->l_sigpend, p, kp);
				if (action == SIG_CATCH)
					signotify(l);
				if (p->p_sa != NULL)
					*actionp = SIG_DFL;
			}
			/* setrunnable will release the lock. */
			setrunnable(l);
			return 1;
		}

		if ((l->l_flag & L_SINTR) != 0) {
			/*
			 * If sleeping interruptably, make it run and take
			 * the signal.  If the process is stopped, and was
			 * sleeping interruptably, it will run until the
			 * kernel boundary and notice that it needs to stop
			 * again.
			 *
			 * setrunnable() will release the LWP lock.
			 */
			sigput(l->l_sigpend, p, kp);
			signotify(l);
			setrunnable(l);
			return 1;
		} 

		if (kp->ksi_lid != 0 || (prop & SA_TOALL) != 0) {
			/*
			 * The signal is LWP private or must be seen by all
			 * LWPs, so save it for later.
			 */
			sigput(l->l_sigpend, p, kp);
			signotify(l);
			lwp_unlock(l);
			return 1;
		}
	
		lwp_unlock(l);
		return 0;
	}

	/*
	 * If the LWP is running or on a run queue, then we win.  If it's
	 * sleeping interruptably, wake it and make it take the signal.  If
	 * the sleep isn't interruptable, then the chances are it will get
	 * to see the signal soon anyhow.  If suspended, it can't take the
	 * signal right now.  If it's LWP private or for all LWPs, save it
	 * for later; otherwise punt.
	 */
	switch (l->l_stat) {
	case LSRUN:
	case LSONPROC:
	case LSSLEEP:
		sigput(l->l_sigpend, p, kp);
		if (action != SIG_HOLD) {
			signotify(l);
			if ((l->l_flag & L_SINTR) != 0) {
				/* setrunnable() will release the lock. */
				setrunnable(l);
				return 1;
			}
		}
		lwp_unlock(l);
		return 1;

	case LSSUSPENDED:
		if (kp->ksi_lid != 0 || (prop & SA_TOALL) != 0) {
			sigput(l->l_sigpend, p, kp);
			if (action != SIG_HOLD)
				signotify(l);
			lwp_unlock(l);
			return 1;
		}
		lwp_unlock(l);
		break;

	default:
		panic("sigpost: impossible");
		break;
	}

	KASSERT(p->p_stat == SACTIVE && p->p_nlwps > 1);
	return 0;
}

/*
 * Notify an LWP that it has a pending signal.
 */
void
signotify(struct lwp *l)
{

	LOCK_ASSERT(lwp_locked(l, NULL));
	l->l_flag |= L_PENDSIG;
	lwp_changepri(l, PUSER);
	cpu_signotify(l);
}

/*
 * Find an LWP within process p that is waiting on signal ksi, and hand
 * it on.
 */
int
sigunwait(struct proc *p, const ksiginfo_t *ksi)
{
	struct lwp *l;
	int signo;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	signo = ksi->ksi_signo;

	if (ksi->ksi_lid != 0) {
		/*
		 * Signal came via _lwp_kill().  Find the LWP and see if
		 * it's interested.
		 */
		l = lwp_byid(p, ksi->ksi_lid);

		if (l == NULL)
			return 0;

		if (l->l_sigwaited == NULL || l->l_stat != LSSLEEP ||
		    !sigismember(l->l_sigwait, signo)) {
			lwp_unlock(l);
			return 0;
		}
	} else {
		/*
		 * Look for any LWP that may be interested.
		 */
		LIST_FOREACH(l, &p->p_sigwaiters, l_sigwaiter) {
			lwp_lock(l);
			KASSERT(l->l_sigwaited != NULL);
			if (l->l_stat == LSSLEEP &&
			    sigismember(l->l_sigwait, signo))
				break;
			lwp_unlock(l);
		}
	}

	if (l != NULL) {
		l->l_sigwaited->ksi_info = ksi->ksi_info;
		l->l_sigwaited = NULL;
		lwp_unlock(l);
		wakeup_one(&l->l_sigwait);
		return 1;
	}

	return 0;
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
 */
void
kpsignal2(struct proc *p, ksiginfo_t *ksi)
{
	int prop, lid, signo = ksi->ksi_signo;
	struct sadata_vp *vp;
	struct lwp *l;
	ksiginfo_t *kp;
	sig_t action;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	KASSERT((ksi->ksi_flags & KSI_QUEUED) == 0);

	KASSERT(signo > 0 && signo < NSIG);

	/*
	 * If the process is being created by fork, is a zombie or is
	 * exiting, then just drop the signal here and bail out.
	 */
	if (p->p_stat != SACTIVE && p->p_stat != SSTOP)
		return;

	/*
	 * Notify any interested parties of the signal.
	 */	
	KNOTE(&p->p_klist, NOTE_SIGNAL | signo);

	/*
	 * Some signals including SIGKILL must act on the entire process.
	 */
	prop = sigprop[signo];

	if ((prop & SA_TOALL) != 0)
		lid = 0;
	else
		lid = ksi->ksi_lid;

	if (p->p_slflag & PSL_TRACED) {
		/*
		 * If proc is traced, always give parent a chance.
		 */
		action = SIG_DFL;
	} else {
		/*
		 * If the signal was the result of a trap and is not being
		 * caught, then reset it to default action so that the
		 * process dumps core immediately.
		 */
		if (KSI_TRAP_P(ksi)) {
			if (!sigismember(&p->p_sigctx.ps_sigcatch, signo)) {
				sigdelset(&p->p_sigctx.ps_sigignore, signo);
				SIGACTION(p, signo).sa_handler = SIG_DFL;
			}
		}

		/*
		 * If the signal is being ignored, then drop it.  Note: we
		 * don't set SIGCONT in ps_sigignore, and if it is set to
		 * SIG_IGN, action will be SIG_DFL here.
		 */
		if (sigismember(&p->p_sigctx.ps_sigignore, signo))
			return;

		else if (sigismember(&p->p_sigctx.ps_sigcatch, signo))
			action = SIG_CATCH;
		else {
			action = SIG_DFL;

			/*
			 * If sending a tty stop signal to a member of an
			 * orphaned process group, discard the signal here if
			 * the action is default; don't stop the process below
			 * if sleeping, and don't clear any pending SIGCONT.
			 */
			if (prop & SA_TTYSTOP &&
			    (p->p_sflag & PS_ORPHANPG) != 0)
				return;

			if (prop & SA_KILL && p->p_nice > NZERO)
				p->p_nice = NZERO;
		}
	}

	/*
	 * If stopping or continuing a process, discard any pending
	 * signals that would do the inverse.
	 */
	if (prop & SA_CONT)
		sigclear(&p->p_sigpend, &stopsigmask);
	if (prop & SA_STOP)
		sigclear(&p->p_sigpend, &contsigmask);

	/* 
	 * If the signal doesn't have SA_CANTMASK (no override for SIGKILL,
	 * please!), check if any LWPs are waiting on it.  If yes, pass on
	 * the signal info.  The signal won't be processed further here.
	 */
	if ((prop & SA_CANTMASK) == 0 && !LIST_EMPTY(&p->p_sigwaiters) &&
	    p->p_stat == SACTIVE && sigunwait(p, ksi))
		return;

	if ((kp = ksiginfo_alloc(p, ksi, PR_NOWAIT)) == NULL)
		return;

	/*
	 * LWP private signals are easy - just find the LWP and post
	 * the signal to it.
	 */
	if (lid != 0) {
		l = lwp_byid(p, lid);
		if (l != NULL) {
			lwp_unlock(l);
			if (!sigpost(l, &action, prop, &kp, 0)) {
#ifdef DIAGNOSTIC
				panic("kpsignal2: did not take private sig");
#endif
			}
		}
		goto out;
	}

	/*
	 * Some signals go to all LWPs, even for SA processes.
	 */
	if (p->p_stat == SACTIVE && (prop & SA_TOALL) != 0) {
		/*
		 * If a child holding parent blocked, stopping could
		 * cause deadlock: discard the signal.
		 */
		if ((p->p_sflag & PS_PPWAIT) != 0)
			goto out;

		if ((prop & SA_STOP) != 0) {
			/*
			 * If already stopped, no further action is required.
			 */
			if (p->p_stat == SSTOP)
				goto out;
			p->p_stat = SSTOP;
		}

		if (p->p_sa != NULL) {
			SLIST_FOREACH(vp, &p->p_sa->sa_vps, savp_next) {
				l = vp->savp_lwp;
				(void)sigpost(l, &action, prop, &kp, 0);
			}
		} else {
			LIST_FOREACH(l, &p->p_lwps, l_sibling)
				(void)sigpost(l, &action, prop, &kp, 0);
		}

		if ((prop & SA_STOP) != 0)
			child_psignal(p, PS_NOCLDSTOP);

		goto out;
	}

	if (p->p_stat == SSTOP) {
		/*
		 * Process is stopped.  If traced, then no further action is
		 * necessary.
		 */
		if (p->p_slflag & PSL_TRACED)
			goto out;

		if (prop & SA_CONT)
			p->p_stat = SACTIVE;
		else if (prop & SA_STOP) {
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			goto out;
		}

		/*
		 * Some signals go to all LWPs.
		 */
		if (prop & SA_TOALL) {
			if (p->p_sa != NULL) {
				SLIST_FOREACH(vp, &p->p_sa->sa_vps, savp_next) {
					l = vp->savp_lwp;
					(void)sigpost(l, &action, prop, &kp, 0);
				}
			} else {
				LIST_FOREACH(l, &p->p_lwps, l_sibling) {
					(void)sigpost(l, &action, prop, &kp, 0);
				}
			}
			goto out;
		}
	}

	/*
	 * Try to find an LWP that can take the signal.
	*/
	if (p->p_sa != NULL) {
		/*
		 * In the SA case, we try to find an idle LWP that can take
		 * the signal.  If that fails, only then do we consider
		 * interrupting active LWPs.
		 */
		l = NULL;
		SLIST_FOREACH(vp, &p->p_sa->sa_vps, savp_next) {
			l = vp->savp_lwp;
			if (sigpost(l, &action, prop, &kp, 1))
				break;
		}

		if (l == NULL) {
			SLIST_FOREACH(vp, &p->p_sa->sa_vps, savp_next) {
				l = vp->savp_lwp;
				if (sigpost(l, &action, prop, &kp, 0))
					break;
			}
		}
	} else {
		/*
		 * For non-SA processes, just try all LWPs until we find one
		 * willing to do the needful.
		 */
		LIST_FOREACH(l, &p->p_lwps, l_sibling)
			if (sigpost(l, &action, prop, &kp, 0))
				break;
	}

	/*
	 * We couldn't find somebody to take the signal.  It must be held by
	 * all LWPs, so put it onto the per process queue in expectation
	 * that an LWP will adjust its signal mask in the future, and will
	 * then pick it up.
	 */
	if (l == NULL)
		sigput(&p->p_sigpend, p, kp);

	return;

 out:
 	/*
 	 * If the signal wasn't taken, then bin the ksiginfo_t we allocated.
 	 */
 	ksiginfo_free(kp);
}

void
kpsendsig(struct lwp *l, const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct proc *p = l->l_proc;
	struct lwp *le, *li;
	siginfo_t *si;
	int f;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	if (p->p_flag & P_SA) {
		/* XXXAD What if already on signal stack? */
		/* XXXUPSXXX What if not on sa_vp ? */

		/* XXXAD SA locking, PR_WAITOK */
		si = siginfo_alloc(PR_WAITOK);

		lwp_lock(l);
		f = l->l_flag & L_SA;
		l->l_flag &= ~L_SA;
		si->_info = ksi->ksi_info;
		le = li = NULL;
		if (KSI_TRAP_P(ksi))
			le = l;
		else
			li = l;
		if (sa_upcall(l, SA_UPCALL_SIGNAL | SA_UPCALL_DEFER, le, li,
		    sizeof(*si), si, siginfo_free) != 0) {
			siginfo_free(si);
#if 0
			if (KSI_TRAP_P(ksi))
				/* XXX What do we do here?? */;
#endif
		}
		l->l_flag |= f;
		lwp_unlock(l);
		return;
	}

	(*p->p_emul->e_sendsig)(ksi, mask);
}

/*
 * Stop the current process and switch away when being stopped or traced.
 */
void
sigswitch(int ppsig, int ppmask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	KASSERT(l->l_stat == LSONPROC);

	lwp_lock(l);

	p->p_nrlwps--;
	l->l_stat = LSSTOP;

	/*
	 * Unlock and switch away.  If we are the last live LWP, and
	 * the stop was a result of a new signal, then take the ugly
	 * hit of relocking and signal the parent.
	 */
	mutex_exit(&p->p_smutex);

	if (p->p_nrlwps == 0 && ppsig) {
		lwp_unlock_to(l, &lwp_mutex);
		mutex_enter(&proclist_mutex);
		mutex_enter(&p->p_smutex);
		if (p->p_nrlwps == 0)
			child_psignal(p, ppmask);
		mutex_exit(&p->p_smutex);
		mutex_exit(&proclist_mutex);
		lwp_lock(l);
	} else
		lwp_relock(l, &lwp_mutex);

	mi_switch(l, NULL);
	mutex_enter(&p->p_smutex);
}

/*
 * Check for a signal from the debugger.
 */
int
sigchecktrace(void)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int signo;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	/*
	 * If we are no longer being traced, or the parent didn't
	 * give us a signal, look for more signals.
	 */
	if ((p->p_slflag & PSL_TRACED) == 0 || p->p_xstat == 0)
		return 0;

	/*
	 * If the new signal is being masked, look for other signals.
	 * `p->p_sigctx.ps_siglist |= mask' is done in setrunnable().
	 */
	signo = p->p_xstat;
	p->p_xstat = 0;
	if (!sigismember(l->l_sigmask, signo))
		signo = 0;

	return signo;
}

/*
 * If the current process has received a signal (should be caught or cause
 * termination, should interrupt current syscall), return the signal number.
 *
 * Stop signals with default action are processed immediately, then cleared;
 * they aren't returned.  This is checked after each entry to the system for
 * a syscall or trap.
 *
 * We will also return -1 if the process is exiting and the current LWP must
 * follow suit.
 *
 * Note that we may be called while on a sleep queue, so MUST NOT sleep.  We
 * can switch away, though.
 */
int
issignal(struct lwp *l)
{
	struct proc *p = l->l_proc;
	int signo = 0, prop;
	sigset_t ss;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	for (;;) {
		/* Discard any signals that we have decided not to take. */
		if (signo != 0)
			(void)sigget(l->l_sigpend, NULL, signo, NULL);

		/* Bail out if we do not own the virtual processor */
		if (l->l_flag & L_SA && l->l_savp->savp_lwp != l)
			break;

		/*
		 * If the process is stopped/stopping, then stop ourselves
		 * now that we're on the kernel/userspace boundary.  When
		 * we awaken, check for a signal from the debugger.
		 */
		if (p->p_stat == SSTOP) {
			sigswitch(0, 0);
			signo = sigchecktrace();
		} else
			signo = 0;

		/*
		 * If the debugger didn't provide a signal, find a pending
		 * signal from our set.
		 */
		if (signo == 0) {
			ss = l->l_sigpend->sp_set;
			if ((p->p_sflag & PS_PPWAIT) != 0)
				sigminusset(&stopsigmask, &ss);
			sigminusset(l->l_sigmask, &ss);

			if ((signo = firstsig(&ss)) == 0) {
				/*
				 * No signal pending - clear the indicator
				 * and bail out.
				 */
				lwp_lock(l);
				l->l_flag &= ~L_PENDSIG;
				lwp_unlock(l);
				break;
			}
		}

		/*
		 * We should see pending but ignored signals only if
		 * we are being traced.
		 */
		if (sigismember(&p->p_sigctx.ps_sigignore, signo) &&
		    (p->p_slflag & PSL_TRACED) == 0) {
			/* Discard the signal. */
			continue;
		}

		/*
		 * If traced, always stop, and stay stopped until released
		 * by the debugger.  If the our parent process is waiting
		 * for us, don't hang as we could deadlock.
		 */
		if ((p->p_slflag & PSL_TRACED) != 0 &&
		    (p->p_sflag & PS_PPWAIT) != 0) {
			/* Take the signal. */
			(void)sigget(l->l_sigpend, NULL, signo, NULL);

			p->p_xstat = signo;

			/* Emulation-specific handling of signal trace */
			if (p->p_emul->e_tracesig == NULL ||
			    (*p->p_emul->e_tracesig)(p, signo) == 0)
				sigswitch(!(p->p_slflag & PSL_FSTRACE), 0);

			/* Check for a signal from the debugger. */
			if ((signo = sigchecktrace()) == 0)
				continue;
		}

		prop = sigprop[signo];

		/*
		 * Decide whether the signal should be returned.
		 */
		switch ((long)SIGACTION(p, signo).sa_handler) {
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
				    p->p_pid, signo);
#endif
				continue;
			}

			/*
			 * If there is a pending stop signal to process with
			 * default action, stop here, then clear the signal. 
			 * However, if process is member of an orphaned
			 * process group, ignore tty stop signals.
			 */
			if (prop & SA_STOP) {
				if (p->p_slflag & PSL_TRACED ||
		    		    ((p->p_sflag & PS_ORPHANPG) != 0 &&
				    prop & SA_TTYSTOP)) {
				    	/* Ignore the signal. */
					continue;
				}
				/* Discard the signal. */
				(void)sigget(l->l_sigpend, NULL, signo, NULL);
				p->p_xstat = signo;
				(void)sigswitch(0, PS_NOCLDSTOP);
			} else if (prop & SA_IGNORE) {
				/*
				 * Except for SIGCONT, shouldn't get here.
				 * Default action is to ignore; drop it.
				 */
				continue;
			}

			break;

		case (long)SIG_IGN:
#ifdef DEBUG_ISSIGNAL
			/*
			 * Masking above should prevent us ever trying
			 * to take action on an ignored signal other
			 * than SIGCONT, unless process is traced.
			 */
			if ((prop & SA_CONT) == 0 &&
			    (p->p_sflag & P_STRACED) == 0)
				printf("issignal\n");
#endif
			continue;

		default:
			/*
			 * This signal has an action, let postsig() process
			 * it.
			 */
			break;
		}

		break;
	}

	return signo;
}

/*
 * Take the action for the specified signal
 * from the current set of pending signals.
 */
void
postsig(int signo)
{
	struct lwp	*l;
	struct proc	*p;
	struct sigacts	*ps;
	sig_t		action;
	sigset_t	*returnmask;
	ksiginfo_t	ksi;

	l = curlwp;
	p = l->l_proc;
	ps = p->p_sigacts;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	KASSERT(signo > 0);

	/*
	 * Set the new mask value and also defer further occurrences of this
	 * signal.
	 *
	 * Special case: user has done a sigpause.  Here the current mask is
	 * not of interest, but rather the mask from before the sigpause is
	 * what we want restored after the signal processing is completed.
	 */
	if (l->l_sigrestore) {
		returnmask = &l->l_sigoldmask;
		l->l_sigrestore = 0;
	} else
		returnmask = l->l_sigmask;

	/*
	 * Commit to taking the signal before releasing the mutex.
	 */
	action = SIGACTION_PS(ps, signo).sa_handler;
	p->p_stats->p_ru.ru_nsignals++;
	sigget(l->l_sigpend, &ksi, signo, NULL);

#ifdef KTRACE
	/* XXXSMP */
	mutex_exit(&p->p_smutex);
	KERNEL_LOCK(1, l);
	if (KTRPOINT(p, KTR_PSIG))
		ktrpsig(l, signo, action, returnmask, NULL);
	(void)KERNEL_UNLOCK(1, l);
	mutex_enter(&p->p_smutex);
#endif

	if (action == SIG_DFL) {
		/*
		 * Default action, where the default is to kill
		 * the process.  (Other cases were ignored above.)
		 */
		sigexit(l, signo);
		return;
	}

	/*
	 * If we get here, the signal must be caught.
	 */
#ifdef DIAGNOSTIC
	if (action == SIG_IGN || sigismember(l->l_sigmask, signo))
		panic("postsig action");
#endif

	kpsendsig(l, &ksi, returnmask);
}

/*
 * sendsig_reset:
 *
 *	Reset the signal action.  Called from emulation specific sendsig()
 *	before unlocking to deliver the signal.
 */
void
sendsig_reset(struct lwp *l, int signo)
{
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	p->p_sigctx.ps_lwp = 0;
	p->p_sigctx.ps_code = 0;
	p->p_sigctx.ps_signo = 0;

	sigplusset(&SIGACTION_PS(ps, signo).sa_mask, l->l_sigmask);
	if (SIGACTION_PS(ps, signo).sa_flags & SA_RESETHAND) {
		sigdelset(&p->p_sigctx.ps_sigcatch, signo);
		if (signo != SIGCONT && sigprop[signo] & SA_IGNORE)
			sigaddset(&p->p_sigctx.ps_sigignore, signo);
		SIGACTION_PS(ps, signo).sa_handler = SIG_DFL;
	}
}

/*
 * Kill the current process for stated reason.
 */
void
killproc(struct proc *p, const char *why)
{
	log(LOG_ERR, "pid %d was killed: %s\n", p->p_pid, why);
	psignal(p, SIGKILL);
}

/*
 * Force the current process to exit with the specified signal, dumping core
 * if appropriate.  We bypass the normal tests for masked and caught
 * signals, allowing unrecoverable failures to terminate the process without
 * changing signal state.  Mark the accounting record with the signal
 * termination.  If dumping core, save the signal number for the debugger. 
 * Calls exit and does not return.
 */
void
sigexit(struct lwp *l, int signo)
{
	int exitsig, error, docore;
	struct proc *p;
	struct lwp *t;

	p = l->l_proc;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	/*
	 * Don't permit coredump() multiple times in the same process.
	 */
	mb_read();
	if ((l->l_flag & L_WCORE) != 0) {
		mutex_exit(&p->p_smutex);
		(void)lwp_exit(l, 0);
		return;
	}

	/*
	 * Prepare all other LWPs for exit.  If dumping core, suspend them
	 * so that their registers are available long enough to be dumped.
 	 */
	if ((docore = (sigprop[signo] & SA_CORE)) != 0) {
		LIST_FOREACH(t, &p->p_lwps, l_sibling) {
			if (t == l)
				continue;
			lwp_lock(t);
			t->l_flag |= (L_WCORE | L_WEXIT);
			lwp_halt(l, t, LSSUSPENDED);
		}

		/*
		 * Wait for everyone else to stop before proceeding.
		 */
		while (!lwp_lastlive(0))
			mtsleep(&p->p_nrlwps, PWAIT, "sigexit", 0,
			    &p->p_smutex);
	}

	/* We don't want to switch away from exiting. */
#if 0
	if (p->p_flag & P_SA) {
		LIST_FOREACH(l2, &p->p_lwps, l_sibling)
		    l2->l_flag &= ~L_SA;
		p->p_flag &= ~P_SA;
	}
#endif

	exitsig = signo;
	p->p_acflag |= AXSIG;
	p->p_sigctx.ps_signo = signo;
	mutex_exit(&p->p_smutex);

	KERNEL_LOCK(1, l);

	if (docore) {
		if ((error = coredump(l, NULL)) == 0)
			exitsig |= WCOREFLAG;

		if (kern_logsigexit) {
			int uid = l->l_cred ?
			    (int)kauth_cred_geteuid(l->l_cred) : -1;

			if (error)
				log(LOG_INFO, lognocoredump, p->p_pid,
				    p->p_comm, uid, signo, error);
			else
				log(LOG_INFO, logcoredump, p->p_pid,
				    p->p_comm, uid, signo);
		}
	}

	/* Acquire the sched state mutex.  exit1() will release it. */
	mutex_enter(&p->p_smutex);

	exit1(l, W_EXITCODE(0, exitsig));
	/* NOTREACHED */
}

static int
filt_sigattach(struct knote *kn)
{
	struct proc *p = curproc;

	kn->kn_ptr.p_proc = p;
	kn->kn_flags |= EV_CLEAR;               /* automatically set */

	SLIST_INSERT_HEAD(&p->p_klist, kn, kn_selnext);

	return (0);
}

static void
filt_sigdetach(struct knote *kn)
{
	struct proc *p = kn->kn_ptr.p_proc;

	SLIST_REMOVE(&p->p_klist, kn, knote, kn_selnext);
}

/*
 * signal knotes are shared with proc knotes, so we apply a mask to
 * the hint in order to differentiate them from process hints.  This
 * could be avoided by using a signal-specific knote list, but probably
 * isn't worth the trouble.
 */
static int
filt_signal(struct knote *kn, long hint)
{

	if (hint & NOTE_SIGNAL) {
		hint &= ~NOTE_SIGNAL;

		if (kn->kn_id == hint)
			kn->kn_data++;
	}
	return (kn->kn_data != 0);
}

const struct filterops sig_filtops = {
	0, filt_sigattach, filt_sigdetach, filt_signal
};
