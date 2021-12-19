/*	$NetBSD: linux_kthread.c,v 1.1 2021/12/19 12:23:07 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_kthread.c,v 1.1 2021/12/19 12:23:07 riastradh Exp $");

#include <sys/types.h>

#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/specificdata.h>

#include <linux/kthread.h>

struct task_struct {
	kmutex_t	kt_lock;
	kcondvar_t	kt_cv;
	bool		kt_shouldstop:1;
	bool		kt_shouldpark:1;
	bool		kt_parked:1;

	int		(*kt_func)(void *);
	void		*kt_cookie;
	struct lwp	*kt_lwp;
};

static specificdata_key_t linux_kthread_key __read_mostly = -1;

int
linux_kthread_init(void)
{
	int error;

	error = lwp_specific_key_create(&linux_kthread_key, NULL);
	if (error)
		goto out;

	/* Success!  */
	error = 0;

out:	if (error)
		linux_kthread_fini();
	return error;
}

void
linux_kthread_fini(void)
{

	if (linux_kthread_key != -1) {
		lwp_specific_key_delete(linux_kthread_key);
		linux_kthread_key = -1;
	}
}

#define	linux_kthread()	_linux_kthread(__func__)
static struct task_struct *
_linux_kthread(const char *caller)
{
	struct task_struct *T;

	T = lwp_getspecific(linux_kthread_key);
	KASSERTMSG(T != NULL, "%s must be called from Linux kthread", caller);

	return T;
}

static void
linux_kthread_start(void *cookie)
{
	struct task_struct *T = cookie;
	int ret;

	lwp_setspecific(linux_kthread_key, T);

	ret = (*T->kt_func)(T->kt_cookie);
	kthread_exit(ret);
}

static struct task_struct *
kthread_alloc(int (*func)(void *), void *cookie)
{
	struct task_struct *T;

	T = kmem_zalloc(sizeof(*T), KM_SLEEP);

	mutex_init(&T->kt_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&T->kt_cv, "lnxkthrd");

	T->kt_func = func;
	T->kt_cookie = cookie;

	return T;
}

static void
kthread_free(struct task_struct *T)
{

	cv_destroy(&T->kt_cv);
	mutex_destroy(&T->kt_lock);
	kmem_free(T, sizeof(*T));
}

struct task_struct *
kthread_run(int (*func)(void *), void *cookie, const char *name)
{
	struct task_struct *T;
	int error;

	T = kthread_alloc(func, cookie);
	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE|KTHREAD_MUSTJOIN, NULL,
	    linux_kthread_start, T, &T->kt_lwp, "%s", name);
	if (error) {
		kthread_free(T);
		T = NULL;
	}

	return T;
}

int
kthread_stop(struct task_struct *T)
{
	int ret;

	mutex_enter(&T->kt_lock);
	T->kt_shouldpark = false;
	T->kt_shouldstop = true;
	cv_broadcast(&T->kt_cv);
	mutex_exit(&T->kt_lock);

	ret = kthread_join(T->kt_lwp);

	kthread_free(T);

	return ret;
}

int
kthread_should_stop(void)
{

	return linux_kthread()->kt_shouldstop;
}

void
kthread_park(struct task_struct *T)
{

	mutex_enter(&T->kt_lock);
	KASSERT(!T->kt_shouldstop);
	T->kt_shouldpark = true;
	while (!T->kt_parked)
		cv_wait(&T->kt_cv, &T->kt_lock);
	mutex_exit(&T->kt_lock);
}

void
kthread_unpark(struct task_struct *T)
{

	mutex_enter(&T->kt_lock);
	T->kt_shouldpark = false;
	cv_broadcast(&T->kt_cv);
	mutex_exit(&T->kt_lock);
}

int
__kthread_should_park(struct task_struct *T)
{
	bool shouldpark;

	mutex_enter(&T->kt_lock);
	shouldpark = T->kt_shouldpark;
	mutex_exit(&T->kt_lock);

	return shouldpark;
}

int
kthread_should_park(void)
{
	struct task_struct *T = linux_kthread();

	return __kthread_should_park(T);
}

void
kthread_parkme(void)
{
	struct task_struct *T = linux_kthread();

	mutex_enter(&T->kt_lock);
	while (T->kt_shouldpark) {
		T->kt_parked = true;
		cv_broadcast(&T->kt_cv);
		cv_wait(&T->kt_cv, &T->kt_lock);
		T->kt_parked = false;
	}
	mutex_exit(&T->kt_lock);
}
