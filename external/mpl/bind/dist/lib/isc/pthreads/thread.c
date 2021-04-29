/*	$NetBSD: thread.c,v 1.6 2021/04/29 17:26:12 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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

#include "trampoline_p.h"

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
	isc__trampoline_t *trampoline_arg;

	trampoline_arg = isc__trampoline_get(func, arg);

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

	ret = pthread_create(thread, &attr, isc__trampoline_run,
			     trampoline_arg);
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

#if defined(HAVE_CPUSET_SETAFFINITY) || defined(HAVE_PTHREAD_SETAFFINITY_NP)
#if defined(HAVE_CPUSET_SETAFFINITY)
static int
getaffinity(cpuset_t *set) {
	return (cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1,
				   sizeof(*set), set));
}
static int
issetaffinity(int cpu, cpuset_t *set) {
	return ((cpu >= CPU_SETSIZE) ? -1 : CPU_ISSET(cpu, set) ? 1 : 0);
}
static int
setaffinity(int cpu, cpuset_t *set) {
	CPU_ZERO(set);
	CPU_SET(cpu, set);
	return (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1,
				   sizeof(*set), set));
}
#elif defined(__NetBSD__)
static int
getaffinity(cpuset_t *set) {
	return (pthread_getaffinity_np(pthread_self(), cpuset_size(set), set));
}
static int
issetaffinity(int cpu, cpuset_t *set) {
	return (cpuset_isset(cpu, set));
}
static int
setaffinity(int cpu, cpuset_t *set) {
	cpuset_zero(set);
	cpuset_set(cpu, set);
	return (pthread_setaffinity_np(pthread_self(), cpuset_size(set), set));
}
#else /* linux ? */
static int
getaffinity(cpu_set_t *set) {
	return (pthread_getaffinity_np(pthread_self(), sizeof(*set), set));
}
static int
issetaffinity(int cpu, cpu_set_t *set) {
	return ((cpu >= CPU_SETSIZE) ? -1 : CPU_ISSET(cpu, set) ? 1 : 0);
}
static int
setaffinity(int cpu, cpu_set_t *set) {
	CPU_ZERO(set);
	CPU_SET(cpu, set);
	return (pthread_setaffinity_np(pthread_self(), sizeof(*set), set));
}
#endif
#endif

isc_result_t
isc_thread_setaffinity(int cpu) {
#if defined(HAVE_CPUSET_SETAFFINITY) || defined(HAVE_PTHREAD_SETAFFINITY_NP)
	int cpu_id = -1, cpu_aff_ok_counter = -1, n;
#if defined(HAVE_CPUSET_SETAFFINITY)
	cpuset_t _set, *set = &_set;
#define cpuset_destroy(x) ((void)0)
#elif defined(__NetBSD__)
	cpuset_t *set = cpuset_create();
	if (set == NULL) {
		return (ISC_R_FAILURE);
	}
#else /* linux? */
	cpu_set_t _set, *set = &_set;
#define cpuset_destroy(x) ((void)0)
#endif

	if (getaffinity(set) != 0) {
		cpuset_destroy(set);
		return (ISC_R_FAILURE);
	}
	while (cpu_aff_ok_counter < cpu) {
		cpu_id++;
		if ((n = issetaffinity(cpu_id, set)) > 0) {
			cpu_aff_ok_counter++;
		} else if (n < 0) {
			cpuset_destroy(set);
			return (ISC_R_FAILURE);
		}
	}
	if (setaffinity(cpu_id, set) != 0) {
		cpuset_destroy(set);
		return (ISC_R_FAILURE);
	}
	cpuset_destroy(set);
#elif defined(HAVE_PROCESSOR_BIND)
	if (processor_bind(P_LWPID, P_MYID, cpu, NULL) != 0) {
		return (ISC_R_FAILURE);
	}
#else  /* if defined(HAVE_CPUSET_SETAFFINITY) */
	UNUSED(cpu);
#endif /* if defined(HAVE_CPUSET_SETAFFINITY) */
	return (ISC_R_SUCCESS);
}
