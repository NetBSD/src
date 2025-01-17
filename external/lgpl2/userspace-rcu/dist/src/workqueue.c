// SPDX-FileCopyrightText: 2010 Paul E. McKenney <paulmck@linux.vnet.ibm.com>
// SPDX-FileCopyrightText: 2017 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Userspace workqeues
 */

#define _LGPL_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>

#include "compat-getcpu.h"
#include <urcu/assert.h>
#include <urcu/wfcqueue.h>
#include <urcu/pointer.h>
#include <urcu/list.h>
#include <urcu/futex.h>
#include <urcu/tls-compat.h>
#include <urcu/ref.h>
#include "urcu-die.h"

#include "workqueue.h"

#define SET_AFFINITY_CHECK_PERIOD		(1U << 8)	/* 256 */
#define SET_AFFINITY_CHECK_PERIOD_MASK		(SET_AFFINITY_CHECK_PERIOD - 1)

/* Data structure that identifies a workqueue. */

struct urcu_workqueue {
	/*
	 * We do not align head on a different cache-line than tail
	 * mainly because workqueue threads use batching ("splice") to
	 * get an entire list of callbacks, which effectively empties
	 * the queue, and requires to touch the tail anyway.
	 */
	struct cds_wfcq_tail cbs_tail;
	struct cds_wfcq_head cbs_head;
	unsigned long flags;
	int32_t futex;
	unsigned long qlen; /* maintained for debugging. */
	pthread_t tid;
	int cpu_affinity;
	unsigned long loop_count;
	void *priv;
	void (*grace_period_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*initialize_worker_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*finalize_worker_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*worker_before_pause_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*worker_after_resume_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*worker_before_wait_fct)(struct urcu_workqueue *workqueue, void *priv);
	void (*worker_after_wake_up_fct)(struct urcu_workqueue *workqueue, void *priv);
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));

struct urcu_workqueue_completion {
	int barrier_count;
	int32_t futex;
	struct urcu_ref ref;
};

struct urcu_workqueue_completion_work {
	struct urcu_work work;
	struct urcu_workqueue_completion *completion;
};

/*
 * Periodically retry setting CPU affinity if we migrate.
 * Losing affinity can be caused by CPU hotunplug/hotplug, or by
 * cpuset(7).
 */
#ifdef HAVE_SCHED_SETAFFINITY
static int set_thread_cpu_affinity(struct urcu_workqueue *workqueue)
{
	cpu_set_t mask;
	int ret;

	if (workqueue->cpu_affinity < 0)
		return 0;
	if (++workqueue->loop_count & SET_AFFINITY_CHECK_PERIOD_MASK)
		return 0;
	if (urcu_sched_getcpu() == workqueue->cpu_affinity)
		return 0;

	CPU_ZERO(&mask);
	CPU_SET(workqueue->cpu_affinity, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);

	/*
	 * EINVAL is fine: can be caused by hotunplugged CPUs, or by
	 * cpuset(7). This is why we should always retry if we detect
	 * migration.
	 */
	if (ret && errno == EINVAL) {
		ret = 0;
		errno = 0;
	}
	return ret;
}
#else
static int set_thread_cpu_affinity(struct urcu_workqueue *workqueue __attribute__((unused)))
{
	return 0;
}
#endif

static void futex_wait(int32_t *futex)
{
	/* Read condition before read futex */
	cmm_smp_mb();
	while (uatomic_read(futex) == -1) {
		if (!futex_async(futex, FUTEX_WAIT, -1, NULL, NULL, 0)) {
			/*
			 * Prior queued wakeups queued by unrelated code
			 * using the same address can cause futex wait to
			 * return 0 even through the futex value is still
			 * -1 (spurious wakeups). Check the value again
			 * in user-space to validate whether it really
			 * differs from -1.
			 */
			continue;
		}
		switch (errno) {
		case EAGAIN:
			/* Value already changed. */
			return;
		case EINTR:
			/* Retry if interrupted by signal. */
			break;	/* Get out of switch. Check again. */
		default:
			/* Unexpected error. */
			urcu_die(errno);
		}
	}
}

static void futex_wake_up(int32_t *futex)
{
	/* Write to condition before reading/writing futex */
	cmm_smp_mb();
	if (caa_unlikely(uatomic_read(futex) == -1)) {
		uatomic_set(futex, 0);
		if (futex_async(futex, FUTEX_WAKE, 1,
				NULL, NULL, 0) < 0)
			urcu_die(errno);
	}
}

/* This is the code run by each worker thread. */

static void *workqueue_thread(void *arg)
{
	unsigned long cbcount;
	struct urcu_workqueue *workqueue = (struct urcu_workqueue *) arg;
	int rt = !!(uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_RT);

	if (set_thread_cpu_affinity(workqueue))
		urcu_die(errno);

	if (workqueue->initialize_worker_fct)
		workqueue->initialize_worker_fct(workqueue, workqueue->priv);

	if (!rt) {
		uatomic_dec(&workqueue->futex);
		/* Decrement futex before reading workqueue */
		cmm_smp_mb();
	}
	for (;;) {
		struct cds_wfcq_head cbs_tmp_head;
		struct cds_wfcq_tail cbs_tmp_tail;
		struct cds_wfcq_node *cbs, *cbs_tmp_n;
		enum cds_wfcq_ret splice_ret;

		if (set_thread_cpu_affinity(workqueue))
			urcu_die(errno);

		if (uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_PAUSE) {
			/*
			 * Pause requested. Become quiescent: remove
			 * ourself from all global lists, and don't
			 * process any callback. The callback lists may
			 * still be non-empty though.
			 */
			if (workqueue->worker_before_pause_fct)
				workqueue->worker_before_pause_fct(workqueue, workqueue->priv);
			cmm_smp_mb__before_uatomic_or();
			uatomic_or(&workqueue->flags, URCU_WORKQUEUE_PAUSED);
			while ((uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_PAUSE) != 0)
				(void) poll(NULL, 0, 1);
			uatomic_and(&workqueue->flags, ~URCU_WORKQUEUE_PAUSED);
			cmm_smp_mb__after_uatomic_and();
			if (workqueue->worker_after_resume_fct)
				workqueue->worker_after_resume_fct(workqueue, workqueue->priv);
		}

		cds_wfcq_init(&cbs_tmp_head, &cbs_tmp_tail);
		splice_ret = __cds_wfcq_splice_blocking(&cbs_tmp_head,
			&cbs_tmp_tail, &workqueue->cbs_head, &workqueue->cbs_tail);
		urcu_posix_assert(splice_ret != CDS_WFCQ_RET_WOULDBLOCK);
		urcu_posix_assert(splice_ret != CDS_WFCQ_RET_DEST_NON_EMPTY);
		if (splice_ret != CDS_WFCQ_RET_SRC_EMPTY) {
			if (workqueue->grace_period_fct)
				workqueue->grace_period_fct(workqueue, workqueue->priv);
			cbcount = 0;
			__cds_wfcq_for_each_blocking_safe(&cbs_tmp_head,
					&cbs_tmp_tail, cbs, cbs_tmp_n) {
				struct urcu_work *uwp;

				uwp = caa_container_of(cbs,
					struct urcu_work, next);
				uwp->func(uwp);
				cbcount++;
			}
			uatomic_sub(&workqueue->qlen, cbcount);
		}
		if (uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_STOP)
			break;
		if (workqueue->worker_before_wait_fct)
			workqueue->worker_before_wait_fct(workqueue, workqueue->priv);
		if (!rt) {
			if (cds_wfcq_empty(&workqueue->cbs_head,
					&workqueue->cbs_tail)) {
				futex_wait(&workqueue->futex);
				uatomic_dec(&workqueue->futex);
				/*
				 * Decrement futex before reading
				 * urcu_work list.
				 */
				cmm_smp_mb();
			}
		} else {
			if (cds_wfcq_empty(&workqueue->cbs_head,
					&workqueue->cbs_tail)) {
				(void) poll(NULL, 0, 10);
			}
		}
		if (workqueue->worker_after_wake_up_fct)
			workqueue->worker_after_wake_up_fct(workqueue, workqueue->priv);
	}
	if (!rt) {
		/*
		 * Read urcu_work list before write futex.
		 */
		cmm_smp_mb();
		uatomic_set(&workqueue->futex, 0);
	}
	if (workqueue->finalize_worker_fct)
		workqueue->finalize_worker_fct(workqueue, workqueue->priv);
	return NULL;
}

struct urcu_workqueue *urcu_workqueue_create(unsigned long flags,
		int cpu_affinity, void *priv,
		void (*grace_period_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*initialize_worker_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*finalize_worker_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_before_wait_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_after_wake_up_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_before_pause_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_after_resume_fct)(struct urcu_workqueue *workqueue, void *priv))
{
	struct urcu_workqueue *workqueue;
	int ret;
	sigset_t newmask, oldmask;

	workqueue = malloc(sizeof(*workqueue));
	if (workqueue == NULL)
		urcu_die(errno);
	memset(workqueue, '\0', sizeof(*workqueue));
	cds_wfcq_init(&workqueue->cbs_head, &workqueue->cbs_tail);
	workqueue->qlen = 0;
	workqueue->futex = 0;
	workqueue->flags = flags;
	workqueue->priv = priv;
	workqueue->grace_period_fct = grace_period_fct;
	workqueue->initialize_worker_fct = initialize_worker_fct;
	workqueue->finalize_worker_fct = finalize_worker_fct;
	workqueue->worker_before_wait_fct = worker_before_wait_fct;
	workqueue->worker_after_wake_up_fct = worker_after_wake_up_fct;
	workqueue->worker_before_pause_fct = worker_before_pause_fct;
	workqueue->worker_after_resume_fct = worker_after_resume_fct;
	workqueue->cpu_affinity = cpu_affinity;
	workqueue->loop_count = 0;
	cmm_smp_mb();  /* Structure initialized before pointer is planted. */

	ret = sigfillset(&newmask);
	urcu_posix_assert(!ret);
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	urcu_posix_assert(!ret);

	ret = pthread_create(&workqueue->tid, NULL, workqueue_thread, workqueue);
	if (ret) {
		urcu_die(ret);
	}

	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);

	return workqueue;
}

static void wake_worker_thread(struct urcu_workqueue *workqueue)
{
	if (!(_CMM_LOAD_SHARED(workqueue->flags) & URCU_WORKQUEUE_RT))
		futex_wake_up(&workqueue->futex);
}

static int urcu_workqueue_destroy_worker(struct urcu_workqueue *workqueue)
{
	int ret;
	void *retval;

	uatomic_or(&workqueue->flags, URCU_WORKQUEUE_STOP);
	wake_worker_thread(workqueue);

	ret = pthread_join(workqueue->tid, &retval);
	if (ret) {
		urcu_die(ret);
	}
	if (retval != NULL) {
		urcu_die(EINVAL);
	}
	workqueue->flags &= ~URCU_WORKQUEUE_STOP;
	workqueue->tid = 0;
	return 0;
}

void urcu_workqueue_destroy(struct urcu_workqueue *workqueue)
{
	if (workqueue == NULL) {
		return;
	}
	if (urcu_workqueue_destroy_worker(workqueue)) {
		urcu_die(errno);
	}
	urcu_posix_assert(cds_wfcq_empty(&workqueue->cbs_head, &workqueue->cbs_tail));
	free(workqueue);
}

void urcu_workqueue_queue_work(struct urcu_workqueue *workqueue,
		      struct urcu_work *work,
		      void (*func)(struct urcu_work *work))
{
	cds_wfcq_node_init(&work->next);
	work->func = func;
	cds_wfcq_enqueue(&workqueue->cbs_head, &workqueue->cbs_tail, &work->next);
	uatomic_inc(&workqueue->qlen);
	wake_worker_thread(workqueue);
}

static
void free_completion(struct urcu_ref *ref)
{
	struct urcu_workqueue_completion *completion;

	completion = caa_container_of(ref, struct urcu_workqueue_completion, ref);
	free(completion);
}

static
void _urcu_workqueue_wait_complete(struct urcu_work *work)
{
	struct urcu_workqueue_completion_work *completion_work;
	struct urcu_workqueue_completion *completion;

	completion_work = caa_container_of(work, struct urcu_workqueue_completion_work, work);
	completion = completion_work->completion;
	if (!uatomic_sub_return(&completion->barrier_count, 1))
		futex_wake_up(&completion->futex);
	urcu_ref_put(&completion->ref, free_completion);
	free(completion_work);
}

struct urcu_workqueue_completion *urcu_workqueue_create_completion(void)
{
	struct urcu_workqueue_completion *completion;

	completion = calloc(1, sizeof(*completion));
	if (!completion)
		urcu_die(errno);
	urcu_ref_set(&completion->ref, 1);
	completion->barrier_count = 0;
	return completion;
}

void urcu_workqueue_destroy_completion(struct urcu_workqueue_completion *completion)
{
	urcu_ref_put(&completion->ref, free_completion);
}

void urcu_workqueue_wait_completion(struct urcu_workqueue_completion *completion)
{
	/* Wait for them */
	for (;;) {
		uatomic_dec(&completion->futex);
		/* Decrement futex before reading barrier_count */
		cmm_smp_mb();
		if (!uatomic_read(&completion->barrier_count))
			break;
		futex_wait(&completion->futex);
	}
}

void urcu_workqueue_queue_completion(struct urcu_workqueue *workqueue,
		struct urcu_workqueue_completion *completion)
{
	struct urcu_workqueue_completion_work *work;

	work = calloc(1, sizeof(*work));
	if (!work)
		urcu_die(errno);
	work->completion = completion;
	urcu_ref_get(&completion->ref);
	uatomic_inc(&completion->barrier_count);
	urcu_workqueue_queue_work(workqueue, &work->work, _urcu_workqueue_wait_complete);
}

/*
 * Wait for all in-flight work to complete execution.
 */
void urcu_workqueue_flush_queued_work(struct urcu_workqueue *workqueue)
{
	struct urcu_workqueue_completion *completion;

	completion = urcu_workqueue_create_completion();
	if (!completion)
		urcu_die(ENOMEM);
	urcu_workqueue_queue_completion(workqueue, completion);
	urcu_workqueue_wait_completion(completion);
	urcu_workqueue_destroy_completion(completion);
}

/* To be used in before fork handler. */
void urcu_workqueue_pause_worker(struct urcu_workqueue *workqueue)
{
	uatomic_or(&workqueue->flags, URCU_WORKQUEUE_PAUSE);
	cmm_smp_mb__after_uatomic_or();
	wake_worker_thread(workqueue);

	while ((uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_PAUSED) == 0)
		(void) poll(NULL, 0, 1);
}

/* To be used in after fork parent handler. */
void urcu_workqueue_resume_worker(struct urcu_workqueue *workqueue)
{
	uatomic_and(&workqueue->flags, ~URCU_WORKQUEUE_PAUSE);
	while ((uatomic_read(&workqueue->flags) & URCU_WORKQUEUE_PAUSED) != 0)
		(void) poll(NULL, 0, 1);
}

void urcu_workqueue_create_worker(struct urcu_workqueue *workqueue)
{
	int ret;
	sigset_t newmask, oldmask;

	/* Clear workqueue state from parent. */
	workqueue->flags &= ~URCU_WORKQUEUE_PAUSED;
	workqueue->flags &= ~URCU_WORKQUEUE_PAUSE;
	workqueue->tid = 0;

	ret = sigfillset(&newmask);
	urcu_posix_assert(!ret);
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	urcu_posix_assert(!ret);

	ret = pthread_create(&workqueue->tid, NULL, workqueue_thread, workqueue);
	if (ret) {
		urcu_die(ret);
	}

	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);
}
