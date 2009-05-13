/*	$NetBSD: linux_futex.c,v 1.22.2.1 2009/05/13 17:18:57 jym Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.22.2.1 2009/05/13 17:18:57 jym Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/queue.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/atomic.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

void linux_to_native_timespec(struct timespec *, struct linux_timespec *);

struct futex;

struct waiting_proc {
	lwp_t *wp_l;
	struct futex *wp_new_futex;
	kcondvar_t wp_futex_cv;
	TAILQ_ENTRY(waiting_proc) wp_list;
};
struct futex {
	void *f_uaddr;
	int f_refcount;
	LIST_ENTRY(futex) f_list;
	TAILQ_HEAD(lf_waiting_proc, waiting_proc) f_waiting_proc;
};

static LIST_HEAD(futex_list, futex) futex_list;
static kmutex_t futex_lock;

#define FUTEX_LOCK	mutex_enter(&futex_lock);
#define FUTEX_UNLOCK	mutex_exit(&futex_lock);

#define FUTEX_LOCKED	1
#define FUTEX_UNLOCKED	0

#define FUTEX_SYSTEM_LOCK	KERNEL_LOCK(1, NULL);
#define FUTEX_SYSTEM_UNLOCK	KERNEL_UNLOCK_ONE(0);

#ifdef DEBUG_LINUX_FUTEX
#define FUTEXPRINTF(a) printf a
#else
#define FUTEXPRINTF(a)
#endif

static ONCE_DECL(futex_once);

static int
futex_init(void)
{
	FUTEXPRINTF(("futex_init: initializing futex\n"));
	mutex_init(&futex_lock, MUTEX_DEFAULT, IPL_NONE);
	return 0;
}

static struct futex *futex_get(void *, int);
static void futex_put(struct futex *);
static int futex_sleep(struct futex *, lwp_t *, unsigned long);
static int futex_wake(struct futex *, int, struct futex *, int);
static int futex_atomic_op(lwp_t *, int, void *);

int
linux_sys_futex(struct lwp *l, const struct linux_sys_futex_args *uap, register_t *retval)
{
	/* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct linux_timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */
	int val;
	int ret;
	struct linux_timespec timeout = { 0, 0 };
	int error = 0;
	struct futex *f;
	struct futex *newf;
	int timeout_hz;
	struct timespec ts;
	struct futex *f2;
	int op_ret;

	RUN_ONCE(&futex_once, futex_init);

	/*
	 * Our implementation provides only private futexes. Most of the apps
	 * should use private futexes but don't claim so. Therefore we treat
	 * all futexes as private by clearing the FUTEX_PRIVATE_FLAG. It works
	 * in most cases (ie. when futexes are not shared on file descriptor
	 * or between different processes).
	 */
	switch (SCARG(uap, op) & ~LINUX_FUTEX_PRIVATE_FLAG) {
	case LINUX_FUTEX_WAIT:
		FUTEX_SYSTEM_LOCK;

		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0) {
			FUTEX_SYSTEM_UNLOCK;
			return error;
		}

		if (val != SCARG(uap, val)) {
			FUTEX_SYSTEM_UNLOCK;
			return EWOULDBLOCK;
		}

		if (SCARG(uap, timeout) != NULL) {
			if ((error = copyin(SCARG(uap, timeout), 
			    &timeout, sizeof(timeout))) != 0) {
				FUTEX_SYSTEM_UNLOCK;
				return error;
			}
		}

		FUTEXPRINTF(("FUTEX_WAIT %d.%d: val = %d, uaddr = %p, "
		    "*uaddr = %d, timeout = %lld.%09ld\n", 
		    l->l_proc->p_pid, l->l_lid, SCARG(uap, val), 
		    SCARG(uap, uaddr), val, (long long)timeout.tv_sec,
		    timeout.tv_nsec));

		linux_to_native_timespec(&ts, &timeout);
		if ((error = itimespecfix(&ts)) != 0) {
			FUTEX_SYSTEM_UNLOCK;
			return error;
		}
		timeout_hz = tstohz(&ts);

		/*
		 * If the user process requests a non null timeout,
		 * make sure we do not turn it into an infinite
		 * timeout because timeout_hz is 0.
		 *
		 * We use a minimal timeout of 1/hz. Maybe it would make
		 * sense to just return ETIMEDOUT without sleeping.
		 */
		if (SCARG(uap, timeout) != NULL && timeout_hz == 0)
			timeout_hz = 1;

		f = futex_get(SCARG(uap, uaddr), FUTEX_UNLOCKED);
		ret = futex_sleep(f, l, timeout_hz);
		futex_put(f);

		FUTEXPRINTF(("FUTEX_WAIT %d.%d: uaddr = %p, "
		    "ret = %d\n", l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), ret));

		FUTEX_SYSTEM_UNLOCK;
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
		FUTEX_SYSTEM_LOCK;
		/* 
		 * XXX: Linux is able cope with different addresses 
		 * corresponding to the same mapped memory in the sleeping 
		 * and the waker process(es).
		 */
		FUTEXPRINTF(("FUTEX_WAKE %d.%d: uaddr = %p, val = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), SCARG(uap, val)));

		f = futex_get(SCARG(uap, uaddr), FUTEX_UNLOCKED);
		*retval = futex_wake(f, SCARG(uap, val), NULL, 0);
		futex_put(f);

		FUTEX_SYSTEM_UNLOCK;

		break;

	case LINUX_FUTEX_CMP_REQUEUE:
		FUTEX_SYSTEM_LOCK;

		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0) {
			FUTEX_SYSTEM_UNLOCK;
			return error;
		}

		if (val != SCARG(uap, val3)) {
			FUTEX_SYSTEM_UNLOCK;
			return EAGAIN;
		}

		f = futex_get(SCARG(uap, uaddr), FUTEX_UNLOCKED);
		newf = futex_get(SCARG(uap, uaddr2), FUTEX_UNLOCKED);
		*retval = futex_wake(f, SCARG(uap, val), newf,
		    (int)(unsigned long)SCARG(uap, timeout));
		futex_put(f);
		futex_put(newf);

		FUTEX_SYSTEM_UNLOCK;
		break;

	case LINUX_FUTEX_REQUEUE:
		FUTEX_SYSTEM_LOCK;

		f = futex_get(SCARG(uap, uaddr), FUTEX_UNLOCKED);
		newf = futex_get(SCARG(uap, uaddr2), FUTEX_UNLOCKED);
		*retval = futex_wake(f, SCARG(uap, val), newf,
		    (int)(unsigned long)SCARG(uap, timeout));
		futex_put(f);
		futex_put(newf);

		FUTEX_SYSTEM_UNLOCK;
		break;

	case LINUX_FUTEX_FD:
		FUTEXPRINTF(("linux_sys_futex: unimplemented op %d\n", 
		    SCARG(uap, op)));
		return ENOSYS;
	case LINUX_FUTEX_WAKE_OP:
		FUTEX_SYSTEM_LOCK;
		f = futex_get(SCARG(uap, uaddr), FUTEX_UNLOCKED);
		f2 = futex_get(SCARG(uap, uaddr2), FUTEX_UNLOCKED);
		/*
		 * This function returns positive number as results and
		 * negative as errors
		 */
		op_ret = futex_atomic_op(l, SCARG(uap, val3), SCARG(uap, uaddr2));
		if (op_ret < 0) {
			/* XXX: We don't handle EFAULT yet */
			if (op_ret != -EFAULT) {
				futex_put(f);
				futex_put(f2);
				FUTEX_SYSTEM_UNLOCK;
				return -op_ret;
			}
			futex_put(f);
			futex_put(f2);
			FUTEX_SYSTEM_UNLOCK;
			return EFAULT;
		}

		ret = futex_wake(f, SCARG(uap, val), NULL, 0);
		futex_put(f);
		if (op_ret > 0) {
			op_ret = 0;
			/*
			 * Linux abuses the address of the timespec parameter
			 * as the number of retries
			 */
			op_ret += futex_wake(f2,
			    (int)(unsigned long)SCARG(uap, timeout), NULL, 0);
			ret += op_ret;
		}
		futex_put(f2);
		*retval = ret;
		FUTEX_SYSTEM_UNLOCK;
		break;
	default:
		FUTEXPRINTF(("linux_sys_futex: unknown op %d\n", 
		    SCARG(uap, op)));
		return ENOSYS;
	}
	return 0;
}

static struct futex *
futex_get(void *uaddr, int locked)
{
	struct futex *f;

	if (locked == FUTEX_UNLOCKED)
		FUTEX_LOCK;

	LIST_FOREACH(f, &futex_list, f_list) {
		if (f->f_uaddr == uaddr) {
			f->f_refcount++;
			if (locked == FUTEX_UNLOCKED)
				FUTEX_UNLOCK;
			return f;
		}
	}

	/* Not found, create it */
	f = kmem_zalloc(sizeof(*f), KM_SLEEP);
	f->f_uaddr = uaddr;
	f->f_refcount = 1;
	TAILQ_INIT(&f->f_waiting_proc);
	LIST_INSERT_HEAD(&futex_list, f, f_list);
	if (locked == FUTEX_UNLOCKED)
		FUTEX_UNLOCK;

	return f;
}

static void 
futex_put(struct futex *f)
{

	FUTEX_LOCK;
	f->f_refcount--;
	if (f->f_refcount == 0) {
		KASSERT(TAILQ_EMPTY(&f->f_waiting_proc));
		LIST_REMOVE(f, f_list);
		kmem_free(f, sizeof(*f));
	}
	FUTEX_UNLOCK;

	return;
}

static int 
futex_sleep(struct futex *f, lwp_t *l, unsigned long timeout)
{
	struct waiting_proc *wp;
	int ret;

	wp = kmem_zalloc(sizeof(*wp), KM_SLEEP);
	wp->wp_l = l;
	wp->wp_new_futex = NULL;
	cv_init(&wp->wp_futex_cv, "futex");

	FUTEX_LOCK;
	TAILQ_INSERT_TAIL(&f->f_waiting_proc, wp, wp_list);
	ret = cv_timedwait_sig(&wp->wp_futex_cv, &futex_lock, timeout);
	TAILQ_REMOVE(&f->f_waiting_proc, wp, wp_list);
	FUTEX_UNLOCK;

	/* if we got woken up in futex_wake */
	if ((ret == 0) && (wp->wp_new_futex != NULL)) {
		/* suspend us on the new futex */
		ret = futex_sleep(wp->wp_new_futex, l, timeout);
		/* and release the old one */
		futex_put(wp->wp_new_futex);
	}

	cv_destroy(&wp->wp_futex_cv);
	kmem_free(wp, sizeof(*wp));
	return ret;
}

static int
futex_wake(struct futex *f, int n, struct futex *newf, int n2)
{
	struct waiting_proc *wp;
	int count;

	count = newf ? 0 : 1;

	FUTEX_LOCK;
	TAILQ_FOREACH(wp, &f->f_waiting_proc, wp_list) {
		if (count <= n) {
			cv_signal(&wp->wp_futex_cv);
			count++;
		} else {
			if (newf == NULL)
				continue;
			/* futex_put called after tsleep */
			wp->wp_new_futex = futex_get(newf->f_uaddr,
			    FUTEX_LOCKED);
			cv_signal(&wp->wp_futex_cv);
			if (count - n >= n2)
				break;
		}
	}
	FUTEX_UNLOCK;

	return count;
}

static int
futex_atomic_op(lwp_t *l, int encoded_op, void *uaddr)
{
	const int op = (encoded_op >> 28) & 7;
	const int cmp = (encoded_op >> 24) & 15;
	const int cmparg = (encoded_op << 20) >> 20;
	int oparg = (encoded_op << 8) >> 20;
	int error, oldval, cval;

	if (encoded_op & (FUTEX_OP_OPARG_SHIFT << 28))
		oparg = 1 << oparg;

	/* XXX: linux verifies access here and returns EFAULT */

	if (copyin(uaddr, &cval, sizeof(int)) != 0)
		return -EFAULT;

	for (;;) {
		int nval;

		switch (op) {
		case FUTEX_OP_SET:
			nval = oparg;
			break;
		case FUTEX_OP_ADD:
			nval = cval + oparg;
			break;
		case FUTEX_OP_OR:
			nval = cval | oparg;
			break;
		case FUTEX_OP_ANDN:
			nval = cval & ~oparg;
			break;
		case FUTEX_OP_XOR:
			nval = cval ^ oparg;
			break;
		default:
			return -ENOSYS;
		}

		error = ucas_int(uaddr, cval, nval, &oldval);
		if (oldval == cval || error) {
			break;
		}
		cval = oldval;
	}

	if (error)
		return -EFAULT;

	switch (cmp) {
	case FUTEX_OP_CMP_EQ:
		return (oldval == cmparg);
	case FUTEX_OP_CMP_NE:
		return (oldval != cmparg);
	case FUTEX_OP_CMP_LT:
		return (oldval < cmparg);
	case FUTEX_OP_CMP_GE:
		return (oldval >= cmparg);
	case FUTEX_OP_CMP_LE:
		return (oldval <= cmparg);
	case FUTEX_OP_CMP_GT:
		return (oldval > cmparg);
	default:
		return -ENOSYS;
	}
}

int
linux_sys_set_robust_list(struct lwp *l,
    const struct linux_sys_set_robust_list_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct linux_emuldata *led = p->p_emuldata;

	if (SCARG(uap, len) != sizeof(*(led->robust_futexes)))
		return EINVAL;
	led->robust_futexes = SCARG(uap, head);
	*retval = 0;
	return 0;
}

int
linux_sys_get_robust_list(struct lwp *l,
    const struct linux_sys_get_robust_list_args *uap, register_t *retval)
{
	struct linux_emuldata *led;
	struct linux_robust_list_head **head;
	size_t len = sizeof(*led->robust_futexes);
	int error = 0;

	if (!SCARG(uap, pid)) {
		led = l->l_proc->p_emuldata;
		head = &led->robust_futexes;
	} else {
		struct proc *p;

		mutex_enter(proc_lock); 
		if ((p = p_find(SCARG(uap, pid), PFIND_LOCKED)) == NULL ||
		    p->p_emul != &emul_linux) {
			mutex_exit(proc_lock);
			return ESRCH;
		}
		led = p->p_emuldata;
		head = &led->robust_futexes;
		mutex_exit(proc_lock);
	}

	error = copyout(&len, SCARG(uap, len), sizeof(len));
	if (error)
		return error;
	return copyout(head, SCARG(uap, head), sizeof(*head));
}

static int
handle_futex_death(void *uaddr, pid_t pid, int pi)
{
	int uval, nval, mval;
	struct futex *f;

retry:
	if (copyin(uaddr, &uval, 4))
		return EFAULT;

	if ((uval & FUTEX_TID_MASK) == pid) {
		mval = (uval & FUTEX_WAITERS) | FUTEX_OWNER_DIED;
		nval = atomic_cas_32(uaddr, uval, mval);

		if (nval == -1)
			return EFAULT;

		if (nval != uval)
			goto retry;

		if (!pi && (uval & FUTEX_WAITERS)) {
			f = futex_get(uaddr, FUTEX_UNLOCKED);
			futex_wake(f, 1, NULL, 0);
		}
	}

	return 0;
}

static int
fetch_robust_entry(struct linux_robust_list **entry,
    struct linux_robust_list **head, int *pi)
{
	unsigned long uentry;

	if (copyin((const void *)head, &uentry, sizeof(unsigned long)))
		return EFAULT;

	*entry = (void *)(uentry & ~1UL);
	*pi = uentry & 1;

	return 0;
}

/* This walks the list of robust futexes, releasing them. */
void
release_futexes(struct proc *p)
{
	struct linux_robust_list_head head;
	struct linux_robust_list *entry, *next_entry = NULL, *pending;
	unsigned int limit = 2048, pi, next_pi, pip;
	struct linux_emuldata *led;
	unsigned long futex_offset;
	int rc;

	led = p->p_emuldata;
	if (led->robust_futexes == NULL)
		return;

	if (copyin(led->robust_futexes, &head, sizeof(head)))
		return;

	if (fetch_robust_entry(&entry, &head.list.next, &pi))
		return;

	if (copyin(&head.futex_offset, &futex_offset, sizeof(unsigned long)))
		return;

	if (fetch_robust_entry(&pending, &head.pending_list, &pip))
		return;

	while (entry != &head.list) {
		rc = fetch_robust_entry(&next_entry, &entry->next, &next_pi);

		if (entry != pending)
			if (handle_futex_death((char *)entry + futex_offset,
			    p->p_pid, pi))
				return;

		if (rc)
			return;

		entry = next_entry;
		pi = next_pi;

		if (!--limit)
			break;

		yield();	/* XXX why? */
	}

	if (pending)
		handle_futex_death((char *)pending + futex_offset,
		    p->p_pid, pip);
}
