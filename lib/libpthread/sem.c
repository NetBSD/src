/* $NetBSD: sem.c,v 1.1 2003/01/20 20:10:20 christos Exp $ */

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
 *
 * $FreeBSD: src/lib/libc/sys/sem.c,v 1.3 2003/01/14 03:36:45 tjr Exp $
 */

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/queue.h>
#include <sys/ksem.h>


/*
 * Semaphore definitions.
 */
struct _sem_st {
#define SEM_MAGIC       ((u_int32_t) 0x09fa4012)
        u_int32_t       magic;
        pthread_mutex_t lock;
        pthread_cond_t  gtzero;
        u_int32_t       count;
        u_int32_t       nwaiters;
#define SEM_USER        (NULL)
        semid_t         semid;  /* semaphore id if kernel (shared) semaphore */
        int             syssem; /* 1 if kernel (shared) semaphore */
        LIST_ENTRY(sem) entry;
        struct _sem_st  **backpointer;
};


#define _SEM_CHECK_VALIDITY(sem)		\
	if ((*(sem))->magic != SEM_MAGIC) {	\
		errno = EINVAL;			\
		retval = -1;			\
		goto RETURN;			\
	}

static sem_t sem_alloc(unsigned int value, semid_t semid, int system_sem);
static void sem_free(sem_t sem);

static LIST_HEAD(, sem) named_sems = LIST_HEAD_INITIALIZER(&named_sems);
static pthread_mutex_t named_sems_mtx = PTHREAD_MUTEX_INITIALIZER;

static void
sem_free(sem_t sem)
{

	pthread_mutex_destroy(&sem->lock);
	pthread_cond_destroy(&sem->gtzero);
	sem->magic = 0;
	free(sem);
}

static sem_t
sem_alloc(unsigned int value, semid_t semid, int system_sem)
{
	sem_t sem;

	if (value > SEM_VALUE_MAX) {
		errno = EINVAL;
		return (NULL);
	}

	sem = (sem_t)malloc(sizeof(struct _sem_st));
	if (sem == NULL) {
		errno = ENOSPC;
		return (NULL);
	}

	/*
	 * Initialize the semaphore.
	 */
	if (pthread_mutex_init(&sem->lock, NULL) != 0) {
		free(sem);
		errno = ENOSPC;
		return (NULL);
	}

	if (pthread_cond_init(&sem->gtzero, NULL) != 0) {
		pthread_mutex_destroy(&sem->lock);
		free(sem);
		errno = ENOSPC;
		return (NULL);
	}
	
	sem->count = (u_int32_t)value;
	sem->nwaiters = 0;
	sem->magic = SEM_MAGIC;
	sem->semid = semid;
	sem->syssem = system_sem;
	return (sem);
}

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	int	retval, got_system_sem;
	semid_t	semid;

	got_system_sem = 0;
	semid = SEM_USER;
	/*
	 * Range check the arguments.
	 */
	if (pshared != 0) {
		retval = _ksem_init(value, &semid);
		if (retval == -1)
			goto RETURN;
		got_system_sem = 1;
	}

	(*sem) = sem_alloc(value, semid, got_system_sem);
	if ((*sem) == NULL)
		retval = -1;
	else
		retval = 0;
  RETURN:
	if (retval != 0 && got_system_sem)
		_ksem_destroy(semid);
	return retval;
}

int
sem_destroy(sem_t *sem)
{
	int	retval;
	
	_SEM_CHECK_VALIDITY(sem);

	pthread_mutex_lock(&(*sem)->lock);
	/*
	 * If this is a system semaphore let the kernel track it otherwise
	 * make sure there are no waiters.
	 */
	if ((*sem)->syssem != 0) {
		retval = _ksem_destroy((*sem)->semid);
		if (retval == -1) {
			pthread_mutex_unlock(&(*sem)->lock);
			goto RETURN;
		}
	} else if ((*sem)->nwaiters > 0) {
		pthread_mutex_unlock(&(*sem)->lock);
		errno = EBUSY;
		retval = -1;
		goto RETURN;
	}
	pthread_mutex_unlock(&(*sem)->lock);

	sem_free(*sem);	

	retval = 0;
  RETURN:
	return retval;
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
	sem_t *sem;
	sem_t s;
	semid_t semid;
	mode_t mode;
	unsigned int value;

	mode = 0;
	value = 0;

	if ((oflag & O_CREAT) != 0) {
		va_list ap;

		va_start(ap, oflag);
		mode = va_arg(ap, int);
		value = va_arg(ap, unsigned int);
		va_end(ap);
	}
	/*
	 * we can be lazy and let the kernel handle the "oflag",
	 * we'll just merge duplicate IDs into our list.
	 */
	if (_ksem_open(name, oflag, mode, value, &semid) == -1)
		return (SEM_FAILED);
	/*
	 * search for a duplicate ID, we must return the same sem_t *
	 * if we locate one.
	 */
	pthread_mutex_lock(&named_sems_mtx);
	LIST_FOREACH(s, &named_sems, entry) {
	    if (s->semid == semid) {
		    pthread_mutex_unlock(&named_sems_mtx);
		    return (s->backpointer);
	    }
	}
	sem = (sem_t *)malloc(sizeof(*sem));
	if (sem == NULL)
		goto err;
	*sem = sem_alloc(value, semid, 1);
	if ((*sem) == NULL)
		goto err;
	LIST_INSERT_HEAD(&named_sems, *sem, entry);
	(*sem)->backpointer = sem;
	pthread_mutex_unlock(&named_sems_mtx);
	return (sem);
err:
	pthread_mutex_unlock(&named_sems_mtx);
	_ksem_close(semid);
	if (sem != NULL) {
		if (*sem != NULL)
			sem_free(*sem);
		else
			errno = ENOSPC;
		free(sem);
	} else {
		errno = ENOSPC;
	}
	return (SEM_FAILED);
}

int
sem_close(sem_t *sem)
{

	if ((*sem)->syssem == 0) {
		errno = EINVAL;
		return (-1);
	}
	pthread_mutex_lock(&named_sems_mtx);
	if (_ksem_close((*sem)->semid) == -1) {
		pthread_mutex_unlock(&named_sems_mtx);
		return (-1);
	}
	LIST_REMOVE((*sem), entry);
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
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	if ((*sem)->syssem != 0) {
		retval = _ksem_wait((*sem)->semid);
		goto RETURN;
	}

	pthread_mutex_lock(&(*sem)->lock);

	while ((*sem)->count == 0) {
		(*sem)->nwaiters++;
		pthread_cond_wait(&(*sem)->gtzero, &(*sem)->lock);
		(*sem)->nwaiters--;
	}
	(*sem)->count--;

	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:
	return retval;
}

int
sem_trywait(sem_t *sem)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	if ((*sem)->syssem != 0) {
		retval = _ksem_trywait((*sem)->semid);
		goto RETURN;
	}

	pthread_mutex_lock(&(*sem)->lock);

	if ((*sem)->count > 0) {
		(*sem)->count--;
		retval = 0;
	} else {
		errno = EAGAIN;
		retval = -1;
	}
	
	pthread_mutex_unlock(&(*sem)->lock);

  RETURN:
	return retval;
}

int
sem_post(sem_t *sem)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	if ((*sem)->syssem != 0) {
		retval = _ksem_post((*sem)->semid);
		goto RETURN;
	}

	pthread_mutex_lock(&(*sem)->lock);

	(*sem)->count++;
	if ((*sem)->nwaiters > 0)
		pthread_cond_signal(&(*sem)->gtzero);

	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:
	return retval;
}

int
sem_getvalue(sem_t * __restrict sem, int * __restrict sval)
{
	int	retval;

	_SEM_CHECK_VALIDITY(sem);

	if ((*sem)->syssem != 0) {
		retval = _ksem_getvalue((*sem)->semid, sval);
		goto RETURN;
	}

	pthread_mutex_lock(&(*sem)->lock);
	*sval = (int)(*sem)->count;
	pthread_mutex_unlock(&(*sem)->lock);

	retval = 0;
  RETURN:
	return retval;
}
