/*	$NetBSD: pthread_sa.c,v 1.1.2.11 2001/09/25 19:41:48 nathanw Exp $	*/

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
#include <sa.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <sys/queue.h>
#include <sys/time.h>

#include "pthread.h"
#include "pthread_int.h"

#define PTHREAD_SA_DEBUG

#ifdef PTHREAD_SA_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

extern struct pt_queue_t allqueue;

stack_t recyclable[2][(PT_UPCALLSTACKS/2)+1];
int	recycle_count;
int	recycle_threshold;
int	recycle_side;
pt_spin_t recycle_lock;

extern struct pt_queue_t runqueue;

#define pthread__sa_id(sap) (pthread__id((sap)->sa_context))

int pthread__find_interrupted(struct sa_t *sas[], int nsas, pthread_t *qhead,
    pthread_t self);
void pthread__resolve_locks(pthread_t self, pthread_t *interrupted);
void pthread__recycle_bulk(pthread_t self, pthread_t qhead);

static void
pthread__upcall(int type, struct sa_t *sas[], int ev, int intr, int sig, 
    u_long code, void *arg)
{
	pthread_t t, self, next, intqueue;
	int first = 1;
	int deliversig = 0;

	PTHREADD_ADD(PTHREADD_UPCALLS);

	self = pthread__self();

	SDPRINTF(("(up %p) type %d ev %d intr %d\n", self, type, ev, intr));
	switch (type) {
	case SA_UPCALL_BLOCKED:
		t = pthread__sa_id(sas[1]);
		t->pt_state = PT_STATE_BLOCKED;
		t->blocks++;
		first++; /* Don't handle this SA in the usual processing. */
		PTHREADD_ADD(PTHREADD_UP_BLOCK);
		break;
	case SA_UPCALL_NEWPROC:
		PTHREADD_ADD(PTHREADD_UP_NEW);
		break;
	case SA_UPCALL_PREEMPTED:
		PTHREADD_ADD(PTHREADD_UP_PREEMPT);
		break;
	case SA_UPCALL_UNBLOCKED:
		PTHREADD_ADD(PTHREADD_UP_UNBLOCK);
		break;
	case SA_UPCALL_SIGNAL:
		PTHREADD_ADD(PTHREADD_UP_SIGNAL);
		deliversig = 1;
		break;
	case SA_UPCALL_USER:
		/* We don't send ourselves one of these. */
	default:
		assert(0);
	}

	/*
	 * Do per-thread work, including saving the context.
	 * Briefly run any threads that were in a critical section.
	 * This includes any upcalls that have been interupted, so
	 * they can do their own version of this dance.
	 */

	intqueue = NULL;
	if ((ev + intr) >= first) {
		if (pthread__find_interrupted(sas + first, ev + intr,
		    &intqueue, self) > 0)
			pthread__resolve_locks(self, &intqueue);

		/* Note that we handle signals after handling spinlock
		 * preemption. This is because spinlocks are only used
		 * internally to the thread library and we don't want to
		 * expose the middle of them to a signal.
		 * While this means that synchronous instruction traps
		 * that occur inside critical sections in this library 
		 * (SIGFPE, SIGILL, SIGBUS, SIGSEGV) won't be handled at
		 * the precise location where they occured, that's okay,
		 * because (1) we don't use any FP and 
		 * (2) SIGILL/SIGBUS/SIGSEGV should really just core dump.
		 */
		if (deliversig) {
			if (ev)
				pthread__signal(pthread__sa_id(sas[1]),
				    sig, code);
			else
				pthread__signal(NULL, sig, code);
		}
	}

	pthread__sched_idle2(self);
	if (intqueue)
		pthread__sched_bulk(self, intqueue);
	
	/* At this point everything on our list should be scheduled
	 * (or was an upcall).
	 */

	next = pthread__next(self);
	SDPRINTF(("(up %p) switching to %p (uc: %p pc: %lx)\n", 
	    self, next, next->pt_uc, next->pt_uc->uc_mcontext.sc_pc));
	pthread__upcall_switch(self, next);
	/* NOTREACHED */
	assert(0);
}

/* Build a chain of the threads that were interrupted by the upcall. 
 * Determine if any of them were upcalls or lock-holders that
 * need to be continued early.
 */
int
pthread__find_interrupted(struct sa_t *sas[], int nsas, pthread_t *qhead,
    pthread_t self)
{
	int i, resumecount;
	pthread_t victim, next;

	resumecount = 0;
	next = self;

	for (i = 0; i < nsas; i++) {
		victim = pthread__sa_id(sas[i]);
		victim->preempts++;
		victim->pt_uc = sas[i]->sa_context;
		SDPRINTF(("(fi %p) victim %d %p", self, i, victim));
		if (victim->pt_type == PT_THREAD_UPCALL) {
			/* Case 1: Upcall */
			/* Must be resumed. */
				SDPRINTF((" upcall"));
			resumecount++;
			if (victim->pt_next) {
				/* Case 1A: Upcall in a chain */
				/* Already part of a chain. We want to
				 * splice this chain into our chain, so
				 * we have to find the root.
				 */
				SDPRINTF((" chain"));
				for ( ; victim->pt_parent != NULL; 
				      victim = victim->pt_parent) {
					SDPRINTF((" parent %p", victim->pt_parent));
					assert(victim->pt_parent != victim);

				}
			}
		} else {
			/* Case 2: Plain thread. */
			if (victim->pt_spinlocks > 0) {
				/* Case 2A: Lockholder */
				/* Must be resumed. */
				SDPRINTF((" lockholder %d",
				    victim->pt_spinlocks));
				resumecount++;
				if (victim->pt_next) {
					/* Case 2A1: Lockholder on a chain */
					/* Same deal as 1A. */
					SDPRINTF((" chain"));
					for ( ; victim->pt_parent != NULL; 
					      victim = victim->pt_parent) {
						SDPRINTF((" parent %p", victim->pt_parent));
						assert(victim->pt_parent != victim);
					}


				}
			} else {
				/* Case 2B: Non-lockholder */
					SDPRINTF((" nonlockholder"));
				if (victim->pt_next) {
					/* Case 2B1: Non-lockholder on a chain
					 * (must have just released a lock)
					 */
					SDPRINTF((" chain"));
					resumecount++;
					for ( ; victim->pt_parent != NULL; 
					      victim = victim->pt_parent) {
						SDPRINTF((" parent %p", victim->pt_parent));
						assert(victim->pt_parent != victim);
					}
				}
			}
		}

		assert (victim != self);
		victim->pt_parent = self;
		victim->pt_next = next;
		next = victim;
		SDPRINTF(("\n"));
	}

	*qhead = next;

	return resumecount;
}

void
pthread__resolve_locks(pthread_t self, pthread_t *intqueuep)
{
	pthread_t victim, prev, next, switchto, runq, recycleq, intqueue;
	pt_spin_t *lock;

	PTHREADD_ADD(PTHREADD_RESOLVELOCKS);

	recycleq = NULL;
	runq = NULL;
	intqueue = *intqueuep;

	switchto = NULL;
	victim = intqueue;

	SDPRINTF(("(rl %p) entered\n", self));
	
	while (intqueue != self) {
		/* Make a pass over the interrupted queue, cleaning out
		 * any threads that have dropped all their locks and any
		 * upcalls that have finished.
		 */
		SDPRINTF(("(rl %p) intqueue %p\n",self, intqueue));
		prev = NULL;
		for (victim = intqueue; victim != self; victim = next) {
			next = victim->pt_next;
		SDPRINTF(("(rl %p) victim %p (uc %p)", self, victim, 
		    victim->pt_uc));

			if (victim->pt_switchto) {
				PTHREADD_ADD(PTHREADD_SWITCHTO);
				switchto = victim->pt_switchto;
				switchto->pt_uc = victim->pt_switchtouc;
				victim->pt_switchto = NULL;
				victim->pt_switchtouc = NULL;
				SDPRINTF((" switchto: %p", switchto));
			}

			if (victim->pt_type == PT_THREAD_NORMAL) {
				SDPRINTF((" normal"));
				if (victim->pt_spinlocks == 0) {
					/* We can remove this guy
					 * from the interrupted queue.
					 */
					if (prev)
						prev->pt_next = next;
					else
						intqueue = next;

					/* Check whether the victim was
					 * making a locked switch.
					 */
					if (victim->pt_heldlock) {
						/* Yes. Therefore, it's on
						 * some sleep queue and
						 * all we have to do is
						 * release the lock and
						 * restore the real
						 * sleep contex.
						 */
						lock = victim->pt_heldlock;
						victim->pt_heldlock = NULL;
						__cpu_simple_unlock(lock);
						victim->pt_uc = 
						    victim->pt_sleepuc;
						victim->pt_sleepuc = NULL;
						victim->pt_next = NULL;
						victim->pt_parent = NULL;
						SDPRINTF((" heldlock: %p",lock));
					} else {
						/* No. Queue it for the 
						 * run queue.
						 */
						victim->pt_next = runq;
						runq = victim;
					}
				} else {
					SDPRINTF((" spinlocks: %d", 
					    victim->pt_spinlocks));
					/* Still holding locks.
					 * Leave it in the interrupted queue.
					 */
					prev = victim;
				}
			} else {
				SDPRINTF((" upcall"));
				/* Okay, an upcall. */
				if (victim->pt_state == PT_STATE_RECYCLABLE) {
					/* We're done with you. */
					SDPRINTF((" recyclable"));
					victim->pt_next = recycleq;
					recycleq = victim;
					if (prev)
						prev->pt_next = next;
					else
						intqueue = next;

				}
			}
			if (switchto) {
				assert(switchto->pt_spinlocks == 0);
				/* Threads can have switchto set to themselves
				 * if they hit new_preempt. Don't put them
				 * on the run queue twice.
				 */
				if (switchto != victim) {
					switchto->pt_next = runq;
					runq = switchto;
				}
				switchto = NULL;
			}
			SDPRINTF(("\n"));
		}

		if (intqueue != self) {
			/* There is a chain. Run through the elements
			 * of the chain. If one of them is preempted again,
			 * the upcall that handles it will have us on its
			 * chain, and we will continue here, having
			 * returned from the switch.
			 */
			SDPRINTF(("(rl %p) chain switching to %p\n",
			    self, intqueue));
			pthread__switch(self, intqueue);
			SDPRINTF(("(rl %p) returned from chain switch\n",
			    self));
		}

		if (self->pt_next) {
			/* We're on a chain ourselves. Let the other 
			 * threads in the chain run; our parent upcall
			 * will resume us here after a pass around its
			 * interrupted queue.
			 */
			pthread__switch(self, self->pt_next);
		}

	}

	/* Recycle upcalls. */
	pthread__recycle_bulk(self, recycleq);
	SDPRINTF(("(rl %p) exiting\n", self));
	*intqueuep = runq;
}

void
pthread__recycle_bulk(pthread_t self, pthread_t qhead)
{
	int do_recycle, my_side, ret;
	pthread_t upcall;

	while(qhead != NULL) {
		pthread_spinlock(self, &recycle_lock);
		my_side = recycle_side;
		do_recycle = 0;
		while ((qhead != NULL) && 
		    (recycle_count < recycle_threshold)) {
			upcall = qhead; 
			qhead = qhead->pt_next;
			upcall->pt_parent = NULL;
			upcall->pt_state = PT_STATE_RUNNABLE;
			upcall->pt_next = NULL;
			upcall->pt_parent = NULL;

			recyclable[my_side][recycle_count] = upcall->pt_stack;
			recycle_count++;
		}
		if (recycle_count == recycle_threshold) {
			recycle_side = 1 - recycle_side;
			recycle_count = 0;
			do_recycle = 1;
		}
		pthread_spinunlock(self, &recycle_lock);
		if (do_recycle) {
			ret = sa_stacks(recycle_threshold, 
			    recyclable[my_side]);
			if (ret != recycle_threshold) {
				printf("Error: recycle_threshold\n");
				printf("ret: %d  threshold: %d\n",
				    ret, recycle_threshold);

				assert(0);
			}
		}
	}
	
}

/* Stash away an upcall and its stack, possibly recycling it to the kernel.
 * Must be running in the context of "new".
 */
void
pthread__sa_recycle(pthread_t old, pthread_t new)
{
	int do_recycle, my_side, ret;

	do_recycle = 0;

	pthread_spinlock(new, &recycle_lock);

	my_side = recycle_side;
	recyclable[my_side][recycle_count] = old->pt_stack;
	recycle_count++;
	if (recycle_count == recycle_threshold) {
		/* Switch */
		recycle_side = 1 - recycle_side;
		recycle_count = 0;
		do_recycle = 1;
	}
	pthread_spinunlock(new, &recycle_lock);

	if (do_recycle) {
		ret = sa_stacks(recycle_threshold, recyclable[my_side]);
		if (ret != recycle_threshold)
			assert(0);
	}
}

/* Get things rolling. */
void
pthread__sa_start(void)
{
	pthread_t t;
	stack_t upcall_stacks[PT_UPCALLSTACKS];
	int ret, i;

	ret = sa_register(pthread__upcall, NULL);
	if (ret)
		err(1, "sa_register failed");

	for (i = 0; i < PT_UPCALLSTACKS; i++) {
		if (0 != (ret = pthread__stackalloc(&t)))
			err(1, "Could not allocate upcall stack!");
		upcall_stacks[i] = t->pt_stack;	
		pthread__initthread(t);
		t->pt_type = PT_THREAD_UPCALL;
		t->pt_flags = PT_FLAG_DETACHED;
		sigfillset(&t->pt_sigmask); /* XXX hmmmmmm */
		/* No locking needed, there are no threads yet. */
		PTQ_INSERT_HEAD(&allqueue, t, pt_allq);
	}

	recycle_threshold = PT_UPCALLSTACKS/2;

	ret = sa_stacks(i, upcall_stacks);
	if (ret == -1)
		err(1, "sa_stacks failed");

	sa_enable();

}
