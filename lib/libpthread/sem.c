/*	$NetBSD: sem.c,v 1.19 2007/12/24 16:04:21 ad Exp $	*/

/*-
 * Copyright (c) 2003, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc, and by Andrew Doran.
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

/*
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sem.c,v 1.19 2007/12/24 16:04:21 ad Exp $");

#include <sys/types.h>
#include <sys/ksem.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>

#include "pthread.h"
#include "pthread_int.h"

struct _sem_st {
	unsigned int	usem_magic;
#define	USEM_MAGIC	0x09fa4012

	LIST_ENTRY(_sem_st) usem_list;
	semid_t		usem_semid;	/* 0 -> user (non-shared) */
#define	USEM_USER	0		/* assumes kernel does not use NULL */
	sem_t		*usem_identity;

	/* Protects data below. */
	pthread_mutex_t	usem_interlock;
	pthread_cond_t	usem_cv;
	unsigned int	usem_count;
};

static int sem_alloc(unsigned int value, semid_t semid, sem_t *semp);
static void sem_free(sem_t sem);

static LIST_HEAD(, _sem_st) named_sems = LIST_HEAD_INITIALIZER(&named_sems);
static pthread_mutex_t named_sems_mtx = PTHREAD_MUTEX_INITIALIZER;

static void
sem_free(sem_t sem)
{

	if (sem->usem_semid == USEM_USER) {
		pthread_cond_destroy(&sem->usem_cv);
		pthread_mutex_destroy(&sem->usem_interlock);
	}
	sem->usem_magic = 0;
	free(sem);
}

static int
sem_alloc(unsigned int value, semid_t semid, sem_t *semp)
{
	sem_t sem;

	if (value > SEM_VALUE_MAX)
		return (EINVAL);

	if ((sem = malloc(sizeof(struct _sem_st))) == NULL)
		return (ENOSPC);

	sem->usem_magic = USEM_MAGIC;
	pthread_mutex_init(&sem->usem_interlock, NULL);
	pthread_cond_init(&sem->usem_cv, NULL);
	sem->usem_count = value;
	sem->usem_semid = semid;

	*semp = sem;
	return (0);
}

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	semid_t	semid;
	int error;

	semid = USEM_USER;

	if (pshared && _ksem_init(value, &semid) == -1)
		return (-1);

	if ((error = sem_alloc(value, semid, sem)) != 0) {
		if (semid != USEM_USER)
			_ksem_destroy(semid);
		errno = error;
		return (-1);
	}

	return (0);
}

int
sem_destroy(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	if ((*sem)->usem_semid != USEM_USER) {
		if (_ksem_destroy((*sem)->usem_semid))
			return (-1);
	} else {
		pthread_mutex_lock(&(*sem)->usem_interlock);
		if (!PTQ_EMPTY(&(*sem)->usem_cv.ptc_waiters)) {
			pthread_mutex_unlock(&(*sem)->usem_interlock);
			errno = EBUSY;
			return (-1);
		}
		pthread_mutex_unlock(&(*sem)->usem_interlock);
	}

	sem_free(*sem);

	return (0);
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
	sem_t *sem, s;
	semid_t semid;
	mode_t mode;
	unsigned int value;
	int error;
	va_list ap;

	mode = 0;
	value = 0;

	if (oflag & O_CREAT) {
		va_start(ap, oflag);
		mode = va_arg(ap, int);
		value = va_arg(ap, unsigned int);
		va_end(ap);
	}

	/*
	 * We can be lazy and let the kernel handle the oflag,
	 * we'll just merge duplicate IDs into our list.
	 */
	if (_ksem_open(name, oflag, mode, value, &semid) == -1)
		return (SEM_FAILED);

	/*
	 * Search for a duplicate ID, we must return the same sem_t *
	 * if we locate one.
	 */
	pthread_mutex_lock(&named_sems_mtx);
	LIST_FOREACH(s, &named_sems, usem_list) {
		if (s->usem_semid == semid) {
			pthread_mutex_unlock(&named_sems_mtx);
			return (s->usem_identity);
		}
	}

	if ((sem = malloc(sizeof(*sem))) == NULL) {
		error = ENOSPC;
		goto bad;
	}
	if ((error = sem_alloc(value, semid, sem)) != 0)
		goto bad;

	LIST_INSERT_HEAD(&named_sems, *sem, usem_list);
	(*sem)->usem_identity = sem;
	pthread_mutex_unlock(&named_sems_mtx);

	return (sem);

 bad:
	pthread_mutex_unlock(&named_sems_mtx);
	_ksem_close(semid);
	if (sem != NULL) {
		if (*sem != NULL)
			sem_free(*sem);
		free(sem);
	}
	errno = error;
	return (SEM_FAILED);
}

int
sem_close(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	if ((*sem)->usem_semid == USEM_USER) {
		errno = EINVAL;
		return (-1);
	}

	pthread_mutex_lock(&named_sems_mtx);
	if (_ksem_close((*sem)->usem_semid) == -1) {
		pthread_mutex_unlock(&named_sems_mtx);
		return (-1);
	}
	LIST_REMOVE((*sem), usem_list);
	pthread_mutex_unlock(&named_sems_mtx);
	sem_free(*sem);
	free(sem);
	return (0);
}

int
sem_unlink(const char *name)
{

	return (_ksem_unlink(name));
}

int
sem_wait(sem_t *sem)
{
	pthread_t self;
	extern int pthread__started;

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	self = pthread__self();

	if ((*sem)->usem_semid != USEM_USER) {
		pthread__testcancel(self);
		return (_ksem_wait((*sem)->usem_semid));
	}

	if (pthread__started == 0) {
		sigset_t set, oset;

		sigfillset(&set);
		(void) sigprocmask(SIG_SETMASK, &set, &oset);
		for (;;) {
			if ((*sem)->usem_count > 0) {
				break;
			}
			(void) sigsuspend(&oset);
		}
		(*sem)->usem_count--;
		(void) sigprocmask(SIG_SETMASK, &oset, NULL);
		return 0;
	}

	pthread_mutex_lock(&(*sem)->usem_interlock);
	for (;;) {
		if (self->pt_cancel) {
			pthread_mutex_unlock(&(*sem)->usem_interlock);
			pthread__cancelled();
		}
		if ((*sem)->usem_count > 0)
			break;
		(void)pthread_cond_wait(&(*sem)->usem_cv,
		    &(*sem)->usem_interlock);
	}
	(*sem)->usem_count--;
	pthread_mutex_unlock(&(*sem)->usem_interlock);

	return (0);
}

int
sem_trywait(sem_t *sem)
{
	extern int pthread__started;

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	if ((*sem)->usem_semid != USEM_USER)
		return (_ksem_trywait((*sem)->usem_semid));

	if (pthread__started == 0) {
		sigset_t set, oset;
		int rv = 0;

		sigfillset(&set);
		(void) sigprocmask(SIG_SETMASK, &set, &oset);
		if ((*sem)->usem_count > 0) {
			(*sem)->usem_count--;
		} else {
			errno = EAGAIN;
			rv = -1;
		}
		(void) sigprocmask(SIG_SETMASK, &oset, NULL);
		return rv;
	}

	pthread_mutex_lock(&(*sem)->usem_interlock);
	if ((*sem)->usem_count == 0) {
		pthread_mutex_unlock(&(*sem)->usem_interlock);
		errno = EAGAIN;
		return (-1);
	}
	(*sem)->usem_count--;
	pthread_mutex_unlock(&(*sem)->usem_interlock);

	return (0);
}

int
sem_post(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	if ((*sem)->usem_semid != USEM_USER)
		return (_ksem_post((*sem)->usem_semid));

	pthread_mutex_lock(&(*sem)->usem_interlock);
	(*sem)->usem_count++;
	pthread_cond_signal(&(*sem)->usem_cv);
	pthread_mutex_unlock(&(*sem)->usem_interlock);

	return (0);
}

int
sem_getvalue(sem_t * __restrict sem, int * __restrict sval)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->usem_magic != USEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif
	if ((*sem)->usem_semid != USEM_USER)
		return (_ksem_getvalue((*sem)->usem_semid, sval));

	pthread_mutex_lock(&(*sem)->usem_interlock);
	*sval = (int) (*sem)->usem_count;
	pthread_mutex_unlock(&(*sem)->usem_interlock);

	return (0);
}
