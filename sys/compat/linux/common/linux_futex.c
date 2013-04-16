/*	$NetBSD: linux_futex.c,v 1.29 2013/04/16 23:03:05 christos Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.29 2013/04/16 23:03:05 christos Exp $");

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
#include <sys/atomic.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscallargs.h>

struct futex;

struct waiting_proc {
	lwp_t *wp_l;
	struct futex *wp_new_futex;
	kcondvar_t wp_futex_cv;
	TAILQ_ENTRY(waiting_proc) wp_list;
	TAILQ_ENTRY(waiting_proc) wp_rqlist;
};
struct futex {
	void *f_uaddr;
	int f_refcount;
	uint32_t f_bitset;
	LIST_ENTRY(futex) f_list;
	TAILQ_HEAD(, waiting_proc) f_waiting_proc;
	TAILQ_HEAD(, waiting_proc) f_requeue_proc;
};

static LIST_HEAD(futex_list, futex) futex_list;
static kmutex_t futex_lock;

#define FUTEX_LOCK	mutex_enter(&futex_lock)
#define FUTEX_UNLOCK	mutex_exit(&futex_lock)
#define FUTEX_LOCKASSERT	KASSERT(mutex_owned(&futex_lock))

#define FUTEX_SYSTEM_LOCK	KERNEL_LOCK(1, NULL)
#define FUTEX_SYSTEM_UNLOCK	KERNEL_UNLOCK_ONE(0)

#ifdef DEBUG_LINUX_FUTEX
int debug_futex = 1;
#define FUTEXPRINTF(a) do { if (debug_futex) printf a; } while (0)
#else
#define FUTEXPRINTF(a)
#endif

void
linux_futex_init(void)
{
	FUTEXPRINTF(("%s: initializing futex\n", __func__));
	mutex_init(&futex_lock, MUTEX_DEFAULT, IPL_NONE);
}

void
linux_futex_fini(void)
{
	FUTEXPRINTF(("%s: destroying futex\n", __func__));
	mutex_destroy(&futex_lock);
}

static struct waiting_proc *futex_wp_alloc(void);
static void futex_wp_free(struct waiting_proc *);
static struct futex *futex_get(void *, uint32_t);
static void futex_ref(struct futex *);
static void futex_put(struct futex *);
static int futex_sleep(struct futex **, lwp_t *, int, struct waiting_proc *);
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
	struct linux_timespec lts;
	struct timespec ts = { 0, 0 };
	int error;

	if ((SCARG(uap, op) & LINUX_FUTEX_CMD_MASK) == LINUX_FUTEX_WAIT &&
	    SCARG(uap, timeout) != NULL) {
		if ((error = copyin(SCARG(uap, timeout), 
		    &lts, sizeof(lts))) != 0) {
			return error;
		}
		linux_to_native_timespec(&ts, &lts);
	}
	return linux_do_futex(l, uap, retval, &ts);
}

int
linux_do_futex(struct lwp *l, const struct linux_sys_futex_args *uap, register_t *retval, struct timespec *ts)
{
	/* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct linux_timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */
	int val, val3;
	int ret;
	int error = 0;
	struct futex *f;
	struct futex *newf;
	int tout;
	struct futex *f2;
	struct waiting_proc *wp;
	int op_ret, cmd;
	clockid_t clk;

	cmd = SCARG(uap, op) & LINUX_FUTEX_CMD_MASK;
	val3 = SCARG(uap, val3);

	if (SCARG(uap, op) & LINUX_FUTEX_CLOCK_REALTIME) {
		switch (cmd) {
		case LINUX_FUTEX_WAIT_BITSET:
		case LINUX_FUTEX_WAIT:
			clk = CLOCK_REALTIME;
			break;
		default:
			return ENOSYS;
		}
	} else
		clk = CLOCK_MONOTONIC;

	/*
	 * Our implementation provides only private futexes. Most of the apps
	 * should use private futexes but don't claim so. Therefore we treat
	 * all futexes as private by clearing the FUTEX_PRIVATE_FLAG. It works
	 * in most cases (ie. when futexes are not shared on file descriptor
	 * or between different processes).
	 *
	 * Note that we don't handle bitsets at all at the moment. We need
	 * to move from refcounting uaddr's to handling multiple futex entries
	 * pointing to the same uaddr, but having possibly different bitmask.
	 * Perhaps move to an implementation where each uaddr has a list of
	 * futexes.
	 */
	switch (cmd) {
	case LINUX_FUTEX_WAIT:
		val3 = FUTEX_BITSET_MATCH_ANY;
		/*FALLTHROUGH*/
	case LINUX_FUTEX_WAIT_BITSET:
		if ((error = ts2timo(clk, 0, ts, &tout, NULL)) != 0) {
			/*
			 * If the user process requests a non null timeout,
			 * make sure we do not turn it into an infinite
			 * timeout because tout is 0.
			 *
			 * We use a minimal timeout of 1/hz. Maybe it would make
			 * sense to just return ETIMEDOUT without sleeping.
			 */
			if (error == ETIMEDOUT && SCARG(uap, timeout) != NULL)
				tout = 1;
			else
				return error;
		}
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

		FUTEXPRINTF(("FUTEX_WAIT %d.%d: val = %d, uaddr = %p, "
		    "*uaddr = %d, timeout = %lld.%09ld\n", 
		    l->l_proc->p_pid, l->l_lid, SCARG(uap, val), 
		    SCARG(uap, uaddr), val, (long long)ts->tv_sec,
		    ts->tv_nsec));


		wp = futex_wp_alloc();
		FUTEX_LOCK;
		f = futex_get(SCARG(uap, uaddr), val3);
		ret = futex_sleep(&f, l, tout, wp);
		futex_put(f);
		FUTEX_UNLOCK;
		futex_wp_free(wp);

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
		val = FUTEX_BITSET_MATCH_ANY;
		/*FALLTHROUGH*/
	case LINUX_FUTEX_WAKE_BITSET:
		/* 
		 * XXX: Linux is able cope with different addresses 
		 * corresponding to the same mapped memory in the sleeping 
		 * and the waker process(es).
		 */
		FUTEXPRINTF(("FUTEX_WAKE %d.%d: uaddr = %p, val = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), SCARG(uap, val)));

		FUTEX_SYSTEM_LOCK;
		FUTEX_LOCK;
		f = futex_get(SCARG(uap, uaddr), val3);
		*retval = futex_wake(f, SCARG(uap, val), NULL, 0);
		futex_put(f);
		FUTEX_UNLOCK;
		FUTEX_SYSTEM_UNLOCK;

		break;

	case LINUX_FUTEX_CMP_REQUEUE:
		FUTEX_SYSTEM_LOCK;

		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0) {
			FUTEX_SYSTEM_UNLOCK;
			return error;
		}

		if (val != val3) {
			FUTEX_SYSTEM_UNLOCK;
			return EAGAIN;
		}

		FUTEXPRINTF(("FUTEX_CMP_REQUEUE %d.%d: uaddr = %p, val = %d, "
		    "uaddr2 = %p, val2 = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), SCARG(uap, val), SCARG(uap, uaddr2),
		    (int)(unsigned long)SCARG(uap, timeout)));

		FUTEX_LOCK;
		f = futex_get(SCARG(uap, uaddr), val3);
		newf = futex_get(SCARG(uap, uaddr2), val3);
		*retval = futex_wake(f, SCARG(uap, val), newf,
		    (int)(unsigned long)SCARG(uap, timeout));
		futex_put(f);
		futex_put(newf);
		FUTEX_UNLOCK;

		FUTEX_SYSTEM_UNLOCK;
		break;

	case LINUX_FUTEX_REQUEUE:
		FUTEX_SYSTEM_LOCK;

		FUTEXPRINTF(("FUTEX_REQUEUE %d.%d: uaddr = %p, val = %d, "
		    "uaddr2 = %p, val2 = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), SCARG(uap, val), SCARG(uap, uaddr2),
		    (int)(unsigned long)SCARG(uap, timeout)));

		FUTEX_LOCK;
		f = futex_get(SCARG(uap, uaddr), val3);
		newf = futex_get(SCARG(uap, uaddr2), val3);
		*retval = futex_wake(f, SCARG(uap, val), newf,
		    (int)(unsigned long)SCARG(uap, timeout));
		futex_put(f);
		futex_put(newf);
		FUTEX_UNLOCK;

		FUTEX_SYSTEM_UNLOCK;
		break;

	case LINUX_FUTEX_FD:
		FUTEXPRINTF(("%s: unimplemented op %d\n", __func__, cmd));
		return ENOSYS;
	case LINUX_FUTEX_WAKE_OP:
		FUTEX_SYSTEM_LOCK;

		FUTEXPRINTF(("FUTEX_WAKE_OP %d.%d: uaddr = %p, op = %d, "
		    "val = %d, uaddr2 = %p, val2 = %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    SCARG(uap, uaddr), cmd, SCARG(uap, val),
		    SCARG(uap, uaddr2),
		    (int)(unsigned long)SCARG(uap, timeout)));

		FUTEX_LOCK;
		f = futex_get(SCARG(uap, uaddr), val3);
		f2 = futex_get(SCARG(uap, uaddr2), val3);
		FUTEX_UNLOCK;

		/*
		 * This function returns positive number as results and
		 * negative as errors
		 */
		op_ret = futex_atomic_op(l, val3, SCARG(uap, uaddr2));
		FUTEX_LOCK;
		if (op_ret < 0) {
			futex_put(f);
			futex_put(f2);
			FUTEX_UNLOCK;
			FUTEX_SYSTEM_UNLOCK;
			return -op_ret;
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
		FUTEX_UNLOCK;
		FUTEX_SYSTEM_UNLOCK;
		*retval = ret;
		break;
	default:
		FUTEXPRINTF(("%s: unknown op %d\n", __func__, cmd));
		return ENOSYS;
	}
	return 0;
}

static struct waiting_proc *
futex_wp_alloc(void)
{
	struct waiting_proc *wp;

	wp = kmem_zalloc(sizeof(*wp), KM_SLEEP);
	cv_init(&wp->wp_futex_cv, "futex");
	return wp;
}

static void
futex_wp_free(struct waiting_proc *wp)
{

	cv_destroy(&wp->wp_futex_cv);
	kmem_free(wp, sizeof(*wp));
}

static struct futex *
futex_get(void *uaddr, uint32_t bitset)
{
	struct futex *f;

	FUTEX_LOCKASSERT;

	LIST_FOREACH(f, &futex_list, f_list) {
		if (f->f_uaddr == uaddr) {
			f->f_refcount++;
			return f;
		}
	}

	/* Not found, create it */
	f = kmem_zalloc(sizeof(*f), KM_SLEEP);
	f->f_uaddr = uaddr;
	f->f_bitset = bitset;
	f->f_refcount = 1;
	TAILQ_INIT(&f->f_waiting_proc);
	TAILQ_INIT(&f->f_requeue_proc);
	LIST_INSERT_HEAD(&futex_list, f, f_list);

	return f;
}

static void
futex_ref(struct futex *f)
{

	FUTEX_LOCKASSERT;

	f->f_refcount++;
}

static void 
futex_put(struct futex *f)
{

	FUTEX_LOCKASSERT;

	f->f_refcount--;
	if (f->f_refcount == 0) {
		KASSERT(TAILQ_EMPTY(&f->f_waiting_proc));
		KASSERT(TAILQ_EMPTY(&f->f_requeue_proc));
		LIST_REMOVE(f, f_list);
		kmem_free(f, sizeof(*f));
	}
}

static int 
futex_sleep(struct futex **fp, lwp_t *l, int timeout, struct waiting_proc *wp)
{
	struct futex *f, *newf;
	int ret;

	FUTEX_LOCKASSERT;

	f = *fp;
	wp->wp_l = l;
	wp->wp_new_futex = NULL;

requeue:
	TAILQ_INSERT_TAIL(&f->f_waiting_proc, wp, wp_list);
	ret = cv_timedwait_sig(&wp->wp_futex_cv, &futex_lock, timeout);
	TAILQ_REMOVE(&f->f_waiting_proc, wp, wp_list);

	/* if futex_wake() tells us to requeue ... */
	newf = wp->wp_new_futex;
	if (ret == 0 && newf != NULL) {
		/* ... requeue ourselves on the new futex */
		futex_put(f);
		wp->wp_new_futex = NULL;
		TAILQ_REMOVE(&newf->f_requeue_proc, wp, wp_rqlist);
		*fp = f = newf;
		goto requeue;
	}
	return ret;
}

static int
futex_wake(struct futex *f, int n, struct futex *newf, int n2)
{
	struct waiting_proc *wp, *wpnext;
	int count;

	FUTEX_LOCKASSERT;

	count = newf ? 0 : 1;

	/*
	 * first, wake up any threads sleeping on this futex.
	 * note that sleeping threads are not in the process of requeueing.
	 */

	TAILQ_FOREACH(wp, &f->f_waiting_proc, wp_list) {
		KASSERT(wp->wp_new_futex == NULL);

		FUTEXPRINTF(("%s: signal f %p l %p ref %d\n", __func__,
		    f, wp->wp_l, f->f_refcount));
		cv_signal(&wp->wp_futex_cv);
		if (count <= n) {
			count++;
		} else {
			if (newf == NULL)
				break;

			/* matching futex_put() is called by the other thread. */
			futex_ref(newf);
			wp->wp_new_futex = newf;
			TAILQ_INSERT_TAIL(&newf->f_requeue_proc, wp, wp_rqlist);
			FUTEXPRINTF(("%s: requeue newf %p l %p ref %d\n",
			    __func__, newf, wp->wp_l, newf->f_refcount));
			if (count - n >= n2)
				goto out;
		}
	}

	/*
	 * next, deal with threads that are requeuing to this futex.
	 * we don't need to signal these threads, any thread on the
	 * requeue list has already been signaled but hasn't had a chance
	 * to run and requeue itself yet.  if we would normally wake
	 * a thread, just remove the requeue info.  if we would normally
	 * requeue a thread, change the requeue target.
	 */

	TAILQ_FOREACH_SAFE(wp, &f->f_requeue_proc, wp_rqlist, wpnext) {
		KASSERT(wp->wp_new_futex == f);

		FUTEXPRINTF(("%s: unrequeue f %p l %p ref %d\n", __func__,
		    f, wp->wp_l, f->f_refcount));
		wp->wp_new_futex = NULL;
		TAILQ_REMOVE(&f->f_requeue_proc, wp, wp_rqlist);
		futex_put(f);

		if (count <= n) {
			count++;
		} else {
			if (newf == NULL)
				break;

			/* matching futex_put() is called by the other thread. */
			futex_ref(newf);
			wp->wp_new_futex = newf;
			TAILQ_INSERT_TAIL(&newf->f_requeue_proc, wp, wp_rqlist);
			FUTEXPRINTF(("%s: rerequeue newf %p l %p ref %d\n",
			    __func__, newf, wp->wp_l, newf->f_refcount));
			if (count - n >= n2)
				break;
		}
	}

out:
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
	/* {
		syscallarg(struct linux_robust_list_head *) head;
		syscallarg(size_t) len;
	} */
	struct linux_emuldata *led;

	if (SCARG(uap, len) != sizeof(struct linux_robust_list_head))
		return EINVAL;
	led = l->l_emuldata;
	led->led_robust_head = SCARG(uap, head);
	*retval = 0;
	return 0;
}

int
linux_sys_get_robust_list(struct lwp *l,
    const struct linux_sys_get_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(struct linux_robust_list_head **) head;
		syscallarg(size_t *) len;
	} */
	struct proc *p;
	struct linux_emuldata *led;
	struct linux_robust_list_head *head;
	size_t len;
	int error = 0;

	p = l->l_proc;
	if (!SCARG(uap, pid)) {
		led = l->l_emuldata;
		head = led->led_robust_head;
	} else {
		mutex_enter(p->p_lock);
		l = lwp_find(p, SCARG(uap, pid));
		if (l != NULL) {
			led = l->l_emuldata;
			head = led->led_robust_head;
		}
		mutex_exit(p->p_lock);
		if (l == NULL) {
			return ESRCH;
		}
	}
#ifdef __arch64__
	if (p->p_flag & PK_32) {
		uint32_t u32;

		u32 = 12;
		error = copyout(&u32, SCARG(uap, len), sizeof(u32));
		if (error)
			return error;
		u32 = (uint32_t)(uintptr_t)head;
		return copyout(&u32, SCARG(uap, head), sizeof(u32));
	}
#endif

	len = sizeof(*head);
	error = copyout(&len, SCARG(uap, len), sizeof(len));
	if (error)
		return error;
	return copyout(&head, SCARG(uap, head), sizeof(head));
}

static int
handle_futex_death(void *uaddr, pid_t pid, int pi)
{
	int uval, nval, mval;
	struct futex *f;

retry:
	if (copyin(uaddr, &uval, sizeof(uval)))
		return EFAULT;

	if ((uval & FUTEX_TID_MASK) == pid) {
		mval = (uval & FUTEX_WAITERS) | FUTEX_OWNER_DIED;
		nval = atomic_cas_32(uaddr, uval, mval);

		if (nval == -1)
			return EFAULT;

		if (nval != uval)
			goto retry;

		if (!pi && (uval & FUTEX_WAITERS)) {
			FUTEX_LOCK;
			f = futex_get(uaddr, FUTEX_BITSET_MATCH_ANY);
			futex_wake(f, 1, NULL, 0);
			FUTEX_UNLOCK;
		}
	}

	return 0;
}

static int
fetch_robust_entry(struct lwp *l, struct linux_robust_list **entry,
    struct linux_robust_list **head, int *pi)
{
	struct linux_emuldata *led;
	unsigned long uentry;

	led = l->l_emuldata;
#ifdef __arch64__
	if (l->l_proc->p_flag & PK_32) {
		uint32_t u32;

		if (copyin(head, &u32, sizeof(u32)))
			return EFAULT;
		uentry = (unsigned long)u32;
	} else
#endif
	if (copyin(head, &uentry, sizeof(uentry)))
		return EFAULT;

	*entry = (void *)(uentry & ~1UL);
	*pi = uentry & 1;

	return 0;
}

/* This walks the list of robust futexes, releasing them. */
void
release_futexes(struct lwp *l)
{
	struct linux_robust_list_head head;
	struct linux_robust_list *entry, *next_entry = NULL, *pending;
	unsigned int limit = 2048, pi, next_pi, pip;
	struct linux_emuldata *led;
	unsigned long futex_offset;
	int rc;

	led = l->l_emuldata;
	if (led->led_robust_head == NULL)
		return;

#ifdef __arch64__
	if (l->l_proc->p_flag & PK_32) {
		uint32_t u32s[3];

		if (copyin(led->led_robust_head, u32s, sizeof(u32s)))
			return;

		head.list.next = (void *)(uintptr_t)u32s[0];
		head.futex_offset = (unsigned long)u32s[1];
		head.pending_list = (void *)(uintptr_t)u32s[2];
	} else
#endif
	if (copyin(led->led_robust_head, &head, sizeof(head)))
		return;

	if (fetch_robust_entry(l, &entry, &head.list.next, &pi))
		return;

#ifdef __arch64__
	if (l->l_proc->p_flag & PK_32) {
		uint32_t u32;

		if (copyin(led->led_robust_head, &u32, sizeof(u32)))
			return;

		head.futex_offset = (unsigned long)u32;
	} else
#endif
	if (copyin(&head.futex_offset, &futex_offset, sizeof(unsigned long)))
		return;

	if (fetch_robust_entry(l, &pending, &head.pending_list, &pip))
		return;

	while (entry != &head.list) {
		rc = fetch_robust_entry(l, &next_entry, &entry->next, &next_pi);

		if (entry != pending)
			if (handle_futex_death((char *)entry + futex_offset,
			    l->l_lid, pi))
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
		    l->l_lid, pip);
}
