/*	$NetBSD: subr_workqueue.c,v 1.41.2.1 2023/09/04 16:57:56 martin Exp $	*/

/*-
 * Copyright (c)2002, 2005, 2006, 2007 YAMAMOTO Takashi,
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
__KERNEL_RCSID(0, "$NetBSD: subr_workqueue.c,v 1.41.2.1 2023/09/04 16:57:56 martin Exp $");

#include <sys/param.h>

#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sdt.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

typedef struct work_impl {
	SIMPLEQ_ENTRY(work_impl) wk_entry;
} work_impl_t;

SIMPLEQ_HEAD(workqhead, work_impl);

struct workqueue_queue {
	kmutex_t q_mutex;
	kcondvar_t q_cv;
	struct workqhead q_queue_pending;
	uint64_t q_gen;
	lwp_t *q_worker;
};

struct workqueue {
	void (*wq_func)(struct work *, void *);
	void *wq_arg;
	int wq_flags;

	char wq_name[MAXCOMLEN];
	pri_t wq_prio;
	void *wq_ptr;
};

#define	WQ_SIZE		(roundup2(sizeof(struct workqueue), coherency_unit))
#define	WQ_QUEUE_SIZE	(roundup2(sizeof(struct workqueue_queue), coherency_unit))

#define	POISON	0xaabbccdd

SDT_PROBE_DEFINE7(sdt, kernel, workqueue, create,
    "struct workqueue *"/*wq*/,
    "const char *"/*name*/,
    "void (*)(struct work *, void *)"/*func*/,
    "void *"/*arg*/,
    "pri_t"/*prio*/,
    "int"/*ipl*/,
    "int"/*flags*/);
SDT_PROBE_DEFINE1(sdt, kernel, workqueue, destroy,
    "struct workqueue *"/*wq*/);

SDT_PROBE_DEFINE3(sdt, kernel, workqueue, enqueue,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/,
    "struct cpu_info *"/*ci*/);
SDT_PROBE_DEFINE4(sdt, kernel, workqueue, entry,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/,
    "void (*)(struct work *, void *)"/*func*/,
    "void *"/*arg*/);
SDT_PROBE_DEFINE4(sdt, kernel, workqueue, return,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/,
    "void (*)(struct work *, void *)"/*func*/,
    "void *"/*arg*/);
SDT_PROBE_DEFINE2(sdt, kernel, workqueue, wait__start,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/);
SDT_PROBE_DEFINE2(sdt, kernel, workqueue, wait__self,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/);
SDT_PROBE_DEFINE2(sdt, kernel, workqueue, wait__hit,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/);
SDT_PROBE_DEFINE2(sdt, kernel, workqueue, wait__done,
    "struct workqueue *"/*wq*/,
    "struct work *"/*wk*/);

SDT_PROBE_DEFINE1(sdt, kernel, workqueue, exit__start,
    "struct workqueue *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, kernel, workqueue, exit__done,
    "struct workqueue *"/*wq*/);

static size_t
workqueue_size(int flags)
{

	return WQ_SIZE
	    + ((flags & WQ_PERCPU) != 0 ? ncpu : 1) * WQ_QUEUE_SIZE
	    + coherency_unit;
}

static struct workqueue_queue *
workqueue_queue_lookup(struct workqueue *wq, struct cpu_info *ci)
{
	u_int idx = 0;

	if (wq->wq_flags & WQ_PERCPU) {
		idx = ci ? cpu_index(ci) : cpu_index(curcpu());
	}

	return (void *)((uintptr_t)(wq) + WQ_SIZE + (idx * WQ_QUEUE_SIZE));
}

static void
workqueue_runlist(struct workqueue *wq, struct workqhead *list)
{
	work_impl_t *wk;
	work_impl_t *next;

	for (wk = SIMPLEQ_FIRST(list); wk != NULL; wk = next) {
		next = SIMPLEQ_NEXT(wk, wk_entry);
		SDT_PROBE4(sdt, kernel, workqueue, entry,
		    wq, wk, wq->wq_func, wq->wq_arg);
		(*wq->wq_func)((void *)wk, wq->wq_arg);
		SDT_PROBE4(sdt, kernel, workqueue, return,
		    wq, wk, wq->wq_func, wq->wq_arg);
	}
}

static void
workqueue_worker(void *cookie)
{
	struct workqueue *wq = cookie;
	struct workqueue_queue *q;
	int s, fpu = wq->wq_flags & WQ_FPU;

	/* find the workqueue of this kthread */
	q = workqueue_queue_lookup(wq, curlwp->l_cpu);

	if (fpu)
		s = kthread_fpu_enter();
	mutex_enter(&q->q_mutex);
	for (;;) {
		struct workqhead tmp;

		SIMPLEQ_INIT(&tmp);

		while (SIMPLEQ_EMPTY(&q->q_queue_pending))
			cv_wait(&q->q_cv, &q->q_mutex);
		SIMPLEQ_CONCAT(&tmp, &q->q_queue_pending);
		SIMPLEQ_INIT(&q->q_queue_pending);

		/*
		 * Mark the queue as actively running a batch of work
		 * by setting the generation number odd.
		 */
		q->q_gen |= 1;
		mutex_exit(&q->q_mutex);

		workqueue_runlist(wq, &tmp);

		/*
		 * Notify workqueue_wait that we have completed a batch
		 * of work by incrementing the generation number.
		 */
		mutex_enter(&q->q_mutex);
		KASSERTMSG(q->q_gen & 1, "q=%p gen=%"PRIu64, q, q->q_gen);
		q->q_gen++;
		cv_broadcast(&q->q_cv);
	}
	mutex_exit(&q->q_mutex);
	if (fpu)
		kthread_fpu_exit(s);
}

static void
workqueue_init(struct workqueue *wq, const char *name,
    void (*callback_func)(struct work *, void *), void *callback_arg,
    pri_t prio, int ipl)
{

	KASSERT(sizeof(wq->wq_name) > strlen(name));
	strncpy(wq->wq_name, name, sizeof(wq->wq_name));

	wq->wq_prio = prio;
	wq->wq_func = callback_func;
	wq->wq_arg = callback_arg;
}

static int
workqueue_initqueue(struct workqueue *wq, struct workqueue_queue *q,
    int ipl, struct cpu_info *ci)
{
	int error, ktf;

	KASSERT(q->q_worker == NULL);

	mutex_init(&q->q_mutex, MUTEX_DEFAULT, ipl);
	cv_init(&q->q_cv, wq->wq_name);
	SIMPLEQ_INIT(&q->q_queue_pending);
	q->q_gen = 0;
	ktf = ((wq->wq_flags & WQ_MPSAFE) != 0 ? KTHREAD_MPSAFE : 0);
	if (wq->wq_prio < PRI_KERNEL)
		ktf |= KTHREAD_TS;
	if (ci) {
		error = kthread_create(wq->wq_prio, ktf, ci, workqueue_worker,
		    wq, &q->q_worker, "%s/%u", wq->wq_name, ci->ci_index);
	} else {
		error = kthread_create(wq->wq_prio, ktf, ci, workqueue_worker,
		    wq, &q->q_worker, "%s", wq->wq_name);
	}
	if (error != 0) {
		mutex_destroy(&q->q_mutex);
		cv_destroy(&q->q_cv);
		KASSERT(q->q_worker == NULL);
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
	KASSERT(SIMPLEQ_EMPTY(&q->q_queue_pending));
	mutex_enter(&q->q_mutex);
	q->q_worker = NULL;
	cv_broadcast(&q->q_cv);
	mutex_exit(&q->q_mutex);
	kthread_exit(0);
}

static void
workqueue_finiqueue(struct workqueue *wq, struct workqueue_queue *q)
{
	struct workqueue_exitargs wqe;

	KASSERT(wq->wq_func == workqueue_exit);

	wqe.wqe_q = q;
	KASSERT(SIMPLEQ_EMPTY(&q->q_queue_pending));
	KASSERT(q->q_worker != NULL);
	mutex_enter(&q->q_mutex);
	SIMPLEQ_INSERT_TAIL(&q->q_queue_pending, &wqe.wqe_wk, wk_entry);
	cv_broadcast(&q->q_cv);
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
	int error = 0;

	CTASSERT(sizeof(work_impl_t) <= sizeof(struct work));

	ptr = kmem_zalloc(workqueue_size(flags), KM_SLEEP);
	wq = (void *)roundup2((uintptr_t)ptr, coherency_unit);
	wq->wq_ptr = ptr;
	wq->wq_flags = flags;

	workqueue_init(wq, name, callback_func, callback_arg, prio, ipl);

	if (flags & WQ_PERCPU) {
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;

		/* create the work-queue for each CPU */
		for (CPU_INFO_FOREACH(cii, ci)) {
			q = workqueue_queue_lookup(wq, ci);
			error = workqueue_initqueue(wq, q, ipl, ci);
			if (error) {
				break;
			}
		}
	} else {
		/* initialize a work-queue */
		q = workqueue_queue_lookup(wq, NULL);
		error = workqueue_initqueue(wq, q, ipl, NULL);
	}

	if (error != 0) {
		workqueue_destroy(wq);
	} else {
		*wqp = wq;
	}

	return error;
}

static bool
workqueue_q_wait(struct workqueue *wq, struct workqueue_queue *q,
    work_impl_t *wk_target)
{
	work_impl_t *wk;
	bool found = false;
	uint64_t gen;

	mutex_enter(&q->q_mutex);

	/*
	 * Avoid a deadlock scenario.  We can't guarantee that
	 * wk_target has completed at this point, but we can't wait for
	 * it either, so do nothing.
	 *
	 * XXX Are there use-cases that require this semantics?
	 */
	if (q->q_worker == curlwp) {
		SDT_PROBE2(sdt, kernel, workqueue, wait__self,  wq, wk_target);
		goto out;
	}

	/*
	 * Wait until the target is no longer pending.  If we find it
	 * on this queue, the caller can stop looking in other queues.
	 * If we don't find it in this queue, however, we can't skip
	 * waiting -- it may be hidden in the running queue which we
	 * have no access to.
	 */
    again:
	SIMPLEQ_FOREACH(wk, &q->q_queue_pending, wk_entry) {
		if (wk == wk_target) {
			SDT_PROBE2(sdt, kernel, workqueue, wait__hit,  wq, wk);
			found = true;
			cv_wait(&q->q_cv, &q->q_mutex);
			goto again;
		}
	}

	/*
	 * The target may be in the batch of work currently running,
	 * but we can't touch that queue.  So if there's anything
	 * running, wait until the generation changes.
	 */
	gen = q->q_gen;
	if (gen & 1) {
		do
			cv_wait(&q->q_cv, &q->q_mutex);
		while (gen == q->q_gen);
	}

    out:
	mutex_exit(&q->q_mutex);

	return found;
}

/*
 * Wait for a specified work to finish.  The caller must ensure that no new
 * work will be enqueued before calling workqueue_wait.  Note that if the
 * workqueue is WQ_PERCPU, the caller can enqueue a new work to another queue
 * other than the waiting queue.
 */
void
workqueue_wait(struct workqueue *wq, struct work *wk)
{
	struct workqueue_queue *q;
	bool found;

	ASSERT_SLEEPABLE();

	SDT_PROBE2(sdt, kernel, workqueue, wait__start,  wq, wk);
	if (ISSET(wq->wq_flags, WQ_PERCPU)) {
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;
		for (CPU_INFO_FOREACH(cii, ci)) {
			q = workqueue_queue_lookup(wq, ci);
			found = workqueue_q_wait(wq, q, (work_impl_t *)wk);
			if (found)
				break;
		}
	} else {
		q = workqueue_queue_lookup(wq, NULL);
		(void)workqueue_q_wait(wq, q, (work_impl_t *)wk);
	}
	SDT_PROBE2(sdt, kernel, workqueue, wait__done,  wq, wk);
}

void
workqueue_destroy(struct workqueue *wq)
{
	struct workqueue_queue *q;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	ASSERT_SLEEPABLE();

	SDT_PROBE1(sdt, kernel, workqueue, exit__start,  wq);
	wq->wq_func = workqueue_exit;
	for (CPU_INFO_FOREACH(cii, ci)) {
		q = workqueue_queue_lookup(wq, ci);
		if (q->q_worker != NULL) {
			workqueue_finiqueue(wq, q);
		}
	}
	SDT_PROBE1(sdt, kernel, workqueue, exit__done,  wq);
	kmem_free(wq->wq_ptr, workqueue_size(wq->wq_flags));
}

#ifdef DEBUG
static void
workqueue_check_duplication(struct workqueue_queue *q, work_impl_t *wk)
{
	work_impl_t *_wk;

	SIMPLEQ_FOREACH(_wk, &q->q_queue_pending, wk_entry) {
		if (_wk == wk)
			panic("%s: tried to enqueue a queued work", __func__);
	}
}
#endif

void
workqueue_enqueue(struct workqueue *wq, struct work *wk0, struct cpu_info *ci)
{
	struct workqueue_queue *q;
	work_impl_t *wk = (void *)wk0;

	SDT_PROBE3(sdt, kernel, workqueue, enqueue,  wq, wk0, ci);

	KASSERT(wq->wq_flags & WQ_PERCPU || ci == NULL);
	q = workqueue_queue_lookup(wq, ci);

	mutex_enter(&q->q_mutex);
#ifdef DEBUG
	workqueue_check_duplication(q, wk);
#endif
	SIMPLEQ_INSERT_TAIL(&q->q_queue_pending, wk, wk_entry);
	cv_broadcast(&q->q_cv);
	mutex_exit(&q->q_mutex);
}
