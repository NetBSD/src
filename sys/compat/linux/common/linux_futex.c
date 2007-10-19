/*	$NetBSD: linux_futex.c,v 1.8 2007/10/19 18:52:11 njoly Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.8 2007/10/19 18:52:11 njoly Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

struct futex;

struct waiting_proc {
	struct lwp *wp_l;
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
static struct lock *futex_lock = NULL;

#define FUTEX_LOCK (void)lockmgr(futex_lock, LK_EXCLUSIVE, NULL)
#define FUTEX_UNLOCK (void)lockmgr(futex_lock, LK_RELEASE, NULL)

static struct futex *futex_get(void *);
static void futex_put(struct futex *);
static int futex_sleep(struct futex *, struct lwp *, unsigned long);
static int futex_wake(struct futex *, int, struct futex *);

int
linux_sys_futex(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_futex_args /* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */ *uap = v;
	int val;
	int ret;
	struct timespec timeout = { 0, 0 };
	int error = 0;
	struct futex *f;
	struct futex *newf;
	int timeout_hz;

	/* First time use */
	if (futex_lock == NULL) {
		futex_lock = malloc(sizeof(struct lock), 
		    M_EMULDATA, M_WAITOK);
		lockinit(futex_lock, PZERO|PCATCH, "lockfutex", 0, 0);
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
		}

#ifdef DEBUG_LINUX_FUTEX
		printf("FUTEX_WAIT %d.%d: val = %d, uaddr = %p, "
		    "*uaddr = %d, timeout = %d.%09ld\n", 
		    l->l_proc->p_pid, l->l_lid, SCARG(uap, val), 
		    SCARG(uap, uaddr), val, timeout.tv_sec, timeout.tv_nsec); 
#endif
		timeout_hz =
		    mstohz(timeout.tv_sec * 1000 + timeout.tv_nsec / 1000000);

		/*
		 * If the user process requests a non null timeout, 
		 * make sure we do not turn it into an infinite 
		 * timeout because timeout_hz gets null.
		 * 
		 * We use a minimal timeout of 1/hz. Mayve it would
		 * make sense to just return ETIMEDOUT without sleeping.
		 */
		if (((timeout.tv_sec != 0) || (timeout.tv_nsec != 0)) &&
		    (timeout_hz == 0)) 
			timeout_hz = 1;

		f = futex_get(SCARG(uap, uaddr));
		ret = futex_sleep(f, l, timeout_hz);
		futex_put(f);

#ifdef DEBUG_LINUX_FUTEX
		printf("FUTEX_WAIT %d.%d: uaddr = %p, "
		    "ret = %d\n", l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), ret); 
#endif

		switch (ret) {
		case EWOULDBLOCK:	/* timeout */
			return ETIMEDOUT;
			break;
		case EINTR:		/* signal */
			return EINTR;
			break;
		case 0:			/* FUTEX_WAKE received */
#ifdef DEBUG_LINUX_FUTEX
			printf("FUTEX_WAIT %d.%d: uaddr = %p, got FUTEX_WAKE\n",
			    l->l_proc->p_pid, l->l_lid, SCARG(uap, uaddr)); 
#endif
			return 0;
			break;
		default:
#ifdef DEBUG_LINUX_FUTEX
			printf("FUTEX_WAIT: unexpected ret = %d\n", ret);
#endif
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
#ifdef DEBUG_LINUX_FUTEX
		printf("FUTEX_WAKE %d.%d: uaddr = %p, val = %d\n", 
		    l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), SCARG(uap, val)); 
#endif
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
		printf("linux_sys_futex: unimplemented op %d\n", 
		    SCARG(uap, op));
		break;
	default:
		printf("linux_sys_futex: unknown op %d\n", 
		    SCARG(uap, op));
		break;
	}
	return 0;
}

static struct futex *
futex_get(uaddr)
	void *uaddr;
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
	f = malloc(sizeof(*f), M_EMULDATA, M_WAITOK);
	f->f_uaddr = uaddr;
	f->f_refcount = 1;
	TAILQ_INIT(&f->f_waiting_proc);
	FUTEX_LOCK;
	LIST_INSERT_HEAD(&futex_list, f, f_list);
	FUTEX_UNLOCK;

	return f;
}

static void 
futex_put(f)
	struct futex *f;
{
	f->f_refcount--;
	if (f->f_refcount == 0) {
		FUTEX_LOCK;
		LIST_REMOVE(f, f_list);
		FUTEX_UNLOCK;
		free(f, M_EMULDATA);
	}

	return;
}

static int 
futex_sleep(f, l, timeout)
	struct futex *f;
	struct lwp *l;
	unsigned long timeout;
{
	struct waiting_proc *wp;
	int ret;

	wp = malloc(sizeof(*wp), M_EMULDATA, M_WAITOK);
	wp->wp_l = l;
	wp->wp_new_futex = NULL;
	FUTEX_LOCK;
	TAILQ_INSERT_TAIL(&f->f_waiting_proc, wp, wp_list);
	FUTEX_UNLOCK;

#ifdef DEBUG_LINUX_FUTEX
	printf("FUTEX --> %d.%d tlseep timeout = %ld\n", l->l_proc->p_pid,
	    l->l_lid, timeout);
#endif
	ret = tsleep(wp, PCATCH|PZERO, "linuxfutex", timeout);
#ifdef DEBUG_LINUX_FUTEX
	printf("FUTEX -> %d.%d tsleep returns %d\n", 
	    l->l_proc->p_pid, l->l_lid, ret);
#endif

	FUTEX_LOCK;
	TAILQ_REMOVE(&f->f_waiting_proc, wp, wp_list);
	FUTEX_UNLOCK;

	if ((ret == 0) && (wp->wp_new_futex != NULL)) {
		ret = futex_sleep(wp->wp_new_futex, l, timeout);
		futex_put(wp->wp_new_futex); /* futex_get called in wakeup */
	}

	free(wp, M_EMULDATA);

	return ret;
}

static int
futex_wake(f, n, newf)
	struct futex *f;
	int n;
	struct futex *newf;
{
	struct waiting_proc *wp;
	int count = 0; 

	FUTEX_LOCK;
	TAILQ_FOREACH(wp, &f->f_waiting_proc, wp_list) {
		if (count <= n) {
			wakeup(wp);
			count++;
		} else {
			if (newf != NULL) {
				/* futex_put called after tsleep */
				wp->wp_new_futex = futex_get(newf->f_uaddr);
				wakeup(wp);
			}
		}
	}
	FUTEX_UNLOCK;

	return count;
}
