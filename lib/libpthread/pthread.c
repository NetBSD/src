/*	$NetBSD: pthread.c,v 1.1.2.15 2002/02/06 19:20:19 nathanw Exp $	*/

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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "sched.h"
#include "pthread.h"
#include "pthread_int.h"


static void	pthread__create_tramp(void *(*start)(void *), void *arg);

static pthread_attr_t pthread_default_attr;

pthread_spin_t allqueue_lock;
struct pthread_queue_t allqueue;
static int nthreads;

pthread_spin_t deadqueue_lock;
struct pthread_queue_t deadqueue;
struct pthread_queue_t reidlequeue;


extern struct pthread_queue_t runqueue;
extern struct pthread_queue_t idlequeue;
extern pthread_spin_t runqueue_lock;

static int started;

/* This needs to be started by the library loading code, before main() 
 * gets to run, for various things that use the state of the initial thread
 * to work properly (thread-specific data is an application-visible example; 
 * spinlock counts for mutexes is an internal example).
 */
void pthread_init(void)
{
	pthread_t first;
	int retval;
	extern int __isthreaded;
	extern timer_t pthread_alarmtimer;
	struct sigevent ev;

#ifdef PTHREAD__DEBUG
	pthread__debug_init();
#endif
	/* Basic data structure setup */
	pthread_attr_init(&pthread_default_attr);
	PTQ_INIT(&allqueue);
	PTQ_INIT(&deadqueue);
	PTQ_INIT(&reidlequeue);
	PTQ_INIT(&runqueue);
	PTQ_INIT(&idlequeue);
	
	ev.sigev_notify = SIGEV_SA;
	ev.sigev_signo = 0;
	ev.sigev_value.sival_int = 0;
	retval = timer_create(CLOCK_REALTIME, &ev, &pthread_alarmtimer);
	if (retval)
		err(1, "timer_create");
	/* Create the thread structure corresponding to main() */
	pthread__initmain(&first);
	pthread__initthread(first);
	sigprocmask(0, NULL, &first->pt_sigmask);
	PTQ_INSERT_HEAD(&allqueue, first, pt_allq);

	/* Tell libc that we're here and it should role-play accordingly. */
	__isthreaded = 1;

}

static void
pthread__start(void)
{
	pthread_t self, idle;
	int i, ret;

	self = pthread__self(); /* should be the "main()" thread */

	for (i = 0; i < NIDLETHREADS; i++) {
		/* Create idle threads */
		ret = pthread__stackalloc(&idle);
		if (ret != 0)
			err(1, "Couldn't allocate stack for idle thread!");
		pthread__initthread(idle);
		PTQ_INSERT_HEAD(&allqueue, idle, pt_allq);
		pthread__sched_idle(self, idle);
	}

	nthreads = 1;
	/* Start up the SA subsystem */
	pthread__sa_start();
}

/* General-purpose thread data structure sanitization. */
void
pthread__initthread(pthread_t t)
{
	t->pt_magic = PT_MAGIC;
	t->pt_type = PT_THREAD_NORMAL;
	t->pt_state = PT_STATE_RUNNABLE;
	pthread_lockinit(&t->pt_statelock);
	t->pt_spinlocks = 0;
	t->pt_next = NULL;
	t->pt_exitval = NULL;
	t->pt_flags = 0;
	t->pt_cancel = 0;
	t->pt_errno = 0;
	t->pt_parent = NULL;
	t->pt_heldlock = NULL;
	t->pt_switchto = NULL;
	t->pt_sleepuc = NULL;
	sigemptyset(&t->pt_siglist);
	sigemptyset(&t->pt_sigmask);
	pthread_lockinit(&t->pt_siglock);
	PTQ_INIT(&t->pt_joiners);
	pthread_lockinit(&t->pt_join_lock);
	PTQ_INIT(&t->pt_cleanup_stack);
	memset(&t->pt_specific, 0, sizeof(int) * PTHREAD_KEYS_MAX);
#ifdef PTHREAD__DEBUG
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
	int ret;

	PTHREADD_ADD(PTHREADD_CREATE);
	assert(thread != NULL);

	/* It's okay to check this without a lock because there can
         * only be one thread before it becomes true.
	 */
	if (started == 0) {
		started = 1;
		pthread__start();
	}

	if (attr == NULL)
		nattr = pthread_default_attr;
	else if (((attr != NULL) && (attr->pta_magic == PT_ATTR_MAGIC)))
		nattr = *attr;
	else
		return EINVAL;
		

	self = pthread__self();

	/* 1. Set up a stack and allocate space for a pthread_st. */
	ret = pthread__stackalloc(&newthread);
	if (ret != 0)
		return ret;
		
	/* 2. Set up state. */
	pthread__initthread(newthread);
	newthread->pt_flags = nattr.pta_flags;
	newthread->pt_sigmask = self->pt_sigmask;
	
	/* 3. Set up context. */
	/* The pt_uc pointer points to a location safely below the
	 * stack start; this is arranged by pthread__stackalloc().
	 */
	getcontext(newthread->pt_uc);
	newthread->pt_uc->uc_stack = newthread->pt_stack;
	newthread->pt_uc->uc_link = NULL;
	makecontext(newthread->pt_uc, pthread__create_tramp, 2,
	    startfunc, arg);

	/* 4. Add to list of all threads. */
	pthread_spinlock(self, &allqueue_lock);
	PTQ_INSERT_HEAD(&allqueue, newthread, pt_allq);
	nthreads++;
	pthread_spinunlock(self, &allqueue_lock);
	
	/* 5. Put on run queue. */
	pthread__sched(self, newthread);
	
	*thread = newthread;

	return 0;
}

static void
pthread__create_tramp(void *(*start)(void *), void *arg)
{
	void *retval;

	retval = start(arg);

	pthread_exit(retval);

	/* NOTREACHED */
}


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

	/* The drill here is that we want to yield the processor, 
	 * but for the thread itself to be recovered, we need to be on
	 * a list somewhere for the thread system to know about us. 
	 */
	pthread_spinlock(self, &deadqueue_lock);
	PTQ_INSERT_TAIL(&reidlequeue, self, pt_runq);
	self->pt_flags |= PT_FLAG_IDLED;
	pthread_spinunlock(self, &deadqueue_lock);

	/*
	 * If we get to run this, then no preemption has happened
	 * (because the upcall handler will not contiune an idle thread with
	 * PT_FLAG_IDLED set), and so we can yield the processor safely.
	 */
         sa_yield();
}


void
pthread_exit(void *retval)
{
	pthread_t self, joiner;
	struct pt_clean_t *cleanup;
	int nt;

	self = pthread__self();

	/* Disable cancellability. */
	self->pt_flags |= PT_FLAG_CS_DISABLED;

	/* Call any cancellation cleanup handlers */
	while (!PTQ_EMPTY(&self->pt_cleanup_stack)) {
		cleanup = PTQ_FIRST(&self->pt_cleanup_stack);
		PTQ_REMOVE(&self->pt_cleanup_stack, cleanup, ptc_next);
		(*cleanup->ptc_cleanup)(cleanup->ptc_arg);
	}

	/* Perform cleanup of thread-specific data */
	pthread__destroy_tsd(self);
	
	self->pt_exitval = retval;

	pthread_spinlock(self, &self->pt_join_lock);
	if (self->pt_flags & PT_FLAG_DETACHED) {
		pthread_spinunlock(self, &self->pt_join_lock);

		pthread_spinlock(self, &allqueue_lock);
		PTQ_REMOVE(&allqueue, self, pt_allq);
		nthreads--;
		nt = nthreads;
		pthread_spinunlock(self, &allqueue_lock); 

		self->pt_state = PT_STATE_DEAD;
		if (nt == 0) {
			/* Whoah, we're the last one. Time to go. */
			exit(0);
		}

		/* Yeah, yeah, doing work while we're dead is tacky. */
		pthread_spinlock(self, &deadqueue_lock);
		PTQ_INSERT_HEAD(&deadqueue, self, pt_allq);
		pthread__block(self, &deadqueue_lock);
	} else {
		pthread_spinlock(self, &allqueue_lock);
		nthreads--;
		nt = nthreads;
		pthread_spinunlock(self, &allqueue_lock); 
		self->pt_state = PT_STATE_ZOMBIE;
		if (nt == 0) {
			/* Whoah, we're the last one. Time to go. */
			exit(0);
		}
		/* Wake up all the potential joiners. Only one can win.
		 * (Can you say "Thundering Herd"? I knew you could.)
		 */
		PTQ_FOREACH(joiner, &self->pt_joiners, pt_sleep) 
		    pthread__sched(self, joiner);
		pthread__block(self, &self->pt_join_lock);
	}


	/* NOTREACHED */
	assert(0);
	exit(1);
}


int
pthread_join(pthread_t thread, void **valptr)
{
	pthread_t self;

	if ((thread == NULL) || (thread->pt_magic != PT_MAGIC))
		return EINVAL;
	
	self = pthread__self();

	if (thread == self)
		return EDEADLK;
		
	pthread_spinlock(self, &thread->pt_join_lock);

	if (thread->pt_flags & PT_FLAG_DETACHED) {
		pthread_spinunlock(self, &thread->pt_join_lock);
		return EINVAL;
	}

	if ((thread->pt_state != PT_STATE_ZOMBIE) &&
	    (thread->pt_state != PT_STATE_DEAD)) {
		/*
		 * "I'm not dead yet!"
		 * "You will be soon enough."
		 */
		pthread_spinlock(self, &self->pt_statelock);
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_statelock);
			pthread_spinunlock(self, &thread->pt_join_lock);
			pthread_exit(PTHREAD_CANCELED);
		}
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		pthread_spinunlock(self, &self->pt_statelock);

		PTQ_INSERT_TAIL(&thread->pt_joiners, self, pt_sleep);
		pthread__block(self, &thread->pt_join_lock);
		pthread_spinlock(self, &thread->pt_join_lock);
	}
	
	if ((thread->pt_state == PT_STATE_DEAD) ||
	    (thread->pt_flags & PT_FLAG_DETACHED)) {
		/* Someone beat us to the join, or called pthread_detach(). */
		pthread_spinunlock(self, &thread->pt_join_lock);
		return ESRCH;
	}

	/* All ours. */
	thread->pt_state = PT_STATE_DEAD;
	pthread_spinunlock(self, &thread->pt_join_lock);

	if (valptr != NULL)
		*valptr = thread->pt_exitval;

	/* Cleanup time. Move the dead thread from allqueue to the deadqueue */
	pthread_spinlock(self, &allqueue_lock);
	PTQ_REMOVE(&allqueue, thread, pt_allq);
	pthread_spinunlock(self, &allqueue_lock);

	pthread_spinlock(self, &deadqueue_lock);
	PTQ_INSERT_HEAD(&deadqueue, thread, pt_allq);
	pthread_spinunlock(self, &deadqueue_lock);

	return 0;
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
	pthread_t self, joiner;

	if ((thread == NULL) || (thread->pt_magic != PT_MAGIC))
		return EINVAL;

	self = pthread__self();
	pthread_spinlock(self, &thread->pt_join_lock);
	
	if (thread->pt_flags & PT_FLAG_DETACHED) {
		pthread_spinunlock(self, &thread->pt_join_lock);
		return EINVAL;
	}

	thread->pt_flags |= PT_FLAG_DETACHED;

	/* Any joiners have to be punted now. */
	PTQ_FOREACH(joiner, &thread->pt_joiners, pt_sleep) 
	    pthread__sched(self, joiner);

	pthread_spinunlock(self, &thread->pt_join_lock);

	return 0;
}

int
sched_yield(void)
{
	/* XXX implement me */
	return 0;
}



int
pthread_attr_init(pthread_attr_t *attr)
{

	attr->pta_magic = PT_ATTR_MAGIC;
	attr->pta_flags = 0;

	return 0;
}


int
pthread_attr_destroy(pthread_attr_t *attr)
{

	return 0;
}


int
pthread_attr_getdetachstate(pthread_attr_t *attr, int *detachstate)
{

	if ((attr == NULL) || (attr->pta_magic != PT_ATTR_MAGIC))
		return EINVAL;

	*detachstate = (attr->pta_flags & PT_FLAG_DETACHED);

	return 0;
}


int
pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	if ((attr == NULL) || (attr->pta_magic != PT_ATTR_MAGIC))
		return EINVAL;
	
	switch (detachstate) {
	case PTHREAD_CREATE_JOINABLE:
		attr->pta_flags &= ~PT_FLAG_DETACHED;
		break;
	case PTHREAD_CREATE_DETACHED:
		attr->pta_flags |= PT_FLAG_DETACHED;
		break;
	default:
		return EINVAL;
	}

	return 0;
}


int
pthread_attr_setschedparam(pthread_attr_t *attr, 
    const struct sched_param *param)
{

	if ((attr == NULL) || (attr->pta_magic != PT_ATTR_MAGIC))
		return EINVAL;
	
	if (param == NULL)
		return EINVAL;

	if (param->sched_priority != 0)
		return EINVAL;

	return 0;
}


int
pthread_attr_getschedparam(pthread_attr_t *attr, struct sched_param *param)
{

	if ((attr == NULL) || (attr->pta_magic != PT_ATTR_MAGIC))
		return EINVAL;
	
	if (param == NULL)
		return EINVAL;
	
	param->sched_priority = 0;

	return 0;
}

/* XXX There should be a way for applications to use the efficent
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
	int flags;

	if (!(thread->pt_state == PT_STATE_RUNNABLE || 
	    thread->pt_state == PT_STATE_BLOCKED_QUEUE ||
	    thread->pt_state == PT_STATE_BLOCKED_SYS))
		return ESRCH;

	self = pthread__self();
	flags = thread->pt_flags;

	flags |= PT_FLAG_CS_PENDING;
	if ((flags & PT_FLAG_CS_DISABLED) == 0) {
		thread->pt_cancel = 1;
		pthread_spinlock(self, &thread->pt_statelock);
		if (thread->pt_state == PT_STATE_BLOCKED_SYS) {
			/* It's sleeping in the kernel. If we can
			 * wake it up, it will notice the cancellation
			 * when it returns. If we can't wake it up...
			 * there's not much to be done about it.
			 */
			_lwp_wakeup(thread->pt_blockedlwp);
		} else if (thread->pt_state == PT_STATE_BLOCKED_QUEUE) {
			/* We're blocked somewhere (pthread__block()
			 * was called. Cause it to wakeup and the
			 * caller will check for the cancellation.
			 */
			pthread_spinlock(self, thread->pt_sleeplock);
			PTQ_REMOVE(thread->pt_sleepq, thread, 
			    pt_sleep);
			pthread_spinunlock(self, thread->pt_sleeplock);
			pthread__sched(self, thread);
		} else {
			/* Nothing. The target thread is running and will
			 * notice at the next deferred cancellation point.
			 */
		}
		pthread_spinunlock(self, &thread->pt_statelock);
	}

	thread->pt_flags = flags;

	return 0;
}



int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self;
	int flags;
	
	self = pthread__self();
	flags = self->pt_flags;

	if (oldstate != NULL) {
		if (flags & PT_FLAG_CS_DISABLED)
			*oldstate = PTHREAD_CANCEL_DISABLE;
		else
			*oldstate = PTHREAD_CANCEL_ENABLE;
	}

	if (state == PTHREAD_CANCEL_DISABLE)
		flags |= PT_FLAG_CS_DISABLED;
	else if (state == PTHREAD_CANCEL_ENABLE) {
		flags &= ~PT_FLAG_CS_DISABLED;
		/*
		 * If a cancellation was requested while cancellation
		 * was disabled, note that fact for future
		 * cancellation tests.
		 */
		if (flags & PT_FLAG_CS_PENDING) {
			self->pt_cancel = 1;
			/* This is not a deferred cancellation point. */
			if (flags & PT_FLAG_CS_ASYNC)
				pthread_exit(PTHREAD_CANCELED);
		}
	} else
		return EINVAL;
	
	self->pt_flags = flags;

	return 0;
}


int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self;
	int flags;
	
	self = pthread__self();
	flags = self->pt_flags;

	if (oldtype != NULL) {
		if (flags & PT_FLAG_CS_ASYNC)
			*oldtype = PTHREAD_CANCEL_ASYNCHRONOUS;
		else
			*oldtype = PTHREAD_CANCEL_DEFERRED;
	}

	if (type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		flags |= PT_FLAG_CS_ASYNC;
		if (self->pt_cancel)
			pthread_exit(PTHREAD_CANCELED);
	} else if (type == PTHREAD_CANCEL_DEFERRED)
		flags &= ~PT_FLAG_CS_ASYNC;
	else
		return EINVAL;

	self->pt_flags = flags;

	return 0;
}


void
pthread_testcancel()
{
	pthread_t self;
	
	self = pthread__self();
	if (self->pt_cancel)
		pthread_exit(PTHREAD_CANCELED);
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
