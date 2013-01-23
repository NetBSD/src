/*	$NetBSD: pthread.c,v 1.125.2.5 2013/01/23 00:05:26 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2003, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread.c,v 1.125.2.5 2013/01/23 00:05:26 yamt Exp $");

#define	__EXPOSE_STACK	1

#include <sys/param.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/lwp.h>
#include <sys/lwpctl.h>
#include <sys/resource.h>
#include <sys/tls.h>

#include <assert.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <lwp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <ucontext.h>
#include <unistd.h>
#include <sched.h>

#include "pthread.h"
#include "pthread_int.h"

pthread_rwlock_t pthread__alltree_lock = PTHREAD_RWLOCK_INITIALIZER;
static rb_tree_t	pthread__alltree;

static signed int	pthread__cmp(void *, const void *, const void *);

static const rb_tree_ops_t pthread__alltree_ops = {
	.rbto_compare_nodes = pthread__cmp,
	.rbto_compare_key = pthread__cmp,
	.rbto_node_offset = offsetof(struct __pthread_st, pt_alltree),
	.rbto_context = NULL
};

static void	pthread__create_tramp(void *);
static void	pthread__initthread(pthread_t);
static void	pthread__scrubthread(pthread_t, char *, int);
static void	pthread__initmain(pthread_t *);
static void	pthread__fork_callback(void);
static void	pthread__reap(pthread_t);
static void	pthread__child_callback(void);
static void	pthread__start(void);

void	pthread__init(void);

int pthread__started;
pthread_mutex_t pthread__deadqueue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_queue_t pthread__deadqueue;
pthread_queue_t pthread__allqueue;

static pthread_attr_t pthread_default_attr;
static lwpctl_t pthread__dummy_lwpctl = { .lc_curcpu = LWPCTL_CPU_NONE };

enum {
	DIAGASSERT_ABORT =	1<<0,
	DIAGASSERT_STDERR =	1<<1,
	DIAGASSERT_SYSLOG =	1<<2
};

static int pthread__diagassert;

int pthread__concurrency;
int pthread__nspins;
int pthread__unpark_max = PTHREAD__UNPARK_MAX;
int pthread__dbg;	/* set by libpthread_dbg if active */

/* 
 * We have to initialize the pthread_stack* variables here because
 * mutexes are used before pthread_init() and thus pthread__initmain()
 * are called.  Since mutexes only save the stack pointer and not a
 * pointer to the thread data, it is safe to change the mapping from
 * stack pointer to thread data afterwards.
 */
size_t	pthread__stacksize;
size_t	pthread__pagesize;
static struct __pthread_st pthread__main;

int _sys___sigprocmask14(int, const sigset_t *, sigset_t *);

__strong_alias(__libc_thr_self,pthread_self)
__strong_alias(__libc_thr_create,pthread_create)
__strong_alias(__libc_thr_exit,pthread_exit)
__strong_alias(__libc_thr_errno,pthread__errno)
__strong_alias(__libc_thr_setcancelstate,pthread_setcancelstate)
__strong_alias(__libc_thr_equal,pthread_equal)
__strong_alias(__libc_thr_init,pthread__init)

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

#define	NHASHLOCK	64

static union hashlock {
	pthread_mutex_t	mutex;
	char		pad[64];
} hashlocks[NHASHLOCK] __aligned(64);

/*
 * This needs to be started by the library loading code, before main()
 * gets to run, for various things that use the state of the initial thread
 * to work properly (thread-specific data is an application-visible example;
 * spinlock counts for mutexes is an internal example).
 */
void
pthread__init(void)
{
	pthread_t first;
	char *p;
	int i;
	extern int __isthreaded;

	pthread__pagesize = (size_t)sysconf(_SC_PAGESIZE);
	pthread__concurrency = (int)sysconf(_SC_NPROCESSORS_CONF);

	/* Initialize locks first; they're needed elsewhere. */
	pthread__lockprim_init();
	for (i = 0; i < NHASHLOCK; i++) {
		pthread_mutex_init(&hashlocks[i].mutex, NULL);
	}

	/* Fetch parameters. */
	i = (int)_lwp_unpark_all(NULL, 0, NULL);
	if (i == -1)
		err(1, "_lwp_unpark_all");
	if (i < pthread__unpark_max)
		pthread__unpark_max = i;

	/* Basic data structure setup */
	pthread_attr_init(&pthread_default_attr);
	PTQ_INIT(&pthread__allqueue);
	PTQ_INIT(&pthread__deadqueue);

	rb_tree_init(&pthread__alltree, &pthread__alltree_ops);

	/* Create the thread structure corresponding to main() */
	pthread__initmain(&first);
	pthread__initthread(first);
	pthread__scrubthread(first, NULL, 0);

	first->pt_lid = _lwp_self();
	PTQ_INSERT_HEAD(&pthread__allqueue, first, pt_allq);
	(void)rb_tree_insert_node(&pthread__alltree, first);

	if (_lwp_ctl(LWPCTL_FEATURE_CURCPU, &first->pt_lwpctl) != 0) {
		err(1, "_lwp_ctl");
	}

	/* Start subsystems */
	PTHREAD_MD_INIT

	for (p = pthread__getenv("PTHREAD_DIAGASSERT"); p && *p; p++) {
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
	pthread_atfork(NULL, NULL, pthread__fork_callback);
	__isthreaded = 1;
}

static void
pthread__fork_callback(void)
{
	struct __pthread_st *self = pthread__self();

	/* lwpctl state is not copied across fork. */
	if (_lwp_ctl(LWPCTL_FEATURE_CURCPU, &self->pt_lwpctl)) {
		err(1, "_lwp_ctl");
	}
	self->pt_lid = _lwp_self();
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

	/*
	 * Per-process timers are cleared by fork(); despite the
	 * various restrictions on fork() and threads, it's legal to
	 * fork() before creating any threads. 
	 */
	pthread_atfork(NULL, NULL, pthread__child_callback);
}


/* General-purpose thread data structure sanitization. */
/* ARGSUSED */
static void
pthread__initthread(pthread_t t)
{

	t->pt_self = t;
	t->pt_magic = PT_MAGIC;
	t->pt_willpark = 0;
	t->pt_unpark = 0;
	t->pt_nwaiters = 0;
	t->pt_sleepobj = NULL;
	t->pt_signalled = 0;
	t->pt_havespecific = 0;
	t->pt_early = NULL;
	t->pt_lwpctl = &pthread__dummy_lwpctl;
	t->pt_blocking = 0;
	t->pt_droplock = NULL;

	memcpy(&t->pt_lockops, pthread__lock_ops, sizeof(t->pt_lockops));
	pthread_mutex_init(&t->pt_lock, NULL);
	PTQ_INIT(&t->pt_cleanup_stack);
	pthread_cond_init(&t->pt_joiners, NULL);
	memset(&t->pt_specific, 0, sizeof(t->pt_specific));
}

static void
pthread__scrubthread(pthread_t t, char *name, int flags)
{

	t->pt_state = PT_STATE_RUNNING;
	t->pt_exitval = NULL;
	t->pt_flags = flags;
	t->pt_cancel = 0;
	t->pt_errno = 0;
	t->pt_name = name;
	t->pt_lid = 0;
}

static int
pthread__getstack(pthread_t newthread, const pthread_attr_t *attr)
{
	void *stackbase, *stackbase2, *redzone;
	size_t stacksize, guardsize;
	bool allocated;

	if (attr != NULL) {
		pthread_attr_getstack(attr, &stackbase, &stacksize);
	} else {
		stackbase = NULL;
		stacksize = 0;
	}
	if (stacksize == 0)
		stacksize = pthread__stacksize;

	if (newthread->pt_stack_allocated) {
		if (stackbase == NULL &&
		    newthread->pt_stack.ss_size == stacksize)
			return 0;
		stackbase2 = newthread->pt_stack.ss_sp;
#ifndef __MACHINE_STACK_GROWS_UP
		stackbase2 = (char *)stackbase2 - newthread->pt_guardsize;
#endif
		munmap(stackbase2,
		    newthread->pt_stack.ss_size + newthread->pt_guardsize);
		newthread->pt_stack.ss_sp = NULL;
		newthread->pt_stack.ss_size = 0;
		newthread->pt_guardsize = 0;
		newthread->pt_stack_allocated = false;
	}

	newthread->pt_stack_allocated = false;

	if (stackbase == NULL) {
		stacksize = ((stacksize - 1) | (pthread__pagesize - 1)) + 1;
		guardsize = pthread__pagesize;
		stackbase = mmap(NULL, stacksize + guardsize,
		    PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, (off_t)0);
		if (stackbase == MAP_FAILED)
			return ENOMEM;
		allocated = true;
	} else {
		guardsize = 0;
		allocated = false;
	}
#ifdef __MACHINE_STACK_GROWS_UP
	redzone = (char *)stackbase + stacksize;
	stackbase2 = (char *)stackbase;
#else
	redzone = (char *)stackbase;
	stackbase2 = (char *)stackbase + guardsize;
#endif
	if (allocated && guardsize &&
	    mprotect(redzone, guardsize, PROT_NONE) == -1) {
		munmap(stackbase, stacksize + guardsize);
		return EPERM;
	}
	newthread->pt_stack.ss_size = stacksize;
	newthread->pt_stack.ss_sp = stackbase2;
	newthread->pt_guardsize = guardsize;
	newthread->pt_stack_allocated = allocated;
	return 0;
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	    void *(*startfunc)(void *), void *arg)
{
	pthread_t newthread;
	pthread_attr_t nattr;
	struct pthread_attr_private *p;
	char * volatile name;
	unsigned long flag;
	void *private_area;
	int ret;

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

	newthread = NULL;

	/*
	 * Try to reclaim a dead thread.
	 */
	if (!PTQ_EMPTY(&pthread__deadqueue)) {
		pthread_mutex_lock(&pthread__deadqueue_lock);
		PTQ_FOREACH(newthread, &pthread__deadqueue, pt_deadq) {
			/* Still running? */
			if (newthread->pt_lwpctl->lc_curcpu ==
			    LWPCTL_CPU_EXITED ||
			    (_lwp_kill(newthread->pt_lid, 0) == -1 &&
			    errno == ESRCH))
				break;
		}
		if (newthread)
			PTQ_REMOVE(&pthread__deadqueue, newthread, pt_deadq);
		pthread_mutex_unlock(&pthread__deadqueue_lock);
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
		if (newthread && newthread->pt_tls) {
			_rtld_tls_free(newthread->pt_tls);
			newthread->pt_tls = NULL;
		}
#endif
	}

	/*
	 * If necessary set up a stack, allocate space for a pthread_st,
	 * and initialize it.
	 */
	if (newthread == NULL) {
		newthread = malloc(sizeof(*newthread));
		if (newthread == NULL) {
			free(name);
			return ENOMEM;
		}
		newthread->pt_stack_allocated = false;

		if (pthread__getstack(newthread, attr)) {
			free(newthread);
			free(name);
			return ENOMEM;
		}

		/* This is used only when creating the thread. */
		_INITCONTEXT_U(&newthread->pt_uc);
		newthread->pt_uc.uc_stack = newthread->pt_stack;
		newthread->pt_uc.uc_link = NULL;
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
		newthread->pt_tls = NULL;
#endif

		/* Add to list of all threads. */
		pthread_rwlock_wrlock(&pthread__alltree_lock);
		PTQ_INSERT_TAIL(&pthread__allqueue, newthread, pt_allq);
		(void)rb_tree_insert_node(&pthread__alltree, newthread);
		pthread_rwlock_unlock(&pthread__alltree_lock);

		/* Will be reset by the thread upon exit. */
		pthread__initthread(newthread);
	} else {
		if (pthread__getstack(newthread, attr)) {
			pthread_mutex_lock(&pthread__deadqueue_lock);
			PTQ_INSERT_TAIL(&pthread__deadqueue, newthread, pt_deadq);
			pthread_mutex_unlock(&pthread__deadqueue_lock);
			return ENOMEM;
		}
		_INITCONTEXT_U(&newthread->pt_uc);
		newthread->pt_uc.uc_stack = newthread->pt_stack;
		newthread->pt_uc.uc_link = NULL;
	}

	/*
	 * Create the new LWP.
	 */
	pthread__scrubthread(newthread, name, nattr.pta_flags);
	newthread->pt_func = startfunc;
	newthread->pt_arg = arg;
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	private_area = newthread->pt_tls = _rtld_tls_allocate();
	newthread->pt_tls->tcb_pthread = newthread;
#else
	private_area = newthread;
#endif

	_lwp_makecontext(&newthread->pt_uc, pthread__create_tramp,
	    newthread, private_area, newthread->pt_stack.ss_sp,
	    newthread->pt_stack.ss_size);

	flag = LWP_DETACHED;
	if ((newthread->pt_flags & PT_FLAG_SUSPENDED) != 0 ||
	    (nattr.pta_flags & PT_FLAG_EXPLICIT_SCHED) != 0)
		flag |= LWP_SUSPENDED;
	ret = _lwp_create(&newthread->pt_uc, flag, &newthread->pt_lid);
	if (ret != 0) {
		ret = errno;
		pthread_mutex_lock(&newthread->pt_lock);
		/* Will unlock and free name. */
		pthread__reap(newthread);
		return ret;
	}

	if ((nattr.pta_flags & PT_FLAG_EXPLICIT_SCHED) != 0) {
		if (p != NULL) {
			(void)pthread_setschedparam(newthread, p->ptap_policy,
			    &p->ptap_sp);
		}
		if ((newthread->pt_flags & PT_FLAG_SUSPENDED) == 0) {
			(void)_lwp_continue(newthread->pt_lid);
		}
	}

	*thread = newthread;

	return 0;
}


__dead static void
pthread__create_tramp(void *cookie)
{
	pthread_t self;
	void *retval;

	self = cookie;

	/*
	 * Throw away some stack in a feeble attempt to reduce cache
	 * thrash.  May help for SMT processors.  XXX We should not
	 * be allocating stacks on fixed 2MB boundaries.  Needs a
	 * thread register or decent thread local storage.
	 *
	 * Note that we may race with the kernel in _lwp_create(),
	 * and so pt_lid can be unset at this point, but we don't
	 * care.
	 */
	(void)alloca(((unsigned)self->pt_lid & 7) << 8);

	if (self->pt_name != NULL) {
		pthread_mutex_lock(&self->pt_lock);
		if (self->pt_name != NULL)
			(void)_lwp_setname(0, self->pt_name);
		pthread_mutex_unlock(&self->pt_lock);
	}

	if (_lwp_ctl(LWPCTL_FEATURE_CURCPU, &self->pt_lwpctl)) {
		err(1, "_lwp_ctl");
	}

	retval = (*self->pt_func)(self->pt_arg);

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
	if (pthread__find(thread) != 0)
		return ESRCH;
	if (_lwp_suspend(thread->pt_lid) == 0)
		return 0;
	return errno;
}

int
pthread_resume_np(pthread_t thread)
{
 
	if (pthread__find(thread) != 0)
		return ESRCH;
	if (_lwp_continue(thread->pt_lid) == 0)
		return 0;
	return errno;
}

void
pthread_exit(void *retval)
{
	pthread_t self;
	struct pt_clean_t *cleanup;
	char *name;

	self = pthread__self();

	/* Disable cancellability. */
	pthread_mutex_lock(&self->pt_lock);
	self->pt_flags |= PT_FLAG_CS_DISABLED;
	self->pt_cancel = 0;

	/* Call any cancellation cleanup handlers */
	if (!PTQ_EMPTY(&self->pt_cleanup_stack)) {
		pthread_mutex_unlock(&self->pt_lock);
		while (!PTQ_EMPTY(&self->pt_cleanup_stack)) {
			cleanup = PTQ_FIRST(&self->pt_cleanup_stack);
			PTQ_REMOVE(&self->pt_cleanup_stack, cleanup, ptc_next);
			(*cleanup->ptc_cleanup)(cleanup->ptc_arg);
		}
		pthread_mutex_lock(&self->pt_lock);
	}

	/* Perform cleanup of thread-specific data */
	pthread__destroy_tsd(self);

	/* Signal our exit. */
	self->pt_exitval = retval;
	if (self->pt_flags & PT_FLAG_DETACHED) {
		self->pt_state = PT_STATE_DEAD;
		name = self->pt_name;
		self->pt_name = NULL;
		pthread_mutex_unlock(&self->pt_lock);
		if (name != NULL)
			free(name);
		pthread_mutex_lock(&pthread__deadqueue_lock);
		PTQ_INSERT_TAIL(&pthread__deadqueue, self, pt_deadq);
		pthread_mutex_unlock(&pthread__deadqueue_lock);
		_lwp_exit();
	} else {
		self->pt_state = PT_STATE_ZOMBIE;
		pthread_cond_broadcast(&self->pt_joiners);
		pthread_mutex_unlock(&self->pt_lock);
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
	int error;

	self = pthread__self();

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	if (thread == self)
		return EDEADLK;

	self->pt_droplock = &thread->pt_lock;
	pthread_mutex_lock(&thread->pt_lock);
	for (;;) {
		if (thread->pt_state == PT_STATE_ZOMBIE)
			break;
		if (thread->pt_state == PT_STATE_DEAD) {
			pthread_mutex_unlock(&thread->pt_lock);
			self->pt_droplock = NULL;
			return ESRCH;
		}
		if ((thread->pt_flags & PT_FLAG_DETACHED) != 0) {
			pthread_mutex_unlock(&thread->pt_lock);
			self->pt_droplock = NULL;
			return EINVAL;
		}
		error = pthread_cond_wait(&thread->pt_joiners,
		    &thread->pt_lock);
		if (error != 0) {
			pthread__errorfunc(__FILE__, __LINE__,
			    __func__, "unexpected return from cond_wait()");
		}

	}
	pthread__testcancel(self);
	if (valptr != NULL)
		*valptr = thread->pt_exitval;
	/* pthread__reap() will drop the lock. */
	pthread__reap(thread);
	self->pt_droplock = NULL;

	return 0;
}

static void
pthread__reap(pthread_t thread)
{
	char *name;

	name = thread->pt_name;
	thread->pt_name = NULL;
	thread->pt_state = PT_STATE_DEAD;
	pthread_mutex_unlock(&thread->pt_lock);

	pthread_mutex_lock(&pthread__deadqueue_lock);
	PTQ_INSERT_HEAD(&pthread__deadqueue, thread, pt_deadq);
	pthread_mutex_unlock(&pthread__deadqueue_lock);

	if (name != NULL)
		free(name);
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

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	pthread_mutex_lock(&thread->pt_lock);
	thread->pt_flags |= PT_FLAG_DETACHED;
	if (thread->pt_state == PT_STATE_ZOMBIE) {
		/* pthread__reap() will drop the lock. */
		pthread__reap(thread);
	} else {
		/*
		 * Not valid for threads to be waiting in
		 * pthread_join() (there are intractable
		 * sync issues from the application
		 * perspective), but give those threads
		 * a chance anyway.
		 */
		pthread_cond_broadcast(&thread->pt_joiners);
		pthread_mutex_unlock(&thread->pt_lock);
	}

	return 0;
}


int
pthread_getname_np(pthread_t thread, char *name, size_t len)
{

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	pthread_mutex_lock(&thread->pt_lock);
	if (thread->pt_name == NULL)
		name[0] = '\0';
	else
		strlcpy(name, thread->pt_name, len);
	pthread_mutex_unlock(&thread->pt_lock);

	return 0;
}


int
pthread_setname_np(pthread_t thread, const char *name, void *arg)
{
	char *oldname, *cp, newname[PTHREAD_MAX_NAMELEN_NP];
	int namelen;

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (thread->pt_magic != PT_MAGIC)
		return EINVAL;

	namelen = snprintf(newname, sizeof(newname), name, arg);
	if (namelen >= PTHREAD_MAX_NAMELEN_NP)
		return EINVAL;

	cp = strdup(newname);
	if (cp == NULL)
		return ENOMEM;

	pthread_mutex_lock(&thread->pt_lock);
	oldname = thread->pt_name;
	thread->pt_name = cp;
	(void)_lwp_setname(thread->pt_lid, cp);
	pthread_mutex_unlock(&thread->pt_lock);

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

	if (pthread__find(thread) != 0)
		return ESRCH;
	pthread_mutex_lock(&thread->pt_lock);
	thread->pt_flags |= PT_FLAG_CS_PENDING;
	if ((thread->pt_flags & PT_FLAG_CS_DISABLED) == 0) {
		thread->pt_cancel = 1;
		pthread_mutex_unlock(&thread->pt_lock);
		_lwp_wakeup(thread->pt_lid);
	} else
		pthread_mutex_unlock(&thread->pt_lock);

	return 0;
}


int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_mutex_lock(&self->pt_lock);

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
				pthread_mutex_unlock(&self->pt_lock);
				pthread__cancelled();
			}
		}
	} else
		retval = EINVAL;

	pthread_mutex_unlock(&self->pt_lock);

	return retval;
}


int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	retval = 0;

	pthread_mutex_lock(&self->pt_lock);

	if (oldtype != NULL) {
		if (self->pt_flags & PT_FLAG_CS_ASYNC)
			*oldtype = PTHREAD_CANCEL_ASYNCHRONOUS;
		else
			*oldtype = PTHREAD_CANCEL_DEFERRED;
	}

	if (type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		self->pt_flags |= PT_FLAG_CS_ASYNC;
		if (self->pt_cancel) {
			pthread_mutex_unlock(&self->pt_lock);
			pthread__cancelled();
		}
	} else if (type == PTHREAD_CANCEL_DEFERRED)
		self->pt_flags &= ~PT_FLAG_CS_ASYNC;
	else
		retval = EINVAL;

	pthread_mutex_unlock(&self->pt_lock);

	return retval;
}


void
pthread_testcancel(void)
{
	pthread_t self;

	self = pthread__self();
	if (self->pt_cancel)
		pthread__cancelled();
}


/*
 * POSIX requires that certain functions return an error rather than
 * invoking undefined behavior even when handed completely bogus
 * pthread_t values, e.g. stack garbage.
 */
int
pthread__find(pthread_t id)
{
	pthread_t target;
	int error;

	pthread_rwlock_rdlock(&pthread__alltree_lock);
	target = rb_tree_find_node(&pthread__alltree, id);
	error = (target && target->pt_state != PT_STATE_DEAD) ? 0 : ESRCH;
	pthread_rwlock_unlock(&pthread__alltree_lock);

	return error;
}


void
pthread__testcancel(pthread_t self)
{

	if (self->pt_cancel)
		pthread__cancelled();
}


void
pthread__cancelled(void)
{
	pthread_mutex_t *droplock;
	pthread_t self;

	self = pthread__self();
	droplock = self->pt_droplock;
	self->pt_droplock = NULL;

	if (droplock != NULL && pthread_mutex_held_np(droplock))
		pthread_mutex_unlock(droplock);

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
pthread__park(pthread_t self, pthread_mutex_t *lock,
	      pthread_queue_t *queue, const struct timespec *abstime,
	      int cancelpt, const void *hint)
{
	int rv, error;
	void *obj;

	/*
	 * For non-interlocked release of mutexes we need a store
	 * barrier before incrementing pt_blocking away from zero. 
	 * This is provided by pthread_mutex_unlock().
	 */
	self->pt_willpark = 1;
	pthread_mutex_unlock(lock);
	self->pt_willpark = 0;
	self->pt_blocking++;

	/*
	 * Wait until we are awoken by a pending unpark operation,
	 * a signal, an unpark posted after we have gone asleep,
	 * or an expired timeout.
	 *
	 * It is fine to test the value of pt_sleepobj without
	 * holding any locks, because:
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
	 *
	 * Note: the test in the while() statement dodges the park op if
	 * we have already been awoken, unless there is another thread to
	 * awaken.  This saves a syscall - if we were already awakened,
	 * the next call to _lwp_park() would need to return early in order
	 * to eat the previous wakeup.
	 */
	rv = 0;
	do {
		/*
		 * If we deferred unparking a thread, arrange to
		 * have _lwp_park() restart it before blocking.
		 */
		error = _lwp_park(abstime, self->pt_unpark, hint, hint);
		self->pt_unpark = 0;
		if (error != 0) {
			switch (rv = errno) {
			case EINTR:
			case EALREADY:
				rv = 0;
				break;
			case ETIMEDOUT:
				break;
			default:
				OOPS("_lwp_park failed");
				break;
			}
		}
		/* Check for cancellation. */
		if (cancelpt && self->pt_cancel)
			rv = EINTR;
	} while (self->pt_sleepobj != NULL && rv == 0);

	/*
	 * If we have been awoken early but are still on the queue,
	 * then remove ourself.  Again, it's safe to do the test
	 * without holding any locks.
	 */
	if (__predict_false(self->pt_sleepobj != NULL)) {
		pthread_mutex_lock(lock);
		if ((obj = self->pt_sleepobj) != NULL) {
			PTQ_REMOVE(queue, self, pt_sleep);
			self->pt_sleepobj = NULL;
			if (obj != NULL && self->pt_early != NULL)
				(*self->pt_early)(obj);
		}
		pthread_mutex_unlock(lock);
	}
	self->pt_early = NULL;
	self->pt_blocking--;
	membar_sync();

	return rv;
}

void
pthread__unpark(pthread_queue_t *queue, pthread_t self,
		pthread_mutex_t *interlock)
{
	pthread_t target;
	u_int max;
	size_t nwaiters;

	max = pthread__unpark_max;
	nwaiters = self->pt_nwaiters;
	target = PTQ_FIRST(queue);
	if (nwaiters == max) {
		/* Overflow. */
		(void)_lwp_unpark_all(self->pt_waiters, nwaiters,
		    __UNVOLATILE(&interlock->ptm_waiters));
		nwaiters = 0;
	}
	target->pt_sleepobj = NULL;
	self->pt_waiters[nwaiters++] = target->pt_lid;
	PTQ_REMOVE(queue, target, pt_sleep);
	self->pt_nwaiters = nwaiters;
	pthread__mutex_deferwake(self, interlock);
}

void
pthread__unpark_all(pthread_queue_t *queue, pthread_t self,
		    pthread_mutex_t *interlock)
{
	pthread_t target;
	u_int max;
	size_t nwaiters;

	max = pthread__unpark_max;
	nwaiters = self->pt_nwaiters;
	PTQ_FOREACH(target, queue, pt_sleep) {
		if (nwaiters == max) {
			/* Overflow. */
			(void)_lwp_unpark_all(self->pt_waiters, nwaiters,
			    __UNVOLATILE(&interlock->ptm_waiters));
			nwaiters = 0;
		}
		target->pt_sleepobj = NULL;
		self->pt_waiters[nwaiters++] = target->pt_lid;
	}
	self->pt_nwaiters = nwaiters;
	PTQ_INIT(queue);
	pthread__mutex_deferwake(self, interlock);
}

#undef	OOPS

static void
pthread__initmainstack(void)
{
	struct rlimit slimit;
	const AuxInfo *aux;
	size_t size;

	_DIAGASSERT(_dlauxinfo() != NULL);

	if (getrlimit(RLIMIT_STACK, &slimit) == -1)
		err(1, "Couldn't get stack resource consumption limits");
	size = slimit.rlim_cur;
	pthread__main.pt_stack.ss_size = size;

	for (aux = _dlauxinfo(); aux->a_type != AT_NULL; ++aux) {
		if (aux->a_type == AT_STACKBASE) {
			pthread__main.pt_stack.ss_sp = (void *)aux->a_v;
#ifdef __MACHINE_STACK_GROWS_UP
			pthread__main.pt_stack.ss_sp = (void *)aux->a_v;
#else
			pthread__main.pt_stack.ss_sp = (char *)aux->a_v - size;
#endif
			break;
		}
	}
}

/*
 * Set up the slightly special stack for the "initial" thread, which
 * runs on the normal system stack, and thus gets slightly different
 * treatment.
 */
static void
pthread__initmain(pthread_t *newt)
{
	char *value;

	pthread__initmainstack();

	value = pthread__getenv("PTHREAD_STACKSIZE");
	if (value != NULL) {
		pthread__stacksize = atoi(value) * 1024;
		if (pthread__stacksize > pthread__main.pt_stack.ss_size)
			pthread__stacksize = pthread__main.pt_stack.ss_size;
	}
	if (pthread__stacksize == 0)
		pthread__stacksize = pthread__main.pt_stack.ss_size;
	pthread__stacksize += pthread__pagesize - 1;
	pthread__stacksize &= ~(pthread__pagesize - 1);
	if (pthread__stacksize < 4 * pthread__pagesize)
		errx(1, "Stacksize limit is too low, minimum %zd kbyte.",
		    4 * pthread__pagesize / 1024);

	*newt = &pthread__main;
#ifdef __HAVE___LWP_GETTCB_FAST
	pthread__main.pt_tls = __lwp_gettcb_fast();
#else
	pthread__main.pt_tls = _lwp_getprivate();
#endif
	pthread__main.pt_tls->tcb_pthread = &pthread__main;
}

static signed int
/*ARGSUSED*/
pthread__cmp(void *ctx, const void *n1, const void *n2)
{
	const uintptr_t p1 = (const uintptr_t)n1;
	const uintptr_t p2 = (const uintptr_t)n2;

	if (p1 < p2)
		return -1;
	if (p1 > p2)
		return 1;
	return 0;
}

/* Because getenv() wants to use locks. */
char *
pthread__getenv(const char *name)
{
	extern char **environ;
	size_t l_name, offset;

	l_name = strlen(name);
	for (offset = 0; environ[offset] != NULL; offset++) {
		if (strncmp(name, environ[offset], l_name) == 0 &&
		    environ[offset][l_name] == '=') {
			return environ[offset] + l_name + 1;
		}
	}

	return NULL;
}

pthread_mutex_t *
pthread__hashlock(volatile const void *p)
{
	uintptr_t v;

	v = (uintptr_t)p;
	return &hashlocks[((v >> 9) ^ (v >> 3)) & (NHASHLOCK - 1)].mutex;
}

int
pthread__checkpri(int pri)
{
	static int havepri;
	static long min, max;

	if (!havepri) {
		min = sysconf(_SC_SCHED_PRI_MIN);
		max = sysconf(_SC_SCHED_PRI_MAX);
		havepri = 1;
	}
	return (pri < min || pri > max) ? EINVAL : 0;
}
