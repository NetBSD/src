/*	$NetBSD: kern_threadpool.c,v 1.23 2021/01/23 16:33:49 riastradh Exp $	*/

/*-
 * Copyright (c) 2014, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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

/*
 * Thread pools.
 *
 * A thread pool is a collection of worker threads idle or running
 * jobs, together with a dispatcher thread that does not run jobs but
 * can be given jobs to assign to a worker thread.  Scheduling a job in
 * a thread pool does not allocate or even sleep at all, except perhaps
 * on an adaptive lock, unlike kthread_create.  Jobs reuse threads, so
 * they do not incur the expense of creating and destroying kthreads
 * unless there is not much work to be done.
 *
 * A per-CPU thread pool (threadpool_percpu) is a collection of thread
 * pools, one per CPU bound to that CPU.  For each priority level in
 * use, there is one shared unbound thread pool (i.e., pool of threads
 * not bound to any CPU) and one shared per-CPU thread pool.
 *
 * To use the unbound thread pool at priority pri, call
 * threadpool_get(&pool, pri).  When you're done, call
 * threadpool_put(pool, pri).
 *
 * To use the per-CPU thread pools at priority pri, call
 * threadpool_percpu_get(&pool_percpu, pri), and then use the thread
 * pool returned by threadpool_percpu_ref(pool_percpu) for the current
 * CPU, or by threadpool_percpu_ref_remote(pool_percpu, ci) for another
 * CPU.  When you're done, call threadpool_percpu_put(pool_percpu,
 * pri).
 *
 * +--MACHINE-----------------------------------------------------+
 * | +--CPU 0---------+ +--CPU 1---------+     +--CPU n---------+ |
 * | | <dispatcher 0> | | <dispatcher 1> | ... | <dispatcher n> | |
 * | | <idle 0a>      | | <running 1a>   | ... | <idle na>      | |
 * | | <running 0b>   | | <running 1b>   | ... | <idle nb>      | |
 * | | .              | | .              | ... | .              | |
 * | | .              | | .              | ... | .              | |
 * | | .              | | .              | ... | .              | |
 * | +----------------+ +----------------+     +----------------+ |
 * |            +--unbound-----------+                            |
 * |            | <dispatcher n+1>   |                            |
 * |            | <idle (n+1)a>      |                            |
 * |            | <running (n+1)b>   |                            |
 * |            +--------------------+                            |
 * +--------------------------------------------------------------+
 *
 * XXX Why one dispatcher per CPU?  I did that originally to avoid
 * touching remote CPUs' memory when scheduling a job, but that still
 * requires interprocessor synchronization.  Perhaps we could get by
 * with a single dispatcher thread, at the expense of another pointer
 * in struct threadpool_job to identify the CPU on which it must run in
 * order for the dispatcher to schedule it correctly.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_threadpool.c,v 1.23 2021/01/23 16:33:49 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/percpu.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sdt.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/threadpool.h>

/* Probes */

SDT_PROBE_DEFINE1(sdt, kernel, threadpool, get,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, get__create,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, get__race,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, put,
    "struct threadpool *"/*pool*/, "pri_t"/*pri*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, put__destroy,
    "struct threadpool *"/*pool*/, "pri_t"/*pri*/);

SDT_PROBE_DEFINE1(sdt, kernel, threadpool, percpu__get,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, percpu__get__create,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, percpu__get__race,
    "pri_t"/*pri*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, percpu__put,
    "struct threadpool *"/*pool*/, "pri_t"/*pri*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, percpu__put__destroy,
    "struct threadpool *"/*pool*/, "pri_t"/*pri*/);

SDT_PROBE_DEFINE2(sdt, kernel, threadpool, create,
    "struct cpu_info *"/*ci*/, "pri_t"/*pri*/);
SDT_PROBE_DEFINE3(sdt, kernel, threadpool, create__success,
    "struct cpu_info *"/*ci*/, "pri_t"/*pri*/, "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE3(sdt, kernel, threadpool, create__failure,
    "struct cpu_info *"/*ci*/, "pri_t"/*pri*/, "int"/*error*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, destroy,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, destroy__wait,
    "struct threadpool *"/*pool*/, "uint64_t"/*refcnt*/);

SDT_PROBE_DEFINE2(sdt, kernel, threadpool, schedule__job,
    "struct threadpool *"/*pool*/, "struct threadpool_job *"/*job*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, schedule__job__running,
    "struct threadpool *"/*pool*/, "struct threadpool_job *"/*job*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, schedule__job__dispatcher,
    "struct threadpool *"/*pool*/, "struct threadpool_job *"/*job*/);
SDT_PROBE_DEFINE3(sdt, kernel, threadpool, schedule__job__thread,
    "struct threadpool *"/*pool*/,
    "struct threadpool_job *"/*job*/,
    "struct lwp *"/*thread*/);

SDT_PROBE_DEFINE1(sdt, kernel, threadpool, dispatcher__start,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, dispatcher__dying,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, dispatcher__spawn,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, dispatcher__race,
    "struct threadpool *"/*pool*/,
    "struct threadpool_job *"/*job*/);
SDT_PROBE_DEFINE3(sdt, kernel, threadpool, dispatcher__assign,
    "struct threadpool *"/*pool*/,
    "struct threadpool_job *"/*job*/,
    "struct lwp *"/*thread*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, dispatcher__exit,
    "struct threadpool *"/*pool*/);

SDT_PROBE_DEFINE1(sdt, kernel, threadpool, thread__start,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, thread__dying,
    "struct threadpool *"/*pool*/);
SDT_PROBE_DEFINE2(sdt, kernel, threadpool, thread__job,
    "struct threadpool *"/*pool*/, "struct threadpool_job *"/*job*/);
SDT_PROBE_DEFINE1(sdt, kernel, threadpool, thread__exit,
    "struct threadpool *"/*pool*/);

/* Data structures */

TAILQ_HEAD(job_head, threadpool_job);
TAILQ_HEAD(thread_head, threadpool_thread);

struct threadpool_thread {
	struct lwp			*tpt_lwp;
	char				*tpt_lwp_savedname;
	struct threadpool		*tpt_pool;
	struct threadpool_job		*tpt_job;
	kcondvar_t			tpt_cv;
	TAILQ_ENTRY(threadpool_thread)	tpt_entry;
};

struct threadpool {
	kmutex_t			tp_lock;
	struct threadpool_thread	tp_dispatcher;
	struct job_head			tp_jobs;
	struct thread_head		tp_idle_threads;
	uint64_t			tp_refcnt;
	int				tp_flags;
#define	THREADPOOL_DYING	0x01
	struct cpu_info			*tp_cpu;
	pri_t				tp_pri;
};

static void	threadpool_hold(struct threadpool *);
static void	threadpool_rele(struct threadpool *);

static int	threadpool_percpu_create(struct threadpool_percpu **, pri_t);
static void	threadpool_percpu_destroy(struct threadpool_percpu *);
static void	threadpool_percpu_init(void *, void *, struct cpu_info *);
static void	threadpool_percpu_ok(void *, void *, struct cpu_info *);
static void	threadpool_percpu_fini(void *, void *, struct cpu_info *);

static threadpool_job_fn_t threadpool_job_dead;

static void	threadpool_job_hold(struct threadpool_job *);
static void	threadpool_job_rele(struct threadpool_job *);

static void	threadpool_dispatcher_thread(void *) __dead;
static void	threadpool_thread(void *) __dead;

static pool_cache_t	threadpool_thread_pc __read_mostly;

static kmutex_t		threadpools_lock __cacheline_aligned;

	/* Default to 30 second idle timeout for pool threads. */
static int	threadpool_idle_time_ms = 30 * 1000;

struct threadpool_unbound {
	struct threadpool		tpu_pool;

	/* protected by threadpools_lock */
	LIST_ENTRY(threadpool_unbound)	tpu_link;
	uint64_t			tpu_refcnt;
};

static LIST_HEAD(, threadpool_unbound) unbound_threadpools;

static struct threadpool_unbound *
threadpool_lookup_unbound(pri_t pri)
{
	struct threadpool_unbound *tpu;

	LIST_FOREACH(tpu, &unbound_threadpools, tpu_link) {
		if (tpu->tpu_pool.tp_pri == pri)
			return tpu;
	}
	return NULL;
}

static void
threadpool_insert_unbound(struct threadpool_unbound *tpu)
{
	KASSERT(threadpool_lookup_unbound(tpu->tpu_pool.tp_pri) == NULL);
	LIST_INSERT_HEAD(&unbound_threadpools, tpu, tpu_link);
}

static void
threadpool_remove_unbound(struct threadpool_unbound *tpu)
{
	KASSERT(threadpool_lookup_unbound(tpu->tpu_pool.tp_pri) == tpu);
	LIST_REMOVE(tpu, tpu_link);
}

struct threadpool_percpu {
	percpu_t *			tpp_percpu;
	pri_t				tpp_pri;

	/* protected by threadpools_lock */
	LIST_ENTRY(threadpool_percpu)	tpp_link;
	uint64_t			tpp_refcnt;
};

static LIST_HEAD(, threadpool_percpu) percpu_threadpools;

static struct threadpool_percpu *
threadpool_lookup_percpu(pri_t pri)
{
	struct threadpool_percpu *tpp;

	LIST_FOREACH(tpp, &percpu_threadpools, tpp_link) {
		if (tpp->tpp_pri == pri)
			return tpp;
	}
	return NULL;
}

static void
threadpool_insert_percpu(struct threadpool_percpu *tpp)
{
	KASSERT(threadpool_lookup_percpu(tpp->tpp_pri) == NULL);
	LIST_INSERT_HEAD(&percpu_threadpools, tpp, tpp_link);
}

static void
threadpool_remove_percpu(struct threadpool_percpu *tpp)
{
	KASSERT(threadpool_lookup_percpu(tpp->tpp_pri) == tpp);
	LIST_REMOVE(tpp, tpp_link);
}

static int
sysctl_kern_threadpool_idle_ms(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int val, error;

	node = *rnode;

	val = threadpool_idle_time_ms;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error == 0 && newp != NULL) {
		/* Disallow negative values and 0 (forever). */
		if (val < 1)
			error = EINVAL;
		else
			threadpool_idle_time_ms = val;
	}

	return error;
}

SYSCTL_SETUP_PROTO(sysctl_threadpool_setup);

SYSCTL_SETUP(sysctl_threadpool_setup,
    "sysctl kern.threadpool subtree setup")
{
	const struct sysctlnode *rnode, *cnode;
	int error __diagused;

	error = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "threadpool",
	    SYSCTL_DESCR("threadpool subsystem options"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_CREATE, CTL_EOL);
	KASSERT(error == 0);

	error = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "idle_ms",
	    SYSCTL_DESCR("idle thread timeout in ms"),
	    sysctl_kern_threadpool_idle_ms, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	KASSERT(error == 0);
}

void
threadpools_init(void)
{

	threadpool_thread_pc =
	    pool_cache_init(sizeof(struct threadpool_thread), 0, 0, 0,
		"thplthrd", NULL, IPL_NONE, NULL, NULL, NULL);

	LIST_INIT(&unbound_threadpools);
	LIST_INIT(&percpu_threadpools);
	mutex_init(&threadpools_lock, MUTEX_DEFAULT, IPL_NONE);
}

static void
threadnamesuffix(char *buf, size_t buflen, struct cpu_info *ci, int pri)
{

	buf[0] = '\0';
	if (ci)
		snprintf(buf + strlen(buf), buflen - strlen(buf), "/%d",
		    cpu_index(ci));
	if (pri != PRI_NONE)
		snprintf(buf + strlen(buf), buflen - strlen(buf), "@%d", pri);
}

/* Thread pool creation */

static bool
threadpool_pri_is_valid(pri_t pri)
{
	return (pri == PRI_NONE || (pri >= PRI_USER && pri < PRI_COUNT));
}

static int
threadpool_create(struct threadpool *const pool, struct cpu_info *ci,
    pri_t pri)
{
	struct lwp *lwp;
	char suffix[16];
	int ktflags;
	int error;

	KASSERT(threadpool_pri_is_valid(pri));

	SDT_PROBE2(sdt, kernel, threadpool, create,  ci, pri);

	mutex_init(&pool->tp_lock, MUTEX_DEFAULT, IPL_VM);
	/* XXX dispatcher */
	TAILQ_INIT(&pool->tp_jobs);
	TAILQ_INIT(&pool->tp_idle_threads);
	pool->tp_refcnt = 1;		/* dispatcher's reference */
	pool->tp_flags = 0;
	pool->tp_cpu = ci;
	pool->tp_pri = pri;

	pool->tp_dispatcher.tpt_lwp = NULL;
	pool->tp_dispatcher.tpt_pool = pool;
	pool->tp_dispatcher.tpt_job = NULL;
	cv_init(&pool->tp_dispatcher.tpt_cv, "pooldisp");

	ktflags = 0;
	ktflags |= KTHREAD_MPSAFE;
	if (pri < PRI_KERNEL)
		ktflags |= KTHREAD_TS;
	threadnamesuffix(suffix, sizeof(suffix), ci, pri);
	error = kthread_create(pri, ktflags, ci, &threadpool_dispatcher_thread,
	    &pool->tp_dispatcher, &lwp, "pooldisp%s", suffix);
	if (error)
		goto fail0;

	mutex_spin_enter(&pool->tp_lock);
	pool->tp_dispatcher.tpt_lwp = lwp;
	cv_broadcast(&pool->tp_dispatcher.tpt_cv);
	mutex_spin_exit(&pool->tp_lock);

	SDT_PROBE3(sdt, kernel, threadpool, create__success,  ci, pri, pool);
	return 0;

fail0:	KASSERT(error);
	KASSERT(pool->tp_dispatcher.tpt_job == NULL);
	KASSERT(pool->tp_dispatcher.tpt_pool == pool);
	KASSERT(pool->tp_flags == 0);
	KASSERT(pool->tp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&pool->tp_idle_threads));
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));
	KASSERT(!cv_has_waiters(&pool->tp_dispatcher.tpt_cv));
	cv_destroy(&pool->tp_dispatcher.tpt_cv);
	mutex_destroy(&pool->tp_lock);
	SDT_PROBE3(sdt, kernel, threadpool, create__failure,  ci, pri, error);
	return error;
}

/* Thread pool destruction */

static void
threadpool_destroy(struct threadpool *pool)
{
	struct threadpool_thread *thread;

	SDT_PROBE1(sdt, kernel, threadpool, destroy,  pool);

	/* Mark the pool dying and wait for threads to commit suicide.  */
	mutex_spin_enter(&pool->tp_lock);
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));
	pool->tp_flags |= THREADPOOL_DYING;
	cv_broadcast(&pool->tp_dispatcher.tpt_cv);
	TAILQ_FOREACH(thread, &pool->tp_idle_threads, tpt_entry)
		cv_broadcast(&thread->tpt_cv);
	while (0 < pool->tp_refcnt) {
		SDT_PROBE2(sdt, kernel, threadpool, destroy__wait,
		    pool, pool->tp_refcnt);
		cv_wait(&pool->tp_dispatcher.tpt_cv, &pool->tp_lock);
	}
	mutex_spin_exit(&pool->tp_lock);

	KASSERT(pool->tp_dispatcher.tpt_job == NULL);
	KASSERT(pool->tp_dispatcher.tpt_pool == pool);
	KASSERT(pool->tp_flags == THREADPOOL_DYING);
	KASSERT(pool->tp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&pool->tp_idle_threads));
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));
	KASSERT(!cv_has_waiters(&pool->tp_dispatcher.tpt_cv));
	cv_destroy(&pool->tp_dispatcher.tpt_cv);
	mutex_destroy(&pool->tp_lock);
}

static void
threadpool_hold(struct threadpool *pool)
{

	KASSERT(mutex_owned(&pool->tp_lock));
	pool->tp_refcnt++;
	KASSERT(pool->tp_refcnt != 0);
}

static void
threadpool_rele(struct threadpool *pool)
{

	KASSERT(mutex_owned(&pool->tp_lock));
	KASSERT(0 < pool->tp_refcnt);
	if (--pool->tp_refcnt == 0)
		cv_broadcast(&pool->tp_dispatcher.tpt_cv);
}

/* Unbound thread pools */

int
threadpool_get(struct threadpool **poolp, pri_t pri)
{
	struct threadpool_unbound *tpu, *tmp = NULL;
	int error;

	ASSERT_SLEEPABLE();

	SDT_PROBE1(sdt, kernel, threadpool, get,  pri);

	if (! threadpool_pri_is_valid(pri))
		return EINVAL;

	mutex_enter(&threadpools_lock);
	tpu = threadpool_lookup_unbound(pri);
	if (tpu == NULL) {
		mutex_exit(&threadpools_lock);
		SDT_PROBE1(sdt, kernel, threadpool, get__create,  pri);
		tmp = kmem_zalloc(sizeof(*tmp), KM_SLEEP);
		error = threadpool_create(&tmp->tpu_pool, NULL, pri);
		if (error) {
			kmem_free(tmp, sizeof(*tmp));
			return error;
		}
		mutex_enter(&threadpools_lock);
		tpu = threadpool_lookup_unbound(pri);
		if (tpu == NULL) {
			tpu = tmp;
			tmp = NULL;
			threadpool_insert_unbound(tpu);
		} else {
			SDT_PROBE1(sdt, kernel, threadpool, get__race,  pri);
		}
	}
	KASSERT(tpu != NULL);
	tpu->tpu_refcnt++;
	KASSERT(tpu->tpu_refcnt != 0);
	mutex_exit(&threadpools_lock);

	if (tmp != NULL) {
		threadpool_destroy(&tmp->tpu_pool);
		kmem_free(tmp, sizeof(*tmp));
	}
	KASSERT(tpu != NULL);
	*poolp = &tpu->tpu_pool;
	return 0;
}

void
threadpool_put(struct threadpool *pool, pri_t pri)
{
	struct threadpool_unbound *tpu =
	    container_of(pool, struct threadpool_unbound, tpu_pool);

	ASSERT_SLEEPABLE();
	KASSERT(threadpool_pri_is_valid(pri));

	SDT_PROBE2(sdt, kernel, threadpool, put,  pool, pri);

	mutex_enter(&threadpools_lock);
	KASSERT(tpu == threadpool_lookup_unbound(pri));
	KASSERT(0 < tpu->tpu_refcnt);
	if (--tpu->tpu_refcnt == 0) {
		SDT_PROBE2(sdt, kernel, threadpool, put__destroy,  pool, pri);
		threadpool_remove_unbound(tpu);
	} else {
		tpu = NULL;
	}
	mutex_exit(&threadpools_lock);

	if (tpu) {
		threadpool_destroy(&tpu->tpu_pool);
		kmem_free(tpu, sizeof(*tpu));
	}
}

/* Per-CPU thread pools */

int
threadpool_percpu_get(struct threadpool_percpu **pool_percpup, pri_t pri)
{
	struct threadpool_percpu *pool_percpu, *tmp = NULL;
	int error;

	ASSERT_SLEEPABLE();

	SDT_PROBE1(sdt, kernel, threadpool, percpu__get,  pri);

	if (! threadpool_pri_is_valid(pri))
		return EINVAL;

	mutex_enter(&threadpools_lock);
	pool_percpu = threadpool_lookup_percpu(pri);
	if (pool_percpu == NULL) {
		mutex_exit(&threadpools_lock);
		SDT_PROBE1(sdt, kernel, threadpool, percpu__get__create,  pri);
		error = threadpool_percpu_create(&tmp, pri);
		if (error)
			return error;
		KASSERT(tmp != NULL);
		mutex_enter(&threadpools_lock);
		pool_percpu = threadpool_lookup_percpu(pri);
		if (pool_percpu == NULL) {
			pool_percpu = tmp;
			tmp = NULL;
			threadpool_insert_percpu(pool_percpu);
		} else {
			SDT_PROBE1(sdt, kernel, threadpool, percpu__get__race,
			    pri);
		}
	}
	KASSERT(pool_percpu != NULL);
	pool_percpu->tpp_refcnt++;
	KASSERT(pool_percpu->tpp_refcnt != 0);
	mutex_exit(&threadpools_lock);

	if (tmp != NULL)
		threadpool_percpu_destroy(tmp);
	KASSERT(pool_percpu != NULL);
	*pool_percpup = pool_percpu;
	return 0;
}

void
threadpool_percpu_put(struct threadpool_percpu *pool_percpu, pri_t pri)
{

	ASSERT_SLEEPABLE();

	KASSERT(threadpool_pri_is_valid(pri));

	SDT_PROBE2(sdt, kernel, threadpool, percpu__put,  pool_percpu, pri);

	mutex_enter(&threadpools_lock);
	KASSERT(pool_percpu == threadpool_lookup_percpu(pri));
	KASSERT(0 < pool_percpu->tpp_refcnt);
	if (--pool_percpu->tpp_refcnt == 0) {
		SDT_PROBE2(sdt, kernel, threadpool, percpu__put__destroy,
		    pool_percpu, pri);
		threadpool_remove_percpu(pool_percpu);
	} else {
		pool_percpu = NULL;
	}
	mutex_exit(&threadpools_lock);

	if (pool_percpu)
		threadpool_percpu_destroy(pool_percpu);
}

struct threadpool *
threadpool_percpu_ref(struct threadpool_percpu *pool_percpu)
{
	struct threadpool **poolp, *pool;

	poolp = percpu_getref(pool_percpu->tpp_percpu);
	pool = *poolp;
	percpu_putref(pool_percpu->tpp_percpu);

	return pool;
}

struct threadpool *
threadpool_percpu_ref_remote(struct threadpool_percpu *pool_percpu,
    struct cpu_info *ci)
{
	struct threadpool **poolp, *pool;

	/*
	 * As long as xcalls are blocked -- e.g., by kpreempt_disable
	 * -- the percpu object will not be swapped and destroyed.  We
	 * can't write to it, because the data may have already been
	 * moved to a new buffer, but we can safely read from it.
	 */
	kpreempt_disable();
	poolp = percpu_getptr_remote(pool_percpu->tpp_percpu, ci);
	pool = *poolp;
	kpreempt_enable();

	return pool;
}

static int
threadpool_percpu_create(struct threadpool_percpu **pool_percpup, pri_t pri)
{
	struct threadpool_percpu *pool_percpu;
	bool ok = true;

	pool_percpu = kmem_zalloc(sizeof(*pool_percpu), KM_SLEEP);
	pool_percpu->tpp_pri = pri;
	pool_percpu->tpp_percpu = percpu_create(sizeof(struct threadpool *),
	    threadpool_percpu_init, threadpool_percpu_fini,
	    (void *)(intptr_t)pri);

	/*
	 * Verify that all of the CPUs were initialized.
	 *
	 * XXX What to do if we add CPU hotplug?
	 */
	percpu_foreach(pool_percpu->tpp_percpu, &threadpool_percpu_ok, &ok);
	if (!ok)
		goto fail;

	/* Success!  */
	*pool_percpup = (struct threadpool_percpu *)pool_percpu;
	return 0;

fail:	percpu_free(pool_percpu->tpp_percpu, sizeof(struct threadpool *));
	kmem_free(pool_percpu, sizeof(*pool_percpu));
	return ENOMEM;
}

static void
threadpool_percpu_destroy(struct threadpool_percpu *pool_percpu)
{

	percpu_free(pool_percpu->tpp_percpu, sizeof(struct threadpool *));
	kmem_free(pool_percpu, sizeof(*pool_percpu));
}

static void
threadpool_percpu_init(void *vpoolp, void *vpri, struct cpu_info *ci)
{
	struct threadpool **const poolp = vpoolp;
	pri_t pri = (intptr_t)(void *)vpri;
	int error;

	*poolp = kmem_zalloc(sizeof(**poolp), KM_SLEEP);
	error = threadpool_create(*poolp, ci, pri);
	if (error) {
		KASSERT(error == ENOMEM);
		kmem_free(*poolp, sizeof(**poolp));
		*poolp = NULL;
	}
}

static void
threadpool_percpu_ok(void *vpoolp, void *vokp, struct cpu_info *ci)
{
	struct threadpool **const poolp = vpoolp;
	bool *okp = vokp;

	if (*poolp == NULL)
		atomic_store_relaxed(okp, false);
}

static void
threadpool_percpu_fini(void *vpoolp, void *vprip, struct cpu_info *ci)
{
	struct threadpool **const poolp = vpoolp;

	if (*poolp == NULL)	/* initialization failed */
		return;
	threadpool_destroy(*poolp);
	kmem_free(*poolp, sizeof(**poolp));
}

/* Thread pool jobs */

void __printflike(4,5)
threadpool_job_init(struct threadpool_job *job, threadpool_job_fn_t fn,
    kmutex_t *lock, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vsnprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	job->job_lock = lock;
	job->job_thread = NULL;
	job->job_refcnt = 0;
	cv_init(&job->job_cv, job->job_name);
	job->job_fn = fn;
}

static void
threadpool_job_dead(struct threadpool_job *job)
{

	panic("threadpool job %p ran after destruction", job);
}

void
threadpool_job_destroy(struct threadpool_job *job)
{

	ASSERT_SLEEPABLE();

	KASSERTMSG((job->job_thread == NULL), "job %p still running", job);

	mutex_enter(job->job_lock);
	while (0 < atomic_load_relaxed(&job->job_refcnt))
		cv_wait(&job->job_cv, job->job_lock);
	mutex_exit(job->job_lock);

	job->job_lock = NULL;
	KASSERT(job->job_thread == NULL);
	KASSERT(job->job_refcnt == 0);
	KASSERT(!cv_has_waiters(&job->job_cv));
	cv_destroy(&job->job_cv);
	job->job_fn = threadpool_job_dead;
	(void)strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

static void
threadpool_job_hold(struct threadpool_job *job)
{
	unsigned int refcnt __diagused;

	refcnt = atomic_inc_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != 0);
}

static void
threadpool_job_rele(struct threadpool_job *job)
{
	unsigned int refcnt;

	KASSERT(mutex_owned(job->job_lock));

	refcnt = atomic_dec_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	if (refcnt == 0)
		cv_broadcast(&job->job_cv);
}

void
threadpool_job_done(struct threadpool_job *job)
{

	KASSERT(mutex_owned(job->job_lock));
	KASSERT(job->job_thread != NULL);
	KASSERT(job->job_thread->tpt_lwp == curlwp);

	/*
	 * We can safely read this field; it's only modified right before
	 * we call the job work function, and we are only preserving it
	 * to use here; no one cares if it contains junk afterward.
	 */
	lwp_lock(curlwp);
	curlwp->l_name = job->job_thread->tpt_lwp_savedname;
	lwp_unlock(curlwp);

	/*
	 * Inline the work of threadpool_job_rele(); the job is already
	 * locked, the most likely scenario (XXXJRT only scenario?) is
	 * that we're dropping the last reference (the one taken in
	 * threadpool_schedule_job()), and we always do the cv_broadcast()
	 * anyway.
	 */
	KASSERT(0 < atomic_load_relaxed(&job->job_refcnt));
	unsigned int refcnt __diagused = atomic_dec_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	cv_broadcast(&job->job_cv);
	job->job_thread = NULL;
}

void
threadpool_schedule_job(struct threadpool *pool, struct threadpool_job *job)
{

	KASSERT(mutex_owned(job->job_lock));

	SDT_PROBE2(sdt, kernel, threadpool, schedule__job,  pool, job);

	/*
	 * If the job's already running, let it keep running.  The job
	 * is guaranteed by the interlock not to end early -- if it had
	 * ended early, threadpool_job_done would have set job_thread
	 * to NULL under the interlock.
	 */
	if (__predict_true(job->job_thread != NULL)) {
		SDT_PROBE2(sdt, kernel, threadpool, schedule__job__running,
		    pool, job);
		return;
	}

	threadpool_job_hold(job);

	/* Otherwise, try to assign a thread to the job.  */
	mutex_spin_enter(&pool->tp_lock);
	if (__predict_false(TAILQ_EMPTY(&pool->tp_idle_threads))) {
		/* Nobody's idle.  Give it to the dispatcher.  */
		SDT_PROBE2(sdt, kernel, threadpool, schedule__job__dispatcher,
		    pool, job);
		job->job_thread = &pool->tp_dispatcher;
		TAILQ_INSERT_TAIL(&pool->tp_jobs, job, job_entry);
	} else {
		/* Assign it to the first idle thread.  */
		job->job_thread = TAILQ_FIRST(&pool->tp_idle_threads);
		SDT_PROBE3(sdt, kernel, threadpool, schedule__job__thread,
		    pool, job, job->job_thread->tpt_lwp);
		TAILQ_REMOVE(&pool->tp_idle_threads, job->job_thread,
		    tpt_entry);
		job->job_thread->tpt_job = job;
	}

	/* Notify whomever we gave it to, dispatcher or idle thread.  */
	KASSERT(job->job_thread != NULL);
	cv_broadcast(&job->job_thread->tpt_cv);
	mutex_spin_exit(&pool->tp_lock);
}

bool
threadpool_cancel_job_async(struct threadpool *pool, struct threadpool_job *job)
{

	KASSERT(mutex_owned(job->job_lock));

	/*
	 * XXXJRT This fails (albeit safely) when all of the following
	 * are true:
	 *
	 *	=> "pool" is something other than what the job was
	 *	   scheduled on.  This can legitimately occur if,
	 *	   for example, a job is percpu-scheduled on CPU0
	 *	   and then CPU1 attempts to cancel it without taking
	 *	   a remote pool reference.  (this might happen by
	 *	   "luck of the draw").
	 *
	 *	=> "job" is not yet running, but is assigned to the
	 *	   dispatcher.
	 *
	 * When this happens, this code makes the determination that
	 * the job is already running.  The failure mode is that the
	 * caller is told the job is running, and thus has to wait.
	 * The dispatcher will eventually get to it and the job will
	 * proceed as if it had been already running.
	 */

	if (job->job_thread == NULL) {
		/* Nothing to do.  Guaranteed not running.  */
		return true;
	} else if (job->job_thread == &pool->tp_dispatcher) {
		/* Take it off the list to guarantee it won't run.  */
		job->job_thread = NULL;
		mutex_spin_enter(&pool->tp_lock);
		TAILQ_REMOVE(&pool->tp_jobs, job, job_entry);
		mutex_spin_exit(&pool->tp_lock);
		threadpool_job_rele(job);
		return true;
	} else {
		/* Too late -- already running.  */
		return false;
	}
}

void
threadpool_cancel_job(struct threadpool *pool, struct threadpool_job *job)
{

	/*
	 * We may sleep here, but we can't ASSERT_SLEEPABLE() because
	 * the job lock (used to interlock the cv_wait()) may in fact
	 * legitimately be a spin lock, so the assertion would fire
	 * as a false-positive.
	 */

	KASSERT(mutex_owned(job->job_lock));

	if (threadpool_cancel_job_async(pool, job))
		return;

	/* Already running.  Wait for it to complete.  */
	while (job->job_thread != NULL)
		cv_wait(&job->job_cv, job->job_lock);
}

/* Thread pool dispatcher thread */

static void __dead
threadpool_dispatcher_thread(void *arg)
{
	struct threadpool_thread *const dispatcher = arg;
	struct threadpool *const pool = dispatcher->tpt_pool;
	struct lwp *lwp = NULL;
	int ktflags;
	char suffix[16];
	int error;

	KASSERT((pool->tp_cpu == NULL) || (pool->tp_cpu == curcpu()));
	KASSERT((pool->tp_cpu == NULL) || (curlwp->l_pflag & LP_BOUND));

	/* Wait until we're initialized.  */
	mutex_spin_enter(&pool->tp_lock);
	while (dispatcher->tpt_lwp == NULL)
		cv_wait(&dispatcher->tpt_cv, &pool->tp_lock);

	SDT_PROBE1(sdt, kernel, threadpool, dispatcher__start,  pool);

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&pool->tp_jobs)) {
			if (ISSET(pool->tp_flags, THREADPOOL_DYING)) {
				SDT_PROBE1(sdt, kernel, threadpool,
				    dispatcher__dying,  pool);
				break;
			}
			cv_wait(&dispatcher->tpt_cv, &pool->tp_lock);
		}
		if (__predict_false(TAILQ_EMPTY(&pool->tp_jobs)))
			break;

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&pool->tp_idle_threads)) {
			SDT_PROBE1(sdt, kernel, threadpool, dispatcher__spawn,
			    pool);
			threadpool_hold(pool);
			mutex_spin_exit(&pool->tp_lock);

			struct threadpool_thread *const thread =
			    pool_cache_get(threadpool_thread_pc, PR_WAITOK);
			thread->tpt_lwp = NULL;
			thread->tpt_pool = pool;
			thread->tpt_job = NULL;
			cv_init(&thread->tpt_cv, "pooljob");

			ktflags = 0;
			ktflags |= KTHREAD_MPSAFE;
			if (pool->tp_pri < PRI_KERNEL)
				ktflags |= KTHREAD_TS;
			threadnamesuffix(suffix, sizeof(suffix), pool->tp_cpu,
			    pool->tp_pri);
			error = kthread_create(pool->tp_pri, ktflags,
			    pool->tp_cpu, &threadpool_thread, thread, &lwp,
			    "poolthread%s", suffix);

			mutex_spin_enter(&pool->tp_lock);
			if (error) {
				pool_cache_put(threadpool_thread_pc, thread);
				threadpool_rele(pool);
				/* XXX What to do to wait for memory?  */
				(void)kpause("thrdplcr", false, hz,
				    &pool->tp_lock);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(lwp != NULL);
			TAILQ_INSERT_TAIL(&pool->tp_idle_threads, thread,
			    tpt_entry);
			thread->tpt_lwp = lwp;
			lwp = NULL;
			cv_broadcast(&thread->tpt_cv);
			continue;
		}

		/* There are idle threads, so try giving one a job.  */
		struct threadpool_job *const job = TAILQ_FIRST(&pool->tp_jobs);

		/*
		 * Take an extra reference on the job temporarily so that
		 * it won't disappear on us while we have both locks dropped.
		 */
		threadpool_job_hold(job);
		mutex_spin_exit(&pool->tp_lock);

		mutex_enter(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread.  */
		if (__predict_true(job->job_thread == dispatcher)) {
			mutex_spin_enter(&pool->tp_lock);
			TAILQ_REMOVE(&pool->tp_jobs, job, job_entry);
			if (__predict_false(
				    TAILQ_EMPTY(&pool->tp_idle_threads))) {
				/*
				 * Someone else snagged the thread
				 * first.  We'll have to try again.
				 */
				SDT_PROBE2(sdt, kernel, threadpool,
				    dispatcher__race,  pool, job);
				TAILQ_INSERT_HEAD(&pool->tp_jobs, job,
				    job_entry);
			} else {
				/*
				 * Assign the job to the thread and
				 * wake the thread so it starts work.
				 */
				struct threadpool_thread *const thread =
				    TAILQ_FIRST(&pool->tp_idle_threads);

				SDT_PROBE2(sdt, kernel, threadpool,
				    dispatcher__assign,  job, thread->tpt_lwp);
				KASSERT(thread->tpt_job == NULL);
				TAILQ_REMOVE(&pool->tp_idle_threads, thread,
				    tpt_entry);
				thread->tpt_job = job;
				job->job_thread = thread;
				cv_broadcast(&thread->tpt_cv);
			}
			mutex_spin_exit(&pool->tp_lock);
		}
		threadpool_job_rele(job);
		mutex_exit(job->job_lock);

		mutex_spin_enter(&pool->tp_lock);
	}
	threadpool_rele(pool);
	mutex_spin_exit(&pool->tp_lock);

	SDT_PROBE1(sdt, kernel, threadpool, dispatcher__exit,  pool);

	kthread_exit(0);
}

/* Thread pool thread */

static void __dead
threadpool_thread(void *arg)
{
	struct threadpool_thread *const thread = arg;
	struct threadpool *const pool = thread->tpt_pool;

	KASSERT((pool->tp_cpu == NULL) || (pool->tp_cpu == curcpu()));
	KASSERT((pool->tp_cpu == NULL) || (curlwp->l_pflag & LP_BOUND));

	/* Wait until we're initialized and on the queue.  */
	mutex_spin_enter(&pool->tp_lock);
	while (thread->tpt_lwp == NULL)
		cv_wait(&thread->tpt_cv, &pool->tp_lock);

	SDT_PROBE1(sdt, kernel, threadpool, thread__start,  pool);

	KASSERT(thread->tpt_lwp == curlwp);
	for (;;) {
		/* Wait until we are assigned a job.  */
		while (thread->tpt_job == NULL) {
			if (ISSET(pool->tp_flags, THREADPOOL_DYING)) {
				SDT_PROBE1(sdt, kernel, threadpool,
				    thread__dying,  pool);
				break;
			}
			if (cv_timedwait(&thread->tpt_cv, &pool->tp_lock,
				mstohz(threadpool_idle_time_ms)))
				break;
		}
		if (__predict_false(thread->tpt_job == NULL)) {
			TAILQ_REMOVE(&pool->tp_idle_threads, thread,
			    tpt_entry);
			break;
		}

		struct threadpool_job *const job = thread->tpt_job;
		KASSERT(job != NULL);

		/* Set our lwp name to reflect what job we're doing.  */
		lwp_lock(curlwp);
		char *const lwp_name __diagused = curlwp->l_name;
		thread->tpt_lwp_savedname = curlwp->l_name;
		curlwp->l_name = job->job_name;
		lwp_unlock(curlwp);

		mutex_spin_exit(&pool->tp_lock);

		SDT_PROBE2(sdt, kernel, threadpool, thread__job,  pool, job);

		/* Run the job.  */
		(*job->job_fn)(job);

		/* lwp name restored in threadpool_job_done(). */
		KASSERTMSG((curlwp->l_name == lwp_name),
		    "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		mutex_spin_enter(&pool->tp_lock);
		KASSERT(thread->tpt_job == job);
		thread->tpt_job = NULL;
		TAILQ_INSERT_TAIL(&pool->tp_idle_threads, thread, tpt_entry);
	}
	threadpool_rele(pool);
	mutex_spin_exit(&pool->tp_lock);

	SDT_PROBE1(sdt, kernel, threadpool, thread__exit,  pool);

	KASSERT(!cv_has_waiters(&thread->tpt_cv));
	cv_destroy(&thread->tpt_cv);
	pool_cache_put(threadpool_thread_pc, thread);
	kthread_exit(0);
}
