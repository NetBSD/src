/*	$NetBSD: signalvar.h,v 1.32 2002/03/19 20:50:41 christos Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)signalvar.h	8.6 (Berkeley) 2/19/95
 */

#ifndef	_SYS_SIGNALVAR_H_		/* tmp for user.h */
#define	_SYS_SIGNALVAR_H_

/*
 * Kernel signal definitions and data structures,
 * not exported to user programs.
 */

/*
 * Process signal actions, possibly shared between threads.
 */
struct sigacts {
	struct sigaction sa_sigact[NSIG];      /* disposition of signals */

	int	sa_refcnt;		/* reference count */
};

/*
 * Process signal state.
 */
struct	sigctx {
	/* This needs to be zeroed on fork */
	sigset_t ps_siglist;		/* Signals arrived but not delivered. */
	char	ps_sigcheck;		/* May have deliverable signals. */

	/* This should be copied on fork */
#define	ps_startcopy	ps_sigstk
	struct	sigaltstack ps_sigstk;	/* sp & on stack state variable */
	sigset_t ps_oldmask;		/* saved mask from before sigpause */
	int	ps_flags;		/* signal flags, below */
	int	ps_sig;			/* for core dump/debugger XXX */
	long	ps_code;		/* for core dump/debugger XXX */
	void	*ps_sigcode;		/* address of signal trampoline */
	sigset_t ps_sigmask;		/* Current signal mask. */
	sigset_t ps_sigignore;		/* Signals being ignored. */
	sigset_t ps_sigcatch;		/* Signals being caught by user. */
};

/* signal flags */
#define	SAS_OLDMASK	0x01		/* need to restore mask before pause */

/* additional signal action values, used only temporarily/internally */
#define	SIG_CATCH	(void (*) __P((int)))2
#define	SIG_HOLD	(void (*) __P((int)))3

/*
 * get signal action for process and signal; currently only for current process
 */
#define SIGACTION(p, sig)	(p->p_sigacts->sa_sigact[(sig)])
#define	SIGACTION_PS(ps, sig)	(ps->sa_sigact[(sig)])

/*
 * Mark that signals for a process need to be checked.
 */
#define	CHECKSIGS(p)							\
do {									\
	(p)->p_sigctx.ps_sigcheck = 1;					\
	signotify(p);							\
} while (/* CONSTCOND */ 0)

/*
 * Determine signal that should be delivered to process p, the current
 * process, 0 if none.  If there is a pending stop signal with default
 * action, the process stops in issignal().
 */
#define	CURSIG(p)	(p->p_sigctx.ps_sigcheck ? issignal(p) : 0)

/*
 * Clear a pending signal from a process.
 */
#define	CLRSIG(p, sig)	sigdelset(&p->p_sigctx.ps_siglist, sig)

/*
 * Signal properties and actions.
 * The array below categorizes the signals and their default actions
 * according to the following properties:
 */
#define	SA_KILL		0x01		/* terminates process by default */
#define	SA_CORE		0x02		/* ditto and coredumps */
#define	SA_STOP		0x04		/* suspend process */
#define	SA_TTYSTOP	0x08		/* ditto, from tty */
#define	SA_IGNORE	0x10		/* ignore by default */
#define	SA_CONT		0x20		/* continue if suspended */
#define	SA_CANTMASK	0x40		/* non-maskable, catchable */
#define	SA_NORESET	0x80		/* not reset when caught */

#ifdef	SIGPROP
const int sigprop[NSIG] = {
	0,				/* 0 unused */
	SA_KILL,			/* 1 SIGHUP */
	SA_KILL,			/* 2 SIGINT */
	SA_KILL|SA_CORE,		/* 3 SIGQUIT */
	SA_KILL|SA_CORE|SA_NORESET,	/* 4 SIGILL */
	SA_KILL|SA_CORE|SA_NORESET,	/* 5 SIGTRAP */
	SA_KILL|SA_CORE,		/* 6 SIGABRT */
	SA_KILL|SA_CORE,		/* 7 SIGEMT */
	SA_KILL|SA_CORE,		/* 8 SIGFPE */
	SA_KILL|SA_CANTMASK,		/* 9 SIGKILL */
	SA_KILL|SA_CORE,		/* 10 SIGBUS */
	SA_KILL|SA_CORE,		/* 11 SIGSEGV */
	SA_KILL|SA_CORE,		/* 12 SIGSYS */
	SA_KILL,			/* 13 SIGPIPE */
	SA_KILL,			/* 14 SIGALRM */
	SA_KILL,			/* 15 SIGTERM */
	SA_IGNORE,			/* 16 SIGURG */
	SA_STOP|SA_CANTMASK,		/* 17 SIGSTOP */
	SA_STOP|SA_TTYSTOP,		/* 18 SIGTSTP */
	SA_IGNORE|SA_CONT,		/* 19 SIGCONT */
	SA_IGNORE,			/* 20 SIGCHLD */
	SA_STOP|SA_TTYSTOP,		/* 21 SIGTTIN */
	SA_STOP|SA_TTYSTOP,		/* 22 SIGTTOU */
	SA_IGNORE,			/* 23 SIGIO */
	SA_KILL,			/* 24 SIGXCPU */
	SA_KILL,			/* 25 SIGXFSZ */
	SA_KILL,			/* 26 SIGVTALRM */
	SA_KILL,			/* 27 SIGPROF */
	SA_IGNORE,			/* 28 SIGWINCH  */
	SA_IGNORE,			/* 29 SIGINFO */
	SA_KILL,			/* 30 SIGUSR1 */
	SA_KILL,			/* 31 SIGUSR2 */
	SA_IGNORE|SA_NORESET,		/* 32 SIGPWR */
	SA_KILL,			/* 33 SIGRTMIN + 0 */
	SA_KILL,			/* 34 SIGRTMIN + 1 */
	SA_KILL,			/* 35 SIGRTMIN + 2 */
	SA_KILL,			/* 36 SIGRTMIN + 3 */
	SA_KILL,			/* 37 SIGRTMIN + 4 */
	SA_KILL,			/* 38 SIGRTMIN + 5 */
	SA_KILL,			/* 39 SIGRTMIN + 6 */
	SA_KILL,			/* 40 SIGRTMIN + 7 */
	SA_KILL,			/* 41 SIGRTMIN + 8 */
	SA_KILL,			/* 42 SIGRTMIN + 9 */
	SA_KILL,			/* 43 SIGRTMIN + 10 */
	SA_KILL,			/* 44 SIGRTMIN + 11 */
	SA_KILL,			/* 45 SIGRTMIN + 12 */
	SA_KILL,			/* 46 SIGRTMIN + 13 */
	SA_KILL,			/* 47 SIGRTMIN + 14 */
	SA_KILL,			/* 48 SIGRTMIN + 15 */
	SA_KILL,			/* 49 SIGRTMIN + 16 */
	SA_KILL,			/* 50 SIGRTMIN + 17 */
	SA_KILL,			/* 51 SIGRTMIN + 18 */
	SA_KILL,			/* 52 SIGRTMIN + 19 */
	SA_KILL,			/* 53 SIGRTMIN + 20 */
	SA_KILL,			/* 54 SIGRTMIN + 21 */
	SA_KILL,			/* 55 SIGRTMIN + 22 */
	SA_KILL,			/* 56 SIGRTMIN + 23 */
	SA_KILL,			/* 57 SIGRTMIN + 24 */
	SA_KILL,			/* 58 SIGRTMIN + 25 */
	SA_KILL,			/* 59 SIGRTMIN + 26 */
	SA_KILL,			/* 60 SIGRTMIN + 27 */
	SA_KILL,			/* 61 SIGRTMIN + 28 */
	SA_KILL,			/* 62 SIGRTMIN + 29 */
	SA_KILL,			/* 63 SIGRTMIN + 30 */
};
#endif /* SIGPROP */

#ifdef _KERNEL

extern sigset_t contsigmask, stopsigmask, sigcantmask;

struct vnode;
struct ucred;

/*
 * Machine-independent functions:
 */
int	coredump __P((struct proc *p));
int	coredump_netbsd __P((struct proc *p, struct vnode *vp,
	    struct ucred *cred));
void	execsigs __P((struct proc *p));
void	gsignal __P((int pgid, int sig));
int	issignal __P((struct proc *p));
void	pgsignal __P((struct pgrp *pgrp, int sig, int checkctty));
void	postsig __P((int sig));
void	psignal1 __P((struct proc *p, int sig, int dolock));
#define	psignal(p, sig)		psignal1((p), (sig), 1)
#define	sched_psignal(p, sig)	psignal1((p), (sig), 0)
void	siginit __P((struct proc *p));
void	trapsignal __P((struct proc *p, int sig, u_long code));
void	sigexit __P((struct proc *, int));
void	setsigvec __P((struct proc *, int, struct sigaction *));
int	killpg1 __P((struct proc *, int, int, int));

int	sigaction1 __P((struct proc *p, int signum, \
	    const struct sigaction *nsa, struct sigaction *osa));
int	sigprocmask1 __P((struct proc *p, int how, \
	    const sigset_t *nss, sigset_t *oss));
void	sigpending1 __P((struct proc *p, sigset_t *ss));
int	sigsuspend1 __P((struct proc *p, const sigset_t *ss));
int	sigaltstack1 __P((struct proc *p, \
	    const struct sigaltstack *nss, struct sigaltstack *oss));
int	sigismasked __P((struct proc *, int));

void	signal_init __P((void));

void	sigactsinit __P((struct proc *, struct proc *, int));
void	sigactsunshare __P((struct proc *));
void	sigactsfree __P((struct proc *));

/*
 * Machine-dependent functions:
 */
void	sendsig __P((sig_t action, int sig, sigset_t *returnmask, u_long code));
struct core;
struct core32;
int	cpu_coredump __P((struct proc *, struct vnode *, struct ucred *,
			  struct core *));
int	cpu_coredump32 __P((struct proc *, struct vnode *, struct ucred *, 
			       struct core32 *));

/*
 * Compatibility functions.  See compat/common/kern_sig_13.c.
 */
void	native_sigset13_to_sigset __P((const sigset13_t *, sigset_t *));
void	native_sigset_to_sigset13 __P((const sigset_t *, sigset13_t *));
void	native_sigaction13_to_sigaction __P((const struct sigaction13 *,
	    struct sigaction *));
void	native_sigaction_to_sigaction13 __P((const struct sigaction *,
	    struct sigaction13 *));
void	native_sigaltstack13_to_sigaltstack __P((const struct sigaltstack13 *,
	    struct sigaltstack *));
void	native_sigaltstack_to_sigaltstack13 __P((const struct sigaltstack *,
	    struct sigaltstack13 *));
#endif	/* _KERNEL */
#endif	/* !_SYS_SIGNALVAR_H_ */
