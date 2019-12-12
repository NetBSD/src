/* $NetBSD: module_hook.h,v 1.5 2019/12/12 02:15:43 pgoyette Exp $	*/

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

#define MODULE_HOOK(hook, type, args)				\
extern struct hook ## _t {					\
	struct localcount	lc;				\
        bool			hooked;				\
	type			(*f)args;			\
} hook __cacheline_aligned;

extern kmutex_t		module_hook_mtx;
extern kcondvar_t	module_hook_cv;
extern pserialize_t	module_hook_psz;

/*
 * We use pserialize_perform() to issue a memory barrier on the current
 * CPU and on all other CPUs so that all prior memory operations on the
 * current CPU globally happen before all subsequent memory operations
 * on the current CPU, as perceived by any other CPU.
 *
 * pserialize_perform() might be rather heavy-weight here, but it only
 * happens during module loading, and it allows MODULE_HOOK_CALL() to
 * work without any other memory barriers.
 */

#define MODULE_HOOK_SET(hook, func)				\
do {								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(!hook.hooked);					\
								\
	localcount_init(&hook.lc);				\
	hook.f = func;						\
								\
	/* Make sure it's initialized before anyone uses it */	\
	pserialize_perform(module_hook_psz);			\
								\
	/* Let them use it */					\
	atomic_store_relaxed(&hook.hooked, true);		\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_UNSET(hook)					\
do {								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(hook.hooked);					\
	KASSERT(hook.f);					\
								\
	/* Grab the mutex */					\
	mutex_enter(&module_hook_mtx);				\
								\
	/* Prevent new localcount_acquire calls.  */		\
	atomic_store_relaxed(&hook.hooked, false);		\
								\
	/*							\
	 * Wait for localcount_acquire calls already under way	\
	 * to finish.						\
	 */							\
	pserialize_perform(module_hook_psz);			\
								\
	/* Wait for existing localcount references to drain.  */\
	localcount_drain(&hook.lc, &module_hook_cv,		\
	     &module_hook_mtx);					\
								\
	/* Release the mutex and clean up all resources */	\
	mutex_exit(&module_hook_mtx);				\
	localcount_fini(&hook.lc);				\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_CALL(hook, args, default, retval)		\
do {								\
	bool __hooked;						\
	int __hook_s;						\
								\
	__hook_s = pserialize_read_enter();			\
	__hooked = atomic_load_relaxed(&hook.hooked);		\
	if (__hooked) {						\
		localcount_acquire(&hook.lc);			\
	}							\
	pserialize_read_exit(__hook_s);				\
								\
	if (__hooked) {						\
		retval = (*hook.f)args;				\
		localcount_release(&hook.lc, &module_hook_cv,	\
		    &module_hook_mtx);				\
	} else {						\
		retval = default;				\
	}							\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_CALL_VOID(hook, args, default)		\
do {								\
	bool __hooked;						\
	int __hook_s;						\
								\
	__hook_s = pserialize_read_enter();			\
	__hooked = atomic_load_relaxed(&hook.hooked);		\
	if (__hooked) {						\
		localcount_acquire(&hook.lc);			\
	}							\
	pserialize_read_exit(__hook_s);				\
								\
	if (__hooked) {						\
		(*hook.f)args;					\
		localcount_release(&hook.lc, &module_hook_cv,	\
		    &module_hook_mtx);				\
	} else {						\
		default;					\
	}							\
} while /* CONSTCOND */ (0)

#endif	/* _SYS_MODULE_HOOK_H */
