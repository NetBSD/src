/*	$NetBSD: pthread.h,v 1.1.2.11 2002/05/02 16:49:24 nathanw Exp $	*/

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

#ifndef _LIB_PTHREAD_H
#define _LIB_PTHREAD_H


#include <time.h>	/* For timespec */
#include <sched.h>
#include <signal.h>	/* For sigset_t. XXX pthread_sigmask should
			 * be in signal.h instead of here.
			 */
#include "pthread_types.h"
#include "sched.h"

int	pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
	    void *(*startfunc)(void *), void *arg);
void	pthread_exit(void *retval) __attribute__((__noreturn__));
int	pthread_join(pthread_t thread, void **valptr);
int	pthread_equal(pthread_t t1, pthread_t t2);
pthread_t	pthread_self(void);
int	pthread_detach(pthread_t thread);

int	pthread_kill(pthread_t thread, int sig);
int	pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);

int	pthread_attr_destroy(pthread_attr_t *attr);
int	pthread_attr_getdetachstate(pthread_attr_t *attr, int *detachstate);
int	pthread_attr_init(pthread_attr_t *attr);
int	pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int	pthread_attr_setschedparam(pthread_attr_t *attr,
	    const struct sched_param *param);
int	pthread_attr_getschedparam(pthread_attr_t *attr,
	    struct sched_param *param);

int	pthread_mutex_init(pthread_mutex_t *mutex,
	    const pthread_mutexattr_t *attr);
int	pthread_mutex_destroy(pthread_mutex_t *mutex);
int	pthread_mutex_lock(pthread_mutex_t *mutex);
int	pthread_mutex_trylock(pthread_mutex_t *mutex);
int	pthread_mutex_unlock(pthread_mutex_t *mutex);
int	pthread_mutexattr_init(pthread_mutexattr_t *attr);
int	pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

int	pthread_cond_init(pthread_cond_t *cond,
	    const pthread_condattr_t *attr);
int	pthread_cond_destroy(pthread_cond_t *cond);
int	pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int	pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
	    const struct timespec *abstime);
int	pthread_cond_signal(pthread_cond_t *cond);
int	pthread_cond_broadcast(pthread_cond_t *cond);
int	pthread_condattr_init(pthread_condattr_t *attr);
int	pthread_condattr_destroy(pthread_condattr_t *attr);

int	pthread_once(pthread_once_t *once_control, void (*routine)(void));

int	pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int	pthread_key_delete(pthread_key_t key);
int	pthread_setspecific(pthread_key_t key, const void *value);
void*	pthread_getspecific(pthread_key_t key);

int	pthread_cancel(pthread_t thread);
int	pthread_setcancelstate(int state, int *oldstate);
int	pthread_setcanceltype(int type, int *oldtype);
void	pthread_testcancel(void);

struct pthread_cleanup_store {
	void	*pad[4];
};

#define pthread_cleanup_push(routine,arg)			\
        {							\
		struct pthread_cleanup_store __store;	\
		pthread__cleanup_push((routine),(arg), &__store);

#define pthread_cleanup_pop(execute)				\
		pthread__cleanup_pop((execute), &__store);	\
	}

void	pthread__cleanup_push(void (*routine)(void *), void *arg, void *store);
void	pthread__cleanup_pop(int execute, void *store);

int	pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int	pthread_spin_destroy(pthread_spinlock_t *lock);
int	pthread_spin_lock(pthread_spinlock_t *lock);
int	pthread_spin_trylock(pthread_spinlock_t *lock);
int	pthread_spin_unlock(pthread_spinlock_t *lock);

int 	*pthread__errno(void);

#define	PTHREAD_CREATE_JOINABLE	0
#define	PTHREAD_CREATE_DETACHED	1

#define PTHREAD_PROCESS_PRIVATE	0
#define PTHREAD_PROCESS_SHARED	1

#define PTHREAD_CANCEL_DEFERRED		0
#define PTHREAD_CANCEL_ASYNCHRONOUS	1

#define PTHREAD_CANCEL_ENABLE		0
#define PTHREAD_CANCEL_DISABLE		1

/* POSIX 1003.1-2001, section 2.5.9.3: "The symbolic constant
 * PTHREAD_CANCELED expands to a constant expression of type (void *)
 * whose value matches no pointer to an object in memory nor the value
 * NULL."
 */
#define PTHREAD_CANCELED	((void *) 1)

#define	_POSIX_THREADS

#define PTHREAD_DESTRUCTOR_ITERATIONS	4	/* Min. required */
#define PTHREAD_KEYS_MAX	256
#define PTHREAD_STACK_MIN	4096 /* XXX Pulled out of my butt */
#define PTHREAD_THREADS_MAX	64		/* Min. required */

typedef struct pthread_ops_st {
	int (*mutex_init)(pthread_mutex_t *, const pthread_mutexattr_t *);
	int (*mutex_lock)(pthread_mutex_t *);
	int (*mutex_trylock)(pthread_mutex_t *);
	int (*mutex_unlock)(pthread_mutex_t *);
	int (*mutex_destroy)(pthread_mutex_t *);
       	int (*mutexattr_init)(pthread_mutexattr_t *);
	int (*mutexattr_destroy)(pthread_mutexattr_t *);
	
	int (*cond_init)(pthread_cond_t *, const pthread_condattr_t *);
	int (*cond_signal)(pthread_cond_t *);
	int (*cond_broadcast)(pthread_cond_t *);
	int (*cond_wait)(pthread_cond_t *, pthread_mutex_t *);
	int (*cond_timedwait)(pthread_cond_t *, pthread_mutex_t *, 
	    const struct timespec *);
	int (*cond_destroy)(pthread_cond_t *);
       	int (*condattr_init)(pthread_condattr_t *);
	int (*condattr_destroy)(pthread_condattr_t *);

	int (*thr_keycreate)(pthread_key_t *, void (*)(void *));
	int (*thr_setspecific)(pthread_key_t, const void *);
	void *(*thr_getspecific)(pthread_key_t);
	int (*thr_keydelete)(pthread_key_t);

	int (*thr_once)(pthread_once_t *, void (*)(void));

	pthread_t (*thr_self)(void);

	int (*thr_sigsetmask)(int, const sigset_t *, sigset_t *);

	int *(*thr_errno)(void);
} pthread_ops_t;


#endif /* _LIB_PTHREAD_H */
