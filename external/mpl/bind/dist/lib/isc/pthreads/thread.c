/*	$NetBSD: thread.c,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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

#include <config.h>

#if defined(HAVE_SCHED_H)
#include <sched.h>
#endif

#include <isc/thread.h>
#include <isc/util.h>

#ifndef THREAD_MINSTACKSIZE
#define THREAD_MINSTACKSIZE		(1024U * 1024)
#endif

isc_result_t
isc_thread_create(isc_threadfunc_t func, isc_threadarg_t arg,
		  isc_thread_t *thread)
{
	pthread_attr_t attr;
#if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
    defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
	size_t stacksize;
#endif
	int ret;

	pthread_attr_init(&attr);

#if defined(HAVE_PTHREAD_ATTR_GETSTACKSIZE) && \
    defined(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
	ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0)
		return (ISC_R_UNEXPECTED);

	if (stacksize < THREAD_MINSTACKSIZE) {
		ret = pthread_attr_setstacksize(&attr, THREAD_MINSTACKSIZE);
		if (ret != 0)
			return (ISC_R_UNEXPECTED);
	}
#endif

#if defined(PTHREAD_SCOPE_SYSTEM) && defined(NEED_PTHREAD_SCOPE_SYSTEM)
	ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (ret != 0)
		return (ISC_R_UNEXPECTED);
#endif

	ret = pthread_create(thread, &attr, func, arg);
	if (ret != 0)
		return (ISC_R_UNEXPECTED);

	pthread_attr_destroy(&attr);

	return (ISC_R_SUCCESS);
}

void
isc_thread_setconcurrency(unsigned int level) {
#if defined(CALL_PTHREAD_SETCONCURRENCY)
	(void)pthread_setconcurrency(level);
#else
	UNUSED(level);
#endif
}

void
isc_thread_setname(isc_thread_t thread, const char *name) {
#if defined(HAVE_PTHREAD_SETNAME_NP) && defined(_GNU_SOURCE)
	/*
	 * macOS has pthread_setname_np but only works on the
	 * current thread so it's not used here
	*/
	(void)pthread_setname_np(thread, name);
#elif defined(HAVE_PTHREAD_SET_NAME_NP)
	(void)pthread_set_name_np(thread, name);
#else
	UNUSED(thread);
	UNUSED(name);
#endif
}

void
isc_thread_yield(void) {
#if defined(HAVE_SCHED_YIELD)
	sched_yield();
#elif defined( HAVE_PTHREAD_YIELD)
	pthread_yield();
#elif defined( HAVE_PTHREAD_YIELD_NP)
	pthread_yield_np();
#endif
}
