/*	$NetBSD: subr_workqueue.c,v 1.16 2007/07/18 18:17:03 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_workqueue.c,v 1.16 2007/07/18 18:17:03 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/workqueue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

SIMPLEQ_HEAD(workqhead, work);

struct workqueue_queue {
	kmutex_t q_mutex;
	kcondvar_t q_cv;
	struct workqhead q_queue;
	struct lwp *q_worker;
	struct cpu_info *q_ci;
	SLIST_ENTRY(workqueue_queue) q_list;
};

struct workqueue {
	SLIST_HEAD(, workqueue_queue) wq_queue;
	void (*wq_func)(struct work *, void *);
	void *wq_arg;
	const char *wq_name;
	pri_t wq_prio;
	ipl_cookie_t wq_ipl;
};

#define	POISON	0xaabbccdd

static struct workqueue_queue *
workqueue_queue_lookup(struct workqueue *wq, struct cpu_info *ci)
{
	struct workqueue_queue *q;

	SLIST_FOREACH(q, &wq->wq_queue, q_list)
		if (q->q_ci == ci)
			return q;

	return SLIST_FIRST(&wq->wq_queue);
}

static void
workqueue_runlist(struct workqueue *wq, struct workqhead *list)
{
	struct work *wk;
	struct work *next;

	/*
	 * note that "list" is not a complete SIMPLEQ.
	 */

	for (wk = SIMPLEQ_FIRST(list); wk != NULL; wk = next) {
		next = SIMPLEQ_NEXT(wk, wk_entry);
		(*wq->wq_func)(wk, wq->wq_arg);
	}
}

static void
workqueue_run(struct workqueue *wq)
{
	struct workqueue_queue *q;

	/* find the workqueue of this kthread */
	q = workqueue_queue_lookup(wq, curlwp->l_cpu);
	KASSERT(q != NULL);

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
	SLIST_INIT(&wq->wq_queue);
}

static int
workqueue_initqueue(struct workqueue *wq, int ipl,
    int flags, struct cpu_info *ci)
{
	struct workqueue_queue *q;
	int error, ktf;
	cpuid_t cpuid;

#ifdef MULTIPROCESSOR
	cpuid = ci->ci_cpuid;
#else
	cpuid = 0;
#endif

	q = kmem_alloc(sizeof(struct workqueue_queue), KM_SLEEP);
	SLIST_INSERT_HEAD(&wq->wq_queue, q, q_list);
	q->q_ci = ci;

	mutex_init(&q->q_mutex, MUTEX_DRIVER, ipl);
	cv_init(&q->q_cv, wq->wq_name);
	SIMPLEQ_INIT(&q->q_queue);
	ktf = ((flags & WQ_MPSAFE) != 0 ? KTHREAD_MPSAFE : 0);
	error = kthread_create(wq->wq_prio, ktf, ci, workqueue_worker,
	    wq, &q->q_worker, "%s/%d", wq->wq_name, (int)cpuid);

	return error;
}

struct workqueue_exitargs {
	struct work wqe_wk;
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
	kmem_free(q, sizeof(struct workqueue_queue));
}

/* --- */

int
workqueue_create(struct workqueue **wqp, const char *name,
    void (*callback_func)(struct work *, void *), void *callback_arg,
    pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error = 0;

	wq = kmem_alloc(sizeof(*wq), KM_SLEEP);

	workqueue_init(wq, name, callback_func, callback_arg, prio, ipl);

#ifdef MULTIPROCESSOR   
	if (flags & WQ_PERCPU) {
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;

		/* create the work-queue for each CPU */
		for (CPU_INFO_FOREACH(cii, ci)) {
			error = workqueue_initqueue(wq, ipl, flags, ci);
			if (error)
				break;
		}
		if (error)
			workqueue_destroy(wq);

	} else {
		error = workqueue_initqueue(wq, ipl, flags, curcpu());
		if (error) {
			kmem_free(wq, sizeof(*wq));
			return error;
		}
	}
#else
	error = workqueue_initqueue(wq, ipl, flags, curcpu());
	if (error) {
		kmem_free(wq, sizeof(*wq));
		return error;
	}
#endif

	*wqp = wq;
	return 0;
}

void
workqueue_destroy(struct workqueue *wq)
{
	struct workqueue_queue *q;

	while ((q = SLIST_FIRST(&wq->wq_queue)) != NULL) {
		SLIST_REMOVE_HEAD(&wq->wq_queue, q_list);
		workqueue_finiqueue(wq, q);
	}
	kmem_free(wq, sizeof(*wq));
}

void
workqueue_enqueue(struct workqueue *wq, struct work *wk, struct cpu_info *ci)
{
	struct workqueue_queue *q;

	q = workqueue_queue_lookup(wq, ci);
	KASSERT(q != NULL);

	mutex_enter(&q->q_mutex);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, wk, wk_entry);
	cv_signal(&q->q_cv);
	mutex_exit(&q->q_mutex);
}
