/*	$NetBSD: subr_workqueue.c,v 1.19.2.2 2007/08/05 13:47:26 ad Exp $	*/

/*-
 * Copyright (c)2002, 2005 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_workqueue.c,v 1.19.2.2 2007/08/05 13:47:26 ad Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/workqueue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/queue.h>

typedef struct work_impl {
	SIMPLEQ_ENTRY(work_impl) wk_entry;
} work_impl_t;

SIMPLEQ_HEAD(workqhead, work_impl);

struct workqueue_queue {
	kmutex_t q_mutex;
	kcondvar_t q_cv;
	struct workqhead q_queue;
	struct lwp *q_worker;
	SLIST_ENTRY(workqueue_queue) q_list;
};

struct workqueue {
	void (*wq_func)(struct work *, void *);
	void *wq_arg;
	const char *wq_name;
	pri_t wq_prio;
	int wq_flags;
	void *wq_ptr;
	ipl_cookie_t wq_ipl;
};

#ifdef MULTIPROCESSOR
#define	CPU_ALIGN_SIZE		CACHE_LINE_SIZE
#else
#define	CPU_ALIGN_SIZE		(ALIGNBYTES + 1)
#endif

#define	WQ_SIZE		(roundup2(sizeof(struct workqueue), CPU_ALIGN_SIZE))
#define	WQ_QUEUE_SIZE	(roundup2(sizeof(struct workqueue_queue), CPU_ALIGN_SIZE))

#define	POISON	0xaabbccdd

static struct workqueue_queue *
workqueue_queue_lookup(struct workqueue *wq, struct cpu_info *ci)
{
	u_int idx = 0;

	if (wq->wq_flags & WQ_PERCPU) {
		idx = ci ? cpu_index(ci) : cpu_index(curcpu());
	}

	return (void *)((intptr_t)(wq) + WQ_SIZE + (idx * WQ_QUEUE_SIZE));
}

static void
workqueue_runlist(struct workqueue *wq, struct workqhead *list)
{
	work_impl_t *wk;
	work_impl_t *next;

	/*
	 * note that "list" is not a complete SIMPLEQ.
	 */

	for (wk = SIMPLEQ_FIRST(list); wk != NULL; wk = next) {
		next = SIMPLEQ_NEXT(wk, wk_entry);
		(*wq->wq_func)((void *)wk, wq->wq_arg);
	}
}

static void
workqueue_run(struct workqueue *wq)
{
	struct workqueue_queue *q;

	/* find the workqueue of this kthread */
	q = workqueue_queue_lookup(wq, curlwp->l_cpu);

	for (;;) {
		struct workqhead tmp;

		/*
		 * we violate abstraction of SIMPLEQ.
		 */

#if defined(DIAGNOSTIC)
		tmp.sqh_last = (void *)POISON;
#endif /* defined(DIAGNOSTIC) */

		mutex_enter(&q->q_mutex);
		while (SIMPLEQ_EMPTY(&q->q_queue))
			cv_wait(&q->q_cv, &q->q_mutex);
		tmp.sqh_first = q->q_queue.sqh_first; /* XXX */
		SIMPLEQ_INIT(&q->q_queue);
		mutex_exit(&q->q_mutex);

		workqueue_runlist(wq, &tmp);
	}
}

static void
workqueue_worker(void *arg)
{
	struct workqueue *wq = arg;
	struct lwp *l;

	l = curlwp;
	lwp_lock(l);
	l->l_priority = wq->wq_prio;
	l->l_usrpri = wq->wq_prio;
	lwp_unlock(l);

	workqueue_run(wq);
}

static void
workqueue_init(struct workqueue *wq, const char *name,
    void (*callback_func)(struct work *, void *), void *callback_arg,
    pri_t prio, int ipl)
{

	wq->wq_ipl = makeiplcookie(ipl);
	wq->wq_prio = prio;
	wq->wq_name = name;
	wq->wq_func = callback_func;
	wq->wq_arg = callback_arg;
}

static int
workqueue_initqueue(struct workqueue *wq, struct workqueue_queue *q,
    int ipl, struct cpu_info *ci)
{
	int error, ktf;

	mutex_init(&q->q_mutex, MUTEX_DRIVER, ipl);
	cv_init(&q->q_cv, wq->wq_name);
	q->q_worker = NULL;
	SIMPLEQ_INIT(&q->q_queue);
	ktf = ((wq->wq_flags & WQ_MPSAFE) != 0 ? KTHREAD_MPSAFE : 0);
	if (ci) {
		error = kthread_create(wq->wq_prio, ktf, ci, workqueue_worker,
		    wq, &q->q_worker, "%s/%u", wq->wq_name, (u_int)ci->ci_cpuid);
	} else {
		error = kthread_create(wq->wq_prio, ktf, ci, workqueue_worker,
		    wq, &q->q_worker, "%s", wq->wq_name);
	}

	return error;
}

struct workqueue_exitargs {
	work_impl_t wqe_wk;
	struct workqueue_queue *wqe_q;
};

static void
workqueue_exit(struct work *wk, void *arg)
{
	struct workqueue_exitargs *wqe = (void *)wk;
	struct workqueue_queue *q = wqe->wqe_q;

	/*
	 * only competition at this point is workqueue_finiqueue.
	 */

	KASSERT(q->q_worker == curlwp);
	mutex_enter(&q->q_mutex);
	q->q_worker = NULL;
	cv_signal(&q->q_cv);
	mutex_exit(&q->q_mutex);
	kthread_exit(0);
}

static void
workqueue_finiqueue(struct workqueue *wq, struct workqueue_queue *q)
{
	struct workqueue_exitargs wqe;

	wq->wq_func = workqueue_exit;

	wqe.wqe_q = q;
	KASSERT(SIMPLEQ_EMPTY(&q->q_queue));
	KASSERT(q->q_worker != NULL);
	mutex_enter(&q->q_mutex);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, &wqe.wqe_wk, wk_entry);
	cv_signal(&q->q_cv);
	while (q->q_worker != NULL) {
		cv_wait(&q->q_cv, &q->q_mutex);
	}
	mutex_exit(&q->q_mutex);
	mutex_destroy(&q->q_mutex);
	cv_destroy(&q->q_cv);
}

/* --- */

int
workqueue_create(struct workqueue **wqp, const char *name,
    void (*callback_func)(struct work *, void *), void *callback_arg,
    pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	struct workqueue_queue *q;
	void *ptr;
	int i, error = 0;
	size_t size;

	KASSERT(sizeof(work_impl_t) <= sizeof(struct work));

	i = (flags & WQ_PERCPU) ? ncpu : 1;
	if (ncpu == 1) {
		flags &= ~WQ_PERCPU;
	}

	size = WQ_SIZE + (i * WQ_QUEUE_SIZE) + CPU_ALIGN_SIZE;
	ptr = kmem_alloc(size, KM_SLEEP);

	wq = (void *)roundup2((intptr_t)ptr, CPU_ALIGN_SIZE);
	wq->wq_ptr = ptr;
	wq->wq_flags = flags;
	q = (void *)((intptr_t)(wq) + WQ_SIZE);

	workqueue_init(wq, name, callback_func, callback_arg, prio, ipl);
	i = 0;

	if (flags & WQ_PERCPU) {
#ifdef MULTIPROCESSOR
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;

		/* create the work-queue for each CPU */
		for (CPU_INFO_FOREACH(cii, ci)) {
			error = workqueue_initqueue(wq, q, ipl, ci);
			if (error) {
				break;
			}
			q = (void *)((intptr_t)(q) + WQ_QUEUE_SIZE);
			i++;
		}
#endif
	} else {
		/* initialize a work-queue */
		error = workqueue_initqueue(wq, q, ipl, NULL);
	}

	if (error) {
		/*
		 * workqueue_finiqueue() should be
		 * called for the failing one too.
		 */
		do {
			workqueue_finiqueue(wq, q);
			q = (void *)((intptr_t)(q) - WQ_QUEUE_SIZE);
		} while(i--);
		kmem_free(ptr, size);
		return error;
	}

	*wqp = wq;
	return 0;
}

void
workqueue_destroy(struct workqueue *wq)
{
	struct workqueue_queue *q;
	u_int i = 1;

	if (wq->wq_flags & WQ_PERCPU) {
#ifdef MULTIPROCESSOR
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;

		for (CPU_INFO_FOREACH(cii, ci)) {
			q = workqueue_queue_lookup(wq, ci);
			workqueue_finiqueue(wq, q);
		}
		i = ncpu;
#endif
	} else {
		q = workqueue_queue_lookup(wq, NULL);
		workqueue_finiqueue(wq, q);
	}

	kmem_free(wq->wq_ptr, WQ_SIZE + (i * WQ_QUEUE_SIZE) + CPU_ALIGN_SIZE);
}

void
workqueue_enqueue(struct workqueue *wq, struct work *wk0, struct cpu_info *ci)
{
	struct workqueue_queue *q;
	work_impl_t *wk = (void *)wk0;

	KASSERT(wq->wq_flags & WQ_PERCPU || ci == NULL);
	q = workqueue_queue_lookup(wq, ci);

	mutex_enter(&q->q_mutex);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, wk, wk_entry);
	cv_signal(&q->q_cv);
	mutex_exit(&q->q_mutex);
}
