/*	$NetBSD: pthread.c,v 1.62 2007/02/21 22:31:38 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2003, 2006, 2007 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread.c,v 1.62 2007/02/21 22:31:38 ad Exp $");

#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#ifdef PTHREAD_MLOCK_KLUDGE
#include <sys/mman.h>
#endif

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"

#ifdef PTHREAD_MAIN_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

/* Maximum number of LWPs to unpark in one operation. */
#define	PTHREAD__UNPARK_MAX	128

/* How many times to try acquiring spin locks on MP systems. */
#define	PTHREAD__NSPINS		1000

static void	pthread__create_tramp(void *(*start)(void *), void *arg);
static void	pthread__dead(pthread_t, pthread_t);

int pthread__started;

pthread_spin_t pthread__allqueue_lock = __SIMPLELOCK_UNLOCKED;
struct pthread_queue_t pthread__allqueue;

pthread_spin_t pthread__deadqueue_lock = __SIMPLELOCK_UNLOCKED;
struct pthread_queue_t pthread__deadqueue;
struct pthread_queue_t *pthread__reidlequeue;

static int nthreads;
static int nextthread;
static pthread_spin_t nextthread_lock = __SIMPLELOCK_UNLOCKED;
static pthread_attr_t pthread_default_attr;

enum {
	DIAGASSERT_ABORT =	1<<0,
	DIAGASSERT_STDERR =	1<<1,
	DIAGASSERT_SYSLOG =	1<<2
};

static int pthread__diagassert = DIAGASSERT_ABORT | DIAGASSERT_STDERR;

#ifdef PTHREAD_SA
pthread_spin_t pthread__runqueue_lock = __SIMPLELOCK_UNLOCKED;
struct pthread_queue_t pthread__runqueue;
struct pthread_queue_t pthread__idlequeue;
struct pthread_queue_t pthread__suspqueue;
#endif

int pthread__concurrency, pthread__maxconcurrency, pthread__nspins;
int pthread__unpark_max = PTHREAD__UNPARK_MAX;

int _sys___sigprocmask14(int, const sigset_t *, sigset_t *);

__strong_alias(__libc_thr_self,pthread_self)
__strong_alias(__libc_thr_create,pthread_create)
__strong_alias(__libc_thr_exit,pthread_exit)
__strong_alias(__libc_thr_errno,pthread__errno)
__strong_alias(__libc_thr_setcancelstate,pthread_setcancelstate)

/*
 * Static library kludge.  Place a reference to a symbol any library
 * file which does not already have a reference here.
 */
extern int pthread__cancel_stub_binder;
#ifdef PTHREAD_SA
extern int pthread__sched_binder;
extern struct pthread_queue_t pthread__nanosleeping;
#endif

void *pthread__static_lib_binder[] = {
	&pthread__cancel_stub_binder,
	pthread_cond_init,
	pthread_mutex_init,
	pthread_rwlock_init,
	pthread_barrier_init,
	pthread_key_create,
	pthread_setspecific,
#ifdef PTHREAD_SA
	&pthread__sched_binder,
	&pthread__nanosleeping
#endif
};

/*
 * This needs to be started by the library loading code, before main()
 * gets to run, for various things that use the state of the initial thread
 * to work properly (thread-specific data is an application-visible example;
 * spinlock counts for mutexes is an internal example).
 */
void
pthread_init(void)
{
	pthread_t first;
	char *p;
	int i, mib[2], ncpu;
	size_t len;
	extern int __isthreaded;
#ifdef PTHREAD_MLOCK_KLUDGE
	int ret;
#endif

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU; 

	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	/* Initialize locks first; they're needed elsewhere. */
	pthread__lockprim_init(ncpu);

#ifdef PTHREAD_SA
	/* Find out requested/possible concurrency */
	p = getenv("PTHREAD_CONCURRENCY");
	pthread__maxconcurrency = p ? atoi(p) : 1;
		
	if (pthread__maxconcurrency < 1)
		pthread__maxconcurrency = 1;
	if (pthread__maxconcurrency > ncpu)
		pthread__maxconcurrency = ncpu;

	/* Allocate data structures */
	pthread__reidlequeue = (struct pthread_queue_t *)malloc
		(pthread__maxconcurrency * sizeof(struct pthread_queue_t));
	if (pthread__reidlequeue == NULL)
		err(1, "Couldn't allocate memory for pthread__reidlequeue");

	pthread__nspins = PTHREAD__NSPINS;
#else
	/*
	 * Get number of CPUs, and maximum number of LWPs that can be
	 * unparked at once.
	 */
	if ((pthread__concurrency = ncpu) > 1)
		pthread__nspins = PTHREAD__NSPINS;
	else
		pthread__nspins = 1;
	i = (int)_lwp_unpark_all(NULL, 0, NULL);
	if (i == -1)
		err(1, "_lwp_unpark_all");
	if (i < pthread__unpark_max)
		pthread__unpark_max = i;
#endif

	/* Basic data structure setup */
	pthread_attr_init(&pthread_default_attr);
	PTQ_INIT(&pthread__allqueue);
	PTQ_INIT(&pthread__deadqueue);
#ifdef PTHREAD_MLOCK_KLUDGE
	ret = mlock(&pthread__deadqueue, sizeof(pthread__deadqueue));
	pthread__assert(ret == 0);
#endif
#ifdef PTHREAD_SA
	PTQ_INIT(&pthread__runqueue);
	PTQ_INIT(&pthread__idlequeue);
	for (i = 0; i < pthread__maxconcurrency; i++)
		PTQ_INIT(&pthread__reidlequeue[i]);
#endif
	nthreads = 1;
	/* Create the thread structure corresponding to main() */
	pthread__initmain(&first);
	pthread__initthread(first, first);

	first->pt_state = PT_STATE_RUNNING;
#ifdef PTHREAD_SA
	_sys___sigprocmask14(0, NULL, &first->pt_sigmask);
#else
	first->pt_lid = _lwp_self();
#endif
	PTQ_INSERT_HEAD(&pthread__allqueue, first, pt_allq);

	/* Start subsystems */
#ifdef PTHREAD_SA
	pthread__signal_init();
#endif
	PTHREAD_MD_INIT
#ifdef PTHREAD__DEBUG
	pthread__debug_init(ncpu);
#endif

	for (p = getenv("PTHREAD_DIAGASSERT"); p && *p; p++) {
		switch (*p) {
		case 'a':
			pthread__diagassert |= DIAGASSERT_ABORT;
			break;
		case 'A':
			pthread__diagassert &= ~DIAGASSERT_ABORT;
			break;
		case 'e':
			pthread__diagassert |= DIAGASSERT_STDERR;
			break;
		case 'E':
			pthread__diagassert &= ~DIAGASSERT_STDERR;
			break;
		case 'l':
			pthread__diagassert |= DIAGASSERT_SYSLOG;
			break;
		case 'L':
			pthread__diagassert &= ~DIAGASSERT_SYSLOG;
			break;
		}
	}


	/* Tell libc that we're here and it should role-play accordingly. */
	__isthreaded = 1;
}

static void
pthread__child_callback(void)
{
	/*
	 * Clean up data structures that a forked child process might
	 * trip over. Note that if threads have been created (causing
	 * this handler to be registered) the standards say that the
	 * child will trigger undefined behavior if it makes any
	 * pthread_* calls (or any other calls that aren't
	 * async-signal-safe), so we don't really have to clean up
	 * much. Anything that permits some pthread_* calls to work is
	 * merely being polite.
	 */
	pthread__started = 0;
}

static void
pthread__start(void)
{
	pthread_t self;
#ifdef PTHREAD_SA
	pthread_t idle;
	int i, ret;
#endif

	self = pthread__self(); /* should be the "main()" thread */

	/*
	 * Per-process timers are cleared by fork(); despite the
	 * various restrictions on fork() and threads, it's legal to
	 * fork() before creating any threads. 
	 */
#ifdef PTHREAD_SA
	pthread__alarm_init();

	pthread__signal_start();
#endif

	pthread_atfork(NULL, NULL, pthread__child_callback);

#ifdef PTHREAD_SA
	/*
	 * Create idle threads
	 * XXX need to create more idle threads if concurrency > 3
	 */
	for (i = 0; i < NIDLETHREADS; i++) {
		ret = pthread__stackalloc(&idle);
		if (ret != 0)
			err(1, "Couldn't allocate stack for idle thread!");
		pthread__initthread(self, idle);
		sigfillset(&idle->pt_sigmask);
		idle->pt_type = PT_THREAD_IDLE;
		PTQ_INSERT_HEAD(&pthread__allqueue, idle, pt_allq);
		pthread__sched_idle(self, idle);
	}

	/* Start up the SA subsystem */
	pthread__sa_start();
#endif

	SDPRINTF(("(pthread__start %p) Started.\n", self));
}


/* General-purpose thread data structure sanitization. */
void
pthread__initthread(pthread_t self, pthread_t t)
{
	int id;

	pthread_spinlock(self, &nextthread_lock);
	id = nextthread;
	nextthread++;
	pthread_spinunlock(self, &nextthread_lock);
	t->pt_num = id;

	t->pt_magic = PT_MAGIC;
	pthread_lockinit(&t->pt_flaglock);
	t->pt_spinlocks = 0;
	t->pt_exitval = NULL;
	t->pt_flags = 0;
	t->pt_cancel = 0;
	t->pt_errno = 0;

#ifdef PTHREAD_SA
	t->pt_type = PT_THREAD_NORMAL;
	t->pt_state = PT_STATE_RUNNABLE;
	t->pt_heldlock = NULL;
	t->pt_next = NULL;
	t->pt_parent = NULL;
	t->pt_switchto = NULL;
	t->pt_trapuc = NULL;
	sigemptyset(&t->pt_siglist);
	sigemptyset(&t->pt_sigmask);
	pthread_lockinit(&t->pt_siglock);
#else
	t->pt_state = PT_STATE_RUNNING;
#endif

	pthread_lockinit(&t->pt_statelock);

	PTQ_INIT(&t->pt_joiners);
	pthread_lockinit(&t->pt_join_lock);
	PTQ_INIT(&t->pt_cleanup_stack);
	memset(&t->pt_specific, 0, sizeof(int) * PTHREAD_KEYS_MAX);
	t->pt_name = NULL;

#if defined(PTHREAD__DEBUG) && defined(PTHREAD_SA)
	t->blocks = 0;
	t->preempts = 0;
	t->rescheds = 0;
#endif
}


int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	    void *(*startfunc)(void *), void *arg)
{
	pthread_t self, newthread;
	pthread_attr_t nattr;
	struct pthread_attr_private *p;
	char * volatile name;
	int ret;
#ifndef PTHREAD_SA
	int flag;
#endif

	PTHREADD_ADD(PTHREADD_CREATE);

	/*
	 * It's okay to check this without a lock because there can
	 * only be one thread before it becomes true.
	 */
	if (pthread__started == 0) {
		pthread__start();
		pthread__started = 1;
	}

	if (attr == NULL)
		nattr = pthread_default_attr;
	else if (attr->pta_magic == PT_ATTR_MAGIC)
		nattr = *attr;
	else
		return EINVAL;

	/* Fetch misc. attributes from the attr structure. */
	name = NULL;
	if ((p = nattr.pta_private) != NULL)
		if (p->ptap_name[0] != '\0')
			if ((name = strdup(p->ptap_name)) == NULL)
				return ENOMEM;

	self = pthread__self();

	pthread_spinlock(self, &pthread__deadqueue_lock);
	newthread = PTQ_FIRST(&pthread__deadqueue);
	if (newthread != NULL) {
#ifndef PTHREAD_SA
		if ((newthread->pt_flags & PT_FLAG_DETACHED) != 0 &&
		    (_lwp_kill(newthread->pt_lid, 0) == 0 || errno != ESRCH))
			newthread = NULL;
		else
#endif
			PTQ_REMOVE(&pthread__deadqueue, newthread, pt_allq);
	}
	pthread_spinunlock(self, &pthread__deadqueue_lock);
	if (newthread == NULL) {
		/* Set up a stack and allocate space for a pthread_st. */
		ret = pthread__stackalloc(&newthread);
		if (ret != 0) {
			if (name)
				free(name);
			return ret;
		}
	}

	/* 2. Set up state. */
	pthread__initthread(self, newthread);
	newthread->pt_flags = nattr.pta_flags;
#ifdef PTHREAD_SA
	newthread->pt_sigmask = self->pt_sigmask;
#endif

	/* 3. Set up misc. attributes. */
	newthread->pt_name = name;

	/*
	 * 4. Set up context.
	 *
	 * The pt_uc pointer points to a location safely below the
	 * stack start; this is arranged by pthread__stackalloc().
	 */
	_INITCONTEXT_U(newthread->pt_uc);
#ifdef PTHREAD_MACHINE_HAS_ID_REGISTER
	pthread__uc_id(newthread->pt_uc) = newthread;
#endif
	newthread->pt_uc->uc_stack = newthread->pt_stack;
	newthread->pt_uc->uc_link = NULL;
	makecontext(newthread->pt_uc, pthread__create_tramp, 2,
	    startfunc, arg);

	/* 5. Add to list of all threads. */
	pthread_spinlock(self, &pthread__allqueue_lock);
	PTQ_INSERT_HEAD(&pthread__allqueue, newthread, pt_allq);
	nthreads++;
	pthread_spinunlock(self, &pthread__allqueue_lock);

#ifndef PTHREAD_SA
	/* 5a. Create the new LWP. */
	newthread->pt_sleeponq = 0;
	flag = 0;
	if ((newthread->pt_flags & PT_FLAG_SUSPENDED) != 0)
		flag |= LWP_SUSPENDED;
	if ((newthread->pt_flags & PT_FLAG_DETACHED) != 0)
		flag |= LWP_DETACHED;
	ret = _lwp_create(newthread->pt_uc, (u_long)flag, &newthread->pt_lid);
	if (ret != 0) {
		SDPRINTF(("(pthread_create %p) _lwp_create: %s\n",
		    strerror(errno)));
		free(name);
		pthread_spinlock(self, &pthread__allqueue_lock);
		PTQ_REMOVE(&pthread__allqueue, newthread, pt_allq);
		nthreads--;
		pthread_spinunlock(self, &pthread__allqueue_lock);
		pthread_spinlock(self, &pthread__deadqueue_lock);
		PTQ_INSERT_HEAD(&pthread__deadqueue, newthread, pt_allq);
		pthread_spinunlock(self, &pthread__deadqueue_lock);
		return ret;
	}
#endif

#ifdef PTHREAD_SA
	SDPRINTF(("(pthread_create %p) new thread %p (name pointer %p).\n",
		  self, newthread, newthread->pt_name));
	/* 6. Put on appropriate queue. */
	if (newthread->pt_flags & PT_FLAG_SUSPENDED) {
		pthread_spinlock(self, &newthread->pt_statelock);
		pthread__suspend(self, newthread);
		pthread_spinunlock(self, &newthread->pt_statelock);
	} else
		pthread__sched(self, newthread);
#else
	SDPRINTF(("(pthread_create %p) new thread %p (name %p, lid %d).\n",
		  self, newthread, newthread->pt_name,
		  (int)newthread->pt_lid));
#endif
	
	*thread = newthread;

	return 0;
}


static void
pthread__create_tramp(void *(*start)(void *), void *arg)
{
	void *retval;

	retval = (*start)(arg);

	pthread_exit(retval);

	/*NOTREACHED*/
	pthread__abort();
}

int
pthread_suspend_np(pthread_t thread)
{
	pthread_t self;

	self = pthread__self();
	if (self == thread) {
		return EDEADLK;
	}
#ifdef ERRORCHECK
	if (pthread__find(self, thread) != 0)
		return ESRCH;
#endif
#ifdef PTHREAD_SA
	SDPRINTF(("(pthread_suspend_np %p) Suspend thread %p (state %d).\n",
		     self, thread, thread->pt_state));
	pthread_spinlock(self, &thread->pt_statelock);
	if (thread->pt_blockgen != thread->pt_unblockgen) {
		/* XXX flaglock? */
		thread->pt_flags |= PT_FLAG_SUSPENDED;
		pthread_spinunlock(self, &thread->pt_statelock);
		return 0;
	}
	switch (thread->pt_state) {
	case PT_STATE_RUNNING:
		pthread__abort();	/* XXX */
		break;
	case PT_STATE_SUSPENDED:
		pthread_spinunlock(self, &thread->pt_statelock);
		return 0;
	case PT_STATE_RUNNABLE:
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__runqueue, thread, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		break;
	case PT_STATE_BLOCKED_QUEUE:
		pthread_spinlock(self, thread->pt_sleeplock);
		PTQ_REMOVE(thread->pt_sleepq, thread, pt_sleep);
		pthread_spinunlock(self, thread->pt_sleeplock);
		break;
	case PT_STATE_ZOMBIE:
		goto out;
	default:
		break;			/* XXX */
	}
	pthread__suspend(self, thread);

out:
	pthread_spinunlock(self, &thread->pt_statelock);
	return 0;
#else
	SDPRINTF(("(pthread_suspend_np %p) Suspend thread %p.\n",
		     self, thread));
	return _lwp_suspend(thread->pt_lid);
#endif
}

int
pthread_resume_np(pthread_t thread)
{
	pthread_t self;

	self = pthread__self();
#ifdef ERRORCHECK
	if (pthread__find(self, thread) != 0)
		return ESRCH;
#endif
#ifdef PTHREAD_SA
	SDPRINTF(("(pthread_resume_np %p) Resume thread %p (state %d).\n",
		     self, thread, thread->pt_state));
	pthread_spinlock(self, &thread->pt_statelock);
	/* XXX flaglock? */
	thread->pt_flags &= ~PT_FLAG_SUSPENDED;
	if (thread->pt_state == PT_STATE_SUSPENDED) {
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__suspqueue, thread, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		pthread__sched(self, thread);
	}
	pthread_spinunlock(self, &thread->pt_statelock);
	return 0;
#else
	SDPRINTF(("(pthread_resume_np %p) Resume thread %p.\n",
		     self, thread));
	return _lwp_continue(thread->pt_lid);
#endif
}

#ifdef PTHREAD_SA
/*
 * Other threads will switch to the idle thread so that they
 * can dispose of any awkward locks or recycle upcall state.
 */
void
pthread__idle(void)
{
	pthread_t self;

	PTHREADD_ADD(PTHREADD_IDLE);
	self = pthread__self();
	SDPRINTF(("(pthread__idle %p).\n", self));

	/*
	 * The drill here is that we want to yield the processor,
	 * but for the thread itself to be recovered, we need to be on
	 * a list somewhere for the thread system to know about us.
	 */
	pthread_spinlock(self, &pthread__deadqueue_lock);
	PTQ_INSERT_TAIL(&pthread__reidlequeue[self->pt_vpid], self, pt_runq);
	pthread__concurrency--;
	SDPRINTF(("(yield %p concurrency) now %d\n", self,
		     pthread__concurrency));
	/* Don't need a flag lock; nothing else has a handle on this thread */
	self->pt_flags |= PT_FLAG_IDLED;
	pthread_spinunlock(self, &pthread__deadqueue_lock);

	/*
	 * If we get to run this, then no preemption has happened
	 * (because the upcall handler will not continue an idle thread with
	 * PT_FLAG_IDLED set), and so we can yield the processor safely.
	 */
	SDPRINTF(("(pthread__idle %p) yielding.\n", self));
	sa_yield();

	/* NOTREACHED */
	self->pt_spinlocks++; /* XXX make sure we get to finish the assert! */
	SDPRINTF(("(pthread__idle %p) Returned! Error.\n", self));
	pthread__abort();
}
#endif


void
pthread_exit(void *retval)
{
	pthread_t self;
	struct pt_clean_t *cleanup;
	char *name;
	int nt;

	self = pthread__self();
	SDPRINTF(("(pthread_exit %p) status %p, flags %x, cancel %d\n",
		  self, retval, self->pt_flags, self->pt_cancel));

	/* Disable cancellability. */
	pthread_spinlock(self, &self->pt_flaglock);
	self->pt_flags |= PT_FLAG_CS_DISABLED;
	self->pt_cancel = 0;
	pthread_spinunlock(self, &self->pt_flaglock);

	/* Call any cancellation cleanup handlers */
	while (!PTQ_EMPTY(&self->pt_cleanup_stack)) {
		cleanup = PTQ_FIRST(&self->pt_cleanup_stack);
		PTQ_REMOVE(&self->pt_cleanup_stack, cleanup, ptc_next);
		(*cleanup->ptc_cleanup)(cleanup->ptc_arg);
	}

	/* Perform cleanup of thread-specific data */
	pthread__destroy_tsd(self);

	self->pt_exitval = retval;

	/*
	 * it's safe to check PT_FLAG_DETACHED without pt_flaglock
	 * because it's only set by pthread_detach with pt_join_lock held.
	 */
	pthread_spinlock(self, &self->pt_join_lock);
	if (self->pt_flags & PT_FLAG_DETACHED) {
		self->pt_state = PT_STATE_DEAD;
		pthread_spinunlock(self, &self->pt_join_lock);
		name = self->pt_name;
		self->pt_name = NULL;

		if (name != NULL)
			free(name);

		pthread_spinlock(self, &pthread__allqueue_lock);
		PTQ_REMOVE(&pthread__allqueue, self, pt_allq);
		nthreads--;
		nt = nthreads;
		pthread_spinunlock(self, &pthread__allqueue_lock);

		if (nt == 0) {
			/* Whoah, we're the last one. Time to go. */
			exit(0);
		}

		/* Yeah, yeah, doing work while we're dead is tacky. */
		pthread_spinlock(self, &pthread__deadqueue_lock);
		PTQ_INSERT_TAIL(&pthread__deadqueue, self, pt_allq);

#ifdef PTHREAD_SA
		pthread__block(self, &pthread__deadqueue_lock);
		SDPRINTF(("(pthread_exit %p) walking dead\n", self));
		pthread_spinunlock(self, &pthread__allqueue_lock);
#else
		pthread_spinunlock(self, &pthread__deadqueue_lock);
		_lwp_exit();
#endif
	} else {
		self->pt_state = PT_STATE_ZOMBIE;

		/* Note: name will be freed by the joiner. */
		pthread_spinlock(self, &pthread__allqueue_lock);
		nthreads--;
		nt = nthreads;
		pthread_spinunlock(self, &pthread__allqueue_lock);
		if (nt == 0) {
			/* Whoah, we're the last one. Time to go. */
			exit(0);
		}

#ifdef PTHREAD_SA
		/*
		 * Wake up all the potential joiners. Only one can win.
		 * (Can you say "Thundering Herd"? I knew you could.)
		 */
		pthread__sched_sleepers(self, &self->pt_joiners);
		pthread__block(self, &self->pt_join_lock);
		SDPRINTF(("(pthread_exit %p) walking zombie\n", self));
#else
		pthread_spinunlock(self, &self->pt_join_lock);
		_lwp_exit();
#endif
	}

	/*NOTREACHED*/
	pthread__abort();
	exit(1);
}


int
pthread_join(pthread_t thread, void **valptr)
{
	pthread_t self;
	char *name;
	int num, retval;

	self = pthread__self();
	SDPRINTF(("(pthread_join %p) Joining %p.\n", self, thread));

	if (pthread__find(self, thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	if (thread == self)
		return EDEADLK;

#ifdef PTHREAD_SA
	pthread_spinlock(self, &thread->pt_flaglock);

	if (thread->pt_flags & PT_FLAG_DETACHED) {
		pthread_spinunlock(self, &thread->pt_flaglock);
		return EINVAL;
	}

	num = thread->pt_num;
	pthread_spinlock(self, &thread->pt_join_lock);
	while (thread->pt_state != PT_STATE_ZOMBIE) {
		if ((thread->pt_state == PT_STATE_DEAD) ||
		    (thread->pt_flags & PT_FLAG_DETACHED) ||
		    (thread->pt_num != num)) {
			/*
			 * Another thread beat us to the join, or called
			 * pthread_detach(). If num didn't match, the
			 * thread died and was recycled before we got
			 * another chance to run.
			 */
			pthread_spinunlock(self, &thread->pt_join_lock);
			pthread_spinunlock(self, &thread->pt_flaglock);
			return ESRCH;
		}
		/*
		 * "I'm not dead yet!"
		 * "You will be soon enough."
		 */
		pthread_spinunlock(self, &thread->pt_flaglock);
		pthread_spinlock(self, &self->pt_statelock);
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_statelock);
			pthread_spinunlock(self, &thread->pt_join_lock);
			pthread_exit(PTHREAD_CANCELED);
		}
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = thread;
		self->pt_sleepq = &thread->pt_joiners;
		self->pt_sleeplock = &thread->pt_join_lock;
		pthread_spinunlock(self, &self->pt_statelock);

		PTQ_INSERT_TAIL(&thread->pt_joiners, self, pt_sleep);
		pthread__block(self, &thread->pt_join_lock);
		pthread_spinlock(self, &thread->pt_flaglock);
		pthread_spinlock(self, &thread->pt_join_lock);
	}

	/* All ours. */
	thread->pt_state = PT_STATE_DEAD;
	name = thread->pt_name;
	thread->pt_name = NULL;
	pthread_spinunlock(self, &thread->pt_join_lock);
	pthread_spinunlock(self, &thread->pt_flaglock);

	if (valptr != NULL)
		*valptr = thread->pt_exitval;

	retval = 0;
#else	/* PTHREAD_SA */
	retval = 0;
	name = NULL;
 again:
 	pthread_spinlock(self, &thread->pt_join_lock);
	switch (thread->pt_state) {
	case PT_STATE_RUNNING:
		pthread_spinunlock(self, &thread->pt_join_lock);

		/*
		 * IEEE Std 1003.1, 2004 Edition:
		 *
		 * "The pthread_join() function shall not
		 * return an error code of [EINTR]."
		 */
		if (_lwp_wait(thread->pt_lid, &num) != 0 && errno != EINTR)
			return errno;
		goto again;
	case PT_STATE_ZOMBIE:
		if (valptr != NULL)
			*valptr = thread->pt_exitval;
		if (retval == 0) {
			name = thread->pt_name;
			thread->pt_name = NULL;
		}
		thread->pt_state = PT_STATE_DEAD;
		pthread_spinunlock(self, &thread->pt_join_lock);
		(void)_lwp_detach(thread->pt_lid);
		break;
	default:
		pthread_spinunlock(self, &thread->pt_join_lock);
		return EINVAL;
	}
#endif	/* PTHREAD_SA */

	SDPRINTF(("(pthread_join %p) Joined %p.\n", self, thread));

	pthread__dead(self, thread);

	if (name != NULL)
		free(name);

	return retval;
}


int
pthread_equal(pthread_t t1, pthread_t t2)
{

	/* Nothing special here. */
	return (t1 == t2);
}


int
pthread_detach(pthread_t thread)
{
	pthread_t self;
#ifdef PTHREAD_SA
	int doreclaim = 0;
	char *name = NULL;
#endif

	self = pthread__self();

	if (pthread__find(self, thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

#ifdef PTHREAD_SA
	pthread_spinlock(self, &thread->pt_flaglock);
	pthread_spinlock(self, &thread->pt_join_lock);

	if (thread->pt_flags & PT_FLAG_DETACHED) {
		pthread_spinunlock(self, &thread->pt_join_lock);
		pthread_spinunlock(self, &thread->pt_flaglock);
		return EINVAL;
	}

	thread->pt_flags |= PT_FLAG_DETACHED;

	/* Any joiners have to be punted now. */
	pthread__sched_sleepers(self, &thread->pt_joiners);

	if (thread->pt_state == PT_STATE_ZOMBIE) {
		thread->pt_state = PT_STATE_DEAD;
		name = thread->pt_name;
		thread->pt_name = NULL;
		doreclaim = 1;
	}

	pthread_spinunlock(self, &thread->pt_join_lock);
	pthread_spinunlock(self, &thread->pt_flaglock);

	if (doreclaim) {
		pthread__dead(self, thread);
		if (name != NULL)
			free(name);
	}

	return 0;
#else
	thread->pt_flags |= PT_FLAG_DETACHED;
	return _lwp_detach(thread->pt_lid);
#endif
}


static void
pthread__dead(pthread_t self, pthread_t thread)
{

	SDPRINTF(("(pthread__dead %p) Reclaimed %p.\n", self, thread));
#ifdef PTHREAD_SA
	pthread__assert(thread != self);
#endif
	pthread__assert(thread->pt_state == PT_STATE_DEAD);
	pthread__assert(thread->pt_name == NULL);

	/* Cleanup time. Move the dead thread from allqueue to the deadqueue */
	pthread_spinlock(self, &pthread__allqueue_lock);
	PTQ_REMOVE(&pthread__allqueue, thread, pt_allq);
	pthread_spinunlock(self, &pthread__allqueue_lock);

	pthread_spinlock(self, &pthread__deadqueue_lock);
	PTQ_INSERT_HEAD(&pthread__deadqueue, thread, pt_allq);
	pthread_spinunlock(self, &pthread__deadqueue_lock);
}


int
pthread_getname_np(pthread_t thread, char *name, size_t len)
{
	pthread_t self;

	self = pthread__self();

	if (pthread__find(self, thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	pthread_spinlock(self, &thread->pt_join_lock);
	if (thread->pt_name == NULL)
		name[0] = '\0';
	else
		strlcpy(name, thread->pt_name, len);
	pthread_spinunlock(self, &thread->pt_join_lock);

	return 0;
}


int
pthread_setname_np(pthread_t thread, const char *name, void *arg)
{
	pthread_t self;
	char *oldname, *cp, newname[PTHREAD_MAX_NAMELEN_NP];
	int namelen;

	self = pthread__self();
	if (pthread__find(self, thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	namelen = snprintf(newname, sizeof(newname), name, arg);
	if (namelen >= PTHREAD_MAX_NAMELEN_NP)
		return EINVAL;

	cp = strdup(newname);
	if (cp == NULL)
		return ENOMEM;

	pthread_spinlock(self, &thread->pt_join_lock);

#ifdef PTHREAD_SA
	if (thread->pt_state == PT_STATE_DEAD) {
		pthread_spinunlock(self, &thread->pt_join_lock);
		free(cp);
		return EINVAL;
	}
#endif

	oldname = thread->pt_name;
	thread->pt_name = cp;

	pthread_spinunlock(self, &thread->pt_join_lock);

	if (oldname != NULL)
		free(oldname);

	return 0;
}



/*
 * XXX There should be a way for applications to use the efficent
 *  inline version, but there are opacity/namespace issues.
 */
pthread_t
pthread_self(void)
{

	return pthread__self();
}


int
pthread_cancel(pthread_t thread)
{
	pthread_t self;

	self = pthread__self();
#ifdef ERRORCHECK
	if (pthread__find(self, thread) != 0)
		return ESRCH;
#endif
#ifdef PTHREAD_SA
	if (!(thread->pt_state == PT_STATE_RUNNING ||
	    thread->pt_state == PT_STATE_RUNNABLE ||
	    thread->pt_state == PT_STATE_BLOCKED_QUEUE))
		return ESRCH;

	pthread_spinlock(self, &thread->pt_flaglock);
	thread->pt_flags |= PT_FLAG_CS_PENDING;
	if ((thread->pt_flags & PT_FLAG_CS_DISABLED) == 0) {
		thread->pt_cancel = 1;
		pthread_spinunlock(self, &thread->pt_flaglock);
		pthread_spinlock(self, &thread->pt_statelock);
		if (thread->pt_blockgen != thread->pt_unblockgen) {
			/*
			 * It's sleeping in the kernel. If we can wake
			 * it up, it will notice the cancellation when
			 * it returns. If it doesn't wake up when we
			 * make this call, then it's blocked
			 * uninterruptably in the kernel, and there's
			 * not much to be done about it.
			 */
			_lwp_wakeup(thread->pt_blockedlwp);
		} else if (thread->pt_state == PT_STATE_BLOCKED_QUEUE) {
			/*
			 * We're blocked somewhere (pthread__block()
			 * was called). Cause it to wake up; it will
			 * check for the cancellation if the routine
			 * is a cancellation point, and loop and reblock
			 * otherwise.
			 */
			pthread_spinlock(self, thread->pt_sleeplock);
			PTQ_REMOVE(thread->pt_sleepq, thread,
			    pt_sleep);
			pthread_spinunlock(self, thread->pt_sleeplock);
			pthread__sched(self, thread);
		} else {
			/*
			 * Nothing. The target thread is running and will
			 * notice at the next deferred cancellation point.
			 */
		}
		pthread_spinunlock(self, &thread->pt_statelock);
	} else
		pthread_spinunlock(self, &thread->pt_flaglock);
#else
	pthread_spinlock(self, &thread->pt_flaglock);
	thread->pt_flags |= PT_FLAG_CS_PENDING;
	if ((thread->pt_flags & PT_FLAG_CS_DISABLED) == 0) {
		thread->pt_cancel = 1;
		pthread_spinunlock(self, &thread->pt_flaglock);
		_lwp_wakeup(thread->pt_lid);
	} else
		pthread_spinunlock(self, &thread->pt_flaglock);
#endif

	return 0;
}


int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_spinlock(self, &self->pt_flaglock);
	if (oldstate != NULL) {
		if (self->pt_flags & PT_FLAG_CS_DISABLED)
			*oldstate = PTHREAD_CANCEL_DISABLE;
		else
			*oldstate = PTHREAD_CANCEL_ENABLE;
	}

	if (state == PTHREAD_CANCEL_DISABLE) {
		self->pt_flags |= PT_FLAG_CS_DISABLED;
		if (self->pt_cancel) {
			self->pt_flags |= PT_FLAG_CS_PENDING;
			self->pt_cancel = 0;
		}
	} else if (state == PTHREAD_CANCEL_ENABLE) {
		self->pt_flags &= ~PT_FLAG_CS_DISABLED;
		/*
		 * If a cancellation was requested while cancellation
		 * was disabled, note that fact for future
		 * cancellation tests.
		 */
		if (self->pt_flags & PT_FLAG_CS_PENDING) {
			self->pt_cancel = 1;
			/* This is not a deferred cancellation point. */
			if (self->pt_flags & PT_FLAG_CS_ASYNC) {
				pthread_spinunlock(self, &self->pt_flaglock);
				pthread_exit(PTHREAD_CANCELED);
			}
		}
	} else
		retval = EINVAL;

	pthread_spinunlock(self, &self->pt_flaglock);
	return retval;
}


int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_spinlock(self, &self->pt_flaglock);

	if (oldtype != NULL) {
		if (self->pt_flags & PT_FLAG_CS_ASYNC)
			*oldtype = PTHREAD_CANCEL_ASYNCHRONOUS;
		else
			*oldtype = PTHREAD_CANCEL_DEFERRED;
	}

	if (type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		self->pt_flags |= PT_FLAG_CS_ASYNC;
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_flaglock);
			pthread_exit(PTHREAD_CANCELED);
		}
	} else if (type == PTHREAD_CANCEL_DEFERRED)
		self->pt_flags &= ~PT_FLAG_CS_ASYNC;
	else
		retval = EINVAL;

	pthread_spinunlock(self, &self->pt_flaglock);
	return retval;
}


void
pthread_testcancel()
{
	pthread_t self;

	self = pthread__self();
	if (self->pt_cancel)
		pthread_exit(PTHREAD_CANCELED);
}


/*
 * POSIX requires that certain functions return an error rather than
 * invoking undefined behavior even when handed completely bogus
 * pthread_t values, e.g. stack garbage or (pthread_t)666. This
 * utility routine searches the list of threads for the pthread_t
 * value without dereferencing it.
 */
int
pthread__find(pthread_t self, pthread_t id)
{
	pthread_t target;

	pthread_spinlock(self, &pthread__allqueue_lock);
	PTQ_FOREACH(target, &pthread__allqueue, pt_allq)
	    if (target == id)
		    break;
	pthread_spinunlock(self, &pthread__allqueue_lock);

	if (target == NULL)
		return ESRCH;

	return 0;
}


void
pthread__testcancel(pthread_t self)
{

	if (self->pt_cancel)
		pthread_exit(PTHREAD_CANCELED);
}


void
pthread__cleanup_push(void (*cleanup)(void *), void *arg, void *store)
{
	pthread_t self;
	struct pt_clean_t *entry;

	self = pthread__self();
	entry = store;
	entry->ptc_cleanup = cleanup;
	entry->ptc_arg = arg;
	PTQ_INSERT_HEAD(&self->pt_cleanup_stack, entry, ptc_next);
}


void
pthread__cleanup_pop(int ex, void *store)
{
	pthread_t self;
	struct pt_clean_t *entry;

	self = pthread__self();
	entry = store;

	PTQ_REMOVE(&self->pt_cleanup_stack, entry, ptc_next);
	if (ex)
		(*entry->ptc_cleanup)(entry->ptc_arg);
}


int *
pthread__errno(void)
{
	pthread_t self;

	self = pthread__self();

	return &(self->pt_errno);
}

ssize_t	_sys_write(int, const void *, size_t);

void
pthread__assertfunc(const char *file, int line, const char *function,
		    const char *expr)
{
	char buf[1024];
	int len;

	SDPRINTF(("(af)\n"));

	/*
	 * snprintf should not acquire any locks, or we could
	 * end up deadlocked if the assert caller held locks.
	 */
	len = snprintf(buf, 1024, 
	    "assertion \"%s\" failed: file \"%s\", line %d%s%s%s\n",
	    expr, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");

	_sys_write(STDERR_FILENO, buf, (size_t)len);
	(void)kill(getpid(), SIGABRT);

	_exit(1);
}


void
pthread__errorfunc(const char *file, int line, const char *function,
		   const char *msg)
{
	char buf[1024];
	size_t len;
	
	if (pthread__diagassert == 0)
		return;

	/*
	 * snprintf should not acquire any locks, or we could
	 * end up deadlocked if the assert caller held locks.
	 */
	len = snprintf(buf, 1024, 
	    "%s: Error detected by libpthread: %s.\n"
	    "Detected by file \"%s\", line %d%s%s%s.\n"
	    "See pthread(3) for information.\n",
	    getprogname(), msg, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");

	if (pthread__diagassert & DIAGASSERT_STDERR)
		_sys_write(STDERR_FILENO, buf, len);

	if (pthread__diagassert & DIAGASSERT_SYSLOG)
		syslog(LOG_DEBUG | LOG_USER, "%s", buf);

	if (pthread__diagassert & DIAGASSERT_ABORT) {
		(void)kill(getpid(), SIGABRT);
		_exit(1);
	}
}

#ifndef PTHREAD_SA

/*
 * Thread park/unpark operations.  The kernel operations are
 * modelled after a brief description from "Multithreading in
 * the Solaris Operating Environment":
 *
 * http://www.sun.com/software/whitepapers/solaris9/multithread.pdf
 */

#define	OOPS(msg)			\
    pthread__errorfunc(__FILE__, __LINE__, __func__, msg)

int
pthread__park(pthread_t self, pthread_spin_t *lock,
	      void *obj, struct pthread_queue_t *queue,
	      const struct timespec *abstime, int tail,
	      int cancelpt)
{
	int rv;

	SDPRINTF(("(pthread__park %p) obj %p enter\n", self, obj));

	/*
	 * Enter the object's queue.
	 */
	if (queue != NULL) {
		if (tail) 
			PTQ_INSERT_TAIL(queue, self, pt_sleep);
		else
			PTQ_INSERT_HEAD(queue, self, pt_sleep);
		self->pt_sleeponq = 1;
	}
	self->pt_sleepobj = obj;

	/*
	 * Wait until we are awoken by a pending unpark operation,
	 * a signal, an unpark posted after we have gone asleep,
	 * or an expired timeout.
	 */
	rv = 0;
	do {
		pthread_spinunlock(self, lock);
		if (_lwp_park(abstime, NULL, obj) != 0) {
			switch (rv = errno) {
			case EINTR:
				/* Check for cancellation. */
				if (cancelpt && self->pt_cancel)
					break;
				/* FALLTHROUGH */
			case EALREADY:
				rv = 0;
				break;
			case ETIMEDOUT:
				break;
			default:
				OOPS("_lwp_park failed");
				SDPRINTF(("(pthread__park %p) syscall rv=%d\n",
				    self, rv));	
				break;
			}
		}
		pthread_spinlock(self, lock);
	} while (self->pt_sleepobj != NULL && rv == 0);

	/*
	 * If we have been awoken early but are still on the queue,
	 * then remove ourself.
	 */
	if (queue != NULL && self->pt_sleeponq)
		PTQ_REMOVE(queue, self, pt_sleep);
	self->pt_sleepobj = NULL;
	self->pt_sleeponq = 0;

	SDPRINTF(("(pthread__park %p) obj %p exit\n", self, obj));

	return rv;
}

void
pthread__unpark(pthread_t self, pthread_spin_t *lock, void *obj,
		pthread_t target)
{
	int rv;

	if (target != NULL) {
		SDPRINTF(("(pthread__unpark %p) obj %p target %p\n", self, obj,
		    target));

		/*
		 * Easy: the thread has already been removed from
		 * the queue, so just awaken it.
		 */
		target->pt_sleepobj = NULL;
		target->pt_sleeponq = 0;
		pthread_spinunlock(self, lock);
		rv = _lwp_unpark(target->pt_lid, obj);

		if (rv != 0 && errno != EALREADY && errno != EINTR) {
			SDPRINTF(("(pthread__unpark %p) syscall rv=%d\n",
			    self, rv));
			OOPS("_lwp_unpark failed");
		}
	} else
		pthread_spinunlock(self, lock);
}

void
pthread__unpark_all(pthread_t self, pthread_spin_t *lock, void *obj,
		    struct pthread_queue_t *queue)
{
	lwpid_t waiters[PTHREAD__UNPARK_MAX];
	int n, rv;
	pthread_t thread, next;

	if (PTQ_EMPTY(queue)) {
		pthread_spinunlock(self, lock);
		return;
	}

	/*
	 * First, clear all sleepobj pointers, since we can release the
	 * spin lock before awkening everybody, and must synchronise with
	 * pthread__park().
	 */
	PTQ_FOREACH(thread, queue, pt_sleep) {	
		thread->pt_sleepobj = NULL;
	}

	for (;;) {
		thread = PTQ_FIRST(queue);
		for (n = 0; n < pthread__unpark_max && thread != NULL;
		    thread = next) {
			/*
			 * If the sleepobj pointer is non-NULL, it
			 * means one of two things:
			 *
			 * o The thread has awoken early, spun
			 *   through application code and is
			 *   once more asleep on this object.
			 *
			 * o This is a new thread that has blocked
			 *   on the object after we have released
			 *   the interlock in this loop.
			 *
			 * In both cases we shouldn't remove the
			 * thread from the queue.
			 *
			 * XXXLWP basic fairness issues here.
			 */
			next = PTQ_NEXT(thread, pt_sleep);
			if (thread->pt_sleepobj != NULL)
			    	continue;
			thread->pt_sleeponq = 0;
			waiters[n++] = thread->pt_lid;
			PTQ_REMOVE(queue, thread, pt_sleep);
			SDPRINTF(("(pthread__unpark_all %p) obj %p "
			    "unpark %p\n", self, obj, thread));
		}

		pthread_spinunlock(self, lock);
		switch (n) {
		case 0:
			return;
		case 1:
			rv = _lwp_unpark(waiters[0], obj);
			if (rv != 0 && errno != EALREADY && errno != EINTR) {
				OOPS("_lwp_unpark failed");
				SDPRINTF(("(pthread__unpark_all %p) "
				    "syscall rv=%d\n", self, rv));
			}
			return;
		default:
			rv = _lwp_unpark_all(waiters, n, obj);
			if (rv != 0 && errno != EINTR) {
				OOPS("_lwp_unpark_all failed");
				SDPRINTF(("(pthread__unpark_all %p) "
				    "syscall rv=%d\n", self, rv));
			}
			break;
		}
		pthread_spinlock(self, lock);
	}
}

#undef	OOPS

#endif	/* !PTHREAD_SA */
