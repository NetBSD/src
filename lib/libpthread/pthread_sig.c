/*	$NetBSD: pthread_sig.c,v 1.39.2.3.2.1 2006/06/13 22:31:30 tron Exp $	*/

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
__RCSID("$NetBSD: pthread_sig.c,v 1.39.2.3.2.1 2006/06/13 22:31:30 tron Exp $");

/* We're interposing a specific version of the signal interface. */
#define	__LIBC12_SOURCE__

#define	__PTHREAD_SIGNAL_PRIVATE

#define __EXPOSE_STACK 1
#include <sys/param.h>
#include <errno.h>
#include <lwp.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>		/* for memcpy() */
#include <ucontext.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"

#ifdef PTHREAD_SIG_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

extern int pthread__started;

extern pthread_spin_t pthread__runqueue_lock;
extern struct pthread_queue_t pthread__runqueue;

extern pthread_spin_t pthread__allqueue_lock;
extern struct pthread_queue_t pthread__allqueue;

extern pthread_spin_t pthread__suspqueue_lock;
extern struct pthread_queue_t pthread__suspqueue;

static pthread_spin_t	pt_sigacts_lock = __SIMPLELOCK_UNLOCKED;
static struct sigaction pt_sigacts[_NSIG];

static pthread_spin_t	pt_process_siglock = __SIMPLELOCK_UNLOCKED;
static sigset_t	pt_process_sigmask;
static sigset_t	pt_process_siglist;

/* Queue of threads that are waiting in sigsuspend(). */
static struct pthread_queue_t pt_sigsuspended;
static pthread_spin_t pt_sigsuspended_lock = __SIMPLELOCK_UNLOCKED;

/*
 * Nothing actually signals or waits on this lock, but the sleepobj
 * needs to point to something.
 */
static pthread_cond_t pt_sigsuspended_cond = PTHREAD_COND_INITIALIZER;

/* Queue of threads that are waiting in sigtimedwait(). */
static struct pthread_queue_t pt_sigwaiting;
static pthread_spin_t pt_sigwaiting_lock = __SIMPLELOCK_UNLOCKED;
static pthread_t pt_sigwmaster;
static pthread_cond_t pt_sigwaiting_cond = PTHREAD_COND_INITIALIZER;

static void pthread__make_siginfo(siginfo_t *, int);
static void pthread__kill(pthread_t, pthread_t, siginfo_t *);
static void pthread__kill_self(pthread_t, siginfo_t *);

static void 
pthread__signal_tramp(void (*)(int, siginfo_t *, void *),
    siginfo_t *, ucontext_t *);

static int firstsig(const sigset_t *);

int _sys_execve(const char *, char *const [], char *const []);
int _sys___sigprocmask14(int, const sigset_t *, sigset_t *);

__strong_alias(__libc_thr_sigsetmask,pthread_sigmask)
__strong_alias(__sigprocmask14,pthread_sigmask)
__strong_alias(__exeve,execve)

void
pthread__signal_init(void)
{
	SDPRINTF(("(signal_init) setting process sigmask\n"));
	_sys___sigprocmask14(0, NULL, &pt_process_sigmask);

	PTQ_INIT(&pt_sigsuspended);
	PTQ_INIT(&pt_sigwaiting);
}

void
pthread__signal_start(void)
{
	int i;
	struct sigaction act;

	/* Clear all additional signal masks; we'll handle them ourselves */
	for (i = 1 ; i < NSIG ; i++) {
		__libc_sigaction14(i, NULL, &act);
		__sigemptyset14(&act.sa_mask);
		__libc_sigaction14(i, &act, NULL);
	}
}

static void
pthread__make_siginfo(siginfo_t *si, int sig)
{
	(void)memset(si, 0, sizeof(*si));
	si->si_signo = sig;
	si->si_code = SI_USER;
	/*
	 * XXX: these could be cached, but beware of setuid().
	 */
	si->si_uid = getuid();
	si->si_pid = getpid();
}

int
pthread_kill(pthread_t thread, int sig)
{
	pthread_t self;
	void (*handler)(int);

	self = pthread__self();

	SDPRINTF(("(pthread_kill %p) kill %p sig %d\n", self, thread, sig));

	if ((sig < 0) || (sig >= _NSIG))
		return EINVAL;
	if (pthread__find(self, thread) != 0)
		return ESRCH;

	/*
	 * We only let the thread handle this signal if the action for
	 * the signal is an explicit handler. Otherwise we feed it to
	 * the kernel so that it can affect the whole process.
	 */
	pthread_spinlock(self, &pt_sigacts_lock);
	handler = pt_sigacts[sig].sa_handler;
	pthread_spinunlock(self, &pt_sigacts_lock);

	if (handler == SIG_IGN) {
		SDPRINTF(("(pthread_kill %p) do nothing\n", self, thread, sig));
		/* Do nothing */
	} else if ((sig == SIGKILL) || (sig == SIGSTOP) ||
	    (handler == SIG_DFL)) {
		/* Let the kernel do the work */
		SDPRINTF(("(pthread_kill %p) kernel kill\n", self, thread, sig));
		kill(getpid(), sig);
	} else {
		siginfo_t si;
		pthread__make_siginfo(&si, sig);
		pthread_spinlock(self, &thread->pt_siglock);
		pthread__kill(self, thread, &si);
		pthread_spinunlock(self, &thread->pt_siglock);
	}

	return 0;
}


/*
 * Interpositioning is our friend. We need to intercept sigaction(),
 * sigsuspend() and sigtimedwait().
 */

int
__sigaction14(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct sigaction realact;
	sigset_t oldmask;
	pthread_t self;
	int retval;

	if ((sig <= 0) || (sig >= _NSIG))
		return EINVAL;

	self = pthread__self();
	pthread_spinlock(self, &pt_sigacts_lock);
	oldmask = pt_sigacts[sig].sa_mask;
	if (act != NULL) {
		/* Save the information for our internal dispatch. */
		pt_sigacts[sig] = *act;
		pthread_spinunlock(self, &pt_sigacts_lock);
		/*
		 * We want to handle all signals ourself, and not have
		 * the kernel mask them. Therefore, we clear the
		 * sa_mask field before passing this to the kernel. We
		 * do not set SA_NODEFER, which seems like it might be
		 * appropriate, because that would permit a continuous
		 * stream of signals to exhaust the supply of upcalls.
		 */
		if (pthread__started) {
			realact = *act;
			__sigemptyset14(&realact.sa_mask);
			act = &realact;
		}
	} else
		pthread_spinunlock(self, &pt_sigacts_lock);

	retval = __libc_sigaction14(sig, act, oact);
	if (oact && (retval == 0))
		oact->sa_mask = oldmask;

	return retval;
}

int _sys___sigsuspend14(const sigset_t *);

int
__sigsuspend14(const sigset_t *sigmask)
{
	pthread_t self;
	sigset_t oldmask;

	/* if threading not started yet, just do the syscall */
	if (__predict_false(pthread__started == 0))
		return (_sys___sigsuspend14(sigmask));

	self = pthread__self();

	pthread_spinlock(self, &pt_sigsuspended_lock);
	pthread_spinlock(self, &self->pt_statelock);
	if (self->pt_cancel) {
		pthread_spinunlock(self, &self->pt_statelock);
		pthread_spinunlock(self, &pt_sigsuspended_lock);
		pthread_exit(PTHREAD_CANCELED);
	}
	pthread_sigmask(SIG_SETMASK, sigmask, &oldmask);

	self->pt_state = PT_STATE_BLOCKED_QUEUE;
	self->pt_sleepobj = &pt_sigsuspended_cond;
	self->pt_sleepq = &pt_sigsuspended;
	self->pt_sleeplock = &pt_sigsuspended_lock;
	pthread_spinunlock(self, &self->pt_statelock);

	PTQ_INSERT_TAIL(&pt_sigsuspended, self, pt_sleep);
	pthread__block(self, &pt_sigsuspended_lock);

	pthread__testcancel(self);

	pthread_sigmask(SIG_SETMASK, &oldmask, NULL);

	errno = EINTR;
	return -1;
}

/*
 * Interpositioned sigtimedwait(2), need to multiplex all
 * eventual callers to a single kernel lwp.
 */
int _sigtimedwait(const sigset_t * __restrict, siginfo_t * __restrict,
	const struct timespec * __restrict);

static void
pthread_sigtimedwait__callback(void *arg)
{
	pthread__sched(pthread__self(), (pthread_t) arg);
}

int
sigtimedwait(const sigset_t * __restrict set, siginfo_t * __restrict info,
    const struct timespec * __restrict timeout)
{
	pthread_t self, target;
	sigset_t wset;
	struct timespec timo;
	int sig, error = 0;

	/* if threading not started yet, just do the syscall */
	if (__predict_false(pthread__started == 0))
		return (_sigtimedwait(set, info, timeout));

	self = pthread__self();
	pthread__testcancel(self);

	pthread_spinlock(self, &self->pt_siglock);

	/*
	 * Check if one of the signals that we will be waiting for
	 * is already pending.  If so, return it immediately.
	 * XXX the MP locking of this isn't right.
	 */
	wset = *set;
	__sigandset(&self->pt_siglist, &wset);
	sig = firstsig(&wset);
	if (sig) {
		info->si_signo = sig;
		__sigdelset14(&self->pt_siglist, sig);  /* clear it */
		pthread_spinunlock(self, &self->pt_siglock);
		pthread__testcancel(self);
		return 0;
	}

	/* also call syscall if timeout is zero (i.e. polling) */
	if (timeout && timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
		pthread_spinunlock(self, &self->pt_siglock);
		error = _sigtimedwait(set, info, timeout);
		pthread__testcancel(self);
		return (error);
	}
	if (timeout) {
		if ((u_int) timeout->tv_nsec >= 1000000000) {
			pthread_spinunlock(self, &self->pt_siglock);
			errno = EINVAL;
			return (-1);
		}

		timo = *timeout;
	}

	pthread_spinunlock(self, &self->pt_siglock);
	pthread_spinlock(self, &pt_sigwaiting_lock);

	/*
	 * If there is already a master thread running, arrange things
	 * to accomodate for eventual extra signals to wait for
	 * and join the sigwaiting list.
	 */
	if (pt_sigwmaster) {
		struct pt_alarm_t timoalarm;
		struct timespec etimo;

		SDPRINTF(("(stw %p) not master\n", self));

		/*
		 * Get current time. We need it if we would become master.
		 */
		if (timeout) {
			clock_gettime(CLOCK_MONOTONIC, &etimo);
			timespecadd(&etimo, timeout, &etimo);
		}

		/*
		 * Check if this thread's wait set is different to master set.
		 */
		wset = *set;
		__sigminusset(pt_sigwmaster->pt_sigwait, &wset);
		if (firstsig(&wset)) {
			/*
			 * Some new signal in set, wakeup master. It will
			 * rebuild its wait set.
			 */
			_lwp_wakeup(pt_sigwmaster->pt_blockedlwp);
		}

		/* Save our wait set and info pointer */
		wset = *set;
		self->pt_sigwait = &wset;
		self->pt_wsig = info;

		/* zero to recognize when we get passed the signal from master */
		info->si_signo = 0;

		if (timeout) {
			pthread__alarm_add(self, &timoalarm, &etimo,
				pthread_sigtimedwait__callback, self);
		}

 block:
		pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = &pt_sigwaiting_cond;
		self->pt_sleepq = &pt_sigwaiting;
		self->pt_sleeplock = &pt_sigwaiting_lock;
		pthread_spinunlock(self, &self->pt_statelock);

		PTQ_INSERT_TAIL(&pt_sigwaiting, self, pt_sleep);

		SDPRINTF(("(stw %p) not master blocking\n", self));
		pthread__block(self, &pt_sigwaiting_lock);
		SDPRINTF(("(stw %p) not master unblocked\n", self));

		/* check if we got a signal we waited for */
		if (info->si_signo) {
			/* got the signal from master */
			pthread__testcancel(self);
			return (0);
		}

		/* need the lock from now on */
		pthread_spinlock(self, &pt_sigwaiting_lock);

		/*
		 * If alarm fired, remove us from queue, adjust master
		 * wait set and return with EAGAIN.
		 */
		if (timeout) {
			if (pthread__alarm_fired(&timoalarm)) {
				PTQ_REMOVE(&pt_sigwaiting, self, pt_sleep);

				/*
				 * Signal master. It will rebuild it's wait set.
				 */
				_lwp_wakeup(pt_sigwmaster->pt_blockedlwp);

				pthread_spinunlock(self, &pt_sigwaiting_lock);
				errno = EAGAIN;
				return (-1);
			}
			pthread__alarm_del(self, &timoalarm);
		}

		/*
		 * May have been woken up to deliver signal - check if we are
		 * the master and reblock if appropriate.
		 */
		if (pt_sigwmaster != self)
			goto block;

		/* not signal nor alarm, must have been upgraded to master */
		pthread__assert(pt_sigwmaster == self);

		/* update timeout before upgrading to master */
		if (timeout) {
			struct timespec tnow;

			clock_gettime(CLOCK_MONOTONIC, &tnow);
			/* compute difference to end time */
			timespecsub(&tnow, &etimo, &tnow);
			/* substract the difference from timeout */
			timespecsub(&timo, &tnow, &timo);
		}
	}

	/* MASTER */
	self->pt_sigwait = &wset;
	self->pt_wsig = NULL;

	/* Master thread loop */
	pt_sigwmaster = self;
	SDPRINTF(("(stw %p) i am the master\n", self));
	for(;;) {
		/* Build our wait set */
		wset = *set;
		if (!PTQ_EMPTY(&pt_sigwaiting)) {
			PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep)
				__sigplusset(target->pt_sigwait, &wset);
		}

		pthread_spinunlock(self, &pt_sigwaiting_lock);

		/*
		 * We are either the only one, or wait set was setup already.
		 * Just do the syscall now.
		 */
		SDPRINTF(("(stw %p) master blocking\n", self));
		error = __sigtimedwait(&wset, info, (timeout) ? &timo : NULL);
		SDPRINTF(("(stw %p) master unblocking\n", self));

		pthread_spinlock(self, &pt_sigwaiting_lock);
		if ((error && (errno != ECANCELED || self->pt_cancel))
		    || (!error && __sigismember14(set, info->si_signo)) ) {

			/*
			 * Normal function return. Clear pt_sigwmaster,
			 * and if wait queue is nonempty, make first waiter
			 * new master.
			 */
			SDPRINTF(("(stw %p) master normal self %d\n",
				  self, info->si_signo));
			pt_sigwmaster = NULL;
			if (!PTQ_EMPTY(&pt_sigwaiting)) {
				pt_sigwmaster = PTQ_FIRST(&pt_sigwaiting);
				SDPRINTF(("(stw %p) new master %p\n", self,
					  pt_sigwmaster));
				PTQ_REMOVE(&pt_sigwaiting, pt_sigwmaster,
					pt_sleep);
				pthread__sched(self, pt_sigwmaster);
			}

			pthread_spinunlock(self, &pt_sigwaiting_lock);
			pthread__testcancel(self);
			return (error);
		}

		if (!error) {

			/*
			 * Got a signal, but not from _our_ wait set.
			 * Scan the queue of sigwaiters and wakeup
			 * the first thread waiting for this signal.
			 */
			PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep) {
			    if (__sigismember14(target->pt_sigwait,
						info->si_signo)) {
				pthread__assert(target->pt_state ==
						PT_STATE_BLOCKED_QUEUE);
				SDPRINTF(("(stw %p) master target %p\n",
					  self, target));

				/* copy to waiter siginfo */
				memcpy(target->pt_wsig, info, sizeof(*info));
				PTQ_REMOVE(&pt_sigwaiting, target, pt_sleep);
				pthread__sched(self, target);
				break;
			    }
			}

			if (!target) {

				/*
				 * Didn't find anyone waiting on this signal.
				 * Deliver signal normally. This might
				 * happen if a thread times out, but
				 * 'their' signal arrives before the master
				 * thread would be scheduled after _lwp_wakeup().
				 */
				SDPRINTF(("(stw %p) master orphaned\n", self));
				pthread__signal(self, NULL, info);
			} else {

				/*
				 * Signal waiter removed, adjust our wait set.
				 */
				SDPRINTF(("(stw %p) master raced\n", self));
				wset = *set;
				PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep)
					__sigplusset(target->pt_sigwait, &wset);
			}
		} else {
			/*
			 * ECANCELED - new sigwaiter arrived and added to master
			 * wait set, or some sigwaiter exited due to timeout
			 * and removed from master wait set. All the work
			 * was done already, so just update our timeout
			 * and go back to syscall.
			 */
		}

		/* Timeout was adjusted by the syscall, just call again. */
	}

	/* NOTREACHED */
	return (0);
}

/*
 * firstsig is stolen from kern_sig.c
 * XXX this is an abstraction violation; except for this, all of 
 * the knowledge about the composition of sigset_t's was encapsulated
 * in signal.h.
 * Putting this function in signal.h caused problems with other parts of the
 * kernel that #included <signal.h> but didn't have a prototype for ffs.
 */

static int
firstsig(const sigset_t *ss)
{
	int sig;

	sig = ffs((int)ss->__bits[0]);
	if (sig != 0)
		return (sig);
#if _NSIG > 33
	sig = ffs((int)ss->__bits[1]);
	if (sig != 0)
		return (sig + 32);
#endif
#if _NSIG > 65
	sig = ffs((int)ss->__bits[2]);
	if (sig != 0)
		return (sig + 64);
#endif
#if _NSIG > 97
	sig = ffs((int)ss->__bits[3]);
	if (sig != 0)
		return (sig + 96);
#endif
	return (0);
}

int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	sigset_t tmp, takelist;
	pthread_t self;
	int i;
	siginfo_t si;

	self = pthread__self();
	/*
	 * While other threads may read a process's sigmask,
	 * they won't write it, so we don't need to lock our reads of it.
	 */
	if (oset != NULL)
		*oset = self->pt_sigmask;
	
	if (set == NULL)
		return 0;

	pthread_spinlock(self, &self->pt_siglock);
	if (how == SIG_BLOCK) {
		__sigplusset(set, &self->pt_sigmask);
		/*
		 * Blocking of signals that are now
		 * blocked by all threads will be done
		 * lazily, at signal delivery time.
		 */
		if (__predict_true(pthread__started)) {
			pthread_spinunlock(self, &self->pt_siglock);
			return 0;
		}
	} else if (how == SIG_UNBLOCK)
		__sigminusset(set, &self->pt_sigmask);
	else if (how == SIG_SETMASK)
		self->pt_sigmask = *set;
	else {
		pthread_spinunlock(self, &self->pt_siglock);
		return EINVAL;
	}

	if (__predict_false(pthread__started == 0)) {
		pt_process_sigmask = self->pt_sigmask;
		_sys___sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
		pthread_spinunlock(self, &self->pt_siglock);
		return 0;
	}

	/* See if there are any signals to take */
	__sigemptyset14(&takelist);
	tmp = self->pt_siglist;
	while ((i = firstsig(&tmp)) != 0) {
		if (!__sigismember14(&self->pt_sigmask, i)) {
			__sigaddset14(&takelist, i);
			__sigdelset14(&self->pt_siglist, i);
		}
		__sigdelset14(&tmp, i);
	}
	pthread_spinlock(self, &pt_process_siglock);
	tmp = pt_process_siglist;
	while ((i = firstsig(&tmp)) != 0) {
		if (!__sigismember14(&self->pt_sigmask, i)) {
			__sigaddset14(&takelist, i);
			__sigdelset14(&pt_process_siglist, i);
		}
		__sigdelset14(&tmp, i);
	}
	/* Unblock any signals that were blocked process-wide before this. */
	tmp = pt_process_sigmask;
	__sigandset(&self->pt_sigmask, &tmp);
	if (!__sigsetequal(&tmp, &pt_process_sigmask)) {
		pt_process_sigmask = tmp;
		_sys___sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
	}
	pthread_spinunlock(self, &pt_process_siglock);

	pthread__make_siginfo(&si, 0);
	while ((i = firstsig(&takelist)) != 0) {
		/* Take the signal */	
		SDPRINTF(("(pt_sigmask %p) taking unblocked signal %d\n",
		    self, i));
		si.si_signo = i;
		pthread__kill_self(self, &si);
		__sigdelset14(&takelist, i);
	}

	pthread_spinunlock(self, &self->pt_siglock);

	return 0;
}


/*
 * Dispatch a signal to thread t, if it is non-null, and to any
 * willing thread, if t is null.
 */
void
pthread__signal(pthread_t self, pthread_t t, siginfo_t *si)
{
	pthread_t target, good, okay;

	if (t) {
		target = t;
		pthread_spinlock(self, &target->pt_siglock);
	} else {
		/*
		 * Pick a thread that doesn't have the signal blocked
		 * and can be reasonably forced to run. 
		 */
		okay = good = NULL;
		pthread_spinlock(self, &pthread__allqueue_lock);
		PTQ_FOREACH(target, &pthread__allqueue, pt_allq) {
			/*
			 * Changing to PT_STATE_ZOMBIE is protected by
			 * the pthread__allqueue lock, so we can just
			 * test for it here.
			 */
			if ((target->pt_state == PT_STATE_ZOMBIE) ||
			    (target->pt_type != PT_THREAD_NORMAL))
				continue;
			pthread_spinlock(self, &target->pt_siglock);
			SDPRINTF((
				"(pt_signal %p) target %p: state %d, mask %08x\n", 
				self, target, target->pt_state, target->pt_sigmask.__bits[0]));
			if (!__sigismember14(&target->pt_sigmask,
			    si->si_signo)) {
				if (target->pt_blockgen == target->pt_unblockgen) {
					good = target;
					/* Leave target locked */
					break;
				} else if (okay == NULL) {
					okay = target;
					/* Leave target locked */
					continue;
				}
			}
			pthread_spinunlock(self, &target->pt_siglock);
		}
		pthread_spinunlock(self, &pthread__allqueue_lock);
		if (good) {
			target = good;
			if (okay)
				pthread_spinunlock(self, &okay->pt_siglock);
		} else {
			target = okay;
		}

		if (target == NULL) {
			/*
			 * They all have it blocked. Note that in our
			 * process-wide signal mask, and stash the signal
			 * for later unblocking.  
			 */
			pthread_spinlock(self, &pt_process_siglock);
			__sigaddset14(&pt_process_sigmask, si->si_signo);
			SDPRINTF(("(pt_signal %p) lazily setting proc sigmask to "
			    "%08x\n", self, pt_process_sigmask.__bits[0]));
			_sys___sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
			__sigaddset14(&pt_process_siglist, si->si_signo);
			pthread_spinunlock(self, &pt_process_siglock);
			return;
		}
	}

	/*
	 * We now have a signal and a victim. 
	 * The victim's pt_siglock is locked.
	 */

	/*
	 * Reset the process signal mask so we can take another
	 * signal.  We will not exhaust our supply of upcalls; if
	 * another signal is delivered after this, the resolve_locks
	 * dance will permit us to finish and recycle before the next
	 * upcall reaches this point.
	 */
	pthread_spinlock(self, &pt_process_siglock);
	SDPRINTF(("(pt_signal %p) setting proc sigmask to "
	    "%08x\n", self, pt_process_sigmask.__bits[0]));
	_sys___sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
	pthread_spinunlock(self, &pt_process_siglock);

	pthread__kill(self, target, si);
	pthread_spinunlock(self, &target->pt_siglock);
}

/* 
 * Call the signal handler in the context of this thread. Not quite as
 * suicidal as it sounds.
 * Must be called with target's siglock held.
 */
static void
pthread__kill_self(pthread_t self, siginfo_t *si)
{
	sigset_t oldmask;
	struct sigaction act;
	ucontext_t uc;	/* XXX: we don't pass the right context here */

	pthread_spinlock(self, &pt_sigacts_lock);
	act = pt_sigacts[si->si_signo];
	pthread_spinunlock(self, &pt_sigacts_lock);

	SDPRINTF(("(pthread__kill_self %p) sig %d\n", self, si->si_signo));

	oldmask = self->pt_sigmask;
	 __sigplusset(&act.sa_mask, &self->pt_sigmask);
	if ((act.sa_flags & SA_NODEFER) == 0)
		__sigaddset14(&self->pt_sigmask, si->si_signo);

	pthread_spinunlock(self, &self->pt_siglock);
	(*act.sa_sigaction)(si->si_signo, si, &uc);
	pthread_spinlock(self, &self->pt_siglock);

	self->pt_sigmask = oldmask;
}

/* Must be called with target's siglock held */
static void
pthread__kill(pthread_t self, pthread_t target, siginfo_t *si)
{
	int deliver;

	SDPRINTF(("(pthread__kill %p) target %p sig %d code %d\n", self, target,
	    si->si_signo, si->si_code));

	if (__sigismember14(&target->pt_sigmask, si->si_signo)) {
		SDPRINTF(("(pthread__kill %p) target masked\n", target));

		/*
		 * If the target is waiting for this signal in sigtimedwait(),
		 * make the target runnable but do not deliver the signal.
		 * Otherwise record the signal for later delivery.
		 * XXX not MPsafe.
		 */
		pthread_spinlock(self, &self->pt_statelock);
		if (target->pt_state == PT_STATE_BLOCKED_QUEUE &&
		    target->pt_sleepq == &pt_sigwaiting &&
		    __sigismember14(target->pt_sigwait, si->si_signo)) {
			SDPRINTF(("(pthread__kill %p) stw\n", target));
			target->pt_wsig->si_signo = si->si_signo;
			pthread_spinunlock(self, &self->pt_statelock);
			deliver = 0;
		} else {
			SDPRINTF(("(pthread__kill %p) deferring\n", target));
			pthread_spinunlock(self, &self->pt_statelock);
			__sigaddset14(&target->pt_siglist, si->si_signo);
			return;
		}
	} else {
		SDPRINTF(("(pthread__kill %p) target %p delivering\n",
			  self, target));
		deliver = 1;
	}

	if (self == target) {
		pthread__kill_self(self, si);
		return;
	}

	/*
	 * Ensure the victim is not running.
	 * In a MP world, it could be on another processor somewhere.
	 *
	 * XXX As long as this is uniprocessor, encountering a running
	 * target process is a bug.
	 */
	pthread__assert(target->pt_state != PT_STATE_RUNNING ||
		target->pt_blockgen != target->pt_unblockgen);

	/*
	 * Holding the state lock blocks out cancellation and any other
	 * attempts to set this thread up to take a signal.
	 */
	pthread_spinlock(self, &target->pt_statelock);
	if (target->pt_blockgen != target->pt_unblockgen) {
		SDPRINTF(("(pthread__kill %p) target blocked\n", target));

		/*
		 * The target is not on a queue at all, and
		 * won't run again for a while. Try to wake it
		 * from its torpor, then mark the signal for
		 * later processing.
		 */
		__sigaddset14(&target->pt_sigblocked, si->si_signo);
		__sigaddset14(&target->pt_sigmask, si->si_signo);
		pthread_spinlock(self, &target->pt_flaglock);
		target->pt_flags |= PT_FLAG_SIGDEFERRED;
		pthread_spinunlock(self, &target->pt_flaglock);
		pthread_spinunlock(self, &target->pt_statelock);
		_lwp_wakeup(target->pt_blockedlwp);
		return;
	}
	SDPRINTF(("(pthread__kill %p) target state %d\n", target,
		  target->pt_state));
	switch (target->pt_state) {
	case PT_STATE_SUSPENDED:
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__suspqueue, target, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		break;
	case PT_STATE_RUNNABLE:
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__runqueue, target, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		break;
	case PT_STATE_BLOCKED_QUEUE:
		pthread_spinlock(self, target->pt_sleeplock);
		PTQ_REMOVE(target->pt_sleepq, target, pt_sleep);
		pthread_spinunlock(self, target->pt_sleeplock);
		break;
	case PT_STATE_ZOMBIE:
		pthread_spinunlock(self, &target->pt_statelock);
		return;
	default:
		;
	}

	if (deliver)
		pthread__deliver_signal(self, target, si);
	pthread__sched(self, target);
	pthread_spinunlock(self, &target->pt_statelock);
}

/* Must be called with target's siglock held */
void
pthread__deliver_signal(pthread_t self, pthread_t target, siginfo_t *si)
{
	ucontext_t *uc, *olduc;
	struct sigaction act;
	siginfo_t *siginfop;
	caddr_t sp;
	size_t ucsize;

	pthread_spinlock(self, &pt_sigacts_lock);
	act = pt_sigacts[si->si_signo];
	pthread_spinunlock(self, &pt_sigacts_lock);
	
	if (target->pt_trapuc) {
		olduc = target->pt_trapuc;
		target->pt_trapuc = NULL;
	} else
		olduc = target->pt_uc;

	/*
	 * Get the current signal mask, but unblock the current signal,
	 * because we might have been sigdeferred and the signal would
	 * have been blocked because of that. Since we are delivering
	 * the signal now, we cannot have this signal blocked anyway.
	 * XXX: Is it better to check for SIGDEFERRED here?
	 */
	olduc->uc_sigmask = target->pt_sigmask;
	__sigdelset14(&olduc->uc_sigmask, si->si_signo);
	/*
	 * Prevent anyone else from considering this thread for handling
	 * more instances of this signal.
	 */
	 __sigplusset(&act.sa_mask, &target->pt_sigmask);
	if ((act.sa_flags & SA_NODEFER) == 0)
		__sigaddset14(&target->pt_sigmask, si->si_signo);

	/*
	 * We'd like to just pass oldmask and si to the
	 * pthread__signal_tramp(), but makecontext() can't reasonably
	 * pass structures, just word-size things or smaller. We also
	 * don't want to involve malloc() here, inside the upcall
	 * handler. So we borrow a bit of space from the target's
	 * stack, which we were adjusting anyway.
	 */

	sp = STACK_MAX(olduc, sizeof(ucontext_t));
	sp = STACK_GROW(sp, STACKSPACE);
	siginfop = (void *)STACK_ALLOC(sp, sizeof(siginfo_t));
	sp = STACK_GROW(sp, sizeof(siginfo_t));
	*siginfop = *si;

	/*
	 * XXX We are blatantly ignoring SIGALTSTACK. It would screw
	 * with our notion of stack->thread mappings.
	 */

	sp = STACK_ALIGN(sp, ~_UC_UCONTEXT_ALIGN);
	ucsize = roundup(sizeof(ucontext_t), (~_UC_UCONTEXT_ALIGN) + 1);
	uc = (void *)STACK_ALLOC(sp, ucsize);
	sp = STACK_GROW(sp, ucsize);

	_INITCONTEXT_U(uc);

	uc->uc_stack.ss_sp = sp;
	uc->uc_stack.ss_size = 0;
	uc->uc_link = NULL;

	SDPRINTF(("(makecontext %p): target %p: sig: %d uc: %p oldmask: %08x\n",
	    self, target, si->si_signo, olduc, olduc->uc_sigmask.__bits[0]));
	makecontext(uc, pthread__signal_tramp, 3, act.sa_handler, siginfop,
	    olduc);
	target->pt_uc = uc;
}

void
pthread__signal_deferred(pthread_t self, pthread_t t)
{
	int i;
	siginfo_t si;

	pthread_spinlock(self, &t->pt_siglock);

	pthread__make_siginfo(&si, 0);
	while ((i = firstsig(&t->pt_sigblocked)) != 0) {
		__sigdelset14(&t->pt_sigblocked, i);
		si.si_signo = i;
		pthread__deliver_signal(self, t, &si);
	}
	t->pt_flags &= ~PT_FLAG_SIGDEFERRED;

	pthread_spinunlock(self, &t->pt_siglock);
}

static void 
pthread__signal_tramp(void (*handler)(int, siginfo_t *, void *),
    siginfo_t *info, ucontext_t *uc)
{
	SDPRINTF(("(tramp %p) sig %d code %d uc %p oldmask %08x\n", 
	    pthread__self(), info->si_signo, info->si_code, uc,
	    uc->uc_sigmask.__bits[0]));

	(*handler)(info->si_signo, info, uc);

	pthread__self()->pt_flags |= PT_FLAG_SIGNALED;

	/*
	 * We've finished the handler, so this thread can restore the
	 * original signal mask.  Note that traditional BSD behavior
	 * allows for the handler to change the signal mask; we handle
	 * that here.
	 */
	pthread_sigmask(SIG_SETMASK, &uc->uc_sigmask, NULL);

	/*
	 * Jump back to what we were doing before we were interrupted
	 * by a signal.
	 */
	_setcontext_u(uc);
	/*NOTREACHED*/
	pthread__abort();
}

/*
 * The execve() system call and the libc exec*() calls that use it are
 * specified to propagate the signal mask of the current thread to the
 * initial thread of the new process image. Since thread signal masks
 * are maintained in userlevel, this wrapper is necessary to give the
 * kernel the correct value.
 */
int
execve(const char *path, char *const argv[], char *const envp[])
{
	pthread_t self;
	int ret;

	self = pthread__self();

	/*
	 * Don't acquire pt_process_siglock, even though it seems like
	 * the right thing to do. The most common reason to be here is
	 * that we're on the child side of a fork() or vfork()
	 * call. In either case, another thread could have held
	 * pt_process_siglock at the moment of forking, and acquiring
	 * it here would cause us to deadlock. Additionally, in the
	 * case of vfork(), acquiring the lock here would cause it to
	 * be locked in the parent's address space and cause a
	 * deadlock there the next time a signal routine is called.
	 *
	 * The remaining case is where a live multithreaded program
	 * calls exec*() from one of several threads with no explicit
	 * synchronization. It may get the wrong process sigmask in
	 * the new process image if another thread executes a signal
	 * routine between the sigprocmask and the _sys_execve()
	 * call. I don't have much sympathy for such a program.
	 */
	_sys___sigprocmask14(SIG_SETMASK, &self->pt_sigmask, NULL);
	ret = _sys_execve(path, argv, envp);

	/* Normally, we shouldn't get here; this is an error condition. */
	_sys___sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);

	return ret;
}
