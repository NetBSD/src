/*	$NetBSD: pthread.h,v 1.13 2003/07/18 15:58:43 lukem Exp $	*/

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

#include <sys/cdefs.h>

#include <time.h>	/* For timespec */
#include <sched.h>

#include <pthread_types.h>

__BEGIN_DECLS
int	pthread_atfork(void (*)(void), void (*)(void), void (*)(void));
int	pthread_create(pthread_t *, const pthread_attr_t *, 
	    void *(*)(void *), void *);
void	pthread_exit(void *) __attribute__((__noreturn__));
int	pthread_join(pthread_t, void **);
int	pthread_equal(pthread_t, pthread_t);
pthread_t	pthread_self(void);
int	pthread_detach(pthread_t);

int	pthread_getrrtimer_np(void);
int	pthread_setrrtimer_np(int);

int	pthread_attr_init(pthread_attr_t *);
int	pthread_attr_destroy(pthread_attr_t *);
int	pthread_attr_getschedparam(const pthread_attr_t *,
    struct sched_param *);
int	pthread_attr_setschedparam(pthread_attr_t *,
    const struct sched_param *);
int	pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int	pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int	pthread_attr_getstackaddr(const pthread_attr_t *, void **);
int	pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int	pthread_attr_setdetachstate(pthread_attr_t *, int);
int	pthread_attr_getname_np(const pthread_attr_t *, char *,
	    size_t, void **);
int	pthread_attr_setname_np(pthread_attr_t *, const char *, void *);

int	pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int	pthread_mutex_destroy(pthread_mutex_t *);
int	pthread_mutex_lock(pthread_mutex_t *);
int	pthread_mutex_trylock(pthread_mutex_t *);
int	pthread_mutex_unlock(pthread_mutex_t *);
int	pthread_mutexattr_init(pthread_mutexattr_t *);
int	pthread_mutexattr_destroy(pthread_mutexattr_t *);
int	pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int	pthread_mutexattr_settype(pthread_mutexattr_t *attr, int);

int	pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int	pthread_cond_destroy(pthread_cond_t *);
int	pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int	pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *,
	    const struct timespec *);
int	pthread_cond_signal(pthread_cond_t *);
int	pthread_cond_broadcast(pthread_cond_t *);
int	pthread_condattr_init(pthread_condattr_t *);
int	pthread_condattr_destroy(pthread_condattr_t *);

int	pthread_once(pthread_once_t *, void (*)(void));

int	pthread_key_create(pthread_key_t *, void (*)(void *));
int	pthread_key_delete(pthread_key_t);
int	pthread_setspecific(pthread_key_t, const void *);
void*	pthread_getspecific(pthread_key_t);

int	pthread_cancel(pthread_t);
int	pthread_setcancelstate(int, int *);
int	pthread_setcanceltype(int, int *);
void	pthread_testcancel(void);

int	pthread_getname_np(pthread_t, char *, size_t);
int	pthread_setname_np(pthread_t, const char *, void *);

struct pthread_cleanup_store {
	void	*pad[4];
};

#define pthread_cleanup_push(routine, arg)			\
        {							\
		struct pthread_cleanup_store __store;		\
		pthread__cleanup_push((routine),(arg), &__store);

#define pthread_cleanup_pop(execute)				\
		pthread__cleanup_pop((execute), &__store);	\
	}

void	pthread__cleanup_push(void (*)(void *), void *, void *);
void	pthread__cleanup_pop(int, void *);

int	pthread_spin_init(pthread_spinlock_t *, int);
int	pthread_spin_destroy(pthread_spinlock_t *);
int	pthread_spin_lock(pthread_spinlock_t *);
int	pthread_spin_trylock(pthread_spinlock_t *);
int	pthread_spin_unlock(pthread_spinlock_t *);

int	pthread_rwlock_init(pthread_rwlock_t *,
	    const pthread_rwlockattr_t *);
int	pthread_rwlock_destroy(pthread_rwlock_t *);
int	pthread_rwlock_rdlock(pthread_rwlock_t *);
int	pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int	pthread_rwlock_wrlock(pthread_rwlock_t *);
int	pthread_rwlock_trywrlock(pthread_rwlock_t *);
int	pthread_rwlock_timedrdlock(pthread_rwlock_t *,
	    const struct timespec *);
int	pthread_rwlock_timedwrlock(pthread_rwlock_t *,
	    const struct timespec *);
int	pthread_rwlock_unlock(pthread_rwlock_t *);
int	pthread_rwlockattr_init(pthread_rwlockattr_t *);
int	pthread_rwlockattr_destroy(pthread_rwlockattr_t *);

int	pthread_barrier_init(pthread_barrier_t *,
	    const pthread_barrierattr_t *, unsigned int);
int	pthread_barrier_wait(pthread_barrier_t *);
int	pthread_barrier_destroy(pthread_barrier_t *);
int	pthread_barrierattr_init(pthread_barrierattr_t *);
int	pthread_barrierattr_destroy(pthread_barrierattr_t *);

int 	*pthread__errno(void);
__END_DECLS

#define	PTHREAD_CREATE_JOINABLE	0
#define	PTHREAD_CREATE_DETACHED	1

#define PTHREAD_PROCESS_PRIVATE	0
#define PTHREAD_PROCESS_SHARED	1

#define PTHREAD_CANCEL_DEFERRED		0
#define PTHREAD_CANCEL_ASYNCHRONOUS	1

#define PTHREAD_CANCEL_ENABLE		0
#define PTHREAD_CANCEL_DISABLE		1

#define PTHREAD_BARRIER_SERIAL_THREAD	1234567

/*
 * POSIX 1003.1-2001, section 2.5.9.3: "The symbolic constant
 * PTHREAD_CANCELED expands to a constant expression of type (void *)
 * whose value matches no pointer to an object in memory nor the value
 * NULL."
 */
#define PTHREAD_CANCELED	((void *) 1)

#define PTHREAD_DESTRUCTOR_ITERATIONS	4	/* Min. required */
#define PTHREAD_KEYS_MAX	256
#define PTHREAD_STACK_MIN	4096 /* XXX Pulled out of my butt */
#define PTHREAD_THREADS_MAX	64		/* Min. required */

/*
 * Maximum length of a thread's name, including the terminating NUL.
 */
#define	PTHREAD_MAX_NAMELEN_NP	32

/*
 * Mutex attributes.
 */
#define	PTHREAD_MUTEX_NORMAL		0
#define	PTHREAD_MUTEX_ERRORCHECK	1
#define	PTHREAD_MUTEX_RECURSIVE		2
#define	PTHREAD_MUTEX_DEFAULT		PTHREAD_MUTEX_NORMAL

#define PTHREAD_COND_INITIALIZER	_PTHREAD_COND_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER	_PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_ONCE_INIT		_PTHREAD_ONCE_INIT
#define PTHREAD_RWLOCK_INITIALIZER	_PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_SPINLOCK_INITIALIZER	_PTHREAD_SPINLOCK_INITIALIZER

#endif /* _LIB_PTHREAD_H */
