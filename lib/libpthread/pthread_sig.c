/*	$Id: pthread_sig.c,v 1.1.2.2 2001/07/13 02:21:14 nathanw Exp $ */

/* Copyright */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "sched.h"
#include "pthread.h"
#include "pthread_int.h"

struct sigaction pt_sigacts[_NSIG];
pt_spin_t	pt_sigacts_lock;

sigset_t	pt_process_sigmask;
sigset_t	pt_process_siglist;
pt_spin_t	pt_process_siglock;

static void	pthread__signal_tramp(int sig, int code, struct 
    sigaction *act, ucontext_t *uc);



int
pthread_kill(pthread_t thread, int sig)
{

	/* XXX implement me */
	return 0;
}


/* Interpositioning is our friend. We need to intercept sigaction().
 */

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	pthread_t self;
	quad_t q;
	int rv;

	self = pthread__self();
	/* Just save the information for our internal dispatch. */
	if (act != NULL) {
		pthread_spinlock(self, &pt_sigacts_lock);
		pt_sigacts[sig] = *act;
		pthread_spinunlock(self, &pt_sigacts_lock);
	}

	q = __syscall((quad_t)SYS___sigaction14, sig, act, oact);
	if (/* LINTED constant */ sizeof (quad_t) == sizeof (register_t) ||
	    /* LINTED constant */ BYTE_ORDER == LITTLE_ENDIAN)
		rv = (int)(long)q;
	else
		rv = (int)(long)((u_quad_t)q >> 32);
	return rv;
}


int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	pthread_t self;
	int retval;

	self = pthread__self();

	retval = 0;
 	if (set != NULL) {
		pthread_spinlock(self,&self->pt_siglock);
		if (how == SIG_BLOCK) {
			__sigplusset(set, &self->pt_sigmask);
			/* Blocking of signals that are now
			 * blocked by all threads will be done
			 * lazily, at signal delivery time.
			 */
		} else if (how == SIG_UNBLOCK) {
			pthread_spinlock(self, &pt_process_siglock);
			__sigminusset(set, &self->pt_sigmask);
			/* Unblock any signals that were blocked
			 * process-wide before this.
			 */
			/* Calling sigprocmask here might be
                         * overkill. Checking if the signals were
                         * already unblocked could be more
                         * efficent. 
			 */
			__sigminusset(set, &pt_process_sigmask);
			sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
			pthread_spinunlock(self, &pt_process_siglock);
			/* Take any signals that were pending. */
			/* XXX implement me! */
		} else if (how == SIG_SETMASK) {
			pthread_spinlock(self, &pt_process_siglock);
			self->pt_sigmask = *set;
			/* Unblock any signals that were blocked
			 * process-wide before this.
			 */
			__sigandset(set, &pt_process_sigmask);
			sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
			pthread_spinunlock(self, &pt_process_siglock);
		} else 
			retval = EINVAL;
		pthread_spinunlock(self,&self->pt_siglock);

	}

	/* While other threads may read a process's sigmask,
	 * they won't write it, so we don't need to lock our reads of it.
	 */

	if (oset != NULL) {
		*oset = self->pt_sigmask;
	}
	
	return retval;

}


/* Dispatch a signal to thread t, if it is non-null, and to any
 * willing thread, if t is null.
 */

void
pthread__signal(pthread_t t, int sig, int code)
{
	pthread_t self, target, good, okay;
	extern pt_spin_t allqueue_lock;
	extern struct pt_queue_t allqueue;
	ucontext_t *uc;

	if (t)
		target = t;
	else {
		/* Pick a thread that doesn't have the signal blocked. */
		self = pthread__self();
		okay = good = NULL;
		pthread_spinlock(self, &allqueue_lock);
		PTQ_FOREACH(target, &allqueue, pt_allq) {
			if (!sigismember(&target->pt_sigmask, sig)) {
				okay = target;
				if (target->pt_state == PT_STATE_RUNNABLE) {
					good = target;
					break;
				}
			}
		}
		pthread_spinunlock(self, &allqueue_lock);
		if (good)
			target = good;
		else
			target = okay;

		if (target == NULL) {
			/* They all have it blocked. Note that in our
			 * process-wide signal mask, and stash the signal
			 * for later unblocking.  
			 */
			pthread_spinlock(self, &pt_process_siglock);
			sigaddset(&pt_process_sigmask, sig);
			sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
			sigaddset(&pt_process_siglist, sig);
			pthread_spinunlock(self, &pt_process_siglock);
			return;
		}
	}

	/* We now have a signal and a victim. */


	/* Check if the victim will accept the signal. */
	pthread_spinlock(self, &target->pt_siglock);
	if (sigismember(&target->pt_sigmask, sig)) {
		/* If this wasn't a targeted kill, then perhaps we need to find
		 * some other thread that has the signal unblocked. 
		 * Need to check the spec.
		 */
		/* XXX this loses the code! */
		sigaddset(&target->pt_siglist, sig);
		/* XXX unblock the signal! */
		
	} else {
	
		/* Ensure the victim is not running. */

		/* XXX
		 * remove from runqueue? 
		 * mark as "in signal adjustment"?
		 * lock?
		 */

		/* Victim is now not running, and its state is saved
		 * on the stack. 
		 */
		
		/* XXX this is a gross hack, but makecontext does what
		 * we want.
		 */

		/* Note that we are blatantly ignoring SIGALTSTACK. */

		uc = target->pt_uc - 1;
		_getcontext_u(uc);
		uc->uc_stack.ss_sp = uc;
		uc->uc_stack.ss_size = 0;
		makecontext(uc, pthread__signal_tramp, 4, 
		    sig, code, &pt_sigacts[sig], target->pt_uc);
		target->pt_uc = uc;
	}
	pthread_spinunlock(self, &target->pt_siglock);
	
	
}


static void pthread__signal_tramp(int sig, int code, struct sigaction *act, 
	ucontext_t *uc)
{
	sigset_t set;
	void (*handler)(int, int, struct sigcontext *);

	handler = (void (*)(int, int, struct sigcontext *)) act->sa_handler;

	sigemptyset(&set);
	sigaddset(&set, sig);

	pthread_sigmask(SIG_BLOCK, &set, NULL);
       	handler(sig, code, NULL);
       	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	_setcontext_u(uc);
       	/* NOTREACHED */
}
