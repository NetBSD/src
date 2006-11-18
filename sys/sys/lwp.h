/* 	$NetBSD: lwp.h,v 1.41.4.5 2006/11/18 21:39:47 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006 The NetBSD Foundation, Inc.
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

#if defined(_KERNEL)
#include <machine/cpu.h>		/* curcpu() and cpu_info */
#endif
#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/mutex.h>
#include <sys/signalvar.h>
#include <sys/specificdata.h>

typedef volatile const void *wchan_t;

/*
 * Lightweight process.  Field markings and the corresponding locks: 
 *
 * a:	alllwp_mutex
 * l:	*l->l_mutex
 * p:	p->p_smutex
 * s:	sched_mutex, which may be referenced by l->l_mutex
 * S:	sched_mutex, may be accessed unlocked by the LWP itself
 * (:	unlocked, stable
 * !:	unlocked, may only be safely accessed by the LWP itself
 * ?:	undecided
 */
struct	lwp {
	struct lwp	*l_forw;	/* s: run queue */
	struct lwp	*l_back;	/* s: run queue */
	kmutex_t * volatile l_mutex;	/* l: ptr to mutex on sched state */
	u_int		l_refcnt;	/* l: reference count on this LWP */
	struct cpu_info * volatile l_cpu; /* s: CPU we're on if LSONPROC */
	int		l_flag;		/* l: misc flag values */
	int		l_stat;		/* l: overall LWP status */
	struct turnstile *l_ts;		/* l: current turnstile */
	struct syncobj	*l_syncobj;	/* l: sync object operations set */

	lwpid_t		l_lid;		/* (: LWP identifier; local to proc */

	LIST_ENTRY(lwp)	l_list;		/* a: entry on list of all LWPs */

	struct proc	*l_proc;	/* p: parent process */
	LIST_ENTRY(lwp)	l_sibling;	/* p: entry on proc's list of LWPs */

	specificdata_reference
		l_specdataref;		/* !: subsystem lwp-specific data */

#define l_startzero l_cred

	struct kauth_cred *l_cred;	/* !: cached credentials */
	u_short		l_acflag;	/* !: accounting flags */

	u_int		l_swtime;	/* l: time swapped in or out */
	u_int		l_slptime;	/* l: time since last blocked */
	int		l_locks;	/* l: lockmgr count of held locks */
	u_short		l_shlocks;	/* !: lockdebug: shared locks held */
	u_short		l_exlocks;	/* !: lockdebug: excl. locks held */

	int		l_holdcnt;	/* l: if non-zero, don't swap */
	TAILQ_ENTRY(lwp) l_sleepchain;	/* l: sleep queue */
	wchan_t		l_wchan;	/* l: sleep address */
	struct callout	l_tsleep_ch;	/* l: callout for tsleep */
	const char	*l_wmesg;	/* l: reason for sleep */
	struct sleepq	*l_sleepq;	/* l: current sleep queue */
	long		l_nvcsw;	/* l: voluntary context switches */
	long		l_nivcsw;	/* l: involuntary context switches */
	struct timeval 	l_rtime;	/* l: real time */
	int		l_biglocks;	/* l: biglock count before sleep */

	struct sadata_vp *l_savp;	/* ?: SA "virtual processor" */

	void		*l_private;	/* ?: svr4-style lwp-private data */
	void		*l_ctxlink;	/* ?: uc_link {get,set}context */
	int		l_dupfd;	/* ?: side return from cloning devs XXX */

	int		l_sigrestore;	/* p: need to restore old sig mask */
	stack_t		*l_sigstk;	/* p: sp & on stack state variable */
	sigpend_t	*l_sigpend;	/* p: signals to this LWP */
	sigset_t	*l_sigmask;	/* p: signal mask */
	sigset_t	*l_sigwait;	/* p: signals being waited for */
	struct ksiginfo	*l_sigwaited;	/* p: delivered signals from set */
	LIST_ENTRY(lwp)	l_sigwaiter;	/* p: chain on list of waiting LWPs */
	sigset_t	l_sigoldmask;	/* p: mask from before sigpause */
	sigstore_t	l_sigstore;	/* p: signal state for 1:1 threads */

#define l_endzero l_startcopy

#define l_startcopy l_priority

	u_char		l_priority;	/* l: process priority */
	u_char		l_usrpri;	/* l: user-priority */

#define l_endcopy l_emuldata

	void		*l_emuldata;	/* !: kernel lwp-private data */
	struct user	*l_addr;	/* ?: KVA of u-area (PROC ONLY) */
	struct mdlwp	l_md;		/* ?: machine-dependent fields. */
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
extern kmutex_t	sched_mutex;		/* Mutex on global run queue */
extern kmutex_t alllwp_mutex;		/* Mutex on alllwp */
extern kmutex_t lwp_mutex;		/* Idle LWP mutex */
extern struct lwplist alllwp;		/* List of all LWPs. */

extern struct pool lwp_uc_pool;		/* memory pool for LWP startup args */

extern struct lwp lwp0;			/* LWP for proc0 */
#endif

/* These flags are kept in l_flag. [*] is shared with p_flag */
#define	L_INMEM		0x00000004 /* [*] Loaded into memory. */
#define	L_SELECT	0x00000040 /* [*] Selecting; wakeup/waiting danger. */
#define	L_SINTR		0x00000080 /* [*] Sleep is interruptible. */
#define	L_SYSTEM	0x00000200 /* [*] Kernel thread */
#define	L_SA		0x00000400 /* [*] Scheduler activations LWP */
#define	L_WSUSPEND	0x00020000 /* Suspend before return to user */
#define	L_OWEUPC	0x00040000 /* Owe user profiling tick */
#define	L_WCORE		0x00080000 /* Stop for core dump on return to user */
#define	L_WEXIT		0x00100000 /* Exit before return to user */
#define	L_SA_UPCALL	0x00200000 /* SA upcall is pending */
#define	L_SA_BLOCKING	0x00400000 /* Blocking in tsleep() */
#define	L_DETACHED	0x00800000 /* Won't be waited for. */
#define	L_PENDSIG	0x01000000 /* Pending signal for us */
#define	L_CANCELLED	0x02000000 /* tsleep should not sleep */
#define	L_SA_PAGEFAULT	0x04000000 /* SA LWP in pagefault handler */
#define	L_WREBOOT	0x08000000 /* System is rebooting, please suspend */
#define	L_SA_YIELD	0x10000000 /* LWP on VP is yielding */
#define	L_SA_IDLE	0x20000000 /* VP is idle */
#define	L_COWINPROGRESS	0x40000000 /* UFS: doing copy on write */
#define	L_SA_SWITCHING	0x80000000 /* SA LWP in context switch */

/*
 * Mask indicating that there is "exceptional" work to be done on return to
 * user.
 */
#define	L_USERRET	(L_WEXIT|L_PENDSIG|L_WREBOOT|L_WSUSPEND)

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
#define	LSDEAD		6	/* Process is almost a zombie. */
#define	LSONPROC	7	/* Process is currently on a CPU. */
#define	LSSUSPENDED	8	/* Not running, not signalable. */

#ifdef _KERNEL
#define	PHOLD(l)							\
do {									\
	if ((l)->l_holdcnt++ == 0 && ((l)->l_flag & L_INMEM) == 0)	\
		uvm_swapin(l);						\
} while (/* CONSTCOND */ 0)
#define	PRELE(l)	(--(l)->l_holdcnt)

#define	LWP_CACHE_CREDS(l, p)						\
do {									\
	if ((l)->l_cred != (p)->p_cred)					\
		lwp_update_creds(l);					\
} while (/* CONSTCOND */ 0)

void	preempt (int);
int	mi_switch (struct lwp *, struct lwp *);
#ifndef remrunqueue
void	remrunqueue (struct lwp *);
#endif
void	resetpriority (struct lwp *);
void	setrunnable (struct lwp *);
#ifndef setrunqueue
void	setrunqueue (struct lwp *);
#endif
#ifndef nextrunqueue
struct lwp *nextrunqueue(void);
#endif
void	unsleep (struct lwp *);
#ifndef cpu_switch
int	cpu_switch (struct lwp *, struct lwp *);
#endif
#ifndef cpu_switchto
void	cpu_switchto (struct lwp *, struct lwp *);
#endif

int	lwp_locked(struct lwp *, kmutex_t *);
void	lwp_setlock(struct lwp *, kmutex_t *);
void	lwp_unlock_to(struct lwp *, kmutex_t *);
void	lwp_lock_retry(struct lwp *l, kmutex_t *);
void	lwp_relock(struct lwp *l, kmutex_t *);
void	lwp_addref(struct lwp *l);
void	lwp_delref(struct lwp *l);

/*
 * Lock an LWP. XXXLKM
 */
static inline void
lwp_lock(struct lwp *l)
{
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	kmutex_t *old;

	mutex_enter(old = l->l_mutex);

	/*
	 * mutex_enter() will have posted a read barrier.  Re-test
	 * l->l_mutex.  If it has changed, we need to try again.
	 */
	if (__predict_false(l->l_mutex != old))
		lwp_lock_retry(l, old);
#else
	mutex_enter(l->l_mutex);
#endif
}

/*
 * Unlock an LWP. XXXLKM
 */
static inline void
lwp_unlock(struct lwp *l)
{
	LOCK_ASSERT(mutex_owned(l->l_mutex));

	mutex_exit(l->l_mutex);
}

static inline void
lwp_changepri(struct lwp *l, int pri)
{
	LOCK_ASSERT(mutex_owned(l->l_mutex));

	(*l->l_syncobj->sobj_changepri)(l, pri);
}

static inline void
lwp_unsleep(struct lwp *l)
{
	LOCK_ASSERT(mutex_owned(l->l_mutex));

	(*l->l_syncobj->sobj_unsleep)(l);
}

int newlwp(struct lwp *, struct proc *, vaddr_t, int /* XXX boolean_t */, int,
    void *, size_t, void (*)(void *), void *, struct lwp **);

/* Flags for _lwp_wait1 */
#define LWPWAIT_EXITCONTROL	0x00000001
void	lwpinit(void);
int 	lwp_wait1(struct lwp *, lwpid_t, lwpid_t *, int);
void	lwp_continue(struct lwp *);
void	cpu_setfunc(struct lwp *, void (*)(void *), void *);
void	startlwp(void *);
void	upcallret(struct lwp *);
int	lwp_exit(struct lwp *, int);
void	lwp_exit2(struct lwp *);
struct lwp *proc_representative_lwp(struct proc *, int *, int);
int	lwp_halt(struct lwp *, struct lwp *, int);
int	lwp_create1(struct lwp *, const void *, size_t, u_long, lwpid_t *);
void	lwp_update_creds(struct lwp *);
struct lwp *lwp_byid(struct proc *, int);
void	lwp_userret(struct lwp *);
int	lwp_lastlive(int);

int	lwp_specific_key_create(specificdata_key_t *, specificdata_dtor_t);
void	lwp_specific_key_delete(specificdata_key_t);
void 	lwp_initspecific(struct lwp *);
void 	lwp_finispecific(struct lwp *);
void *	lwp_getspecific(specificdata_key_t);
#if defined(_LWP_API_PRIVATE)
void *	_lwp_getspecific_by_lwp(struct lwp *, specificdata_key_t);
#endif
void	lwp_setspecific(specificdata_key_t, void *);
#endif	/* _KERNEL */

/* Flags for _lwp_create(), as per Solaris. */

#define LWP_DETACHED    0x00000040
#define LWP_SUSPENDED   0x00000080
#define __LWP_ASLWP     0x00000100 /* XXX more icky signal semantics */

#endif	/* !_SYS_LWP_H_ */
