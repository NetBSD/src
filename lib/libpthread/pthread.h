/*	$NetBSD: pthread.h,v 1.1.2.5 2001/08/06 20:51:41 nathanw Exp $	*/

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


#include <sys/time.h>	/* For timespec */
#include <signal.h>	/* For sigset_t. XXX perhaps pthread_sigmask should
			 * be in signal.h instead of here.
			 */
#include "pthread_types.h"
#include "sched.h"

int	pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
	    void *(*startfunc)(void *), void *arg);
void	pthread_exit(void *retval);
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


#define	PTHREAD_CREATE_JOINABLE	0
#define	PTHREAD_CREATE_DETACHED	1

#define PTHREAD_PROCESS_PRIVATE	0
#define PTHREAD_PROCESS_SHARED	1

#define	_POSIX_THREADS

/* XXX not actually implemented but required to exist */
#define PTHREAD_DESTRUCTOR_ITERATIONS	4	/* Min. required */
#define	PTHREAD_KEYS_MAX	128		/* Min. required */
#define PTHREAD_STACK_MIN	4096 /* XXX Pulled out of my butt */
#define PTHREAD_THREADS_MAX	64		/* Min. required */


#endif /* _LIB_PTHREAD_H */
