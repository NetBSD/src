/* Copyright */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/queue.h>

#include "pthread.h"
#include "pthread_int.h"

pt_spin_t runqueue_lock;
struct pt_queue_t runqueue;
struct pt_queue_t idlequeue;

extern pt_spin_t deadqueue_lock;
extern struct pt_queue_t reidlequeue;

/* Go do another thread, without putting us on the run queue. */
void
pthread__block(pthread_t self, pt_spin_t *queuelock)
{
	pthread_t next;

	next = pthread__next(self);

	pthread__locked_switch(self, next, queuelock);
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
	}
	pthread_spinunlock(self, &runqueue_lock);

	return next;
}


/* Put a thread back on the run queue */
void
pthread__sched(pthread_t self, pthread_t thread)
{
	thread->pt_state = PT_STATE_RUNNABLE;
	assert (thread->pt_type == PT_THREAD_NORMAL);
	thread->rescheds++;
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
			qhead->rescheds++;
			PTQ_INSERT_TAIL(&runqueue, qhead, pt_runq);
		} else if (qhead->pt_type == PT_THREAD_IDLE) {
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
