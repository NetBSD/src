/*	$NetBSD: pthread_int.h,v 1.34 2006/10/03 09:37:07 yamt Exp $	*/

/*-
 * Copyright (c) 2001,2002,2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#ifndef _LIB_PTHREAD_INT_H
#define _LIB_PTHREAD_INT_H

#define PTHREAD__DEBUG
#define ERRORCHECK

#include "pthread_types.h"
#include "pthread_queue.h"
#include "pthread_debug.h"
#include "pthread_md.h"

#include <sa.h>
#include <signal.h>

#define PTHREAD_KEYS_MAX 256
/*
 * The size of this structure needs to be no larger than struct
 * __pthread_cleanup_store, defined in pthread.h.
 */
struct pt_clean_t {
	PTQ_ENTRY(pt_clean_t)	ptc_next;
	void	(*ptc_cleanup)(void *);
	void	*ptc_arg;
};

struct pt_alarm_t {
	PTQ_ENTRY(pt_alarm_t)	pta_next;
	pthread_spin_t	pta_lock;
	const struct timespec	*pta_time;
	void	(*pta_func)(void *);
	void	*pta_arg;
	int	pta_fired;
};

/* Private data for pthread_attr_t */
struct pthread_attr_private {
	char ptap_name[PTHREAD_MAX_NAMELEN_NP];
	void *ptap_namearg;
	void *ptap_stackaddr;
	size_t ptap_stacksize;
	size_t ptap_guardsize;
};

struct	__pthread_st {
	unsigned int	pt_magic;
	/* Identifier, for debugging and for preventing recycling. */
	int		pt_num;

	int	pt_type;	/* normal, upcall, or idle */
	int	pt_state;	/* running, blocked, etc. */
	pthread_spin_t pt_statelock;	/* lock on pt_state */
	int	pt_flags;	/* see PT_FLAG_* below */
	pthread_spin_t pt_flaglock;	/* lock on pt_flag */
	int	pt_cancel;	/* Deferred cancellation */
	int	pt_spinlocks;	/* Number of spinlocks held. */
	int	pt_blockedlwp;	/* LWP/SA number when blocked */
	int	pt_vpid;	/* VP number */
	int	pt_blockgen;	/* SA_UPCALL_BLOCKED counter */
	int	pt_unblockgen;	/* SA_UPCALL_UNBLOCKED counter */

	int	pt_errno;	/* Thread-specific errno. */

	/* Entry on the run queue */
	PTQ_ENTRY(__pthread_st)	pt_runq;
	/* Entry on the list of all threads */
	PTQ_ENTRY(__pthread_st)	pt_allq;
	/* Entry on the sleep queue (xxx should be same as run queue?) */
	PTQ_ENTRY(__pthread_st)	pt_sleep;
	/* Object we're sleeping on */
	void			*pt_sleepobj;
	/* Queue we're sleeping on */
	struct pthread_queue_t	*pt_sleepq;
	/* Lock protecting that queue */
	pthread_spin_t		*pt_sleeplock;

	stack_t		pt_stack;	/* Our stack */
	ucontext_t	*pt_uc;		/* Saved context when we're stopped */
	ucontext_t	*pt_trapuc;   	/* Kernel-saved context */
	ucontext_t	*__pt_blockuc;  /* Kernel-saved context when blocked */

	sigset_t	pt_sigmask;	/* Signals we won't take. */
	sigset_t	pt_siglist;	/* Signals pending for us. */
	sigset_t	pt_sigblocked;	/* Signals delivered while blocked. */
	sigset_t	*pt_sigwait;	/* Signals waited for in sigwait */
	siginfo_t	*pt_wsig;	
	pthread_spin_t	pt_siglock;	/* Lock on above */

	void *		pt_exitval;	/* Read by pthread_join() */

	/* Stack of cancellation cleanup handlers and their arguments */
	PTQ_HEAD(, pt_clean_t)	pt_cleanup_stack;

	/* Thread's name, set by the application. */
	char*		pt_name;

	/* Other threads trying to pthread_join() us. */
	struct pthread_queue_t	pt_joiners;
	/* Lock for above, and for changing pt_state to ZOMBIE or DEAD,
	 * and for setting the DETACHED flag.  Also protects pt_name.
	 */
	pthread_spin_t	pt_join_lock;

	/* Thread we were going to switch to before we were preempted
	 * ourselves. Will be used by the upcall that's continuing us.
	 */
	pthread_t	pt_switchto;
	ucontext_t*	pt_switchtouc;

	/* Threads that are preempted with spinlocks held will be
	 * continued until they unlock their spinlock. When they do
	 * so, they should jump ship to the thread pointed to by
	 * pt_next.
	 */
	pthread_t	pt_next;

	/* The upcall that is continuing this thread */
	pthread_t	pt_parent;
	
	/* A queue lock that this thread held while trying to 
	 * context switch to another process.
	 */
	pthread_spin_t*	pt_heldlock;

	/* Upcall stack information shared between kernel and
	 * userland.
	 */
	struct sa_stackinfo_t	pt_stackinfo;

	/* Thread-specific data */
	void*		pt_specific[PTHREAD_KEYS_MAX];

#ifdef PTHREAD__DEBUG
	int	blocks;
	int	preempts;
	int	rescheds;
#endif
};

struct pthread_lock_ops {
	void	(*plo_init)(__cpu_simple_lock_t *);
	int	(*plo_try)(__cpu_simple_lock_t *);
	void	(*plo_unlock)(__cpu_simple_lock_t *);
};

/* Thread types */
#define PT_THREAD_NORMAL	1
#define PT_THREAD_UPCALL	2
#define PT_THREAD_IDLE		3

/* Thread states */
#define PT_STATE_RUNNING	1
#define PT_STATE_RUNNABLE	2
#define _PT_STATE_BLOCKED_SYS	3	/* Only used in libpthread_dbg */
#define PT_STATE_BLOCKED_QUEUE	4
#define PT_STATE_ZOMBIE		5
#define PT_STATE_DEAD		6
#define PT_STATE_SUSPENDED	7

/* Flag values */

#define PT_FLAG_DETACHED	0x0001
#define PT_FLAG_IDLED		0x0002
#define PT_FLAG_CS_DISABLED	0x0004	/* Cancellation disabled */
#define PT_FLAG_CS_ASYNC	0x0008  /* Cancellation is async */
#define PT_FLAG_CS_PENDING	0x0010
#define PT_FLAG_SIGDEFERRED     0x0020	/* There are signals to take */
#define PT_FLAG_SCOPE_SYSTEM	0x0040
#define PT_FLAG_EXPLICIT_SCHED	0x0080
#define PT_FLAG_SUSPENDED	0x0100	/* In the suspended queue */
#define PT_FLAG_SIGNALED	0x0200

#define PT_MAGIC	0x11110001
#define PT_DEAD		0xDEAD0001

#define PT_ATTR_MAGIC	0x22220002
#define PT_ATTR_DEAD	0xDEAD0002

#ifdef PT_FIXEDSTACKSIZE_LG

#define	PT_STACKSIZE_LG	PT_FIXEDSTACKSIZE_LG 
#define	PT_STACKSIZE	(1<<(PT_STACKSIZE_LG)) 
#define	PT_STACKMASK	(PT_STACKSIZE-1)

#else  /* PT_FIXEDSTACKSIZE_LG */

extern	int		pthread_stacksize_lg;
extern	size_t		pthread_stacksize;
extern	vaddr_t		pthread_stackmask;

#define	PT_STACKSIZE_LG	pthread_stacksize_lg
#define	PT_STACKSIZE	pthread_stacksize
#define	PT_STACKMASK	pthread_stackmask

#endif /* PT_FIXEDSTACKSIZE_LG */


#define PT_UPCALLSTACKS	16

#define PT_ALARMTIMER_MAGIC	0x88880010
#define PT_RRTIMER_MAGIC	0x88880020
#define NIDLETHREADS	4

/* Flag to be used in a ucontext_t's uc_flags indicating that
 * the saved register state is "user" state only, not full
 * trap state.
 */
#define _UC_USER_BIT		30
#define _UC_USER		(1LU << _UC_USER_BIT)

void	pthread_init(void)  __attribute__ ((__constructor__));

/* Utility functions */

/* Set up/clean up a thread's basic state. */
void	pthread__initthread(pthread_t self, pthread_t t);
/* Get offset from stack start to struct sa_stackinfo */
ssize_t	pthread__stackinfo_offset(void);

/* Go do something else. Don't go back on the run queue */
void	pthread__block(pthread_t self, pthread_spin_t* queuelock);
/* Put a thread back on the suspended queue */
void	pthread__suspend(pthread_t self, pthread_t thread);
/* Put a thread back on the run queue */
void	pthread__sched(pthread_t self, pthread_t thread);
void	pthread__sched_sleepers(pthread_t self, struct pthread_queue_t *threadq);
void	pthread__sched_idle(pthread_t self, pthread_t thread);
void	pthread__sched_idle2(pthread_t self);

void	pthread__sched_bulk(pthread_t self, pthread_t qhead);

void	pthread__idle(void);

/* Get the next thread */
pthread_t pthread__next(pthread_t self);

int	pthread__stackalloc(pthread_t *t);
void	pthread__initmain(pthread_t *t);

void	pthread__sa_start(void);
void	pthread__sa_recycle(pthread_t old, pthread_t new);
void	pthread__setconcurrency(int);

/* Alarm code */
void	pthread__alarm_init(void);
void	pthread__alarm_add(pthread_t, struct pt_alarm_t *,
    const struct timespec *, void (*)(void *), void *);
void	pthread__alarm_del(pthread_t, struct pt_alarm_t *);
int	pthread__alarm_fired(struct pt_alarm_t *);
void	pthread__alarm_process(pthread_t self, void *arg);

/* Internal locking primitives */
void	pthread__lockprim_init(int ncpu);
void	pthread_lockinit(pthread_spin_t *lock);
void	pthread_spinlock(pthread_t thread, pthread_spin_t *lock);
int	pthread_spintrylock(pthread_t thread, pthread_spin_t *lock);
void	pthread_spinunlock(pthread_t thread, pthread_spin_t *lock);

extern const struct pthread_lock_ops *pthread__lock_ops;

#define	pthread__simple_lock_init(alp)	(*pthread__lock_ops->plo_init)(alp)
#define	pthread__simple_lock_try(alp)	(*pthread__lock_ops->plo_try)(alp)
#define	pthread__simple_unlock(alp)	(*pthread__lock_ops->plo_unlock)(alp)

#ifndef _getcontext_u
int	_getcontext_u(ucontext_t *);
#endif
#ifndef _setcontext_u
int	_setcontext_u(const ucontext_t *);
#endif
#ifndef _swapcontext_u
int	_swapcontext_u(ucontext_t *, const ucontext_t *);
#endif

void	pthread__testcancel(pthread_t self);
int	pthread__find(pthread_t self, pthread_t target);

#ifndef PTHREAD_MD_INIT
#define PTHREAD_MD_INIT
#endif

#ifndef _INITCONTEXT_U_MD
#define _INITCONTEXT_U_MD(ucp)
#endif

#define _INITCONTEXT_U(ucp) do {					\
	(ucp)->uc_flags = _UC_CPU | _UC_STACK;				\
	_INITCONTEXT_U_MD(ucp)						\
	} while (/*CONSTCOND*/0)

#ifdef PTHREAD_MACHINE_HAS_ID_REGISTER
#define pthread__id(reg) (reg)
#else
/* Stack location of pointer to a particular thread */
#define pthread__id(sp) \
	((pthread_t) (((vaddr_t)(sp)) & ~PT_STACKMASK))

#define pthread__id_reg() pthread__sp()
#endif

#define pthread__self() (pthread__id(pthread__id_reg()))

#define pthread__abort()						\
	pthread__assertfunc(__FILE__, __LINE__, __func__, "unreachable")

#define pthread__assert(e) do {						\
	if (__predict_false(!(e)))					\
       	       pthread__assertfunc(__FILE__, __LINE__, __func__, #e);	\
        } while (/*CONSTCOND*/0)

#define pthread__error(err, msg, e) do {				\
	if (__predict_false(!(e))) {					\
       	       pthread__errorfunc(__FILE__, __LINE__, __func__, msg);	\
	       return (err);						\
	} 								\
        } while (/*CONSTCOND*/0)



/* These three routines are defined in processor-specific code. */
void	pthread__upcall_switch(pthread_t self, pthread_t next);
void	pthread__switch(pthread_t self, pthread_t next);
void	pthread__locked_switch(pthread_t self, pthread_t next, 
    pthread_spin_t *lock);

void	pthread__signal_init(void);
void	pthread__signal_start(void);

void	pthread__signal(pthread_t self, pthread_t t, siginfo_t *si);
void	pthread__deliver_signal(pthread_t self, pthread_t t, siginfo_t *si);
void	pthread__signal_deferred(pthread_t self, pthread_t t);

void	pthread__destroy_tsd(pthread_t self);
void	pthread__assertfunc(const char *file, int line, const char *function,
		const char *expr);
void	pthread__errorfunc(const char *file, int line, const char *function,
		const char *msg);

#endif /* _LIB_PTHREAD_INT_H */
