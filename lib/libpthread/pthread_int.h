/*	$NetBSD: pthread_int.h,v 1.1.2.23 2002/04/26 17:45:29 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <sa.h>
#include <signal.h>

#define PTHREAD__DEBUG
#define ERRORCHECK

#include "pthread_types.h"
#include "pthread_queue.h"

/* The size of this structure needs to be no larger than struct
 * __pthread_cleanup_store, defined in pthread.h.
 */
struct pt_clean_t {
	PTQ_ENTRY(pt_clean_t)	ptc_next;
	void	(*ptc_cleanup)(void *);
	void	*ptc_arg;
};

struct	pthread_st {
	unsigned int	pt_magic;

	int	pt_type;	/* normal, upcall, or idle */
	int	pt_state;	/* running, blocked, etc. */
	pthread_spin_t pt_statelock;	/* lock on pt_state */
	int	pt_flags;	/* see PT_FLAG_* below */
	int	pt_cancel;	/* Deferred cancellation */
	int	pt_spinlocks;	/* Number of spinlocks held. */
	int	pt_blockedlwp;	/* LWP/SA number when blocked */

	int	pt_errno;	/* Thread-specific errno. */

	/* Entry on the run queue */
	PTQ_ENTRY(pthread_st)	pt_runq;
	/* Entry on the list of all threads */
	PTQ_ENTRY(pthread_st)	pt_allq;
	/* Entry on the sleep queue (xxx should be same as run queue?) */
	PTQ_ENTRY(pthread_st)	pt_sleep;
	/* Objecy we're sleeping on */
	void	*pt_sleepobj;
	/* Queue we're sleeping on */
	struct pthread_queue_t		*pt_sleepq;
	/* Lock protecting that queue */
	pthread_spin_t		*pt_sleeplock;

	stack_t		pt_stack;	/* Our stack */
	ucontext_t	*pt_uc;		/* Saved context when we're stopped */

	sigset_t	pt_sigmask;	/* Signals we won't take. */
	sigset_t	pt_siglist;	/* Signals pending for us. */
	pthread_spin_t	pt_siglock;	/* Lock on above */

	void *		pt_exitval;	/* Read by pthread_join() */

	/* Stack of cancellation cleanup handlers and their arguments */
	PTQ_HEAD(, pt_clean_t)	pt_cleanup_stack;

	/* Other threads trying to pthread_join() us. */
	struct pthread_queue_t	pt_joiners;
	/* Lock for above, and for changing pt_state to ZOMBIE or DEAD,
	 * and for setting the DETACHED flag
	 */
	pthread_spin_t	pt_join_lock;

	/* Thread we were going to switch to before we were preempted
	 * ourselves. Will be used by the upcall that's continuing us.
	 */
	pthread_t	pt_switchto;
	ucontext_t*	pt_switchtouc;

	/* The context we saved in pthread__locked_switch but which
	 * was trashed when we were preempted before switching stacks.
	 */
	ucontext_t*	pt_sleepuc;

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

	/* Identifier, for debugging. */
	int		pt_num;

	/* Thread-specific data */
	void*		pt_specific[PTHREAD_KEYS_MAX];

#ifdef PTHREAD__DEBUG
	int	blocks;
	int	preempts;
	int	rescheds;
#endif
};



/* Thread types */
#define PT_THREAD_NORMAL	1
#define PT_THREAD_UPCALL	2
#define PT_THREAD_IDLE		3

/* Thread states */
#define PT_STATE_RUNNING	1
#define PT_STATE_RUNNABLE	2
#define PT_STATE_BLOCKED_SYS	3
#define PT_STATE_BLOCKED_QUEUE	4
#define PT_STATE_ZOMBIE		5
#define PT_STATE_DEAD		6
#define PT_STATE_RECYCLABLE	7

/* Flag values */

#define PT_FLAG_DETACHED	0x0001
#define PT_FLAG_IDLED		0x0002
#define PT_FLAG_CS_DISABLED	0x0004	/* Cancellation disabled */
#define PT_FLAG_CS_ASYNC	0x0008  /* Cancellation is async */
#define PT_FLAG_CS_PENDING	0x0010

#define PT_MAGIC	0x11110001
#define PT_DEAD		0xDEAD0001

#define PT_ATTR_MAGIC	0x22220002
#define PT_ATTR_DEAD	0xDEAD0002

#define PT_STACKSIZE	(1<<16) 
#define PT_STACKMASK	(PT_STACKSIZE-1)

#define PT_UPCALLSTACKS	16

#define PT_ALARMTIMER_MAGIC	0x88880008
#define NIDLETHREADS	4
#define IDLESPINS	1000

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

/* Go do something else. Don't go back on the run queue */
void	pthread__block(pthread_t self, pthread_spin_t* queuelock);
/* Put a thread back on the run queue */
void	pthread__sched(pthread_t self, pthread_t thread);
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

/* Alarm code */
void	pthread__alarm_init(void);
void	*pthread__alarm_add(pthread_t, const struct timespec *,
    void (*)(void *), void *);
void	pthread__alarm_del(pthread_t, void *);
int	pthread__alarm_fired(void *);
void	pthread__alarm_process(pthread_t self, void *arg);

/* Internal locking primitives */
void	pthread_lockinit(pthread_spin_t *lock);
void	pthread_spinlock(pthread_t thread, pthread_spin_t *lock);
int	pthread_spintrylock(pthread_t thread, pthread_spin_t *lock);
void	pthread_spinunlock(pthread_t thread, pthread_spin_t *lock);

#include "pthread_md.h"

int	_getcontext_u(ucontext_t *);
int	_setcontext_u(const ucontext_t *);
int	_swapcontext_u(ucontext_t *, const ucontext_t *);

void	pthread__testcancel(pthread_t self);

/* Stack location of pointer to a particular thread */
#define pthread__id(sp) \
	((pthread_t) (((vaddr_t)(sp)) & ~PT_STACKMASK))

#define pthread__self() (pthread__id(pthread__sp()))

/* These three routines are defined in processor-specific code. */
void	pthread__upcall_switch(pthread_t self, pthread_t next);
void	pthread__switch(pthread_t self, pthread_t next);
void	pthread__locked_switch(pthread_t self, pthread_t next, 
    pthread_spin_t *lock);


void	pthread__signal(pthread_t t, int sig, int code);

void	pthread__destroy_tsd(pthread_t self);


#define PTHREADD_CREATE		0
#define PTHREADD_IDLE		1
#define PTHREADD_UPCALLS	2
#define PTHREADD_UP_BLOCK       3
#define PTHREADD_UP_NEW		4
#define PTHREADD_UP_PREEMPT	5
#define PTHREADD_UP_UNBLOCK	6
#define PTHREADD_UP_SIGNAL	7
#define PTHREADD_SPINLOCKS	8
#define PTHREADD_SPINUNLOCKS	9
#define PTHREADD_SPINPREEMPT	10
#define PTHREADD_RESOLVELOCKS	11
#define PTHREADD_SWITCHTO	12
#define PTHREADD_NCOUNTERS	13

#ifdef PTHREAD__DEBUG

#define PTHREAD__DEBUG_SHMKEY	(0x000f)
#define PTHREAD__DEBUG_SHMSIZE	(1<<18)

extern int pthread__debug_counters[PTHREADD_NCOUNTERS];

extern int pthread__dbg; /* Set by libpthread_dbg */

#define PTHREADD_ADD(x) (pthread__debug_counters[(x)]++)

#define DPRINTF(x) pthread__debuglog_printf x

struct	pthread_msgbuf {
#define BUF_MAGIC	0x090976
	int	msg_magic;
	long	msg_bufw;
	long	msg_bufr;
	long	msg_bufs;
	char	msg_bufc[1];
};

void pthread__debug_init(void);
struct pthread_msgbuf* pthread__debuglog_init(int force);
void pthread__debuglog_printf(const char *fmt, ...);

#else /* PTHREAD_DEBUG */

#define PTHREADD_ADD(x)
#define DPRINTF(x)

#endif /* PTHREAD_DEBUG */

#endif /* _LIB_PTHREAD_INT_H */

