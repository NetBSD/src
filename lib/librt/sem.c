/*	$NetBSD: sem.c,v 1.9 2019/02/21 21:33:34 christos Exp $	*/

/*-
 * Copyright (c) 2003, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__RCSID("$NetBSD: sem.c,v 1.9 2019/02/21 21:33:34 christos Exp $");

#ifndef __LIBPTHREAD_SOURCE__
/*
 * There is no longer any difference between the libpthread and the librt
 * versions of sem.c; both are fully kernel-assisted via the _ksem_*()
 * system calls.  The only difference is the need to lock some internal
 * data structures in the pthread version, which could be achieved by
 * different means.  However, in order to maintain binary compatibility
 * with applications that use POSIX semaphores and linked against only
 * libpthread, we continue to maintain a copy of the implementation here
 * that does not depend on any additional libraries (other than libc).
 */
#define	sem_init	_librt_sem_init
#define	sem_destroy	_librt_sem_destroy
#define	sem_open	_librt_sem_open
#define	sem_close	_librt_sem_close
#define	sem_unlink	_librt_sem_unlink
#define	sem_wait	_librt_sem_wait
#define	sem_timedwait	_librt_sem_timedwait
#define	sem_trywait	_librt_sem_trywait
#define	sem_post	_librt_sem_post
#define	sem_getvalue	_librt_sem_getvalue
#endif /* ! __LIBPTHREAD_SOURCE__ */

#undef _LIBC
#define	_LIBC

#include <sys/types.h>
#include <sys/ksem.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdarg.h>

#ifdef __LIBPTHREAD_SOURCE__
#include "pthread.h"
#endif /* __LIBPTHREAD_SOURCE__ */

#define	SEM_NAMED		0x4e414d44U	/* 'NAMD' */
#define	SEM_MAGIC		0x90af0421U
#define	SEM_MAGIC_NAMED		(SEM_MAGIC ^ SEM_NAMED)

#define	SEMID_IS_KSEMID(id)	(((uint32_t)(id) & KSEM_MARKER_MASK)	\
						== KSEM_PSHARED_MARKER)
#define	SEM_IS_KSEMID(k)	SEMID_IS_KSEMID((intptr_t)(k))	
			

#define	SEM_IS_UNNAMED(k)	(SEM_IS_KSEMID(k) ||			\
				 (k)->ksem_magic == SEM_MAGIC)

#define	SEM_IS_NAMED(k)		(!SEM_IS_UNNAMED(k))

#define	SEM_MAGIC_OK(k)		(SEM_IS_KSEMID(k) ||			\
				 (k)->ksem_magic == SEM_MAGIC || 	\
				 (k)->ksem_magic == SEM_MAGIC_NAMED)

struct _sem_st {
	unsigned int	ksem_magic;
	intptr_t	ksem_semid;

	/* Used only to de-dup named semaphores. */
	LIST_ENTRY(_sem_st) ksem_list;
	sem_t		*ksem_identity;
};

static LIST_HEAD(, _sem_st) named_sems = LIST_HEAD_INITIALIZER(&named_sems);
#ifdef __LIBPTHREAD_SOURCE__
static pthread_mutex_t named_sems_mtx = PTHREAD_MUTEX_INITIALIZER;

#define	LOCK_NAMED_SEMS()	pthread_mutex_lock(&named_sems_mtx)
#define	UNLOCK_NAMED_SEMS()	pthread_mutex_unlock(&named_sems_mtx)
#else /* ! __LIBPTHREAD_SOURCE__ */
#define	LOCK_NAMED_SEMS()	__nothing
#define	UNLOCK_NAMED_SEMS()	__nothing
#endif /* __LIBPTHREAD_SOURCE__ */

#ifndef __LIBPTHREAD_SOURCE__
#ifdef __weak_alias
__weak_alias(sem_init,_librt_sem_init)
__weak_alias(sem_destroy,_librt_sem_destroy)
__weak_alias(sem_open,_librt_sem_open)
__weak_alias(sem_close,_librt_sem_close)
__weak_alias(sem_unlink,_librt_sem_unlink)
__weak_alias(sem_wait,_librt_sem_wait)
__weak_alias(sem_timedwait,_librt_sem_timedwait)
__weak_alias(sem_trywait,_librt_sem_trywait)
__weak_alias(sem_post,_librt_sem_post)
__weak_alias(sem_getvalue,_librt_sem_getvalue)
#else
#error Weak aliases required to build POSIX semaphore support.
#endif /* __weak_alias */
#endif /* __LIBPTHREAD_SOURCE__ */

static inline intptr_t
sem_to_semid(sem_t *sem)
{

	if (SEM_IS_KSEMID(*sem))
		return (intptr_t)*sem;
	
	return (*sem)->ksem_semid;
}

static void
sem_free(sem_t sem)
{

	sem->ksem_magic = 0;
	free(sem);
}

static int
sem_alloc(unsigned int value, intptr_t semid, unsigned int magic, sem_t *semp)
{
	sem_t sem;

	if (value > SEM_VALUE_MAX)
		return EINVAL;

	if ((sem = malloc(sizeof(struct _sem_st))) == NULL)
		return ENOSPC;

	sem->ksem_magic = magic;
	sem->ksem_semid = semid;
 
	*semp = sem;
	return 0;
}

/* ARGSUSED */
int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	intptr_t	semid = pshared ? KSEM_PSHARED : 0;
	int error;

	if (_ksem_init(value, &semid) == -1)
		return -1;

	/*
	 * pshared anonymous semaphores are treated a little differently.
	 * We don't allocate a sem structure and return a pointer to it.
	 * That pointer might live in the shared memory segment that's
	 * shared between processes, but the _sem_st that contains the
	 * important bits certainly would not be.
	 *
	 * So, instead, we return the ksem ID given to us by the kernel.
	 * The kernel has arranged for the least-significant bit of the
	 * ksem ID to always be 1 so as to ensure we can always tell
	 * these IDs apart from the pointers that we vend out for other
	 * non-pshared semaphores.
	 */
	if (pshared) {
		if (!SEMID_IS_KSEMID(semid)) {
			_ksem_destroy(semid);
			errno = ENOTSUP;
			return -1;
		}
		*sem = (sem_t)semid;
		return 0;
	}

	if ((error = sem_alloc(value, semid, SEM_MAGIC, sem)) != 0) {
		_ksem_destroy(semid);
		errno = error;
		return -1;
	}

	return 0;
}

int
sem_destroy(sem_t *sem)
{
	int error, save_errno;

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	if (SEM_IS_KSEMID(*sem)) {
		error = _ksem_destroy((intptr_t)*sem);
	} else {
		if (SEM_IS_NAMED(*sem)) {
			errno = EINVAL;
			return -1;
		}

		error = _ksem_destroy((*sem)->ksem_semid);
		save_errno = errno;
		sem_free(*sem);
		if (error == -1)
			errno = save_errno;
	}

	return error;
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
	sem_t *sem, s;
	intptr_t semid;
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
		return SEM_FAILED;

	/*
	 * Search for a duplicate ID, we must return the same sem_t *
	 * if we locate one.
	 */
	LOCK_NAMED_SEMS();
	LIST_FOREACH(s, &named_sems, ksem_list) {
		if (s->ksem_semid == semid) {
			UNLOCK_NAMED_SEMS();
			return s->ksem_identity;
		}
	}

	if ((sem = malloc(sizeof(*sem))) == NULL) {
		error = ENOSPC;
		goto bad;
	}
	if ((error = sem_alloc(value, semid, SEM_MAGIC_NAMED, sem)) != 0)
		goto bad;

	LIST_INSERT_HEAD(&named_sems, *sem, ksem_list);
	UNLOCK_NAMED_SEMS();
	(*sem)->ksem_identity = sem;

	return sem;

 bad:
	UNLOCK_NAMED_SEMS();
	_ksem_close(semid);
	if (sem != NULL) {
		if (*sem != NULL)
			sem_free(*sem);
		free(sem);
	}
	errno = error;
	return SEM_FAILED;
}

int
sem_close(sem_t *sem)
{
	int error, save_errno;

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	if (!SEM_IS_NAMED(*sem)) {
		errno = EINVAL;
		return -1;
	}

	LOCK_NAMED_SEMS();
	error = _ksem_close((*sem)->ksem_semid);
	LIST_REMOVE((*sem), ksem_list);
	save_errno = errno;
	UNLOCK_NAMED_SEMS();
	sem_free(*sem);
	free(sem);
	if (error == -1)
		errno = save_errno;
	return error;
}

int
sem_unlink(const char *name)
{

	return _ksem_unlink(name);
}

int
sem_wait(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return _ksem_wait(sem_to_semid(sem));
}

int
sem_timedwait(sem_t *sem, const struct timespec * __restrict abstime)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return _ksem_timedwait(sem_to_semid(sem), abstime);
}

int
sem_trywait(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return _ksem_trywait(sem_to_semid(sem));
}

int
sem_post(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return _ksem_post(sem_to_semid(sem));
}

int
sem_getvalue(sem_t * __restrict sem, int * __restrict sval)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || !SEM_MAGIC_OK(*sem)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return _ksem_getvalue(sem_to_semid(sem), sval);
}
