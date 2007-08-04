/*	$NetBSD: pthread.c,v 1.71 2007/08/04 18:54:12 ad Exp $	*/

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
__RCSID("$NetBSD: pthread.c,v 1.71 2007/08/04 18:54:12 ad Exp $");

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
#define	PTHREAD__NSPINS		1024

static void	pthread__create_tramp(void *(*)(void *), void *);
static void	pthread__initthread(pthread_t);

int pthread__started;

pthread_spin_t pthread__queue_lock = __SIMPLELOCK_UNLOCKED;
pthread_queue_t pthread__allqueue;
pthread_queue_t pthread__deadqueue;

static pthread_attr_t pthread_default_attr;

enum {
	DIAGASSERT_ABORT =	1<<0,
	DIAGASSERT_STDERR =	1<<1,
	DIAGASSERT_SYSLOG =	1<<2
};

static int pthread__diagassert = DIAGASSERT_ABORT | DIAGASSERT_STDERR;

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

void *pthread__static_lib_binder[] = {
	&pthread__cancel_stub_binder,
	pthread_cond_init,
	pthread_mutex_init,
	pthread_rwlock_init,
	pthread_barrier_init,
	pthread_key_create,
	pthread_setspecific,
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

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU; 

	len = sizeof(ncpu);
	if (sysctl(mib, 2, &ncpu, &len, NULL, 0) == -1)
		err(1, "sysctl(hw.ncpu");

	/* Initialize locks first; they're needed elsewhere. */
	pthread__lockprim_init(ncpu);

	/*
	 * Get number of CPUs, and maximum number of LWPs that can be
	 * unparked at once.
	 */
	if ((pthread__concurrency = ncpu) > 1)
		pthread__nspins = PTHREAD__NSPINS;
	else
		pthread__nspins = 1;
	if ((p = getenv("PTHREAD_NSPINS")) != NULL)
		pthread__nspins = atoi(p);
	i = (int)_lwp_unpark_all(NULL, 0, NULL);
	if (i == -1)
		err(1, "_lwp_unpark_all");
	if (i < pthread__unpark_max)
		pthread__unpark_max = i;

	/* Basic data structure setup */
	pthread_attr_init(&pthread_default_attr);
	PTQ_INIT(&pthread__allqueue);
	PTQ_INIT(&pthread__deadqueue);
	/* Create the thread structure corresponding to main() */
	pthread__initmain(&first);
	pthread__initthread(first);

	first->pt_state = PT_STATE_RUNNING;
	first->pt_lid = _lwp_self();
	PTQ_INSERT_HEAD(&pthread__allqueue, first, pt_allq);

	/* Start subsystems */
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

	self = pthread__self(); /* should be the "main()" thread */

	/*
	 * Per-process timers are cleared by fork(); despite the
	 * various restrictions on fork() and threads, it's legal to
	 * fork() before creating any threads. 
	 */
	pthread_atfork(NULL, NULL, pthread__child_callback);
	SDPRINTF(("(pthread__start %p) Started.\n", self));
}


/* General-purpose thread data structure sanitization. */
/* ARGSUSED */
static void
pthread__initthread(pthread_t t)
{

	t->pt_magic = PT_MAGIC;
	t->pt_spinlocks = 0;
	t->pt_exitval = NULL;
	t->pt_flags = 0;
	t->pt_cancel = 0;
	t->pt_errno = 0;
	t->pt_state = PT_STATE_RUNNING;

	pthread_lockinit(&t->pt_lock);
	PTQ_INIT(&t->pt_cleanup_stack);
	PTQ_INIT(&t->pt_joiners);
	memset(&t->pt_specific, 0, sizeof(int) * PTHREAD_KEYS_MAX);
	t->pt_name = NULL;
}


int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	    void *(*startfunc)(void *), void *arg)
{
	pthread_t self, newthread;
	pthread_attr_t nattr;
	struct pthread_attr_private *p;
	char * volatile name;
	int ret, flag;

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
	newthread = NULL;

	if (!PTQ_EMPTY(&pthread__deadqueue)) {
		pthread_spinlock(self, &pthread__queue_lock);
		newthread = PTQ_FIRST(&pthread__deadqueue);
		if (newthread != NULL) {
			PTQ_REMOVE(&pthread__deadqueue, newthread, pt_allq);
			pthread_spinunlock(self, &pthread__queue_lock);
			if ((newthread->pt_flags & PT_FLAG_DETACHED) != 0) {
				/* Still running? */
				if (_lwp_kill(newthread->pt_lid, 0) == 0 ||
				    errno != ESRCH) {
					pthread_spinlock(self,
					    &pthread__queue_lock);
					PTQ_INSERT_TAIL(&pthread__deadqueue,
					    newthread, pt_allq);
					pthread_spinunlock(self,
					    &pthread__queue_lock);
					newthread = NULL;
				}
			}
		} else
			pthread_spinunlock(self, &pthread__queue_lock);
	}

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
	pthread__initthread(newthread);
	newthread->pt_flags = nattr.pta_flags;

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
	pthread_spinlock(self, &pthread__queue_lock);
	PTQ_INSERT_HEAD(&pthread__allqueue, newthread, pt_allq);
	pthread_spinunlock(self, &pthread__queue_lock);

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
		pthread_spinlock(self, &pthread__queue_lock);
		PTQ_REMOVE(&pthread__allqueue, newthread, pt_allq);
		PTQ_INSERT_HEAD(&pthread__deadqueue, newthread, pt_allq);
		pthread_spinunlock(self, &pthread__queue_lock);
		return ret;
	}

	/* XXX must die */
	newthread->pt_num = newthread->pt_lid;

	SDPRINTF(("(pthread_create %p) new thread %p (name %p, lid %d).\n",
		  self, newthread, newthread->pt_name,
		  (int)newthread->pt_lid));
	
	*thread = newthread;

	return 0;
}


static void
pthread__create_tramp(void *(*start)(void *), void *arg)
{
	void *retval;

	/*
	 * Throw away some stack in a feeble attempt to reduce cache
	 * thrash.  May help for SMT processors.  XXX We should not
	 * be allocating stacks on fixed 2MB boundaries.  Needs a
	 * thread register or decent thread local storage.
	 */
	(void)alloca(((unsigned)pthread__self()->pt_lid & 7) << 8);

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
	SDPRINTF(("(pthread_suspend_np %p) Suspend thread %p.\n",
		     self, thread));
	return _lwp_suspend(thread->pt_lid);
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
	SDPRINTF(("(pthread_resume_np %p) Resume thread %p.\n",
		     self, thread));
	return _lwp_continue(thread->pt_lid);
}

void
pthread_exit(void *retval)
{
	pthread_t self;
	struct pt_clean_t *cleanup;
	char *name;

	self = pthread__self();
	SDPRINTF(("(pthread_exit %p) status %p, flags %x, cancel %d\n",
		  self, retval, self->pt_flags, self->pt_cancel));

	/* Disable cancellability. */
	pthread_spinlock(self, &self->pt_lock);
	self->pt_flags |= PT_FLAG_CS_DISABLED;
	self->pt_cancel = 0;
	pthread_spinunlock(self, &self->pt_lock);

	/* Call any cancellation cleanup handlers */
	while (!PTQ_EMPTY(&self->pt_cleanup_stack)) {
		cleanup = PTQ_FIRST(&self->pt_cleanup_stack);
		PTQ_REMOVE(&self->pt_cleanup_stack, cleanup, ptc_next);
		(*cleanup->ptc_cleanup)(cleanup->ptc_arg);
	}

	/* Perform cleanup of thread-specific data */
	pthread__destroy_tsd(self);

	self->pt_exitval = retval;

	pthread_spinlock(self, &self->pt_lock);
	if (self->pt_flags & PT_FLAG_DETACHED) {
		self->pt_state = PT_STATE_DEAD;
		name = self->pt_name;
		self->pt_name = NULL;
		pthread_spinlock(self, &pthread__queue_lock);
		PTQ_REMOVE(&pthread__allqueue, self, pt_allq);
		PTQ_INSERT_TAIL(&pthread__deadqueue, self, pt_allq);
		pthread_spinunlock(self, &pthread__queue_lock);
		pthread_spinunlock(self, &self->pt_lock);
		if (name != NULL)
			free(name);
		_lwp_exit();
	} else {
		self->pt_state = PT_STATE_ZOMBIE;
		pthread_spinunlock(self, &self->pt_lock);
		/* Note: name will be freed by the joiner. */
		_lwp_exit();
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

	retval = 0;
	name = NULL;
 again:
 	pthread_spinlock(self, &thread->pt_lock);
	switch (thread->pt_state) {
	case PT_STATE_RUNNING:
		pthread_spinunlock(self, &thread->pt_lock);

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
		pthread_spinlock(self, &pthread__queue_lock);
		PTQ_REMOVE(&pthread__allqueue, thread, pt_allq);
		PTQ_INSERT_HEAD(&pthread__deadqueue, thread, pt_allq);
		pthread_spinunlock(self, &pthread__queue_lock);
		pthread_spinunlock(self, &thread->pt_lock);
		SDPRINTF(("(pthread_join %p) Joined %p.\n", self, thread));
		if (name != NULL)
			free(name);
		(void)_lwp_detach(thread->pt_lid);
		return retval;
	default:
		pthread_spinunlock(self, &thread->pt_lock);
		return EINVAL;
	}

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

	self = pthread__self();

	if (pthread__find(self, thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	pthread_spinlock(self, &self->pt_lock);
	thread->pt_flags |= PT_FLAG_DETACHED;
	pthread_spinunlock(self, &self->pt_lock);

	return _lwp_detach(thread->pt_lid);
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

	pthread_spinlock(self, &thread->pt_lock);
	if (thread->pt_name == NULL)
		name[0] = '\0';
	else
		strlcpy(name, thread->pt_name, len);
	pthread_spinunlock(self, &thread->pt_lock);

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

	pthread_spinlock(self, &thread->pt_lock);
	oldname = thread->pt_name;
	thread->pt_name = cp;
	pthread_spinunlock(self, &thread->pt_lock);

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
	if (pthread__find(self, thread) != 0)
		return ESRCH;
	pthread_spinlock(self, &thread->pt_lock);
	thread->pt_flags |= PT_FLAG_CS_PENDING;
	if ((thread->pt_flags & PT_FLAG_CS_DISABLED) == 0) {
		thread->pt_cancel = 1;
		pthread_spinunlock(self, &thread->pt_lock);
		_lwp_wakeup(thread->pt_lid);
	} else
		pthread_spinunlock(self, &thread->pt_lock);

	return 0;
}


int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_spinlock(self, &self->pt_lock);

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
				pthread_spinunlock(self, &self->pt_lock);
				pthread_exit(PTHREAD_CANCELED);
			}
		}
	} else
		retval = EINVAL;

	pthread_spinunlock(self, &self->pt_lock);

	return retval;
}


int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_spinlock(self, &self->pt_lock);

	if (oldtype != NULL) {
		if (self->pt_flags & PT_FLAG_CS_ASYNC)
			*oldtype = PTHREAD_CANCEL_ASYNCHRONOUS;
		else
			*oldtype = PTHREAD_CANCEL_DEFERRED;
	}

	if (type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		self->pt_flags |= PT_FLAG_CS_ASYNC;
		if (self->pt_cancel) {
			pthread_spinunlock(self, &self->pt_lock);
			pthread_exit(PTHREAD_CANCELED);
		}
	} else if (type == PTHREAD_CANCEL_DEFERRED)
		self->pt_flags &= ~PT_FLAG_CS_ASYNC;
	else
		retval = EINVAL;

	pthread_spinunlock(self, &self->pt_lock);

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

	pthread_spinlock(self, &pthread__queue_lock);
	PTQ_FOREACH(target, &pthread__allqueue, pt_allq)
	    if (target == id)
		    break;
	pthread_spinunlock(self, &pthread__queue_lock);

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
	      pthread_queue_t *queue, const struct timespec *abstime,
	      int cancelpt, const void *hint)
{
	int rv;

	SDPRINTF(("(pthread__park %p) queue %p enter\n", self, queue));

	/*
	 * Wait until we are awoken by a pending unpark operation,
	 * a signal, an unpark posted after we have gone asleep,
	 * or an expired timeout.
	 *
	 * It is fine to test the value of both pt_sleepobj and
	 * pt_sleeponq without holding any locks, because:
	 *
	 * o Only the blocking thread (this thread) ever sets them
	 *   to a non-NULL value.
	 *
	 * o Other threads may set them NULL, but if they do so they
	 *   must also make this thread return from _lwp_park.
	 *
	 * o _lwp_park, _lwp_unpark and _lwp_unpark_all are system
	 *   calls and all make use of spinlocks in the kernel.  So
	 *   these system calls act as full memory barriers, and will
	 *   ensure that the calling CPU's store buffers are drained.
	 *   In combination with the spinlock release before unpark,
	 *   this means that modification of pt_sleepobj/onq by another
	 *   thread will become globally visible before that thread
	 *   schedules an unpark operation on this thread.
	 */
	rv = 0;
	while (self->pt_sleepobj != NULL && rv == 0) {
		if (_lwp_park(abstime, NULL, hint) != 0) {
			switch (rv = errno) {
			case EINTR:
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
		/* Check for cancellation. */
		if (cancelpt && self->pt_cancel) {
			/*
			 * Ensure visibility of the correct value.
			 * _lwp_park/_lwp_wakeup also provide a
			 * barrier.
			 */
			pthread_spinlock(self, &self->pt_lock);
			if (self->pt_cancel)
				rv = EINTR;
			pthread_spinunlock(self, &self->pt_lock);
		}
	}

	/*
	 * If we have been awoken early but are still on the queue,
	 * then remove ourself.  Again, it's safe to do the test
	 * without holding any locks.
	 */
	if (self->pt_sleeponq) {
		pthread_spinlock(self, lock);
		if (self->pt_sleeponq) {
			PTQ_REMOVE(queue, self, pt_sleep);
			self->pt_sleepobj = NULL;
			self->pt_sleeponq = 0;
		}
		pthread_spinunlock(self, lock);
	}

	SDPRINTF(("(pthread__park %p) queue %p exit\n", self, queue));

	return rv;
}

void
pthread__unpark(pthread_t self, pthread_spin_t *lock,
		pthread_queue_t *queue, pthread_t target)
{
	int rv;

	if (target == NULL) {
		pthread_spinunlock(self, lock);
		return;
	}

	SDPRINTF(("(pthread__unpark %p) queue %p target %p\n",
	    self, queue, target));

	/*
	 * Easy: the thread has already been removed from
	 * the queue, so just awaken it.
	 */
	target->pt_sleepobj = NULL;
	target->pt_sleeponq = 0;

	/*
	 * Releasing the spinlock serves as a store barrier,
	 * which ensures that all our modifications are visible
	 * to the thread in pthread__park() before the unpark
	 * operation is set in motion.
	 */
	pthread_spinunlock(self, lock);
	rv = _lwp_unpark(target->pt_lid, queue);

	if (rv != 0 && errno != EALREADY && errno != EINTR) {
		SDPRINTF(("(pthread__unpark %p) syscall rv=%d\n",
		    self, rv));
		OOPS("_lwp_unpark failed");
	}
}

void
pthread__unpark_all(pthread_t self, pthread_spin_t *lock,
		    pthread_queue_t *queue)
{
	lwpid_t waiters[PTHREAD__UNPARK_MAX];
	ssize_t n, rv;
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
		if (thread == PTQ_NEXT(thread, pt_sleep))
			OOPS("unpark: thread linked to self");
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
			 */
			next = PTQ_NEXT(thread, pt_sleep);
			if (thread->pt_sleepobj != NULL)
			    	continue;
			thread->pt_sleeponq = 0;
			waiters[n++] = thread->pt_lid;
			PTQ_REMOVE(queue, thread, pt_sleep);
			SDPRINTF(("(pthread__unpark_all %p) queue %p "
			    "unpark %p\n", self, queue, thread));
		}

		/*
		 * Releasing the spinlock serves as a store barrier,
		 * which ensures that all our modifications are visible
		 * to the thread in pthread__park() before the unpark
		 * operation is set in motion.
		 */
		pthread_spinunlock(self, lock);
		switch (n) {
		case 0:
			return;
		case 1:
			rv = (ssize_t)_lwp_unpark(waiters[0], queue);
			if (rv != 0 && errno != EALREADY && errno != EINTR) {
				OOPS("_lwp_unpark failed");
				SDPRINTF(("(pthread__unpark_all %p) "
				    "syscall rv=%d\n", self, rv));
			}
			return;
		default:
			rv = _lwp_unpark_all(waiters, (size_t)n, queue);
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
