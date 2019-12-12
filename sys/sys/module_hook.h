/* $NetBSD: module_hook.h,v 1.6 2019/12/12 22:55:20 pgoyette Exp $	*/

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
#include <sys/localcount.h>
#include <sys/stdbool.h>

void module_hook_init(void);
void module_hook_set(bool *, struct localcount *);
void module_hook_unset(bool *, struct localcount *);
bool module_hook_tryenter(bool *, struct localcount *);
void module_hook_exit(struct localcount *); 

/*
 * Macros for creating MP-safe vectored function calls, where
 * the function implementations are in modules which could be
 * unloaded.
 */

#define MODULE_HOOK(hook, type, args)				\
extern struct hook ## _t {					\
	struct localcount	lc;				\
	type			(*f)args;			\
	bool			hooked;				\
} hook __cacheline_aligned;

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
	(hook).f = func;					\
	module_hook_set(&(hook).hooked, &(hook).lc);		\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_UNSET(hook)					\
do {								\
	KASSERT((hook).f);					\
	module_hook_unset(&(hook).hooked, &(hook).lc);		\
	(hook).f = NULL;	/* paranoia */			\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_CALL(hook, args, default, retval)		\
do {								\
	if (module_hook_tryenter(&(hook).hooked, &(hook).lc)) {	\
		(retval) = (*(hook).f)args;			\
		module_hook_exit(&(hook).lc);			\
	} else {						\
		(retval) = (default);				\
	}							\
} while /* CONSTCOND */ (0)

#define MODULE_HOOK_CALL_VOID(hook, args, default)		\
do {								\
	if (module_hook_tryenter(&(hook).hooked, &(hook).lc)) {	\
		(*(hook).f)args;				\
		module_hook_exit(&(hook).lc);			\
	} else {						\
		default;					\
	}							\
} while /* CONSTCOND */ (0)

#endif	/* _SYS_MODULE_HOOK_H */
