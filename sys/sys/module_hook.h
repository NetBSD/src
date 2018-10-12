/* $NetBSD: module_hook.h,v 1.1.2.7 2018/10/12 10:28:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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

#ifndef _SYS_MODULE_HOOK_H
#define _SYS_MODULE_HOOK_H

#include <sys/param.h>	/* for COHERENCY_UNIT, for __cacheline_aligned */
#include <sys/mutex.h>
#include <sys/localcount.h>
#include <sys/condvar.h>
#include <sys/pserialize.h>
#include <sys/atomic.h>

/*
 * Macros for creating MP-safe vectored function calls, where
 * the function implementations are in modules which could be
 * unloaded.
 */

#define MODULE_HOOK(hook,args)					\
extern struct hook ## _t {					\
	kmutex_t		mtx;				\
	kcondvar_t		cv;				\
	struct localcount	lc;				\
	pserialize_t		psz;				\
        bool			hooked;				\
	int			(*f)args;			\
} hook __cacheline_aligned;

#define MODULE_HOOK2(hook,args1,args2)				\
extern struct hook ## _t {					\
	kmutex_t		mtx;				\
	kcondvar_t		cv;				\
	struct localcount	lc;				\
	pserialize_t		psz;				\
        bool			hooked;				\
	int			(*f1)args1;			\
	int			(*f2)args2;			\
} hook __cacheline_aligned;

#define MODULE_SET_HOOK(hook, waitchan, func)			\
static void hook ## _set(void);					\
static void hook ## _set(void)					\
{								\
								\
	KASSERT(!hook.hooked);					\
								\
	hook.psz = pserialize_create();				\
	mutex_init(&hook.mtx, MUTEX_DEFAULT, IPL_NONE);		\
	cv_init(&hook.cv, waitchan);				\
	localcount_init(&hook.lc);				\
	hook.f = func;						\
								\
	/* Make sure it's initialized before anyone uses it */	\
	membar_producer();					\
								\
	/* Let them use it */					\
	hook.hooked = true;					\
}

#define MODULE_SET_HOOK2(hook, waitchan, func1, func2)		\
static void hook ## _set(void);					\
static void hook ## _set(void)					\
{								\
								\
	KASSERT(!hook.hooked);					\
								\
	hook.psz = pserialize_create();				\
	mutex_init(&hook.mtx, MUTEX_DEFAULT, IPL_NONE);		\
	cv_init(&hook.cv, waitchan);				\
	localcount_init(&hook.lc);				\
	hook.f1 = func1;					\
	hook.f2 = func2;					\
								\
	/* Make sure it's initialized before anyone uses it */	\
	membar_producer();					\
								\
	/* Let them use it */					\
	hook.hooked = true;					\
}

#define MODULE_UNSET_HOOK(hook)					\
static void (hook ## _unset)(void);				\
static void (hook ## _unset)(void)				\
{								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(hook.hooked);					\
	KASSERT(hook.f);					\
								\
	/* Grab the mutex */					\
	mutex_enter(&hook.mtx);					\
								\
	/* Prevent new localcount_acquire calls.  */		\
	hook.hooked = false;					\
								\
	/* Wait for existing localcount_acquire calls to drain.  */ \
	pserialize_perform(hook.psz);				\
								\
	/* Wait for existing localcount references to drain.  */\
	localcount_drain(&hook.lc, &hook.cv, &hook.mtx);	\
								\
	/* Release the mutex and clean up all resources */	\
	mutex_exit(&hook.mtx);					\
	localcount_fini(&hook.lc);				\
	cv_destroy(&hook.cv);					\
	mutex_destroy(&hook.mtx);				\
	pserialize_destroy(hook.psz);				\
}

#define MODULE_UNSET_HOOK2(hook)				\
static void (hook ## _unset)(void);				\
static void (hook ## _unset)(void)				\
{								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(hook.hooked);					\
	KASSERT(hook.f1);					\
	KASSERT(hook.f2);					\
								\
	/* Grab the mutex */					\
	mutex_enter(&hook.mtx);					\
								\
	/* Prevent new localcount_acquire calls.  */		\
	hook.hooked = false;					\
								\
	/* Wait for existing localcount_acquire calls to drain.  */ \
	pserialize_perform(hook.psz);				\
								\
	/* Wait for existing localcount references to drain.  */\
	localcount_drain(&hook.lc, &hook.cv, &hook.mtx);	\
								\
	/* Release the mutex and clean up all resources */	\
	mutex_exit(&hook.mtx);					\
	localcount_fini(&hook.lc);				\
	cv_destroy(&hook.cv);					\
	mutex_destroy(&hook.mtx);				\
	pserialize_destroy(hook.psz);				\
}

#define MODULE_CALL_HOOK_DECL(hook, which, decl)		\
int								\
hook ## _ ## which ## _call decl;
#define MODULE_CALL_HOOK(hook, which, decl, args, default)	\
int								\
hook ## _ ## which ## _call decl				\
{								\
	bool __hooked;						\
	int __hook_error, __hook_s;				\
								\
	__hook_s = pserialize_read_enter();			\
	__hooked = hook.hooked;					\
	if (__hooked) {						\
		membar_consumer();				\
		localcount_acquire(&hook.lc);			\
	}							\
	pserialize_read_exit(__hook_s);				\
								\
	if (__hooked) {						\
		__hook_error = (*hook.which)args;		\
		localcount_release(&hook.lc, &hook.cv,		\
		    &hook.mtx);					\
	} else {						\
		__hook_error = default;				\
	}							\
	return __hook_error;					\
}

#endif	/* _SYS_MODULE_HOOK_H */
