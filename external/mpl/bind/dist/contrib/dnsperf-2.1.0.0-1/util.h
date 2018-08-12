/*	$NetBSD: util.h,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2012 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pthread.h>
#include <string.h>

#include <sys/time.h>

#include <isc/types.h>

#include "log.h"

#ifndef PERF_UTIL_H
#define PERF_UTIL_H 1

#define MILLION ((isc_uint64_t) 1000000)

#define THREAD(thread, start, arg) do {					\
	int __n = pthread_create((thread), NULL, (start), (arg));	\
	if (__n != 0)							\
		perf_log_fatal("pthread_create failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define JOIN(thread, valuep) do {					\
	int __n = pthread_join((thread), (valuep));			\
	if (__n != 0)							\
		perf_log_fatal("pthread_join failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define MUTEX_INIT(mutex) do {						\
	int __n = pthread_mutex_init((mutex), NULL);			\
	if (__n != 0)							\
		perf_log_fatal("pthread_mutex_init failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define MUTEX_DESTROY(mutex) do {						\
	int __n = pthread_mutex_destroy((mutex));			\
	if (__n != 0)							\
		perf_log_fatal("pthread_mutex_destroy failed: %s",	\
			       strerror(__n));				\
	} while (0)

#define LOCK(mutex) do {						\
	int __n = pthread_mutex_lock((mutex));				\
	if (__n != 0)							\
		perf_log_fatal("pthread_mutex_lock failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define UNLOCK(mutex) do {						\
	int __n = pthread_mutex_unlock((mutex));			\
	if (__n != 0)							\
		perf_log_fatal("pthread_mutex_unlock failed: %s",	\
			       strerror(__n));				\
	} while (0)

#define COND_INIT(cond) do {						\
	int __n = pthread_cond_init((cond), NULL);			\
	if (__n != 0)							\
		perf_log_fatal("pthread_cond_init failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define SIGNAL(cond) do {						\
	int __n = pthread_cond_signal((cond));				\
	if (__n != 0)							\
		perf_log_fatal("pthread_cond_signal failed: %s",	\
			       strerror(__n));				\
	} while (0)

#define BROADCAST(cond) do {						\
	int __n = pthread_cond_broadcast((cond));			\
	if (__n != 0)							\
		perf_log_fatal("pthread_cond_broadcast failed: %s",	\
			       strerror(__n));				\
	} while (0)

#define WAIT(cond, mutex) do {						\
	int __n = pthread_cond_wait((cond), (mutex));			\
	if (__n != 0)							\
		perf_log_fatal("pthread_cond_wait failed: %s",		\
			       strerror(__n));				\
	} while (0)

#define TIMEDWAIT(cond, mutex, when, timedout) do {			\
	int __n = pthread_cond_timedwait((cond), (mutex), (when));	\
	isc_boolean_t *res = (timedout);				\
	if (__n != 0 && __n != ETIMEDOUT)				\
		perf_log_fatal("pthread_cond_timedwait failed: %s",	\
			       strerror(__n));				\
	if (res != NULL)						\
		*res = ISC_TF(__n != 0);				\
	} while (0)

static __inline__ isc_uint64_t
get_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * MILLION + tv.tv_usec;
}

#define SAFE_DIV(n, d) ( (d) == 0 ? 0 : (n) / (d) )

#endif
