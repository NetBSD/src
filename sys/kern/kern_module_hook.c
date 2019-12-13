/*	$NetBSD: kern_module_hook.c,v 1.4 2019/12/13 08:02:53 skrll Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * Kernel module support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_module_hook.c,v 1.4 2019/12/13 08:02:53 skrll Exp $");

#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/module_hook.h>
#include <sys/mutex.h>
#include <sys/pserialize.h>

#include <uvm/uvm_extern.h>

/* Locking/synchronization stuff for module hooks */

static struct {
	kmutex_t	mtx;
	kcondvar_t	cv;
	pserialize_t	psz;
} module_hook __cacheline_aligned;

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

void
module_hook_set(bool *hooked, struct localcount *lc)
{

	KASSERT(kernconfig_is_held());
	KASSERT(!*hooked);

	localcount_init(lc);

	/* Wait until setup has been witnessed by all CPUs.  */
	pserialize_perform(module_hook.psz);

	/* Let others use it */
	atomic_store_relaxed(hooked, true);
}

void
module_hook_unset(bool *hooked, struct localcount *lc)
{

	KASSERT(kernconfig_is_held());
	KASSERT(*hooked);

	/* Get exclusive with pserialize and localcount. */
	mutex_enter(&module_hook.mtx);

	/* Prevent new calls to module_hook_tryenter(). */
	atomic_store_relaxed(hooked, false);

	/* Wait for existing calls to module_hook_tryenter(). */
	pserialize_perform(module_hook.psz);

	/* Wait for module_hook_exit. */
	localcount_drain(lc, &module_hook.cv, &module_hook.mtx);

	/* All done! */
	mutex_exit(&module_hook.mtx);
	localcount_fini(lc);
}

bool
module_hook_tryenter(bool *hooked, struct localcount *lc)
{
	bool call_hook;
	int s;

	s = pserialize_read_enter();
	call_hook = atomic_load_relaxed(hooked);
	if (call_hook)
		localcount_acquire(lc);
	pserialize_read_exit(s);

	return call_hook;
}

void
module_hook_exit(struct localcount *lc)
{

	localcount_release(lc, &module_hook.cv, &module_hook.mtx);
}

void
module_hook_init(void)
{

	mutex_init(&module_hook.mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&module_hook.cv, "mod_hook");
	module_hook.psz = pserialize_create();
}
