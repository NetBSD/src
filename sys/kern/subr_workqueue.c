/*	$NetBSD: subr_workqueue.c,v 1.12.2.1 2007/03/21 19:59:18 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_workqueue.c,v 1.12.2.1 2007/03/21 19:59:18 ad Exp $");

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
	struct proc *q_worker;
};

struct workqueue {
	struct workqueue_queue wq_queue; /* todo: make this per-cpu */

	void (*wq_func)(struct work *, void *);
	void *wq_arg;
	const char *wq_name;
	pri_t wq_prio;
	ipl_cookie_t wq_ipl;
};

#define	POISON	0xaabbccdd

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
	struct workqueue_queue *q = &wq->wq_queue;
	
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
workqueue_initqueue(struct workqueue *wq, int ipl)
{
	struct workqueue_queue *q = &wq->wq_queue;
	int error;

	mutex_init(&q->q_mutex, MUTEX_SPIN, ipl);
	cv_init(&q->q_cv, wq->wq_name);
	SIMPLEQ_INIT(&q->q_queue);
	error = kthread_create1(workqueue_worker, wq, &q->q_worker,
	    wq->wq_name);

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

	KASSERT(q->q_worker == curproc);
	mutex_enter(&q->q_mutex);
	q->q_worker = NULL;
	cv_signal(&q->q_cv);
	mutex_exit(&q->q_mutex);
	kthread_exit(0);
}

static void
workqueue_finiqueue(struct workqueue *wq)
{
	struct workqueue_queue *q = &wq->wq_queue;
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
	int error;

	wq = kmem_alloc(sizeof(*wq), KM_SLEEP);
	if (wq == NULL) {
		return ENOMEM;
	}

	workqueue_init(wq, name, callback_func, callback_arg, prio, ipl);

	error = workqueue_initqueue(wq, ipl);
	if (error) {
		kmem_free(wq, sizeof(*wq));
		return error;
	}

	*wqp = wq;
	return 0;
}

void
workqueue_destroy(struct workqueue *wq)
{

	workqueue_finiqueue(wq);
	kmem_free(wq, sizeof(*wq));
}

void
workqueue_enqueue(struct workqueue *wq, struct work *wk)
{
	struct workqueue_queue *q = &wq->wq_queue;

	mutex_enter(&q->q_mutex);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, wk, wk_entry);
	cv_signal(&q->q_cv);
	mutex_exit(&q->q_mutex);
}
