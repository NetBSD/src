/*	$NetBSD: pthread_sig.c,v 1.6 2003/01/28 21:04:37 jdolecek Exp $	*/

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

/* We're interposing a specific version of the signal interface. */
#define	__LIBC12_SOURCE__

#define	__PTHREAD_SIGNAL_PRIVATE

#include <assert.h>
#include <errno.h>
#include <lwp.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>		/* for memcpy() */
#include <ucontext.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_SIG_DEBUG

#ifdef PTHREAD_SIG_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

extern pthread_spin_t pthread__runqueue_lock;
extern struct pthread_queue_t pthread__runqueue;

extern pthread_spin_t pthread__allqueue_lock;
extern struct pthread_queue_t pthread__allqueue;

static pthread_spin_t	pt_sigacts_lock;
static struct sigaction pt_sigacts[_NSIG];

static pthread_spin_t	pt_process_siglock;
static sigset_t	pt_process_sigmask;
static sigset_t	pt_process_siglist;

/* Queue of threads that are waiting in sigsuspend(). */
static struct pthread_queue_t pt_sigsuspended;
static pthread_spin_t pt_sigsuspended_lock;

/*
 * Nothing actually signals or waits on this lock, but the sleepobj
 * needs to point to something.
 */
static pthread_cond_t pt_sigsuspended_cond = PTHREAD_COND_INITIALIZER;


static void 
pthread__signal_tramp(int, int, void (*)(int, int, struct sigcontext *),
    ucontext_t *, sigset_t *);

__strong_alias(__libc_thr_sigsetmask,pthread_sigmask)

void
pthread__signal_init(void)
{
	SDPRINTF(("(signal_init) setting process sigmask\n"));
	__sigprocmask14(0, NULL, &pt_process_sigmask);
}

int
/*ARGSUSED*/
pthread_kill(pthread_t thread, int sig)
{

	/* We only let the thread handle this signal if the action for
	 * the signal is an explicit handler. Otherwise we feed it to
	 * the kernel so that it can affect the whole process.
	 */

	/* XXX implement me */
	return -1;
}


/*
 * Interpositioning is our friend. We need to intercept sigaction() and
 * sigsuspend().
 */

int
__sigaction14(int sig, const struct sigaction *act, struct sigaction *oact)
{
	pthread_t self;
	struct sigaction realact;
	
	self = pthread__self();
	if (act != NULL) {
		if (sig <= 0 || sig >= _NSIG)
			return (EINVAL);

		/* Save the information for our internal dispatch. */
		pthread_spinlock(self, &pt_sigacts_lock);
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
		realact = *act;
		__sigemptyset14(&realact.sa_mask);
		act = &realact;
	}

	return (__libc_sigaction14(sig, act, oact));
}

int
__sigsuspend14(const sigset_t *sigmask)
{
	pthread_t self;
	sigset_t oldmask;

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
#if NSIG > 33
	sig = ffs((int)ss->__bits[1]);
	if (sig != 0)
		return (sig + 32);
#endif
#if NSIG > 65
	sig = ffs((int)ss->__bits[2]);
	if (sig != 0)
		return (sig + 64);
#endif
#if NSIG > 97
	sig = ffs((int)ss->__bits[3]);
	if (sig != 0)
		return (sig + 96);
#endif
	return (0);
}

int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	pthread_t self;
	sigset_t tmp;
	int i, retval, unblocked, procmaskset;
	void (*handler)(int, int, struct sigcontext *);
	struct sigaction *act;
	struct sigcontext xxxsc;
	sigset_t oldmask, takelist;

	self = pthread__self();

	retval = 0;
	unblocked = 0;
	if (set != NULL) {
		pthread_spinlock(self, &self->pt_siglock);
		if (how == SIG_BLOCK) {
			__sigplusset(set, &self->pt_sigmask);
			/*
			 * Blocking of signals that are now
			 * blocked by all threads will be done
			 * lazily, at signal delivery time.
			 */
		} else if (how == SIG_UNBLOCK) {
			pthread_spinlock(self, &pt_process_siglock);
			__sigminusset(set, &self->pt_sigmask);
			/*
			 * Unblock any signals that were blocked
			 * process-wide before this.
			 */
			tmp = pt_process_sigmask;
			__sigminusset(set, &tmp);
			if (!__sigsetequal(&tmp, &pt_process_sigmask))
				pt_process_sigmask = tmp;
			unblocked = 1;
		} else if (how == SIG_SETMASK) {
			pthread_spinlock(self, &pt_process_siglock);
			self->pt_sigmask = *set;
			SDPRINTF(("(pt_sigmask %p) (set) set thread mask to "
			    "%08x\n", self, self->pt_sigmask.__bits[0]));
			/*
			 * Unblock any signals that were blocked
			 * process-wide before this.
			 */
			tmp = pt_process_sigmask;
			__sigandset(set, &tmp);
			if (!__sigsetequal(&tmp, &pt_process_sigmask))
				pt_process_sigmask = tmp;
			unblocked = 1; /* Well, maybe */
		} else 
			retval = EINVAL;
	}

	if (unblocked) {
		SDPRINTF(("(pt_sigmask %p) Process mask was changed to %08x\n",
		    self, pt_process_sigmask.__bits[0]));
		/* See if there are any signals to take */
		__sigemptyset14(&takelist);
		while ((i = firstsig(&self->pt_siglist)) != 0) {
			if (!__sigismember14(&self->pt_sigmask, i)) {
				__sigaddset14(&takelist, i);
				__sigdelset14(&self->pt_siglist, i);
			}
		}
		while ((i = firstsig(&pt_process_siglist)) != 0) {
			if (!__sigismember14(&self->pt_sigmask, i)) {
				__sigaddset14(&takelist, i);
				__sigdelset14(&pt_process_siglist, i);
			}
		}

		procmaskset = 0;
		while ((i = firstsig(&takelist)) != 0) {
			if (!__sigismember14(&self->pt_sigmask, i)) {
				/* Take the signal */
				act = &pt_sigacts[i];
				oldmask = self->pt_sigmask;
				__sigplusset(&self->pt_sigmask,
				    &act->sa_mask);
				__sigaddset14(&self->pt_sigmask, i);
				pthread_spinunlock(self, &pt_process_siglock);
				pthread_spinunlock(self, &self->pt_siglock);
				SDPRINTF(("(pt_sigmask %p) taking unblocked signal %d\n", self, i));
				handler =
				    (void (*)(int, int, struct sigcontext *))
				    act->sa_handler;
				handler(i, 0, &xxxsc);
				pthread_spinlock(self, &self->pt_siglock);
				pthread_spinlock(self, &pt_process_siglock);
				__sigdelset14(&takelist, i);
				/* Reset masks */
				self->pt_sigmask = oldmask;
				tmp = pt_process_sigmask;
				__sigandset(&oldmask, &tmp);
				if (!__sigsetequal(&tmp, &pt_process_sigmask)){
					pt_process_sigmask = tmp;
					SDPRINTF(("(pt_sigmask %p) setting proc sigmask to %08x\n", pt_process_sigmask.__bits[0]));
					__sigprocmask14(SIG_SETMASK, 
					    &pt_process_sigmask, NULL);
					procmaskset = 1;
				}
			}
		}
		if (!procmaskset) {
			SDPRINTF(("(pt_sigmask %p) setting proc sigmask to %08x\n", self, pt_process_sigmask.__bits[0]));
			__sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
		}
	} 
	if (set != NULL) {
		if ((how == SIG_UNBLOCK) || (how == SIG_SETMASK))
			pthread_spinunlock(self, &pt_process_siglock);
		pthread_spinunlock(self, &self->pt_siglock);
	}

	/*
	 * While other threads may read a process's sigmask,
	 * they won't write it, so we don't need to lock our reads of it.
	 */
	if (oset != NULL) {
		*oset = self->pt_sigmask;
	}

	return retval;
}


/*
 * Dispatch a signal to thread t, if it is non-null, and to any
 * willing thread, if t is null.
 */
void
pthread__signal(pthread_t self, pthread_t t, int sig, int code)
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
			if (!__sigismember14(&target->pt_sigmask, sig)) {
				if (target->pt_state != PT_STATE_BLOCKED_SYS) {
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
			__sigaddset14(&pt_process_sigmask, sig);
			SDPRINTF(("(pt_signal %p) lazily setting proc sigmask to "
			    "%08x\n", self, pt_process_sigmask.__bits[0]));
			__sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
			__sigaddset14(&pt_process_siglist, sig);
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
	__sigprocmask14(SIG_SETMASK, &pt_process_sigmask, NULL);
	pthread_spinunlock(self, &pt_process_siglock);

	if (t && __sigismember14(&target->pt_sigmask, sig)) {
		/* Record the signal for later delivery. */
		__sigaddset14(&target->pt_siglist, sig);
		pthread_spinunlock(self, &target->pt_siglock);
		return;
	}

	/*
	 * Ensure the victim is not running.
	 * In a MP world, it could be on another processor somewhere.
	 *
	 * XXX As long as this is uniprocessor, encountering a running
	 * target process is a bug.
	 */
	assert(target->pt_state != PT_STATE_RUNNING);

	/*
	 * Holding the state lock blocks out cancellation and any other
	 * attempts to set this thread up to take a signal.
	 */
	pthread_spinlock(self, &target->pt_statelock);
	switch (target->pt_state) {
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
	case PT_STATE_BLOCKED_SYS:
		/*
		 * The target is not on a queue at all, and won't run
		 * again for a while. Try to wake it from its torpor, then
		 * mark the signal for later processing.
		 */
		__sigaddset14(&target->pt_sigblocked, sig);
		__sigaddset14(&target->pt_sigmask, sig);
		target->pt_flags |= PT_FLAG_SIGDEFERRED;
		pthread_spinunlock(self, &target->pt_siglock);
		pthread_spinunlock(self, &target->pt_statelock);
		_lwp_wakeup(target->pt_blockedlwp);
		return;
	default:
		;
	}

	pthread__deliver_signal(self, target, sig, code);
	pthread__sched(self, target);
	pthread_spinunlock(self, &target->pt_statelock);
}

/* Must be called with target's siglock held */
void
pthread__deliver_signal(pthread_t self, pthread_t target, int sig, int code)
{
	sigset_t oldmask, *maskp;
	ucontext_t *uc;

	/*
	 * Prevent anyone else from considering this thread for handling
	 * more instances of this signal.
	 */
	oldmask = target->pt_sigmask;
	__sigplusset(&target->pt_sigmask, &pt_sigacts[sig].sa_mask);
	__sigaddset14(&target->pt_sigmask, sig);
	pthread_spinunlock(self, &target->pt_siglock);

	/*
	 * We'd like to just pass oldmask to the
	 * pthread__signal_tramp(), but makecontext() can't reasonably
	 * pass structures, just word-size things or smaller. We also
	 * don't want to involve malloc() here, inside the upcall
	 * handler. So we borrow a bit of space from the target's
	 * stack, which we were adjusting anyway.
	 */
	maskp = (sigset_t *)(void *)((char *)(void *)target->pt_uc -
	    STACKSPACE - sizeof(sigset_t));
	*maskp = oldmask;

	/*
	 * XXX We are blatantly ignoring SIGALTSTACK. It would screw
	 * with our notion of stack->thread mappings.
	 */
	uc = (ucontext_t *)(void *)((char *)(void *)maskp - sizeof(ucontext_t));
#ifdef _UC_UCONTEXT_ALIGN
	uc = (ucontext_t *)((uintptr_t)uc & _UC_UCONTEXT_ALIGN);
#endif

	_INITCONTEXT_U(uc);
	uc->uc_stack.ss_sp = maskp;
	uc->uc_stack.ss_size = 0;
	uc->uc_link = NULL;

	SDPRINTF(("(makecontext %p): target %p: sig: %d %d uc: %p oldmask: %08x\n",
	    self, target, sig, code, target->pt_uc, maskp->__bits[0]));
	makecontext(uc, pthread__signal_tramp, 5,
	    sig, code, pt_sigacts[sig].sa_handler, target->pt_uc, maskp);
	target->pt_uc = uc;
}

void
pthread__signal_deferred(pthread_t self, pthread_t t)
{
	int i;

	pthread_spinlock(self, &t->pt_siglock);

	while ((i = firstsig(&t->pt_sigblocked)) != 0) {
		__sigdelset14(&t->pt_sigblocked, i);
		pthread__deliver_signal(self, t, i, 0);
	}
	t->pt_flags &= ~PT_FLAG_SIGDEFERRED;

	pthread_spinunlock(self, &t->pt_siglock);
}

static void 
pthread__signal_tramp(int sig, int code,
    void (*handler)(int, int, struct sigcontext *),
    ucontext_t *uc, sigset_t *oldmask)
{
	struct pthread__sigcontext psc;

	SDPRINTF(("(tramp %p) sig %d uc %p oldmask %08x\n", 
	    pthread__self(), sig, uc, oldmask->__bits[0]));

	/*
	 * XXX we don't support siginfo here yet.
	 */
	PTHREAD_UCONTEXT_TO_SIGCONTEXT(oldmask, uc, &psc);
	handler(sig, code, &psc.psc_context);
	PTHREAD_SIGCONTEXT_TO_UCONTEXT(&psc, uc);

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
       	/*NOTREACHED*//*CONSTCOND*/
	assert(0);
}
