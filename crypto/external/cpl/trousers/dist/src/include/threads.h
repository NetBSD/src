
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef __THREADS_H__
#define __THREADS_H__

#ifdef HAVE_PTHREAD_H

#include <pthread.h>

/* mutex abstractions */
#define MUTEX_INIT(m)		pthread_mutex_init(&m, NULL)
#define MUTEX_LOCK(m)		pthread_mutex_lock(&m)
#define MUTEX_UNLOCK(m)		pthread_mutex_unlock(&m)
#define MUTEX_DECLARE(m)	pthread_mutex_t m
#define MUTEX_DECLARE_INIT(m)	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define MUTEX_DECLARE_EXTERN(m)	extern pthread_mutex_t m

/* condition variable abstractions */
#define COND_DECLARE(c)		pthread_cond_t c
#define COND_INIT(c)		pthread_cond_init(&c, NULL)
#define COND_VAR		pthread_cond_t
#define COND_WAIT(c,m)		pthread_cond_wait(c,m)
#define COND_SIGNAL(c)		pthread_cond_signal(c)

/* thread abstractions */
#define THREAD_ID			((THREAD_TYPE)pthread_self())
#define THREAD_TYPE			pthread_t
#define THREAD_JOIN			pthread_join
#define THREAD_DETACH			pthread_detach
#define THREAD_ATTR_DECLARE(a)		pthread_attr_t a
#define THREAD_ATTR_INIT(a)		pthread_attr_init(&a)
#define THREAD_ATTR_SETJOINABLE(a)	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE)
#define THREAD_EXIT			pthread_exit
#define THREAD_CREATE(a,b,c,d)		pthread_create(a,b,c,d)
#define THREAD_SET_SIGNAL_MASK		pthread_sigmask
#define THREAD_NULL			(THREAD_TYPE *)0

#else

#error No threading library defined! (Cannot find pthread.h)

#endif

#endif
