/*	$NetBSD: thread.c,v 1.4 2020/05/24 19:46:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#if defined(HAVE_SCHED_H)
#include <sched.h>
#endif /* if defined(HAVE_SCHED_H) */

#if defined(HAVE_CPUSET_H)
#include <sys/cpuset.h>
#include <sys/param.h>
#endif /* if defined(HAVE_CPUSET_H) */

#if defined(HAVE_SYS_PROCSET_H)
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/types.h>
#endif /* if defined(HAVE_SYS_PROCSET_H) */

#include <isc/strerr.h>
#include <isc/thread.h>
#include <isc/util.h>

#ifndef THREAD_MINSTACKSIZE
#define THREAD_MINSTACKSIZE (1024U * 1024)
#endif /* ifndef THREAD_MINSTACKSIZE */

#define _FATAL(r, f)                                                          \
	{                                                                     \
		char strbuf[ISC_STRERRORSIZE];                                \
		strerror_r(r, strbuf, sizeof(strbuf));                        \
		isc_error_fatal(__FILE__, __LINE__, f " failed: %s", strbuf); \
	}

void
isc_thread_create(isc_threadfunc_t func, isc_threadarg_t arg,
		  isc_thread_t *thread) {
	pthread_attr_t attr;
#if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
	defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
	size_t stacksize;
#endif /* if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
	* defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE) */
	int ret;

	pthread_attr_init(&attr);

#if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
	defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
	ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0) {
		_FATAL(ret, "pthread_attr_getstacksize()");
	}

	if (stacksize < THREAD_MINSTACKSIZE) {
		ret = pthread_attr_setstacksize(&attr, THREAD_MINSTACKSIZE);
		if (ret != 0) {
			_FATAL(ret, "pthread_attr_setstacksize()");
		}
	}
#endif /* if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
	* defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE) */

	ret = pthread_create(thread, &attr, func, arg);
	if (ret != 0) {
		_FATAL(ret, "pthread_create()");
	}

	pthread_attr_destroy(&attr);

	return;
}

void
isc_thread_join(isc_thread_t thread, isc_threadresult_t *result) {
	int ret = pthread_join(thread, result);
	if (ret != 0) {
		_FATAL(ret, "pthread_join()");
	}
}

#ifdef __NetBSD__
#define pthread_setconcurrency(a) (void)a /* nothing */
#endif					  /* ifdef __NetBSD__ */

void
isc_thread_setconcurrency(unsigned int level) {
	(void)pthread_setconcurrency(level);
}

void
isc_thread_setname(isc_thread_t thread, const char *name) {
#if defined(HAVE_PTHREAD_SETNAME_NP) && !defined(__APPLE__)
	/*
	 * macOS has pthread_setname_np but only works on the
	 * current thread so it's not used here
	 */
#if defined(__NetBSD__)
	(void)pthread_setname_np(thread, name, NULL);
#else  /* if defined(__NetBSD__) */
	(void)pthread_setname_np(thread, name);
#endif /* if defined(__NetBSD__) */
#elif defined(HAVE_PTHREAD_SET_NAME_NP)
	(void)pthread_set_name_np(thread, name);
#else  /* if defined(HAVE_PTHREAD_SETNAME_NP) && !defined(__APPLE__) */
	UNUSED(thread);
	UNUSED(name);
#endif /* if defined(HAVE_PTHREAD_SETNAME_NP) && !defined(__APPLE__) */
}

void
isc_thread_yield(void) {
#if defined(HAVE_SCHED_YIELD)
	sched_yield();
#elif defined(HAVE_PTHREAD_YIELD)
	pthread_yield();
#elif defined(HAVE_PTHREAD_YIELD_NP)
	pthread_yield_np();
#endif /* if defined(HAVE_SCHED_YIELD) */
}

isc_result_t
isc_thread_setaffinity(int cpu) {
#if defined(HAVE_CPUSET_SETAFFINITY)
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1,
			       sizeof(cpuset), &cpuset) != 0)
	{
		return (ISC_R_FAILURE);
	}
#elif defined(HAVE_PTHREAD_SETAFFINITY_NP)
#if defined(__NetBSD__)
	cpuset_t *cset;
	cset = cpuset_create();
	if (cset == NULL) {
		return (ISC_R_FAILURE);
	}
	cpuset_set(cpu, cset);
	if (pthread_setaffinity_np(pthread_self(), cpuset_size(cset), cset) !=
	    0) {
		cpuset_destroy(cset);
		return (ISC_R_FAILURE);
	}
	cpuset_destroy(cset);
#else  /* linux? */
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set) !=
	    0) {
		return (ISC_R_FAILURE);
	}
#endif /* __NetBSD__ */
#elif defined(HAVE_PROCESSOR_BIND)
	if (processor_bind(P_LWPID, P_MYID, cpu, NULL) != 0) {
		return (ISC_R_FAILURE);
	}
#else  /* if defined(HAVE_CPUSET_SETAFFINITY) */
	UNUSED(cpu);
#endif /* if defined(HAVE_CPUSET_SETAFFINITY) */
	return (ISC_R_SUCCESS);
}
