/*	$NetBSD: pthread_run.c,v 1.1.2.11 2002/02/08 22:18:59 nathanw Exp $	*/

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
#include <ucontext.h>

#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_RUN_DEBUG

#ifdef PTHREAD_RUN_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

pthread_spin_t runqueue_lock;
struct pthread_queue_t runqueue;
struct pthread_queue_t idlequeue;

extern pthread_spin_t deadqueue_lock;
extern struct pthread_queue_t reidlequeue;

/* Go do another thread, without putting us on the run queue. */
void
pthread__block(pthread_t self, pthread_spin_t *queuelock)
{
	pthread_t next;
	
	next = pthread__next(self);
	SDPRINTF(("(calling locked_switch %p, %p) spinlock %d, %d\n",
 		self, next, self->pt_spinlocks, next->pt_spinlocks));
	pthread__locked_switch(self, next, queuelock);
	SDPRINTF(("(back from locked_switch %p, %p) spinlock %d, %d\n",
 		self, next, self->pt_spinlocks, next->pt_spinlocks));
  
}


/* Get the next thread to switch to. Will never return NULL. */
pthread_t
pthread__next(pthread_t self)
{
	pthread_t next;

	pthread_spinlock(self, &runqueue_lock);
	next = PTQ_FIRST(&runqueue);
	if (next) {
		assert(next->pt_type == PT_THREAD_NORMAL);
		PTQ_REMOVE(&runqueue, next, pt_runq);
	} else {
		next = PTQ_FIRST(&idlequeue);
		assert(next != 0);
		PTQ_REMOVE(&idlequeue, next, pt_runq);
		assert(next->pt_type == PT_THREAD_IDLE);
		SDPRINTF(("(next %p) returning idle thread %p\n", self, next));
	}
	pthread_spinunlock(self, &runqueue_lock);

	return next;
}


/* Put a thread back on the run queue */
void
pthread__sched(pthread_t self, pthread_t thread)
{

	SDPRINTF(("(sched %p) scheduling %p\n", self, thread));
	thread->pt_state = PT_STATE_RUNNABLE;
	assert (thread->pt_type == PT_THREAD_NORMAL);
#ifdef PTHREAD__DEBUG
	thread->rescheds++;
#endif
	pthread_spinlock(self, &runqueue_lock);
	PTQ_INSERT_TAIL(&runqueue, thread, pt_runq);
	pthread_spinunlock(self, &runqueue_lock);
}


/* Make a thread a candidate idle thread. */
void
pthread__sched_idle(pthread_t self, pthread_t thread)
{

	thread->pt_state = PT_STATE_RUNNABLE;
	thread->pt_type = PT_THREAD_IDLE;
	thread->pt_flags &= ~PT_FLAG_IDLED;
	_getcontext_u(thread->pt_uc);
	thread->pt_uc->uc_stack = thread->pt_stack;
	thread->pt_uc->uc_link = NULL;
	makecontext(thread->pt_uc, pthread__idle, 0);

	pthread_spinlock(self, &runqueue_lock);
	PTQ_INSERT_TAIL(&idlequeue, thread, pt_runq);
	pthread_spinunlock(self, &runqueue_lock);
}

void
pthread__sched_idle2(pthread_t self)
{
	pthread_t idlethread, qhead;
	
	qhead = NULL;
	pthread_spinlock(self, &deadqueue_lock);
	while (!PTQ_EMPTY(&reidlequeue)) {
		idlethread = PTQ_FIRST(&reidlequeue);
		PTQ_REMOVE(&reidlequeue, idlethread, pt_runq);
		idlethread->pt_next = qhead;
		qhead = idlethread;
	}
	pthread_spinunlock(self, &deadqueue_lock);

	if (qhead)
		pthread__sched_bulk(self, qhead);
}

/*
 * Put a series of threads, linked by their pt_next fields, onto the
 * run queue. 
 */
void
pthread__sched_bulk(pthread_t self, pthread_t qhead)
{
	pthread_t next;

	pthread_spinlock(self, &runqueue_lock);
	for ( ; qhead && (qhead != self) ; qhead = next) {
		next = qhead->pt_next;
		if (qhead->pt_type == PT_THREAD_NORMAL) {
			qhead->pt_state = PT_STATE_RUNNABLE;
			qhead->pt_next = NULL;
			qhead->pt_parent = NULL;
#ifdef PTHREAD__DEBUG
			qhead->rescheds++;
#endif
			SDPRINTF(("(bulk %p) scheduling %p\n", self, qhead));
			assert(PTQ_LAST(&runqueue, pthread_queue_t) != qhead);
			assert(PTQ_FIRST(&runqueue) != qhead);
			PTQ_INSERT_TAIL(&runqueue, qhead, pt_runq);
		} else if (qhead->pt_type == PT_THREAD_IDLE) {
			qhead->pt_state = PT_STATE_RUNNABLE;
			qhead->pt_flags &= ~PT_FLAG_IDLED;
			qhead->pt_next = NULL;
			qhead->pt_parent = NULL;
			_getcontext_u(qhead->pt_uc);
			qhead->pt_uc->uc_stack = qhead->pt_stack;
			qhead->pt_uc->uc_link = NULL;
			makecontext(qhead->pt_uc, pthread__idle, 0);
			PTQ_INSERT_TAIL(&idlequeue, qhead, pt_runq);
		}
	}
	pthread_spinunlock(self, &runqueue_lock);
}
