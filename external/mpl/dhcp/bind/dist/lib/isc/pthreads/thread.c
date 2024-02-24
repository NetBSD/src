/*	$NetBSD: thread.c,v 1.1.2.2 2024/02/24 13:07:29 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
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
