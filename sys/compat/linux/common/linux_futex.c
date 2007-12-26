/*	$NetBSD: linux_futex.c,v 1.9.4.1 2007/12/26 19:49:16 ad Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.9.4.1 2007/12/26 19:49:16 ad Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/queue.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/kernel.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

struct futex;

struct waiting_proc {
	kcondvar_t wp_futex_cv;
	struct futex *wp_new_futex;
	TAILQ_ENTRY(waiting_proc) wp_list;
};
struct futex {
	void *f_uaddr;
	int f_refcount;
	LIST_ENTRY(futex) f_list;
	TAILQ_HEAD(lf_waiting_proc, waiting_proc) f_waiting_proc;
};

static LIST_HEAD(futex_list, futex) futex_list;
static kmutex_t *futex_lock = NULL;

#define FUTEX_LOCK	mutex_enter(futex_lock);
#define FUTEX_UNLOCK	mutex_exit(futex_lock);

#ifdef DEBUG_LINUX_FUTEX
#define FUTEXPRINTF(a) printf a
#else
#define FUTEXPRINTF(a)
#endif

static struct futex *futex_get(void *);
static void futex_put(struct futex *);
static int futex_sleep(struct futex *, unsigned int);
static int futex_wake(struct futex *, int, struct futex *);

int
linux_sys_futex(struct lwp *l, const struct linux_sys_futex_args *uap, register_t *retval)
{
	/* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */
	int val;
	int ret;
	struct timespec timeout = { 0, 0 };
	int error = 0;
	struct futex *f;
	struct futex *newf;
	int timeout_hz = 0;

	/* First time use */
	if (__predict_false(futex_lock == NULL)) {
		futex_lock = kmem_alloc(sizeof(kmutex_t), KM_SLEEP);
		mutex_init(futex_lock, MUTEX_DEFAULT, IPL_NONE);
		FUTEX_LOCK;
		LIST_INIT(&futex_list);
		FUTEX_UNLOCK;
	}

	switch (SCARG(uap, op)) {
	case LINUX_FUTEX_WAIT:
		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0)
			return error;

		if (val != SCARG(uap, val))
			return EWOULDBLOCK;

		if (SCARG(uap, timeout) != NULL) {
			if ((error = copyin(SCARG(uap, timeout), 
			    &timeout, sizeof(timeout))) != 0)
				return error;
			error = itimespecfix(&timeout);
			if (error)
				return error;
			timeout_hz = tstohz(&timeout);
		}

		FUTEXPRINTF(("FUTEX_WAIT %d.%d: val = %d, uaddr = %p, "
		    "*uaddr = %d, timeout = %d.%09ld\n", 
		    l->l_proc->p_pid, l->l_lid, SCARG(uap, val), 
		    SCARG(uap, uaddr), val, timeout.tv_sec, timeout.tv_nsec));

		f = futex_get(SCARG(uap, uaddr));
		ret = futex_sleep(f, timeout_hz);
		futex_put(f);

		FUTEXPRINTF(("FUTEX_WAIT %d.%d: uaddr = %p, "
		    "ret = %d\n", l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), ret));

		switch (ret) {
		case EWOULDBLOCK:	/* timeout */
			return ETIMEDOUT;
			break;
		case EINTR:		/* signal */
			return EINTR;
			break;
		case 0:			/* FUTEX_WAKE received */
			FUTEXPRINTF(("FUTEX_WAIT %d.%d: uaddr = %p, got it\n",
			    l->l_proc->p_pid, l->l_lid, SCARG(uap, uaddr)));
			return 0;
			break;
		default:
			FUTEXPRINTF(("FUTEX_WAIT: unexpected ret = %d\n", ret));
			break;
		}

		/* NOTREACHED */
		break;
		
	case LINUX_FUTEX_WAKE:
		/* 
		 * XXX: Linux is able cope with different addresses 
		 * corresponding to the same mapped memory in the sleeping 
		 * and the waker process.
		 */
		FUTEXPRINTF(("FUTEX_WAKE %d.%d: uaddr = %p, val = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), SCARG(uap, val)));
		f = futex_get(SCARG(uap, uaddr));
		*retval = futex_wake(f, SCARG(uap, val), NULL);
		futex_put(f);
		break;

	case LINUX_FUTEX_CMP_REQUEUE:
		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0)
			return error;

		if (val != SCARG(uap, val3))
			return EAGAIN;
		/* FALLTHROUGH */

	case LINUX_FUTEX_REQUEUE:
		f = futex_get(SCARG(uap, uaddr));
		newf = futex_get(SCARG(uap, uaddr2));
		*retval = futex_wake(f, SCARG(uap, val), newf);
		futex_put(f);
		futex_put(newf);
		break;

	case LINUX_FUTEX_FD:
		FUTEXPRINTF(("linux_sys_futex: unimplemented op %d\n", 
		    SCARG(uap, op)));
		break;
	default:
		FUTEXPRINTF(("linux_sys_futex: unknown op %d\n", 
		    SCARG(uap, op)));
		break;
	}
	return 0;
}

static struct futex *
futex_get(void *uaddr)
{
	struct futex *f;

	FUTEX_LOCK;
	LIST_FOREACH(f, &futex_list, f_list) {
		if (f->f_uaddr == uaddr) {
			f->f_refcount++;
			FUTEX_UNLOCK;
			return f;
		}
	}
	FUTEX_UNLOCK;

	/* Not found, create it */
	f = kmem_zalloc(sizeof(*f), KM_SLEEP);
	f->f_uaddr = uaddr;
	f->f_refcount = 1;
	TAILQ_INIT(&f->f_waiting_proc);
	FUTEX_LOCK;
	LIST_INSERT_HEAD(&futex_list, f, f_list);
	FUTEX_UNLOCK;

	return f;
}

static void 
futex_put(struct futex *f)
{
	f->f_refcount--;
	if (f->f_refcount == 0) {
		FUTEX_LOCK;
		LIST_REMOVE(f, f_list);
		FUTEX_UNLOCK;
		kmem_free(f, sizeof(*f));
	}

	return;
}

static int 
futex_sleep(struct futex *f, unsigned int timeout)
{
	struct waiting_proc *wp;
	int ret;

	wp = kmem_zalloc(sizeof(*wp), KM_SLEEP);
	cv_init(&wp->wp_futex_cv, "lnxftxcv");

	FUTEX_LOCK;
	TAILQ_INSERT_TAIL(&f->f_waiting_proc, wp, wp_list);
	ret = cv_timedwait_sig(&wp->wp_futex_cv, futex_lock, timeout);
	TAILQ_REMOVE(&f->f_waiting_proc, wp, wp_list);
	FUTEX_UNLOCK;

	if ((ret == 0) && (wp->wp_new_futex != NULL)) {
		ret = futex_sleep(wp->wp_new_futex, timeout);
		futex_put(wp->wp_new_futex); /* futex_get called in wakeup */
	}

	kmem_free(wp, sizeof(*wp));
	return ret;
}

static int
futex_wake(struct futex *f, int n, struct futex *newf)
{
	struct waiting_proc *wp;
	int count = 0; 

	FUTEX_LOCK;
	TAILQ_FOREACH(wp, &f->f_waiting_proc, wp_list) {
		if (count <= n) {
			cv_broadcast(&wp->wp_futex_cv);
			count++;
		} else {
			if (newf == NULL)
				continue;
			/* futex_put called after tsleep */
			wp->wp_new_futex = futex_get(newf->f_uaddr);
			cv_broadcast(&wp->wp_futex_cv);
		}
	}
	FUTEX_UNLOCK;

	return count;
}
