/*     $NetBSD: pthread_sa.c,v 1.37.6.5 2008/01/04 21:42:26 wrstuden Exp $    */

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_sa.c,v 1.37.6.5 2008/01/04 21:42:26 wrstuden Exp $");

#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <sa.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>

#include "pthread.h"
#include "pthread_int.h"

#ifdef PTHREAD_SA_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

#define UC(t) ((t)->pt_trapuc ? (t)->pt_trapuc : (t)->pt_uc)

#define PUC(t)  ((t)->pt_trapuc ? 'T':'U') , UC(t)

extern struct pthread_queue_t pthread__allqueue;
extern int pthread__concurrency, pthread__maxconcurrency;

#define	PTHREAD_RRTIMER_INTERVAL_DEFAULT	100
static pthread_mutex_t rrtimer_mutex = PTHREAD_MUTEX_INITIALIZER;
static timer_t pthread_rrtimer;
static int pthread_rrtimer_interval = PTHREAD_RRTIMER_INTERVAL_DEFAULT;

int pthread__maxlwps;


#define pthread__sa_id(sap) (pthread__id((sap)->sa_context))

void pthread__upcall(int type, struct sa_t *sas[], int ev, int intr, 
    void *arg);
void pthread__find_interrupted(int type, struct sa_t *sas[], int ev, int intr,
    pthread_t *qhead, pthread_t *schedqhead, pthread_t self);
void pthread__resolve_locks(pthread_t self, pthread_t *interrupted);

extern void pthread__switch_return_point(void);

typedef void (*fptr_t)(void);

void
pthread__upcall(int type, struct sa_t *sas[], int ev, int intr, void *arg)
{
	pthread_t t, self, next, intqueue, schedqueue;
	int flags, i;
	siginfo_t *si;

	PTHREADD_ADD(PTHREADD_UPCALLS);

	self = pthread__self();
	self->pt_state = PT_STATE_RUNNING;

	if (sas[0]->sa_id > pthread__maxlwps)
		pthread__maxlwps = sas[0]->sa_id;

	self->pt_vpid = sas[0]->sa_cpu;
	self->pt_lastlwp = sas[0]->sa_id;

	SDPRINTF(("(up %p) type %d LWP %d ev %d intr %d\n", self, 
	    type, sas[0]->sa_id, ev, intr));

	if (type != SA_UPCALL_BLOCKED) {
		/*
		 * Do per-thread work, including saving the context.
		 * Briefly run any threads that were in a critical section.
		 * This includes any upcalls that have been interrupted, so
		 * they can do their own version of this dance.
		 */
		pthread__find_interrupted(type, sas, ev, intr,
		    &intqueue, &schedqueue, self);
		if (intqueue != self)
			pthread__resolve_locks(self, &intqueue);
		/* We can take spinlocks now */
		if (intqueue != self)
			pthread__sched_bulk(self, intqueue);
		if (schedqueue != self)
			pthread__sched_bulk(self, schedqueue);
	}

	switch (type) {
	case SA_UPCALL_BLOCKED:
		PTHREADD_ADD(PTHREADD_UP_BLOCK);
		t = pthread__sa_id(sas[1]);
		SDPRINTF(("(up %p) blocker %d %p(%d)\n", self,
			     sas[1]->sa_id, t, t->pt_type));
		pthread__assert(t->pt_vpid == sas[1]->sa_cpu);
		t->pt_lastlwp = sas[1]->sa_id;
		t->pt_blockgen += 2;
		/* pthread__assert(t->pt_blockgen <= t->pt_unblockgen + 2); */
		if (t->pt_cancel)
			_lwp_wakeup(t->pt_lastlwp);
#ifdef PTHREAD__DEBUG
		t->blocks++;
#endif
		break;
	case SA_UPCALL_NEWPROC:
		PTHREADD_ADD(PTHREADD_UP_NEW);
		break;
	case SA_UPCALL_PREEMPTED:
		PTHREADD_ADD(PTHREADD_UP_PREEMPT);
		break;
	case SA_UPCALL_UNBLOCKED:
		PTHREADD_ADD(PTHREADD_UP_UNBLOCK);
		for (i = 0; i < ev; i++) {
			t = pthread__sa_id(sas[1 + i]);
			/*
			 * A signal may have been presented to this
			 * thread while it was in the kernel.
			 */
			pthread_spinlock(self, &t->pt_flaglock);
			flags = t->pt_flags;
			pthread_spinunlock(self, &t->pt_flaglock);
			if (t->pt_type == PT_THREAD_IDLE &&
			    (flags & PT_FLAG_IDLED))
				t->pt_unblockgen++;
			if (flags & PT_FLAG_SIGDEFERRED)
				pthread__signal_deferred(self, t);
		}
		break;
	case SA_UPCALL_SIGNAL:
		PTHREADD_ADD(PTHREADD_UP_SIGNAL);
		/*
		 * Note that we handle signals after handling
		 * spinlock preemption. This is because spinlocks are only
		 * used internally to the thread library and we don't want to
		 * expose the middle of them to a signal.  While this means
		 * that synchronous instruction traps that occur inside
		 * critical sections in this library (SIGFPE, SIGILL, SIGBUS,
		 * SIGSEGV) won't be handled at the precise location where
		 * they occurred, that's okay, because (1) we don't use any FP
		 * and (2) SIGILL/SIGBUS/SIGSEGV should really just core dump.
		 *
		 * This also means that a thread that was interrupted to take
		 * a signal will be on a run queue, and not in upcall limbo.
		 */
		si = arg;
		SDPRINTF(("(up %p) signal %d %p\n", self,
			     ev ? sas[1]->sa_id : -1, si));
		if (ev)
			pthread__signal(self, pthread__sa_id(sas[1]), si);
		else
			pthread__signal(self, NULL, si);
		break;
	case SA_UPCALL_SIGEV:
		PTHREADD_ADD(PTHREADD_UP_SIGEV);
		si = arg;
		SDPRINTF(("(up %p) sigev val %x\n", self,
		    si->si_sigval.sival_int));
		if (si->si_sigval.sival_int == PT_ALARMTIMER_MAGIC)
			pthread__alarm_process(self, arg);
		/*
		 * PT_RRTIMER_MAGIC doesn't need explicit handling;
		 * the per-thread work above will put the interrupted
		 * thread on the back of the run queue, and
		 * pthread_next() will get one from the front.
		 */
		break;
	case SA_UPCALL_USER:
		PTHREADD_ADD(PTHREADD_UP_USER);
		/*
		 * We send these so that we can deliver signals to running
		 * threads. So check for pending signals.
		 */
		t = pthread__sa_id(sas[1]);
		pthread_spinlock(self, &t->pt_flaglock);
		flags = t->pt_flags;
		pthread_spinunlock(self, &t->pt_flaglock);
		if (flags & PT_FLAG_SIGDEFERRED)
			pthread__signal_deferred(self, t);
		break;
	default:
		pthread__abort();
	}

	if (type != SA_UPCALL_BLOCKED)
		pthread__sched_idle2(self);

	/*
	 * At this point everything on our list should be scheduled
	 * (or was an upcall).
	 */
	pthread__assert(self->pt_spinlocks == 0);
	if (self->pt_next) {
		SDPRINTF(("(up %p) chain switching to %p (uc: %c %p pc: %lx)\n", 
			     self, self->pt_next, PUC(self->pt_next),
			     pthread__uc_pc(UC(self->pt_next))));
		self->pt_switchtouc = UC(self);
		self->pt_switchto = self;
		pthread__assert(self->pt_blockgen == self->pt_unblockgen);
		pthread__switch(self, self->pt_next);
		/*NOTREACHED*/
		pthread__abort();
	}
	next = pthread__next(self);
	SDPRINTF(("(up %p) switching to %p (uc: %c %p pc: %lx)\n", 
		     self, next, PUC(next), pthread__uc_pc(UC(next))));
	pthread__upcall_switch(self, next);
	/*NOTREACHED*/
	pthread__abort();
}

/*
 * Build a chain of the threads that were interrupted by the upcall. 
 * Determine if any of them were upcalls or lock-holders that
 * need to be continued early.
 */
void
pthread__find_interrupted(int type, struct sa_t *sas[], int ev, int intr,
    pthread_t *intqhead, pthread_t *schedqhead, pthread_t self)
{
	int i, resume;
	pthread_t victim, nextint, nextsched;

	nextint = nextsched = self;

	for (i = 0; i < ev + intr; i++) {
		resume = 0;
		victim = pthread__sa_id(sas[1 + i]);
#ifdef PTHREAD__DEBUG
		victim->preempts++;
#endif
		victim->pt_trapuc = sas[1 + i]->sa_context;
		victim->pt_trapuc->uc_flags &= ~_UC_SIGMASK;
		SDPRINTF(("(fi %p) victim %d %p(%d)", self, sas[1 + i]->sa_id,
			     victim, victim->pt_type));
		if (type == SA_UPCALL_UNBLOCKED && i < ev) {
			victim->pt_unblockgen++;
#ifdef PTHREAD__DEBUG
			if (victim->pt_blockgen != victim->pt_unblockgen + 1) {
				SDPRINTF((" unblock before block"));
			} else if (victim->pt_type == PT_THREAD_UPCALL ||
			    victim->pt_spinlocks > 0 || victim->pt_next) {
				SDPRINTF((" critical"));
			} else {
				SDPRINTF((" event"));
			}
#endif
		}
		if (victim->pt_type == PT_THREAD_UPCALL) {
			/* Case 1: Upcall. Must be resumed. */
			SDPRINTF((" upcall"));
			if (victim->pt_spinlocks > 0) {
				SDPRINTF((" lockholder %d",
				    victim->pt_spinlocks));
			} else {
				SDPRINTF((" nonlockholder"));
			}
			resume = 1;
			if (victim->pt_next) {
				/*
				 * Case 1A: Upcall in a chain.
				 *
				 * Already part of a chain. We want to
				 * splice this chain into our chain, so
				 * we have to find the root.
				 */
				SDPRINTF((" chain"));
				for ( ; victim->pt_parent != NULL; 
				      victim = victim->pt_parent) {
					SDPRINTF((" parent %p", victim->pt_parent));
					pthread__assert(victim->pt_parent != victim);
				}
			}
		} else {
			/* Case 2: Normal or idle thread. */
			if (victim->pt_spinlocks > 0) {
				/* Case 2A: Lockholder. Must be resumed. */
				SDPRINTF((" lockholder %d",
				    victim->pt_spinlocks));
				resume = 1;
				if (victim->pt_next) {
					/*
					 * Case 2A1: Lockholder on a chain.
					 * Same deal as 1A.
					 */
					SDPRINTF((" chain"));
					for ( ; victim->pt_parent != NULL; 
					      victim = victim->pt_parent) {
						SDPRINTF((" parent %p", victim->pt_parent));
						pthread__assert(victim->pt_parent != victim);
					}


				}
			} else {
				/* Case 2B: Non-lockholder. */
				SDPRINTF((" nonlockholder"));
				if (victim->pt_next) {
					/*
					 * Case 2B1: Non-lockholder on a chain
					 * (must have just released a lock).
					 */
					SDPRINTF((" chain"));
					resume = 1;
					for ( ; victim->pt_parent != NULL; 
					      victim = victim->pt_parent) {
						SDPRINTF((" parent %p", victim->pt_parent));
						pthread__assert(victim->pt_parent != victim);
					}
				} else if (victim->pt_flags & PT_FLAG_IDLED) {
					/*
					 * Idle threads that have already 
					 * idled must be skipped so 
					 * that we don't (a) idle-queue them
					 * twice and (b) get the pt_next
					 * queue of threads to put on the run 
					 * queue mangled by 
					 * pthread__sched_idle2()
					 */
					SDPRINTF((" idled\n"));
					continue;
			        }
					
			}
		}
		pthread__assert(victim != self);
		if (resume) {
			pthread__assert(victim->pt_parent == NULL);
			victim->pt_parent = self;
			pthread__assert(victim->pt_next == NULL);
			victim->pt_next = nextint;
			nextint = victim;
#ifdef PTHREAD__DEBUG
			for (; victim; victim = victim->pt_next)
				pthread__assert(victim->pt_next != nextint);
#endif
		} else {
			pthread__assert(victim->pt_parent == NULL);
			pthread__assert(victim->pt_next == NULL);
			victim->pt_next = nextsched;
			nextsched = victim;
#ifdef PTHREAD__DEBUG
			for (; victim; victim = victim->pt_next)
				pthread__assert(victim->pt_next != nextsched);
#endif
		}
		SDPRINTF(("\n"));
	}

	*intqhead = nextint;
	*schedqhead = nextsched;
}

void
pthread__resolve_locks(pthread_t self, pthread_t *intqueuep)
{
	pthread_t victim, prev, next, switchto, runq, intqueue;
	pthread_t tmp;
	pthread_spin_t *lock;

	PTHREADD_ADD(PTHREADD_RESOLVELOCKS);

	runq = self;
	intqueue = *intqueuep;
	switchto = NULL;
	victim = intqueue;

	SDPRINTF(("(rl %p) entered\n", self));

	while (intqueue != self) {
		/*
		 * Make a pass over the interrupted queue, cleaning out
		 * any threads that have dropped all their locks and any
		 * upcalls that have finished.
		 */
		SDPRINTF(("(rl %p) intqueue %p\n", self, intqueue));
		prev = NULL;
		for (victim = intqueue; victim != self; victim = next) {
			next = victim->pt_next;
			SDPRINTF(("(rl %p) victim %p (uc %c %p)", self,
			    victim, PUC(victim)));

			if (victim->pt_type == PT_THREAD_NORMAL) {
				fptr_t psrp, pc;

				SDPRINTF((" normal"));
				psrp = pthread__switch_return_point;
				pc = (fptr_t)((intptr_t)
					pthread__uc_pc(victim->pt_uc));
				if ((victim->pt_spinlocks == 0) &&
				    ((victim->pt_switchto != NULL) ||
					(pc == psrp))) {

					/*
					 * We can remove this thread
					 * from the interrupted queue.
					 */
					if (prev)
						prev->pt_next = next;
					else
						intqueue = next;
					/*
					 * Clear trap context, which is
					 * no longer useful.
					 */
					victim->pt_trapuc = NULL;
					/*
					 * Check whether the victim was
					 * making a locked switch.
					 */
					if (victim->pt_heldlock) {
						/*
						 * Yes. Therefore, it's on
						 * some sleep queue and
						 * all we have to do is
						 * release the lock.
						 */
						lock = victim->pt_heldlock;
						victim->pt_heldlock = NULL;
						pthread__simple_unlock(lock);
						victim->pt_next = NULL;
						victim->pt_parent = NULL;
						SDPRINTF((" heldlock: %p",lock));
					} else {
						/* 
						 * No. Queue it for the 
						 * run queue.
						 */
						victim->pt_next = runq;
						runq = victim;
					}
				} else {
					SDPRINTF((" spinlocks: %d", 
					    victim->pt_spinlocks));
					/*
					 * Still holding locks.
					 * Leave it in the interrupted queue.
					 */
					prev = victim;
				}
			} else if (victim->pt_type == PT_THREAD_UPCALL) {
				SDPRINTF((" upcall"));
				/* Okay, an upcall. */
				if (victim->pt_switchto) {
					/* We're done with you. */
					SDPRINTF((" recyclable"));
					pthread__assert(victim != prev);
					pthread__assert(!prev || prev->pt_next);
					/*
					 * Clear trap context, which is
					 * no longer useful.
					 */
					victim->pt_trapuc = NULL;
					if (prev)
						prev->pt_next = next;
					else
						intqueue = next;
					if (victim->pt_switchto == victim) {
						victim->pt_switchto = NULL;
						victim->pt_switchtouc = NULL;
						SDPRINTF((" switchto self"));
					}
					pthread__sa_recycle(victim, self);
				} else {
					/*
					 * Not finished yet.
					 * Leave it in the interrupted queue.
					 */
					prev = victim;
				}
			} else {
				SDPRINTF((" idle"));
				/*
				 * Idle threads should be given an opportunity
				 * to put themselves on the reidle queue. 
				 * We know that they're done when they have no
				 * locks and PT_FLAG_IDLED is set.
				 */
				if (victim->pt_spinlocks != 0) {
					/* Still holding locks. */
					SDPRINTF((" spinlocks: %d", 
					    victim->pt_spinlocks));
					prev = victim;
				} else if (!(victim->pt_flags & PT_FLAG_IDLED)) {
					/*
					 * Hasn't yet put itself on the
					 * reidle queue. 
					 */
					SDPRINTF((" not done"));
					prev = victim;
				} else {
					/* Done! */
					if (prev)
						prev->pt_next = next;
					else
						intqueue = next;
					/* Permit moving off the reidlequeue */
					victim->pt_next = NULL;
				}
			}

			if (victim->pt_switchto) {
				PTHREADD_ADD(PTHREADD_SWITCHTO);
				switchto = victim->pt_switchto;
				switchto->pt_uc = victim->pt_switchtouc;
				switchto->pt_trapuc = NULL;
				victim->pt_switchto = NULL;
				victim->pt_switchtouc = NULL;
				SDPRINTF((" switchto: %p (uc %p pc %lx)",
					     switchto, switchto->pt_uc,
					     pthread__uc_pc(switchto->pt_uc)));

				/*
				 * Threads can have switchto set to themselves
				 * if they hit new_preempt. Don't put them
				 * on the run queue twice.
				 */
				if (switchto != victim) {
					if ((switchto->pt_next) ||
					    (switchto->pt_spinlocks != 0) ||
					    (switchto->pt_type == PT_THREAD_UPCALL)) {
						/*
						 * The thread being switched
						 * to was preempted and
						 * continued. Find the
						 * preempter and put it on 
						 * our continuation chain.
						 */
						SDPRINTF((" switchto chained"));
						for ( tmp = switchto;
						      tmp->pt_parent != NULL; 
						      tmp = tmp->pt_parent)
							SDPRINTF((" parent: %p", tmp->pt_parent));
						pthread__assert(tmp->pt_parent == NULL);
						tmp->pt_parent = self;
						pthread__assert(tmp->pt_next == NULL);
						tmp->pt_next = intqueue;
						intqueue = tmp;
						if (switchto->pt_type == PT_THREAD_NORMAL &&
						    switchto->pt_spinlocks == 0) {
							/*
							 * We set switchto to 
							 * ourselves so that we
							 * get off the intqueue
							 */
							switchto->pt_switchto = switchto;
							switchto->pt_switchtouc = switchto->pt_uc;
						}
					} else if (switchto->pt_type ==
					    PT_THREAD_IDLE &&
					    switchto->pt_flags & PT_FLAG_IDLED) {
						SDPRINTF((" idle done"));
					} else {
						switchto->pt_next = runq;
						runq = switchto;
					}
				}
				switchto = NULL;
			}
			SDPRINTF(("\n"));
		}

		if (intqueue != self) {
			/*
			 * There is a chain. Run through the elements
			 * of the chain. If one of them is preempted again,
			 * the upcall that handles it will have us on its
			 * chain, and we will continue here, having
			 * returned from the switch.
			 */
			SDPRINTF(("(rl %p) starting chain %p (uc %c %p pc %lx sp %lx)\n",
				     self, intqueue, PUC(intqueue), 
				     pthread__uc_pc(UC(intqueue)), 
				     pthread__uc_sp(UC(intqueue))));
			pthread__assert(self->pt_blockgen == self->pt_unblockgen);
			pthread__switch(self, intqueue);
			SDPRINTF(("(rl %p) returned from chain\n",
			    self));
		}

		if (self->pt_next) {
			/*
			 * We're on a chain ourselves. Let the other 
			 * threads in the chain run; our parent upcall
			 * will resume us here after a pass around its
			 * interrupted queue.
			 */
			SDPRINTF(("(rl %p) upcall chain switch to %p (uc %c %p pc %lx sp %lx)\n",
				     self, self->pt_next, 
				     PUC(self->pt_next),
				     pthread__uc_pc(UC(self->pt_next)), 
				     pthread__uc_sp(UC(self->pt_next))));
			pthread__assert(self->pt_blockgen == self->pt_unblockgen);
			pthread__switch(self, self->pt_next);
		}

	}

	SDPRINTF(("(rl %p) exiting\n", self));
	*intqueuep = runq;
}

/*
 * Stash away an upcall and its stack, possibly recycling it to the kernel.
 * Must be running in the context of "new".
 */
void
pthread__sa_recycle(pthread_t old, pthread_t new)
{

	old->pt_next = NULL;
	old->pt_parent = NULL;
	old->pt_state = PT_STATE_RUNNABLE;

#ifdef PTHREAD__DEBUG
	if (pthread__debuglog_newline())
		SDPRINTF(("(recycle %p) recycling %p\n", new, old));
	else
		SDPRINTF((" (recycling %p)", old));
#endif
	old->pt_stackinfo.sasi_stackgen++;
}

/*
 * Set the round-robin timeslice timer.
 */
static int
pthread__setrrtimer(int msec, int startit)
{
	static int rrtimer_created;
	struct itimerspec it;

	/*
	 * This check is safe -- we will either be called before there
	 * are any threads, or with the rrtimer_mutex held.
	 */
	if (rrtimer_created == 0) {
		struct sigevent ev;

		ev.sigev_notify = SIGEV_SA;
		ev.sigev_signo = 0;
		ev.sigev_value.sival_int = (int) PT_RRTIMER_MAGIC;
		if (timer_create(CLOCK_VIRTUAL, &ev, &pthread_rrtimer) == -1)
			return (errno);

		rrtimer_created = 1;
	}

	if (startit) {
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = (long)msec * 1000000;
		it.it_value = it.it_interval;
		if (timer_settime(pthread_rrtimer, TIMER_RELTIME, &it, NULL) ==
		    -1)
			return (errno);
	}

	pthread_rrtimer_interval = msec;

	return (0);
}

/* Get things rolling. */
void
pthread__sa_start(void)
{
	pthread_t self, t;
	stack_t upcall_stacks[PT_UPCALLSTACKS];
	int ret, i, errnosave, flags, rr;
	char *value;

	flags = 0;
	value = getenv("PTHREAD_PREEMPT");
	if (value && strcmp(value, "yes") == 0)
		flags |= SA_FLAG_PREEMPT;

	/*
	 * It's possible the user's program has set the round-robin
	 * interval before starting any threads.
	 *
	 * Allow the environment variable to override the default.
	 *
	 * XXX Should we just nuke the environment variable?
	 */
	rr = pthread_rrtimer_interval;
	value = getenv("PTHREAD_RRTIME");
	if (value)
		rr = atoi(value);

	flags |= SA_FLAG_STACKINFO;
	ret = sa_register(pthread__upcall, NULL, flags,
	    pthread__stackinfo_offset());
	if (ret) {
		if (errno == ENOSYS)
			errx(1,
			    "libpthread: SA system calls are not available.\n"
				);
		err(1, "libpthread: sa_register failed\n");
	}

	self = pthread__self();
	for (i = 0; i < PT_UPCALLSTACKS; i++) {
		if (0 != (ret = pthread__stackalloc(&t)))
			abort();
		upcall_stacks[i] = t->pt_stack;	
		pthread__initthread(self, t);
		t->pt_type = PT_THREAD_UPCALL;
		t->pt_flags = PT_FLAG_DETACHED;
		sigfillset(&t->pt_sigmask); /* XXX hmmmmmm */
		/* No locking needed, there are no threads yet. */
		PTQ_INSERT_HEAD(&pthread__allqueue, t, pt_allq);
	}

	ret = sa_stacks(i, upcall_stacks);
	if (ret == -1)
		abort();

	/* XXX 
	 * Calling sa_enable() can mess with errno in bizzare ways,
	 * because the kernel doesn't really return from it as a
	 * normal system call. The kernel will launch an upcall
	 * handler which will jump back to the inside of sa_enable()
	 * and permit us to continue here. However, since the kernel
	 * doesn't get a chance to set up the return-state properly,
	 * the syscall stub may interpret the unmodified register
	 * state as an error return and stuff an inappropriate value
	 * into errno.
	 *
	 * Therefore, we need to keep errno from being changed by this
	 * slightly weird control flow.
	 */
	errnosave = errno;
	sa_enable();
	errno = errnosave;

	/* Start the round-robin timer. */
	if (rr != 0 && pthread__setrrtimer(rr, 1) != 0)
		abort();

	pthread__concurrency = 1;
	if (pthread__maxconcurrency > pthread__concurrency) {
		pthread__concurrency += sa_setconcurrency(pthread__maxconcurrency);
	}
		
}

/*
 * Interface routines to get/set the round-robin timer interval.
 *
 * XXX Sanity check the behavior for MP systems.
 */

int
pthread_getrrtimer_np(void)
{

	return (pthread_rrtimer_interval);
}

int
pthread_setrrtimer_np(int msec)
{
	extern int pthread__started;
	int ret = 0;

	if (msec < 0)
		return (EINVAL);

	pthread_mutex_lock(&rrtimer_mutex);

	ret = pthread__setrrtimer(msec, pthread__started);

	pthread_mutex_unlock(&rrtimer_mutex);

	return (ret);
}

void
pthread__setconcurrency(int concurrency)
{
	pthread_t self;
	int ret;

	self = pthread__self();
	SDPRINTF(("(setconcurrency %p) requested delta %d, current %d\n",
		     self, concurrency, pthread__concurrency));

	concurrency += pthread__concurrency;
	if (concurrency > pthread__maxconcurrency)
		concurrency = pthread__maxconcurrency;

	if (concurrency > pthread__concurrency) {
		ret = sa_setconcurrency(concurrency);
		/* pthread__concurrency += ret; */

		SDPRINTF(("(setconcurrency %p) requested %d, now %d, ret %d\n",
			     self, concurrency, pthread__concurrency, ret));
	}
	SDPRINTF(("(set %p concurrency) now %d\n",
		     self, pthread__concurrency));
}
