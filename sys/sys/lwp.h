/* 	$NetBSD: lwp.h,v 1.56.2.10 2007/07/01 19:25:01 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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

#ifndef _SYS_LWP_H_
#define _SYS_LWP_H_

#include <sys/time.h>
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/signalvar.h>
#include <sys/specificdata.h>
#include <sys/syncobj.h>

#if defined(_KERNEL)
#include <machine/cpu.h>		/* curcpu() and cpu_info */
#endif

#include <machine/proc.h>		/* Machine-dependent proc substruct. */

/*
 * Lightweight process.  Field markings and the corresponding locks: 
 *
 * a:	proclist_mutex
 * c:	condition variable interlock, passed to cv_wait()
 * l:	*l_mutex
 * p:	l_proc->p_smutex
 * s:	spc_mutex, which may or may not be referenced by l_mutex
 * t:	l_proc->p_stmutex
 * S:	select_lock
 * (:	unlocked, stable
 * !:	unlocked, may only be safely accessed by the LWP itself
 * ?:	undecided
 *
 * Fields are clustered together by usage (to increase the likelyhood
 * of cache hits) and by size (to reduce dead space in the structure).
 */
struct lwp {
	/* Scheduling and overall state */
	TAILQ_ENTRY(lwp) l_runq;	/* s: run queue */
	void		*l_sched_info;	/* s: Scheduler-specific structure */
	struct cpu_info *volatile l_cpu;/* s: CPU we're on if LSONPROC */
	kmutex_t * volatile l_mutex;	/* l: ptr to mutex on sched state */
	struct user	*l_addr;	/* l: KVA of u-area (PROC ONLY) */
	struct mdlwp	l_md;		/* l: machine-dependent fields. */
	int		l_flag;		/* l: misc flag values */
	int		l_stat;		/* l: overall LWP status */
	struct timeval 	l_rtime;	/* l: real time */
	u_int		l_swtime;	/* l: time swapped in or out */
	int		l_holdcnt;	/* l: if non-zero, don't swap */
	int		l_biglocks;	/* l: biglock count before sleep */
	pri_t		l_priority;	/* l: process priority */
	pri_t		l_usrpri;	/* l: user-priority */
	pri_t		l_inheritedprio;/* l: inherited priority */
	SLIST_HEAD(, turnstile) l_pi_lenders; /* l: ts lending us priority */
	uint64_t	l_ncsw;		/* l: total context switches */
	uint64_t	l_nivcsw;	/* l: involuntary context switches */
	int		l_cpticks;	/* t: Ticks of CPU time */
	fixpt_t		l_pctcpu;	/* t: %cpu during l_swtime */
	kmutex_t	l_swaplock;	/* l: lock to prevent swapping */

	/* Synchronisation */
	struct turnstile *l_ts;		/* l: current turnstile */
	struct syncobj	*l_syncobj;	/* l: sync object operations set */
	TAILQ_ENTRY(lwp) l_sleepchain;	/* l: sleep queue */
	wchan_t		l_wchan;	/* l: sleep address */
	const char	*l_wmesg;	/* l: reason for sleep */
	struct sleepq	*l_sleepq;	/* l: current sleep queue */
	int		l_sleeperr;	/* !: error before unblock */
	u_int		l_slptime;	/* l: time since last blocked */
	callout_t	l_tsleep_ch;	/* !: callout for tsleep */

	/* Process level and global state, misc. */
	LIST_ENTRY(lwp)	l_list;		/* a: entry on list of all LWPs */
	void		*l_ctxlink;	/* p: uc_link {get,set}context */
	struct proc	*l_proc;	/* p: parent process */
	LIST_ENTRY(lwp)	l_sibling;	/* p: entry on proc's list of LWPs */
	lwpid_t		l_waiter;	/* p: first LWP waiting on us */
	lwpid_t 	l_waitingfor;	/* p: specific LWP we are waiting on */
	int		l_prflag;	/* p: process level flags */
	u_int		l_refcnt;	/* p: reference count on this LWP */
	lwpid_t		l_lid;		/* (: LWP identifier; local to proc */
	int		l_selflag;	/* S: select() flags */
	SLIST_HEAD(,selinfo) l_selwait;	/* S: descriptors waited on */
	char		*l_name;	/* (: name, optional */

	/* Signals */
	int		l_sigrestore;	/* p: need to restore old sig mask */
	sigset_t	l_sigwaitset;	/* p: signals being waited for */
	kcondvar_t	l_sigcv;	/* p: for sigsuspend() */
	struct ksiginfo	*l_sigwaited;	/* p: delivered signals from set */
	sigpend_t	*l_sigpendset;	/* p: XXX issignal()/postsig() baton */
	LIST_ENTRY(lwp)	l_sigwaiter;	/* p: chain on list of waiting LWPs */
	stack_t		l_sigstk;	/* p: sp & on stack state variable */
	sigset_t	l_sigmask;	/* p: signal mask */
	sigpend_t	l_sigpend;	/* p: signals to this LWP */
	sigset_t	l_sigoldmask;	/* p: mask for sigpause */

	/* Private data */
	specificdata_reference
		l_specdataref;		/* !: subsystem lwp-specific data */
	union {
		struct timeval tv;
		struct timespec ts;
	} l_ktrcsw;			/* !: for ktrace CSW trace XXX */
	void		*l_private;	/* !: svr4-style lwp-private data */
	lwp_t		*l_pinned;	/* !: lwp under this one */
	struct kauth_cred *l_cred;	/* !: cached credentials */
	void		*l_emuldata;	/* !: kernel lwp-private data */
	u_int		l_cv_signalled;	/* c: restarted by cv_signal() */
	u_short		l_shlocks;	/* !: lockdebug: shared locks held */
	u_short		l_exlocks;	/* !: lockdebug: excl. locks held */
	u_short		l_locks;	/* !: lockmgr count of held locks */
	u_short		l_blcnt;	/* !: count of kernel_lock held */
	int		l_pflag;	/* !: LWP private flags */
	int		l_dupfd;	/* !: side return from cloning devs XXX */

	/* These are only used by 'options SYSCALL_TIMES' */
	uint32_t        l_syscall_time; /* !: time epoch for current syscall */
	uint64_t        *l_syscall_counter; /* !: counter for current process */
};

#if !defined(USER_TO_UAREA)
#if !defined(UAREA_USER_OFFSET)
#define	UAREA_USER_OFFSET	0
#endif /* !defined(UAREA_USER_OFFSET) */
#define	USER_TO_UAREA(user)	((vaddr_t)(user) - UAREA_USER_OFFSET)
#define	UAREA_TO_USER(uarea)	((struct user *)((uarea) + UAREA_USER_OFFSET))
#endif /* !defined(UAREA_TO_USER) */

LIST_HEAD(lwplist, lwp);		/* a list of LWPs */

#ifdef _KERNEL
extern kmutex_t alllwp_mutex;		/* Mutex on alllwp */
extern struct lwplist alllwp;		/* List of all LWPs. */

extern struct pool lwp_uc_pool;		/* memory pool for LWP startup args */

extern lwp_t lwp0;			/* LWP for proc0 */
#endif

/* These flags are kept in l_flag. */
#define	LW_IDLE		0x00000001 /* Idle lwp. */
#define	LW_INMEM	0x00000004 /* Loaded into memory. */
#define	LW_SINTR	0x00000080 /* Sleep is interruptible. */
#define	LW_SYSTEM	0x00000200 /* Kernel thread */
#define	LW_WSUSPEND	0x00020000 /* Suspend before return to user */
#define	LW_WCORE	0x00080000 /* Stop for core dump on return to user */
#define	LW_WEXIT	0x00100000 /* Exit before return to user */
#define	LW_PENDSIG	0x01000000 /* Pending signal for us */
#define	LW_CANCELLED	0x02000000 /* tsleep should not sleep */
#define	LW_WUSERRET	0x04000000 /* Call proc::p_userret on return to user */
#define	LW_WREBOOT	0x08000000 /* System is rebooting, please suspend */
#define	LW_UNPARKED	0x10000000 /* Unpark op pending */
#define	LW_RUNNING	0x20000000 /* Active on a CPU (except if LSZOMB) */
#define	LW_INTR		0x40000000 /* Soft interrupt handler */
#define	LW_BOUND	0x80000000 /* Bound to a CPU */

/* The second set of flags is kept in l_pflag. */
#define	LP_KTRACTIVE	0x00000001 /* Executing ktrace operation */
#define	LP_KTRCSW	0x00000002 /* ktrace context switch marker */
#define	LP_KTRCSWUSER	0x00000004 /* ktrace context switch marker */
#define	LP_UFSCOW	0x00000008 /* UFS: doing copy on write */
#define	LP_OWEUPC	0x00000010 /* Owe user profiling tick */
#define	LP_MPSAFE	0x00000020 /* Starts life without kernel_lock */

/* The third set is kept in l_prflag. */
#define	LPR_DETACHED	0x00800000 /* Won't be waited for. */

/*
 * Mask indicating that there is "exceptional" work to be done on return to
 * user.
 */
#define	LW_USERRET (LW_WEXIT|LW_PENDSIG|LW_WREBOOT|LW_WSUSPEND|LW_WCORE|\
		    LW_WUSERRET)

/*
 * Status values.
 *
 * A note about SRUN and SONPROC: SRUN indicates that a process is
 * runnable but *not* yet running, i.e. is on a run queue.  SONPROC
 * indicates that the process is actually executing on a CPU, i.e.
 * it is no longer on a run queue.
 */
#define	LSIDL		1	/* Process being created by fork. */
#define	LSRUN		2	/* Currently runnable. */
#define	LSSLEEP		3	/* Sleeping on an address. */
#define	LSSTOP		4	/* Process debugging or suspension. */
#define	LSZOMB		5	/* Awaiting collection by parent. */
/* unused, for source compatibility with NetBSD 4.0 and earlier. */
#define	LSDEAD		6	/* Process is almost a zombie. */
#define	LSONPROC	7	/* Process is currently on a CPU. */
#define	LSSUSPENDED	8	/* Not running, not signalable. */

#ifdef _KERNEL
#define	LWP_CACHE_CREDS(l, p)						\
do {									\
	if ((l)->l_cred != (p)->p_cred)					\
		lwp_update_creds(l);					\
} while (/* CONSTCOND */ 0)

void	lwp_startup(lwp_t *, lwp_t *);

int	lwp_locked(lwp_t *, kmutex_t *);
void	lwp_setlock(lwp_t *, kmutex_t *);
void	lwp_unlock_to(lwp_t *, kmutex_t *);
void	lwp_lock_retry(lwp_t *, kmutex_t *);
void	lwp_relock(lwp_t *, kmutex_t *);
int	lwp_trylock(lwp_t *);
void	lwp_addref(lwp_t *);
void	lwp_delref(lwp_t *);
void	lwp_drainrefs(lwp_t *);

/* Flags for _lwp_wait1 */
#define LWPWAIT_EXITCONTROL	0x00000001
void	lwpinit(void);
int 	lwp_wait1(lwp_t *, lwpid_t, lwpid_t *, int);
void	lwp_continue(lwp_t *);
void	cpu_setfunc(lwp_t *, void (*)(void *), void *);
void	startlwp(void *);
void	upcallret(lwp_t *);
void	lwp_exit(lwp_t *) __attribute__((__noreturn__));
void	lwp_exit_switchaway(lwp_t *);
lwp_t *proc_representative_lwp(struct proc *, int *, int);
int	lwp_suspend(lwp_t *, lwp_t *);
int	lwp_create1(lwp_t *, const void *, size_t, u_long, lwpid_t *);
void	lwp_update_creds(lwp_t *);
lwp_t *lwp_find(struct proc *, int);
void	lwp_userret(lwp_t *);
void	lwp_need_userret(lwp_t *);
void	lwp_free(lwp_t *, bool, bool);
void	lwp_sys_init(void);

int	lwp_specific_key_create(specificdata_key_t *, specificdata_dtor_t);
void	lwp_specific_key_delete(specificdata_key_t);
void 	lwp_initspecific(lwp_t *);
void 	lwp_finispecific(lwp_t *);
void	*lwp_getspecific(specificdata_key_t);
#if defined(_LWP_API_PRIVATE)
void	*_lwp_getspecific_by_lwp(lwp_t *, specificdata_key_t);
#endif
void	lwp_setspecific(specificdata_key_t, void *);

/*
 * Lock an LWP. XXXLKM
 */
static inline void
lwp_lock(lwp_t *l)
{
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	kmutex_t *old;

	mutex_spin_enter(old = l->l_mutex);

	/*
	 * mutex_enter() will have posted a read barrier.  Re-test
	 * l->l_mutex.  If it has changed, we need to try again.
	 */
	if (__predict_false(l->l_mutex != old))
		lwp_lock_retry(l, old);
#else
	mutex_spin_enter(l->l_mutex);
#endif
}

/*
 * Unlock an LWP. XXXLKM
 */
static inline void
lwp_unlock(lwp_t *l)
{
	KASSERT(mutex_owned(l->l_mutex));

	mutex_spin_exit(l->l_mutex);
}

static inline void
lwp_changepri(lwp_t *l, pri_t pri)
{
	KASSERT(mutex_owned(l->l_mutex));

	if (l->l_priority == pri)
		return;

	(*l->l_syncobj->sobj_changepri)(l, pri);
}

static inline void
lwp_lendpri(lwp_t *l, pri_t pri)
{
	KASSERT(mutex_owned(l->l_mutex));

	if (l->l_inheritedprio == pri)
		return;

	(*l->l_syncobj->sobj_lendpri)(l, pri);
}

static inline void
lwp_unsleep(lwp_t *l)
{
	KASSERT(mutex_owned(l->l_mutex));

	(*l->l_syncobj->sobj_unsleep)(l);
}

static inline int
lwp_eprio(lwp_t *l)
{

	return MAX(l->l_inheritedprio, l->l_priority);
}

int newlwp(lwp_t *, struct proc *, vaddr_t, bool, int,
    void *, size_t, void (*)(void *), void *, lwp_t **);

/*
 * We should provide real stubs for the below that LKMs can use.
 */

static inline void
spc_lock(struct cpu_info *ci)
{
	mutex_spin_enter(ci->ci_schedstate.spc_mutex);
}

static inline void
spc_unlock(struct cpu_info *ci)
{
	mutex_spin_exit(ci->ci_schedstate.spc_mutex);
}

#endif /* _KERNEL */

/* Flags for _lwp_create(), as per Solaris. */

#define LWP_DETACHED    0x00000040
#define LWP_SUSPENDED   0x00000080

#endif	/* !_SYS_LWP_H_ */
