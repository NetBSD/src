/*	$NetBSD: lwp.h,v 1.89.2.4 2009/07/18 14:53:27 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
#include <sys/sched.h>
#include <sys/specificdata.h>
#include <sys/syncobj.h>
#include <sys/resource.h>

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
 * p:	l_proc->p_lock
 * s:	spc_mutex, which may or may not be referenced by l_mutex
 * S:	l_selcpu->sc_lock
 * (:	unlocked, stable
 * !:	unlocked, may only be reliably accessed by the LWP itself
 * ?:	undecided
 *
 * Fields are clustered together by usage (to increase the likelyhood
 * of cache hits) and by size (to reduce dead space in the structure).
 */
struct lockdebug;
struct sadata_vp;
struct sysent;

struct lwp {
	/* Scheduling and overall state */
	TAILQ_ENTRY(lwp) l_runq;	/* s: run queue */
	union {
		void *	info;		/* s: scheduler-specific structure */
		u_int	timeslice;	/* l: time-quantum for SCHED_M2 */
	} l_sched;
	struct cpu_info *volatile l_cpu;/* s: CPU we're on if LSONPROC */
	kmutex_t * volatile l_mutex;	/* l: ptr to mutex on sched state */
	int		l_ctxswtch;	/* l: performing a context switch */
	struct user	*l_addr;	/* l: KVA of u-area (PROC ONLY) */
	struct mdlwp	l_md;		/* l: machine-dependent fields. */
	int		l_flag;		/* l: misc flag values */
	int		l_stat;		/* l: overall LWP status */
	struct bintime 	l_rtime;	/* l: real time */
	struct bintime	l_stime;	/* l: start time (while ONPROC) */
	u_int		l_swtime;	/* l: time swapped in or out */
	u_int		l_holdcnt;	/* l: if non-zero, don't swap */
	u_int		l_rticks;	/* l: Saved start time of run */
	u_int		l_rticksum;	/* l: Sum of ticks spent running */
	u_int		l_slpticks;	/* l: Saved start time of sleep */
	u_int		l_slpticksum;	/* l: Sum of ticks spent sleeping */
	int		l_biglocks;	/* l: biglock count before sleep */
	int		l_class;	/* l: scheduling class */
	int		l_kpriority;	/* !: has kernel priority boost */
	pri_t		l_kpribase;	/* !: kernel priority base level */
	pri_t		l_priority;	/* l: scheduler priority */
	pri_t		l_inheritedprio;/* l: inherited priority */
	SLIST_HEAD(, turnstile) l_pi_lenders; /* l: ts lending us priority */
	uint64_t	l_ncsw;		/* l: total context switches */
	uint64_t	l_nivcsw;	/* l: involuntary context switches */
	u_int		l_cpticks;	/* (: Ticks of CPU time */
	fixpt_t		l_pctcpu;	/* p: %cpu during l_swtime */
	fixpt_t		l_estcpu;	/* l: cpu time for SCHED_4BSD */
	psetid_t	l_psid;		/* l: assigned processor-set ID */
	struct cpu_info *l_target_cpu;	/* l: target CPU to migrate */
	kmutex_t	l_swaplock;	/* l: lock to prevent swapping */
	struct lwpctl	*l_lwpctl;	/* p: lwpctl block kernel address */
	struct lcpage	*l_lcpage;	/* p: lwpctl containing page */
	kcpuset_t	*l_affinity;	/* l: CPU set for affinity */
	struct sadata_vp *l_savp;	/* p: SA "virtual processor" */

	/* Synchronisation */
	struct turnstile *l_ts;		/* l: current turnstile */
	struct syncobj	*l_syncobj;	/* l: sync object operations set */
	TAILQ_ENTRY(lwp) l_sleepchain;	/* l: sleep queue */
	wchan_t		l_wchan;	/* l: sleep address */
	const char	*l_wmesg;	/* l: reason for sleep */
	struct sleepq	*l_sleepq;	/* l: current sleep queue */
	int		l_sleeperr;	/* !: error before unblock */
	u_int		l_slptime;	/* l: time since last blocked */
	callout_t	l_timeout_ch;	/* !: callout for tsleep */
	u_int		l_emap_gen;	/* !: emap generation number */

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
	struct selcpu	*l_selcpu;	/* !: associated per-CPU select data */
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
	struct timespec l_ktrcsw;	/* !: for ktrace CSW trace XXX */
	void		*l_private;	/* !: svr4-style lwp-private data */
	struct lwp	*l_switchto;	/* !: mi_switch: switch to this LWP */
	struct kauth_cred *l_cred;	/* !: cached credentials */
	struct filedesc	*l_fd;		/* !: cached copy of proc::p_fd */
	void		*l_emuldata;	/* !: kernel lwp-private data */
	u_int		l_cv_signalled;	/* c: restarted by cv_signal() */
	u_short		l_shlocks;	/* !: lockdebug: shared locks held */
	u_short		l_exlocks;	/* !: lockdebug: excl. locks held */
	u_short		l_unused;	/* !: unused */
	u_short		l_blcnt;	/* !: count of kernel_lock held */
	int		l_nopreempt;	/* !: don't preempt me! */
	u_int		l_dopreempt;	/* s: kernel preemption pending */
	int		l_pflag;	/* !: LWP private flags */
	int		l_dupfd;	/* !: side return from cloning devs XXX */
	const struct sysent * volatile l_sysent;/* !: currently active syscall */
	struct rusage	l_ru;		/* !: accounting information */
	uint64_t	l_pfailtime;	/* !: for kernel preemption */
	uintptr_t	l_pfailaddr;	/* !: for kernel preemption */
	uintptr_t	l_pfaillock;	/* !: for kernel preemption */
	_TAILQ_HEAD(,struct lockdebug,volatile) l_ld_locks;/* !: locks held by LWP */
	int		l_tcgen;	/* !: for timecounter removal */
	int		l_unused2;	/* !: for future use */

	/* These are only used by 'options SYSCALL_TIMES' */
	uint32_t        l_syscall_time; /* !: time epoch for current syscall */
	uint64_t        *l_syscall_counter; /* !: counter for current process */
};

/*
 * USER_TO_UAREA/UAREA_TO_USER: macros to convert between
 * the lowest address of the uarea (UAREA) and lwp::l_addr (USER).
 *
 * the default is just a cast.  MD code can modify it by defining
 * either these macros or UAREA_USER_OFFSET in <machine/proc.h>.
 */

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
#define	LW_SA_SWITCHING	0x00000100 /* SA LWP in context switch */
#define	LW_SYSTEM	0x00000200 /* Kernel thread */
#define	LW_SA		0x00000400 /* Scheduler activations LWP */
#define	LW_WSUSPEND	0x00020000 /* Suspend before return to user */
#define	LW_BATCH	0x00040000 /* LWP tends to hog CPU */
#define	LW_WCORE	0x00080000 /* Stop for core dump on return to user */
#define	LW_WEXIT	0x00100000 /* Exit before return to user */
#define	LW_AFFINITY	0x00200000 /* Affinity is assigned to the thread */
#define	LW_SA_UPCALL	0x00400000 /* SA upcall is pending */
#define	LW_SA_BLOCKING	0x00800000 /* Blocking in tsleep() */
#define	LW_PENDSIG	0x01000000 /* Pending signal for us */
#define	LW_CANCELLED	0x02000000 /* tsleep should not sleep */
#define	LW_WUSERRET	0x04000000 /* Call proc::p_userret on return to user */
#define	LW_WREBOOT	0x08000000 /* System is rebooting, please suspend */
#define	LW_UNPARKED	0x10000000 /* Unpark op pending */
#define	LW_SA_YIELD	0x40000000 /* LWP on VP is yielding */
#define	LW_SA_IDLE	0x80000000 /* VP is idle */

/* The second set of flags is kept in l_pflag. */
#define	LP_KTRACTIVE	0x00000001 /* Executing ktrace operation */
#define	LP_KTRCSW	0x00000002 /* ktrace context switch marker */
#define	LP_KTRCSWUSER	0x00000004 /* ktrace context switch marker */
#define	LP_OWEUPC	0x00000010 /* Owe user profiling tick */
#define	LP_MPSAFE	0x00000020 /* Starts life without kernel_lock */
#define	LP_INTR		0x00000040 /* Soft interrupt handler */
#define	LP_SYSCTLWRITE	0x00000080 /* sysctl write lock held */
#define	LP_SA_PAGEFAULT	0x00000200 /* SA LWP in pagefault handler */
#define	LP_SA_NOBLOCK	0x00000400 /* SA don't upcall on block */
#define	LP_TIMEINTR	0x00010000 /* Time this soft interrupt */
#define	LP_RUNNING	0x20000000 /* Active on a CPU */
#define	LP_BOUND	0x80000000 /* Bound to a CPU */

/* The third set is kept in l_prflag. */
#define	LPR_DETACHED	0x00800000 /* Won't be waited for. */
#define	LPR_CRMOD	0x00000100 /* Credentials modified */

/*
 * Mask indicating that there is "exceptional" work to be done on return to
 * user.
 */
#define	LW_USERRET (LW_WEXIT|LW_PENDSIG|LW_WREBOOT|LW_WSUSPEND|LW_WCORE|\
		    LW_WUSERRET|LW_SA_BLOCKING|LW_SA_UPCALL)

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
	(void)p;							\
	if (__predict_false((l)->l_prflag & LPR_CRMOD))			\
		lwp_update_creds(l);					\
} while (/* CONSTCOND */ 0)

void	lwp_startup(lwp_t *, lwp_t *);

int	lwp_locked(lwp_t *, kmutex_t *);
void	lwp_setlock(lwp_t *, kmutex_t *);
void	lwp_unlock_to(lwp_t *, kmutex_t *);
kmutex_t *lwp_lock_retry(lwp_t *, kmutex_t *);
void	lwp_relock(lwp_t *, kmutex_t *);
int	lwp_trylock(lwp_t *);
void	lwp_addref(lwp_t *);
void	lwp_delref(lwp_t *);
void	lwp_drainrefs(lwp_t *);
bool	lwp_alive(lwp_t *);
lwp_t	*lwp_find_first(proc_t *);

/* Flags for _lwp_wait1 */
#define LWPWAIT_EXITCONTROL	0x00000001
void	lwpinit(void);
int 	lwp_wait1(lwp_t *, lwpid_t, lwpid_t *, int);
void	lwp_continue(lwp_t *);
void	cpu_setfunc(lwp_t *, void (*)(void *), void *);
void	startlwp(void *);
void	upcallret(lwp_t *);
void	lwp_exit(lwp_t *) __dead;
void	lwp_exit_switchaway(lwp_t *) __dead;
int	lwp_suspend(lwp_t *, lwp_t *);
int	lwp_create1(lwp_t *, const void *, size_t, u_long, lwpid_t *);
void	lwp_update_creds(lwp_t *);
void	lwp_migrate(lwp_t *, struct cpu_info *);
lwp_t *lwp_find2(pid_t, lwpid_t);
lwp_t *lwp_find(proc_t *, int);
void	lwp_userret(lwp_t *);
void	lwp_need_userret(lwp_t *);
void	lwp_free(lwp_t *, bool, bool);
void	lwp_sys_init(void);
u_int	lwp_unsleep(lwp_t *, bool);
uint64_t lwp_pctr(void);

int	lwp_specific_key_create(specificdata_key_t *, specificdata_dtor_t);
void	lwp_specific_key_delete(specificdata_key_t);
void 	lwp_initspecific(lwp_t *);
void 	lwp_finispecific(lwp_t *);
void	*lwp_getspecific(specificdata_key_t);
#if defined(_LWP_API_PRIVATE)
void	*_lwp_getspecific_by_lwp(lwp_t *, specificdata_key_t);
#endif
void	lwp_setspecific(specificdata_key_t, void *);

/* Syscalls */
int	lwp_park(struct timespec *, const void *);
int	lwp_unpark(lwpid_t, const void *);

/* ddb */
void lwp_whatis(uintptr_t, void (*)(const char *, ...));


/*
 * Lock an LWP. XXX _MODULE
 */
static inline void
lwp_lock(lwp_t *l)
{
	kmutex_t *old;

	mutex_spin_enter(old = l->l_mutex);

	/*
	 * mutex_enter() will have posted a read barrier.  Re-test
	 * l->l_mutex.  If it has changed, we need to try again.
	 */
	if (__predict_false(l->l_mutex != old))
		lwp_lock_retry(l, old);
}

/*
 * Unlock an LWP. XXX _MODULE
 */
static inline void
lwp_unlock(lwp_t *l)
{
	mutex_spin_exit(l->l_mutex);
}

static inline void
lwp_changepri(lwp_t *l, pri_t pri)
{
	KASSERT(mutex_owned(l->l_mutex));

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

static inline pri_t
lwp_eprio(lwp_t *l)
{
	pri_t pri;

	pri = l->l_priority;
	if (l->l_kpriority && pri < PRI_KERNEL)
		pri = (pri >> 1) + l->l_kpribase;
	return MAX(l->l_inheritedprio, pri);
}

int lwp_create(lwp_t *, struct proc *, vaddr_t, bool, int,
    void *, size_t, void (*)(void *), void *, lwp_t **, int);

/*
 * XXX _MODULE
 * We should provide real stubs for the below that modules can use.
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

static inline void
spc_dlock(struct cpu_info *ci1, struct cpu_info *ci2)
{
	struct schedstate_percpu *spc1 = &ci1->ci_schedstate;
	struct schedstate_percpu *spc2 = &ci2->ci_schedstate;

	KASSERT(ci1 != ci2);
	if (ci1 < ci2) {
		mutex_spin_enter(spc1->spc_mutex);
		mutex_spin_enter(spc2->spc_mutex);
	} else {
		mutex_spin_enter(spc2->spc_mutex);
		mutex_spin_enter(spc1->spc_mutex);
	}
}

/*
 * Allow machine-dependent code to override curlwp in <machine/cpu.h> for
 * its own convenience.  Otherwise, we declare it as appropriate.
 */
#if !defined(curlwp)
#if defined(MULTIPROCESSOR)
#define	curlwp		curcpu()->ci_curlwp	/* Current running LWP */
#else
extern struct lwp	*curlwp;		/* Current running LWP */
#endif /* MULTIPROCESSOR */
#endif /* ! curlwp */
#define	curproc		(curlwp->l_proc)

static inline bool
CURCPU_IDLE_P(void)
{
	struct cpu_info *ci = curcpu();
	return ci->ci_data.cpu_onproc == ci->ci_data.cpu_idlelwp;
}

/*
 * Disable and re-enable preemption.  Only for low-level kernel
 * use.  Device drivers and anything that could potentially be
 * compiled as a module should use kpreempt_disable() and
 * kpreempt_enable().
 */
static inline void
KPREEMPT_DISABLE(lwp_t *l)
{

	KASSERT(l == curlwp);
	l->l_nopreempt++;
	__insn_barrier();
}

static inline void
KPREEMPT_ENABLE(lwp_t *l)
{

	KASSERT(l == curlwp);
	KASSERT(l->l_nopreempt > 0);
	__insn_barrier();
	if (--l->l_nopreempt != 0)
		return;
	__insn_barrier();
	if (__predict_false(l->l_dopreempt))
		kpreempt(0);
	__insn_barrier();
}

/* For lwp::l_dopreempt */
#define	DOPREEMPT_ACTIVE	0x01
#define	DOPREEMPT_COUNTED	0x02

#endif /* _KERNEL */

/* Flags for _lwp_create(), as per Solaris. */
#define LWP_DETACHED    0x00000040
#define LWP_SUSPENDED   0x00000080
#define	LWP_VFORK	0x80000000

#endif	/* !_SYS_LWP_H_ */
