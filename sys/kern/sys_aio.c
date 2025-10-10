/*	$NetBSD: sys_aio.c,v 1.52 2025/10/10 17:08:01 kre Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * Copyright (c) 2007 Mindaugas Rasiukevicius <rmind at NetBSD org>
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

/*
 * NetBSD asynchronous I/O service pool implementation
 * 
 * Design overview
 * 
 * Thread pool architecture: 
 * Each process owns an aiosp (service pool) with work threads (aiost).
 * Workes are reused via freelist/active lists to avoid churn.
 * Workers sleep on service_cv until a job is assigned.
 * On process teardown, outstanding working is quiesced and threads are
 # destroyed. 
 *
 * Job distribution:
 * Jobs are appended to aiosp->jobs which are then distributed to a worker
 * thread.
 * Regular files: Jobs are grouped together by file handle to allow for future
 * optimisaton.
 * Non-regular files: No grouping. Each jobs is handled directly by a discrete
 * worker thread.
 * Only regular files are candidates for non-blocking operation, however the
 * non-blocking path is not implemented yet. Everything currently falls back to
 * blocking I/O
 * Distribution is triggered by aiosp_distribute_jobs
 *
 * Job tracking:
 * A hash table (by userspace aiocb pointer) maps aiocb -> kernel job.
 * This gives O(1)ish lookup for aio_error/aio_return/aio_suspend. 
 * Resubmission of the same aiocb updates the mapping. To allow userspace to
 * reuse aiocb storage liberally.
 *
 * File group management:
 * RB tree (aiost_file_tree) maintains active file groups.
 * Groups are created ondemand when regular file jobs are distributed.
 * Groups are destroyed when all jobs for that fp complete.
 * Enables future enhancements like dynamic job appending during processing.
 * 
 * Implementation notes
 * io_read/io_write currently use fallback implementations
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_aio.c,v 1.52 2025/10/10 17:08:01 kre Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/hash.h>
#include <sys/uio.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sdt.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

MODULE(MODULE_CLASS_MISC, aio, NULL);

/*
 * System-wide limits and counter of AIO operations.
 */
u_int			aio_listio_max = AIO_LISTIO_MAX;
static u_int		aio_max = AIO_MAX;
static u_int		aio_jobs_count;

static struct pool	aio_job_pool;
static struct pool	aio_lio_pool;
static void *		aio_ehook;

static int		aio_enqueue_job(int, void *, struct lio_req *);
static void		aio_exit(proc_t *, void *);

static int		sysctl_aio_listio_max(SYSCTLFN_PROTO);
static int		sysctl_aio_max(SYSCTLFN_PROTO);

/* Service pool functions */
static int		aiost_create(struct aiosp *, struct aiost **);
static int		aiost_terminate(struct aiost *);
static void		aiost_entry(void *);
static void		aiost_sigsend(struct proc *, struct sigevent *);
static int		aiosp_worker_extract(struct aiosp *, struct aiost **);

static int		io_write(struct aio_job *);
static int		io_read(struct aio_job *);
static int		io_sync(struct aio_job *);
static int		uio_construct(struct aio_job *, struct file **,
				struct iovec *, struct uio *);
static int		io_write_fallback(struct aio_job *);
static int		io_read_fallback(struct aio_job *);

static void		aio_job_fini(struct aio_job *);
static void		aio_job_mark_complete(struct aio_job *);
static void		aio_file_hold(struct file *);
static void		aio_file_release(struct file *);

static void		aiocbp_destroy(struct aiosp *);
static int		aiocbp_init(struct aiosp *, u_int);
static int		aiocbp_insert(struct aiosp *, struct aiocbp *);
static int		aiocbp_lookup_job(struct aiosp *, const void *,
				struct aio_job **);
static int		aiocbp_remove_job(struct aiosp *, const void *,
				struct aio_job **, struct aiocbp **);

static const struct syscall_package aio_syscalls[] = {
	{ SYS_aio_cancel, 0, (sy_call_t *)sys_aio_cancel },
	{ SYS_aio_error, 0, (sy_call_t *)sys_aio_error },
	{ SYS_aio_fsync, 0, (sy_call_t *)sys_aio_fsync },
	{ SYS_aio_read, 0, (sy_call_t *)sys_aio_read },
	{ SYS_aio_return, 0, (sy_call_t *)sys_aio_return },
	{ SYS___aio_suspend50, 0, (sy_call_t *)sys___aio_suspend50 },
	{ SYS_aio_write, 0, (sy_call_t *)sys_aio_write },
	{ SYS_lio_listio, 0, (sy_call_t *)sys_lio_listio },
	{ 0, 0, NULL },
};

/*
 * Order RB with respect to fp
 */
static int
aiost_file_group_cmp(struct aiost_file_group *a, struct aiost_file_group *b)
{
	if (a == NULL || b == NULL) {
		return (a == b) ? 0 : (a ? 1 : -1);
	}

	uintptr_t ap = (uintptr_t)a->fp;
	uintptr_t bp = (uintptr_t)b->fp;

	return (ap < bp) ? -1 : (ap > bp) ? 1 : 0;
}

RB_HEAD(aiost_file_tree, aiost_file_group);
RB_PROTOTYPE(aiost_file_tree, aiost_file_group, tree, aiost_file_group_cmp);
RB_GENERATE(aiost_file_tree, aiost_file_group, tree, aiost_file_group_cmp);

/*
 * Tear down all AIO state.
 */
static int
aio_fini(bool interface)
{
	int error;
	proc_t *p;

	if (interface) {
		/* Stop syscall activity. */
		error = syscall_disestablish(NULL, aio_syscalls);
		if (error != 0)
			return error;
		/* Abort if any processes are using AIO. */
		mutex_enter(&proc_lock);
		PROCLIST_FOREACH(p, &allproc) {
			if (p->p_aio != NULL)
				break;
		}
		mutex_exit(&proc_lock);
		if (p != NULL) {
			error = syscall_establish(NULL, aio_syscalls);
			KASSERT(error == 0);
			return SET_ERROR(EBUSY);
		}
	}

	KASSERT(aio_jobs_count == 0);
	exithook_disestablish(aio_ehook);
	pool_destroy(&aio_job_pool);
	pool_destroy(&aio_lio_pool);
	return 0;
}

/*
 * Initialize global AIO state.
 */
static int
aio_init(void)
{
	int error;

	pool_init(&aio_job_pool, sizeof(struct aio_job), 0, 0, 0,
		"aio_jobs_pool", &pool_allocator_nointr, IPL_NONE);
	pool_init(&aio_lio_pool, sizeof(struct lio_req), 0, 0, 0,
		"aio_lio_pool", &pool_allocator_nointr, IPL_NONE);
	aio_ehook = exithook_establish(aio_exit, NULL);

	error = syscall_establish(NULL, aio_syscalls);
	if (error != 0) {
		aio_fini(false);
	}
	return error;
}

/*
 * Module interface.
 */
static int
aio_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return aio_init();
	case MODULE_CMD_FINI:
		return aio_fini(true);
	default:
		return SET_ERROR(ENOTTY);
	}
}

/*
 * Initialize Asynchronous I/O data structures for the process.
 */
static int
aio_procinit(struct proc *p)
{
	struct aioproc *aio;
	int error;

	/* Allocate and initialize AIO structure */
	aio = kmem_zalloc(sizeof(*aio), KM_SLEEP);

	/* Initialize the service pool */
	error = aiosp_initialize(&aio->aiosp);
	if (error) {
		kmem_free(aio, sizeof(*aio));
		return error;
	}

	error = aiocbp_init(&aio->aiosp, 256);
	if (error) {
		aiosp_destroy(&aio->aiosp, NULL);
		kmem_free(aio, sizeof(*aio));
		return error;
	}

	/* Initialize queue and their synchronization structures */
	mutex_init(&aio->aio_mtx, MUTEX_DEFAULT, IPL_NONE);

	/* Recheck if we are really first */
	mutex_enter(p->p_lock);
	if (p->p_aio) {
		mutex_exit(p->p_lock);
		aio_exit(p, aio);
		return 0;
	}
	p->p_aio = aio;
	mutex_exit(p->p_lock);

	return 0;
}

/*
 * Exit of Asynchronous I/O subsystem of process.
 */
static void
aio_exit(struct proc *p, void *cookie)
{
	struct aioproc *aio;

	if (cookie != NULL) {
		aio = cookie;
	} else if ((aio = p->p_aio) == NULL) {
		return;
	}

	aiocbp_destroy(&aio->aiosp);
	aiosp_destroy(&aio->aiosp, NULL);
	mutex_destroy(&aio->aio_mtx);
	kmem_free(aio, sizeof(*aio));
}

/*
 * Destroy job structure
 */
static void
aio_job_fini(struct aio_job *job)
{
	mutex_enter(&job->mtx);
	aiowaitgrouplk_fini(&job->lk);
	mutex_exit(&job->mtx);
	mutex_destroy(&job->mtx);
}

/*
 * Mark job as complete
 */
static void
aio_job_mark_complete(struct aio_job *job)
{
	mutex_enter(&job->mtx);
	job->completed = true;
	aio_file_release(job->fp);
	job->fp = NULL;

	aiowaitgrouplk_flush(&job->lk);
	mutex_exit(&job->mtx);

	aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);
}

/*
 * Acquire a file reference for async ops
 */
static void
aio_file_hold(struct file *fp)
{
	mutex_enter(&fp->f_lock);
	fp->f_count++;
	mutex_exit(&fp->f_lock);
}

/*
 * Release a file reference for async ops
 */
static void
aio_file_release(struct file *fp)
{
	mutex_enter(&fp->f_lock);
	fp->f_count--;
	if (!fp->f_count) {
		mutex_exit(&fp->f_lock);
		closef(fp);
		return;
	}
	mutex_exit(&fp->f_lock);
}

/*
 * Release a job back to the pool
 */
static inline void
aio_job_release(struct aio_job *job)
{
	if (job->fp) {
		aio_file_release(job->fp);
		job->fp = NULL;
	}

	aio_job_fini(job);
	pool_put(&aio_job_pool, job);
	atomic_dec_uint(&aio_jobs_count);
}

/*
 * Cancel a job pending on aiosp->jobs
 */
static inline void
aio_job_cancel(struct aiosp *aiosp, struct aio_job *job)
{
	mutex_enter(&job->mtx);
	TAILQ_REMOVE(&aiosp->jobs, job, list);
	aiosp->jobs_pending--;
	job->on_queue = false;
	job->aiocbp._errno = ECANCELED;
	mutex_exit(&job->mtx);
}

/*
 * Remove file group from tree locked
 */
static inline void
aiosp_fg_teardown_locked(struct aiosp *sp, struct aiost_file_group *fg)
{
	if (fg == NULL) {
		return;
	}

	RB_REMOVE(aiost_file_tree, sp->fg_root, fg);
	mutex_destroy(&fg->mtx);
	kmem_free(fg, sizeof(*fg));
}

/*
 * Remove file group from tree
 */
static inline void
aiosp_fg_teardown(struct aiosp *sp, struct aiost_file_group *fg)
{
	if (fg == NULL) {
		return;
	}

	mutex_enter(&sp->mtx);
	aiosp_fg_teardown_locked(sp, fg);
	mutex_exit(&sp->mtx);
}

/*
 * Group jobs by file descriptor and distribute to service threads.
 * Regular files are coalesced per-fp, others get individual threads.
 * Must be called with jobs queued in sp->jobs
 */
int
aiosp_distribute_jobs(struct aiosp *sp)
{
	struct aio_job *job, *tmp;
	struct file *fp;
	int error = 0;

	mutex_enter(&sp->mtx);
	if (!sp->jobs_pending) {
		mutex_exit(&sp->mtx);
		return 0;
	}

	TAILQ_FOREACH_SAFE(job, &sp->jobs, list, tmp) {
		fp = job->fp;
		KASSERT(fp);

		struct aiost_file_group *fg = NULL;
		struct aiost *aiost = NULL;

		if (fp->f_vnode != NULL && fp->f_vnode->v_type == VREG) {
			struct aiost_file_group key = { .fp = fp };
			fg = RB_FIND(aiost_file_tree, sp->fg_root, &key);

			if (fg == NULL) {
				fg = kmem_zalloc(sizeof(*fg), KM_SLEEP);
				fg->fp = fp;
				fg->queue_size = 0;
				mutex_init(&fg->mtx, MUTEX_DEFAULT, IPL_NONE);
				TAILQ_INIT(&fg->queue);

				error = aiosp_worker_extract(sp, &aiost);
				if (error) {
					kmem_free(fg, sizeof(*fg));
					mutex_exit(&sp->mtx);
					return error;
				}
				RB_INSERT(aiost_file_tree, sp->fg_root, fg);
				fg->aiost = aiost;

				aiost->fg = fg;
				aiost->job = NULL;
			} else {
				aiost = fg->aiost;
			}
		} else {
			error = aiosp_worker_extract(sp, &aiost);
			if (error) {
				mutex_exit(&sp->mtx);
				return error;
			}
			aiost->fg = NULL;
			aiost->job = job;
		}

		TAILQ_REMOVE(&sp->jobs, job, list);
		sp->jobs_pending--;
		job->on_queue = false;

		if (fg) {
			mutex_enter(&fg->mtx);
			TAILQ_INSERT_TAIL(&fg->queue, job, list);
			fg->queue_size++;
			mutex_exit(&fg->mtx);
		}

		mutex_enter(&aiost->mtx);
		aiost->freelist = false;
		aiost->state = AIOST_STATE_OPERATION;
		mutex_exit(&aiost->mtx);
		cv_signal(&aiost->service_cv);
	}

	mutex_exit(&sp->mtx);
	return error;
}

/*
 * Wait for specified AIO operations to complete
 * Create a waitgroup to monitor the specified aiocb list.
 * Returns when timeout expires or completion criteria met
 *
 * AIOSP_SUSPEND_ANY return when any job completes
 * AIOSP_SUSPEND_ALL return when all jobs complete
 */
int
aiosp_suspend(struct aiosp *aiosp, struct aiocb **aiocbp_list, int nent,
	struct timespec *ts, int flags)
{
	struct aio_job *job;
	struct aiowaitgroup *wg;
	int error = 0, timo = 0;
	size_t joined = 0;

	if (ts) {
		timo = tstohz(ts);
		if (timo <= 0) {
			error = SET_ERROR(EAGAIN);
			return error;
		}
	}

	wg = kmem_zalloc(sizeof(*wg), KM_SLEEP);
	aiowaitgroup_init(wg);

	for (int i = 0; i < nent; i++) {
		if (aiocbp_list[i] == NULL) {
			continue;
		}

		error = aiocbp_lookup_job(aiosp, aiocbp_list[i], &job);
		if (error) {
			goto done;
		}
		if (job == NULL) {
			continue;
		}

		if (job->completed) {
			mutex_enter(&wg->mtx);
			wg->completed++;
			wg->total++;
			mutex_exit(&wg->mtx);
			mutex_exit(&job->mtx);
			continue;
		}

		aiowaitgroup_join(wg, &job->lk);
		joined++;
		mutex_exit(&job->mtx);
	}

	if (!joined) {
		goto done;
	}

	mutex_enter(&wg->mtx);
	const size_t target = (flags & AIOSP_SUSPEND_ANY) ? 1 : wg->total;
	while (wg->completed < target) {
		error = aiowaitgroup_wait(wg, timo);
		if (error) {
			break;
		}
	}
	mutex_exit(&wg->mtx);
done:
	mutex_enter(&wg->mtx);
	wg->active = false;
	if (--wg->refcnt == 0) {
		mutex_exit(&wg->mtx);
		aiowaitgroup_fini(wg);
	} else {
		mutex_exit(&wg->mtx);
	}
	return error;
}

int
aio_suspend1(struct lwp *l, struct aiocb **aiocbp_list, int nent,
	struct timespec *ts)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	struct aiosp *aiosp = &aio->aiosp;

	return aiosp_suspend(aiosp, aiocbp_list, nent, ts, AIOSP_SUSPEND_ANY);
}

/*
 * Initializes a servicing pool.
 */
int
aiosp_initialize(struct aiosp *sp)
{
	mutex_init(&sp->mtx, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&sp->freelist);
	TAILQ_INIT(&sp->active);
	TAILQ_INIT(&sp->jobs);
	sp->fg_root = kmem_zalloc(sizeof(*sp->fg_root), KM_SLEEP);
	RB_INIT(sp->fg_root);

	return 0;
}

/*
 * Extract an available worker thread from pool or create new one
 */
static int
aiosp_worker_extract(struct aiosp *sp, struct aiost **aiost)
{
	int error;

	if (sp->nthreads_free == 0) {
		error = aiost_create(sp, aiost);
		if (error) {
			return error;
		}
	} else {
		*aiost = TAILQ_LAST(&sp->freelist, aiost_list);
	}

	TAILQ_REMOVE(&sp->freelist, *aiost, list);
	sp->nthreads_free--;
	TAILQ_INSERT_TAIL(&sp->active, *aiost, list);
	sp->nthreads_active++;

	return 0;
}

/*
 * Each process keeps track of all the service threads instantiated to service
 * an asynchronous operation by the process. When a process is terminated we
 * must also terminate all of its active and pending asynchronous operation.
 */
int
aiosp_destroy(struct aiosp *sp, int *cn)
{
	struct aiost *st;
	int error, cnt = 0;

	for (;;) {
		/*
		 * peek one worker under sp->mtx
		 */
		mutex_enter(&sp->mtx);
		st = TAILQ_FIRST(&sp->freelist);
		if (st == NULL) {
			st = TAILQ_FIRST(&sp->active);
		}
		mutex_exit(&sp->mtx);

		if (st == NULL)
			break;

		error = aiost_terminate(st);
		if (error) {
			return error;	
		}
		st->lwp = NULL;

		kmem_free(st, sizeof(*st));
		cnt++;
	}

	if (cn) {
		*cn = cnt;
	}

	mutex_destroy(&sp->mtx);
	return 0;
}

/*
 * Enqueue a job for processing by the process's servicing pool
 */
int
aiosp_enqueue_job(struct aiosp *aiosp, struct aio_job *job)
{
	mutex_enter(&aiosp->mtx);

	TAILQ_INSERT_TAIL(&aiosp->jobs, job, list);
	aiosp->jobs_pending++;
	job->on_queue = true;

	mutex_exit(&aiosp->mtx);

	return 0;
}

/*
 * Create and initialise a new servicing thread and append it to the freelist.
 */
static int
aiost_create(struct aiosp *sp, struct aiost **ret)
{
	struct proc *p = curlwp->l_proc;
	struct aiost *st;

	st = kmem_zalloc(sizeof(*st), KM_SLEEP);

	mutex_init(&st->mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&st->service_cv, "aioservice");

	st->job = NULL; 
	st->state = AIOST_STATE_NONE;
	st->aiosp = sp;
	st->freelist = true;

	TAILQ_INSERT_TAIL(&sp->freelist, st, list);
	sp->nthreads_free++;
	sp->nthreads_total++;

	int error = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN | KTHREAD_TS,
	    NULL, aiost_entry, st, &st->lwp, "aio_%d_%zu", p->p_pid,
	    sp->nthreads_total);
	if (error) {
		return error;
	}

	if (ret) {
		*ret = st;
	}

	return 0;
}

/*
 * Process single job without coalescing.
 */
static void 
aiost_process_singleton(struct aio_job *job)
{
	if ((job->aio_op & AIO_READ) == AIO_READ) {
		io_read(job);
	} else if ((job->aio_op & AIO_WRITE) == AIO_WRITE) {
		io_write(job);
	} else if ((job->aio_op & AIO_SYNC) == AIO_SYNC) {
		io_sync(job);
	} else {
		panic("%s: invalid operation code {%x}n", __func__,
		    job->aio_op);
	}

	aio_job_mark_complete(job);
}

/*
 * Process all jobs in a file group.
 */
static void
aiost_process_fg(struct aiosp *sp, struct aiost_file_group *fg)
{
	for (struct aio_job *job;;) {
		mutex_enter(&fg->mtx);
		job = TAILQ_FIRST(&fg->queue);
		if (job) {
			TAILQ_REMOVE(&fg->queue, job, list);
			fg->queue_size--;
		}
		mutex_exit(&fg->mtx);
		if (job == NULL) {
			break;
		}

		aiost_process_singleton(job);
	}
}

/*
 * Service thread entry point. Processes assigned jobs until termination.
 * Handles both singleton jobs and file-grouped job batches.
 */
static void
aiost_entry(void *arg)
{
	struct aiost *st = arg;
	struct aiosp *sp = st->aiosp;
	int error;

	/*
	 * We want to handle abrupt process terminations effectively. We use
	 * st->exit to indicate that the thread must exit. When a thread is
	 * terminated aiost_terminate(st) unblocks those sleeping on
	 * st->service_cv
	 */
	mutex_enter(&st->mtx);
	for(;;) {
		for (; st->state == AIOST_STATE_NONE;) {
			error = cv_wait_sig(&st->service_cv, &st->mtx);
			if (error) {
				/*
				 * Thread was interrupt. Check for pending exit 
				 * or suspension
				 */
				mutex_exit(&st->mtx);
				lwp_userret(curlwp);
				mutex_enter(&st->mtx);
			}
		}

		if (st->state == AIOST_STATE_TERMINATE) {
			break;
		}

		if (st->state != AIOST_STATE_OPERATION) {
			panic("aio_process: invalid aiost state {%x}\n",
				st->state);
		}

		if (st->fg) {
			struct aiost_file_group *fg = st->fg;
			st->fg = NULL;

			mutex_exit(&st->mtx);
			aiost_process_fg(sp, fg);
			mutex_enter(&st->mtx);

			aiosp_fg_teardown(sp, fg);
		} else if (st->job) {
			struct aio_job *job = st->job;

			mutex_exit(&st->mtx);
			aiost_process_singleton(job);
			mutex_enter(&st->mtx);
		} else {
			KASSERT(0);
		}

		/*
		 * check whether or not a termination was queued while handling
		 * a job
		 */
		if (st->state == AIOST_STATE_TERMINATE) {
			break;
		}

		st->state = AIOST_STATE_NONE;
		st->job = NULL;
		st->fg = NULL;

		/*
		 * Remove st from list of active service threads, append to
		 * freelist, dance around locks, then iterate loop and block on
		 * st->service_cv
		 */
		mutex_exit(&st->mtx);
		mutex_enter(&sp->mtx);
		mutex_enter(&st->mtx);

		st->freelist = true;

		TAILQ_REMOVE(&sp->active, st, list);
		sp->nthreads_active--;

		TAILQ_INSERT_TAIL(&sp->freelist, st, list);
		sp->nthreads_free++;

		mutex_exit(&sp->mtx);
	}

	if (st->job) {
		aio_job_release(st->job);
	} else if (st->fg) {
		struct aiost_file_group *fg = st->fg;
		st->fg = NULL;

		for (struct aio_job *job;;) {
			mutex_enter(&fg->mtx);
			job = TAILQ_FIRST(&fg->queue);
			if (job) {
				TAILQ_REMOVE(&fg->queue, job, list);
				fg->queue_size--;
			}
			mutex_exit(&fg->mtx);
			if (job == NULL) {
				break;
			}

			aio_job_release(job);
		}

		aiosp_fg_teardown(sp, fg);
	}


	mutex_exit(&st->mtx);
	mutex_enter(&sp->mtx);

	if (st->freelist) {
		TAILQ_REMOVE(&sp->freelist, st, list);
		sp->nthreads_free--;
	} else {
		TAILQ_REMOVE(&sp->active, st, list);
		sp->nthreads_active--;
	}
	sp->nthreads_total--;

	mutex_exit(&sp->mtx);
	kthread_exit(0);
}

/*
 * send AIO signal.
 */
static void
aiost_sigsend(struct proc *p, struct sigevent *sig)
{
	ksiginfo_t ksi;

	if (sig->sigev_signo == 0 || sig->sigev_notify == SIGEV_NONE)
		return;

	KSI_INIT(&ksi);
	ksi.ksi_signo = sig->sigev_signo;
	ksi.ksi_code = SI_ASYNCIO;
	ksi.ksi_value = sig->sigev_value;

	mutex_enter(&proc_lock);
	kpsignal(p, &ksi, NULL);
	mutex_exit(&proc_lock);
}

/*
 * Process write operation for non-blocking jobs.
 */
static int
io_write(struct aio_job *job)
{
	return io_write_fallback(job);
}

/*
 * Process read operation for non-blocking jobs.
 */
static int
io_read(struct aio_job *job)
{
	return io_read_fallback(job);
}

/*
 * Initialize UIO structure for I/O operation.
 */
static int
uio_construct(struct aio_job *job, struct file **fp, struct iovec *aiov,
	struct uio *auio)
{
	struct aiocb *aiocbp = &job->aiocbp;

	if (aiocbp->aio_nbytes > SSIZE_MAX)
		return SET_ERROR(EINVAL);

	*fp = job->fp;
	if (*fp == NULL) {
		return SET_ERROR(EBADF);
	}

	aiov->iov_base = aiocbp->aio_buf;
	aiov->iov_len = aiocbp->aio_nbytes;

	auio->uio_iov = aiov;
	auio->uio_iovcnt = 1;
	auio->uio_resid = aiocbp->aio_nbytes;
	auio->uio_offset = aiocbp->aio_offset;
	auio->uio_vmspace = job->p->p_vmspace;

	return 0;
}

/*
 * Perform synchronous write via file operations.
 */
static int
io_write_fallback(struct aio_job *job)
{
	struct file *fp = NULL;
	struct iovec aiov;
	struct uio auio;
	struct aiocb *aiocbp = &job->aiocbp;
	int error;

	error = uio_construct(job, &fp, &aiov, &auio);
	if (error) {
		goto done;
	}

	/* Write using pinned file */
	if ((fp->f_flag & FWRITE) == 0) {
		error = SET_ERROR(EBADF);
		goto done;
	}

	auio.uio_rw = UIO_WRITE;
	error = (*fp->f_ops->fo_write)(fp, &aiocbp->aio_offset,
		&auio, fp->f_cred, FOF_UPDATE_OFFSET);

	/* result */
	job->aiocbp.aio_nbytes -= auio.uio_resid;
	job->aiocbp._retval = (error == 0) ? job->aiocbp.aio_nbytes : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;
	return 0;
}

/*
 * Perform synchronous read via file operations.
 */
static int
io_read_fallback(struct aio_job *job)
{
	struct file *fp = NULL;
	struct iovec aiov;
	struct uio auio;
	struct aiocb *aiocbp = &job->aiocbp;
	int error;

	error = uio_construct(job, &fp, &aiov, &auio);
	if (error)
		goto done;

	/* Read using pinned file */
	if ((fp->f_flag & FREAD) == 0) {
		error = SET_ERROR(EBADF);
		goto done;
	}

	auio.uio_rw = UIO_READ;
	error = (*fp->f_ops->fo_read)(fp, &aiocbp->aio_offset,
		&auio, fp->f_cred, FOF_UPDATE_OFFSET);

	job->aiocbp.aio_nbytes -= auio.uio_resid;
	job->aiocbp._retval = (error == 0) ? job->aiocbp.aio_nbytes : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;
	return 0;
}

/*
 * Perform sync via file operations
 */
static int
io_sync(struct aio_job *job)
{
	struct file *fp = job->fp;
	int error = 0;

	if (fp == NULL) {
		error = SET_ERROR(EBADF);
		goto done;
	}

	if ((fp->f_flag & FWRITE) == 0) {
		error = SET_ERROR(EBADF);
		goto done;
	}

	struct vnode *vp = fp->f_vnode;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VREG) {
		if (job->aio_op & AIO_DSYNC) {
			error = VOP_FSYNC(vp, fp->f_cred,
				FSYNC_WAIT | FSYNC_DATAONLY, 0, 0);
		} else {
			error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT, 0, 0);
		}
	}
	VOP_UNLOCK(vp);

	job->aiocbp._retval = (error == 0) ? 0 : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;

	copyout(&job->aiocbp, job->aiocb_uptr, sizeof(job->aiocbp));

	return 0;
}

/*
 * Destroy a servicing thread. Set st->exit high such that when we unblock the
 * thread blocking on st->service_cv it will invoke an exit routine within
 * aiost_entry.
 */
static int
aiost_terminate(struct aiost *st)
{
	int error = 0;

	mutex_enter(&st->mtx);

	st->state = AIOST_STATE_TERMINATE;

	mutex_exit(&st->mtx);

	cv_signal(&st->service_cv);
	kthread_join(st->lwp);

	cv_destroy(&st->service_cv);
	mutex_destroy(&st->mtx);

	return error;
}

/*
 * Ensure that the same job can not be enqueued twice. 
 */
int
aiosp_validate_conflicts(struct aiosp *aiosp, const void *uptr)
{
	struct aiost *st;
	struct aio_job *job;

	mutex_enter(&aiosp->mtx);

	/* check active threads */
	TAILQ_FOREACH(st, &aiosp->active, list) {
		job = st->job;
		if (job && st->job->aiocb_uptr == uptr) {
			mutex_exit(&aiosp->mtx);
			return EINVAL;
		} else if (st->fg) {
			mutex_enter(&st->fg->mtx);
			TAILQ_FOREACH(job, &st->fg->queue, list) {
				if (job->aiocb_uptr == uptr) {
					mutex_exit(&st->fg->mtx);
					mutex_exit(&aiosp->mtx);
					return EINVAL;
				}
			}
			mutex_exit(&st->fg->mtx);
		}
	}

	/* no need to check freelist threads as they have no jobs */

	mutex_exit(&aiosp->mtx);
	return 0;
}

/*
 * Get error status of async I/O operation
 */
int
aiosp_error(struct aiosp *aiosp, const void *uptr, register_t *retval)
{
	struct aio_job *job;
	int error = 0;

	error = aiocbp_lookup_job(aiosp, uptr, &job);
	if (error || job == NULL) {
		return error;
	}

	if (job->aiocbp._state == JOB_NONE) {
		mutex_exit(&job->mtx);
		return SET_ERROR(EINVAL);
	}

	*retval = job->aiocbp._errno;
	mutex_exit(&job->mtx);

	return error;
}

/*
 * Get return value of completed async I/O operation
 */
int
aiosp_return(struct aiosp *aiosp, const void *uptr, register_t *retval)
{
	struct aiocbp *handle = NULL;
	struct aio_job *job = NULL;
	int error;

	error = aiocbp_remove_job(aiosp, uptr, &job, &handle);
	if (error) {
		return error;
	}

	if (job == NULL) {
		if (handle) {
			kmem_free(handle, sizeof(*handle));
		}
		return SET_ERROR(ENOENT);
	}

	if (job->aiocbp._state != JOB_DONE) {
		mutex_exit(&job->mtx);
		if (handle) {
			kmem_free(handle, sizeof(*handle));
		}
		return SET_ERROR(EINVAL);
	}

	*retval = job->aiocbp._retval;

	if (job->fp) {
		aio_file_release(job->fp);
		job->fp = NULL;
	}

	job->aiocbp._errno = 0;
	job->aiocbp._retval = -1;
	job->aiocbp._state = JOB_NONE;

	mutex_exit(&job->mtx);
	if (handle) {
		kmem_free(handle, sizeof(*handle));
	}

	aio_job_fini(job);
	pool_put(&aio_job_pool, job);
	atomic_dec_uint(&aio_jobs_count);

	return 0;
}

/*
 * Hash function for aiocb user pointers.
 */
static inline u_int
aiocbp_hash(const void *uptr)
{
	return hash32_buf(&uptr, sizeof(uptr), HASH32_BUF_INIT);
}

/*
 * Find aiocb entry by user pointer and locks.
 */
static int
aiocbp_lookup_job(struct aiosp *aiosp, const void *uptr,
	struct aio_job **jobp)
{
	struct aiocbp *aiocbp;
	struct aio_job *job = NULL;
	u_int hash;

	*jobp = NULL;
	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;

	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH(aiocbp, &aiosp->aio_hash[hash], list) {
		if (aiocbp->uptr == uptr) {
			job = aiocbp->job;
			if (job) {
				mutex_enter(&job->mtx);
			}

			mutex_exit(&aiosp->aio_hash_mtx);
			*jobp = job;
			return 0;
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);

	*jobp = NULL;
	return SET_ERROR(ENOENT);
}

/*
 * Detach job and return job with job->mtx held 
 */
static int
aiocbp_remove_job(struct aiosp *aiosp, const void *uptr,
	struct aio_job **jobp, struct aiocbp **handlep)
{
	struct aiocbp *aiocbp;
	struct aio_job *job = NULL;
	u_int hash;

	*jobp = NULL;
	if (handlep) {
		*handlep = NULL;
	}
	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;

	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH(aiocbp, &aiosp->aio_hash[hash], list) {
		if (aiocbp->uptr == uptr) {
			job = aiocbp->job;
			if (job) {
				mutex_enter(&job->mtx);
			}

			TAILQ_REMOVE(&aiosp->aio_hash[hash], aiocbp, list);
			mutex_exit(&aiosp->aio_hash_mtx);
			if (handlep) {
				*handlep = aiocbp;
			}
			*jobp = job;

			return 0;
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);

	return SET_ERROR(ENOENT);
}

/*
 * Insert aiocb entry into hash table.
 */
int
aiocbp_insert(struct aiosp *aiosp, struct aiocbp *aiocbp)
{
	struct aiocbp *found;
	const void *uptr;
	u_int hash;

	uptr = aiocbp->uptr;
	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;
	
	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH(found, &aiosp->aio_hash[hash], list) {
		if (found->uptr == uptr) {
			found->job = aiocbp->job;
			mutex_exit(&aiosp->aio_hash_mtx);
			return EEXIST;
		}
	}

	TAILQ_INSERT_HEAD(&aiosp->aio_hash[hash], aiocbp, list);
	mutex_exit(&aiosp->aio_hash_mtx);
	
	return 0;
}

/*
 * Initialize aiocb hash table.
 */
int
aiocbp_init(struct aiosp *aiosp, u_int hashsize)
{
	if (!powerof2(hashsize)) {
		return EINVAL;
	}

	aiosp->aio_hash = kmem_zalloc(hashsize * sizeof(*aiosp->aio_hash),
		KM_SLEEP);

	aiosp->aio_hash_mask = hashsize - 1;
	aiosp->aio_hash_size = hashsize;

	mutex_init(&aiosp->aio_hash_mtx, MUTEX_DEFAULT, IPL_NONE);

	for (size_t i = 0; i < hashsize; i++) {
		TAILQ_INIT(&aiosp->aio_hash[i]);
	}

	return 0;
}

/*
 * Destroy aiocb hash table and free entries.
 */
void
aiocbp_destroy(struct aiosp *aiosp)
{
	if (aiosp->aio_hash == NULL) {
		return;
	}

	struct aiocbp *aiocbp;

	mutex_enter(&aiosp->aio_hash_mtx);
	for (size_t i = 0; i < aiosp->aio_hash_size; i++) {
		struct aiocbp *tmp;
		TAILQ_FOREACH_SAFE(aiocbp, &aiosp->aio_hash[i], list, tmp) {
			TAILQ_REMOVE(&aiosp->aio_hash[i], aiocbp, list);
			kmem_free(aiocbp, sizeof(*aiocbp));
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);

	kmem_free(aiosp->aio_hash,
		aiosp->aio_hash_size * sizeof(*aiosp->aio_hash));
	aiosp->aio_hash = NULL;
	aiosp->aio_hash_mask = 0;
	aiosp->aio_hash_size = 0;
	mutex_destroy(&aiosp->aio_hash_mtx);
}

/*
 * Initialize wait group for suspend operations.
 */
void
aiowaitgroup_init(struct aiowaitgroup *wg)
{
	wg->completed = 0;
	wg->total = 0;
	wg->refcnt = 1;
	wg->active = true;
	cv_init(&wg->done_cv, "aiodone");
	mutex_init(&wg->mtx, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Clean up wait group resources.
 */
void
aiowaitgroup_fini(struct aiowaitgroup *wg)
{
	cv_destroy(&wg->done_cv);
	mutex_destroy(&wg->mtx);
	kmem_free(wg, sizeof(*wg));
}

/*
 * Block until wait group signals completion.
 */
int
aiowaitgroup_wait(struct aiowaitgroup *wg, int timo)
{
	int error;
	
	error = cv_timedwait_sig(&wg->done_cv, &wg->mtx, timo);
	if (error) {
		if (error == EWOULDBLOCK) {
			error = SET_ERROR(EAGAIN);
		}
		return error;
	}

	return 0;
}

/*
 * Initialize wait group link for job tracking.
 */
void
aiowaitgrouplk_init(struct aiowaitgrouplk *lk)
{
	mutex_init(&lk->mtx, MUTEX_DEFAULT, IPL_NONE);
	lk->n = 0;
	lk->s = 2;
	lk->wgs = kmem_alloc(sizeof(*lk->wgs) * lk->s, KM_SLEEP);
}

/*
 * Clean up wait group link resources.
 * Caller must hold job->mtx
 */
void
aiowaitgrouplk_fini(struct aiowaitgrouplk *lk)
{
	mutex_enter(&lk->mtx);

	for (size_t i = 0; i < lk->n; i++) {
		struct aiowaitgroup *wg = lk->wgs[i];
		if (!wg) {
			continue;
		}

		lk->wgs[i] = NULL;

		mutex_enter(&wg->mtx);
		if (--wg->refcnt == 0) {
			mutex_exit(&wg->mtx);
			aiowaitgroup_fini(wg);
		} else {
			mutex_exit(&wg->mtx);
		}
	}

	if (lk->wgs) {
		kmem_free(lk->wgs, lk->s * sizeof(*lk->wgs));
	}
	lk->wgs = NULL;
	lk->n = 0;
	lk->s = 0;

	mutex_exit(&lk->mtx);
	mutex_destroy(&lk->mtx);
}

/*
 * Notify all wait groups of job completion.
 */
void
aiowaitgrouplk_flush(struct aiowaitgrouplk *lk)
{
	mutex_enter(&lk->mtx);
	for (int i = 0; i < lk->n; i++) {
		struct aiowaitgroup *wg = lk->wgs[i];
		if (wg == NULL) {
			continue;
		}

		mutex_enter(&wg->mtx);

		if (wg->active) {
			wg->completed++;
			cv_signal(&wg->done_cv);
		}

		if (--wg->refcnt == 0) {
			mutex_exit(&wg->mtx);
			aiowaitgroup_fini(wg);
		} else {
			mutex_exit(&wg->mtx);
		}
	}

	if (lk->n) {
		kmem_free(lk->wgs, sizeof(*lk->wgs) * lk->s);

		lk->n = 0;
		lk->s = 2;
		lk->wgs = kmem_alloc(sizeof(*lk->wgs) * lk->s, KM_SLEEP);
	}

	mutex_exit(&lk->mtx);
}

/*
 * Attach wait group to jobs notification list.
 */
void
aiowaitgroup_join(struct aiowaitgroup *wg, struct aiowaitgrouplk *lk)
{
	mutex_enter(&lk->mtx);
	if (lk->n == lk->s) {
		size_t new_size = lk->s * lk->s;

		void **new_wgs = kmem_zalloc(new_size * 
			sizeof(*new_wgs), KM_SLEEP);

		memcpy(new_wgs, lk->wgs, lk->n * sizeof(*lk->wgs));
		kmem_free(lk->wgs, lk->s * sizeof(*lk->wgs));

		lk->s = new_size;
		lk->wgs = new_wgs;
	}
	lk->wgs[lk->n] = wg;
	lk->n++;
	wg->total++;
	wg->refcnt++;
	mutex_exit(&lk->mtx);
}

/*
 * Enqueue the job.
 */
static int
aio_enqueue_job(int op, void *aiocb_uptr, struct lio_req *lio)
{
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio;
	struct aio_job *a_job;
	struct aiocb aiocb;
	struct sigevent *sig;
	int error;

	/* Get the data structure from user-space */
	error = copyin(aiocb_uptr, &aiocb, sizeof(aiocb));
	if (error) {
		return error;
	}

	/* Check if signal is set, and validate it */
	sig = &aiocb.aio_sigevent;
	if (sig->sigev_signo < 0 || sig->sigev_signo >= NSIG ||
		sig->sigev_notify < SIGEV_NONE || sig->sigev_notify > SIGEV_SA) {
		return SET_ERROR(EINVAL);
	}

	/* Buffer and byte count */
	if (((AIO_SYNC | AIO_DSYNC) & op) == 0)
		if (aiocb.aio_buf == NULL || aiocb.aio_nbytes > SSIZE_MAX)
			return SET_ERROR(EINVAL);

	/* Check the opcode, if LIO_NOP - simply ignore */
	if (op == AIO_LIO) {
		KASSERT(lio != NULL);
		if (aiocb.aio_lio_opcode == LIO_WRITE) {
			op = AIO_WRITE;
		} else if (aiocb.aio_lio_opcode == LIO_READ) {
			op = AIO_READ;
		} else {
			if (aiocb.aio_lio_opcode == LIO_NOP) {
				return 0;
			} else {
				return SET_ERROR(EINVAL);
			}
		}
	} else {
		KASSERT(lio == NULL);
	}

	/*
	 * Look for already existing job. If found the job is in-progress.
	 * According to POSIX this is invalid, so return the error.
	 */
	aio = p->p_aio;
	if (aio) {
		error = aiosp_validate_conflicts(&aio->aiosp, aiocb_uptr);
		if (error) {
			return SET_ERROR(error);
		}
	}

	/*
	 * Check if AIO structure is initialized, if not initialize it
	 */
	if (p->p_aio == NULL) {
		if (aio_procinit(p)) {
			return SET_ERROR(EAGAIN);
		}
	}
	aio = p->p_aio;

	/*
	 * Set the state with errno, and copy data
	 * structure back to the user-space.
	 */
	aiocb._state = JOB_WIP;
	aiocb._errno = SET_ERROR(EINPROGRESS);
	aiocb._retval = -1;
	error = copyout(&aiocb, aiocb_uptr, sizeof(aiocb));
	if (error) {
		return error;
	}

	/* Allocate and initialize a new AIO job */
	a_job = pool_get(&aio_job_pool, PR_WAITOK | PR_ZERO);

	memcpy(&a_job->aiocbp, &aiocb, sizeof(aiocb));
	a_job->aiocb_uptr = aiocb_uptr;
	a_job->aio_op |= op;
	a_job->lio = lio;
	mutex_init(&a_job->mtx, MUTEX_DEFAULT, IPL_NONE);
	aiowaitgrouplk_init(&a_job->lk);
	a_job->p = p;
	a_job->on_queue = false;
	a_job->completed = false;
	a_job->fp = NULL;

	const int fd = aiocb.aio_fildes;
	struct file *fp = fd_getfile2(p, fd);
	if (fp == NULL) {
		aio_job_fini(a_job);
		pool_put(&aio_job_pool, a_job);
		return SET_ERROR(EBADF);
	}
	
	aio_file_hold(fp);
	a_job->fp = fp;

	struct aiocbp *aiocbp = kmem_zalloc(sizeof(*aiocbp), KM_SLEEP);
	aiocbp->job = a_job;
	aiocbp->uptr = aiocb_uptr;
	error = aiocbp_insert(&aio->aiosp, aiocbp);
	if (error) {
		aio_file_release(a_job->fp);
		a_job->fp = NULL;
		kmem_free(aiocbp, sizeof(*aiocbp));
		aio_job_fini(a_job);
		pool_put(&aio_job_pool, a_job);
		return SET_ERROR(error);
	}

	/*
	 * Add the job to the queue, update the counters, and
	 * notify the AIO worker thread to handle the job.
	 */
	mutex_enter(&aio->aio_mtx);
	if (atomic_inc_uint_nv(&aio_jobs_count) > aio_max ||
		aio->jobs_count >= aio_listio_max) {
		mutex_exit(&aio->aio_mtx);
		error = SET_ERROR(EAGAIN);
		goto error;
	}

	mutex_exit(&aio->aio_mtx);

	error = aiosp_enqueue_job(&aio->aiosp, a_job);
	if (error) {
		error = SET_ERROR(EAGAIN);
		goto error;
	}

	mutex_enter(&aio->aio_mtx);
	aio->jobs_count++;
	if (lio) {
		lio->refcnt++;
	}
	mutex_exit(&aio->aio_mtx);

	return 0;
error:
	aiocbp_remove_job(&aio->aiosp, aiocb_uptr, &a_job, NULL);
	kmem_free(aiocbp, sizeof(*aiocbp));

	aio_file_release(a_job->fp);
	a_job->fp = NULL;

	aio_job_fini(a_job);
	atomic_dec_uint(&aio_jobs_count);
	pool_put(&aio_job_pool, a_job);

	return SET_ERROR(error);
}

/*
 * Syscall functions.
 */
int
sys_aio_cancel(struct lwp *l, const struct sys_aio_cancel_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aiocb *aiocbp_uptr;
	struct filedesc	*fdp = p->p_fd;
	struct aiosp *aiosp;
	struct aio_job *job;
	struct file *fp;
	struct aiost_file_group find = { 0 }, *fg;
	unsigned int fildes, canceled = 0;
	bool have_active = false;
	fdtab_t *dt;
	int error = 0;

	fildes = (unsigned int)SCARG(uap, fildes);
	dt = atomic_load_consume(&fdp->fd_dt);
	if (fildes >= dt->dt_nfiles) {
		return SET_ERROR(EBADF);
	}
	if (dt->dt_ff[fildes] == NULL || dt->dt_ff[fildes]->ff_file == NULL) {
		return SET_ERROR(EBADF);
	}
	fp = dt->dt_ff[fildes]->ff_file;

	/* Check if AIO structure is initialized */
	if (p->p_aio == NULL) {
		*retval = AIO_NOTCANCELED;
		return 0;
	}

	aio = p->p_aio;
	aiocbp_uptr = (struct aiocb *)SCARG(uap, aiocbp);
	aiosp = &aio->aiosp;

	mutex_enter(&aio->aio_mtx);
	mutex_enter(&aiosp->mtx);

	/*
	 * If there is a live file-group for this fp, then some requests
	 * are active and could not be canceled.
	 */
	find.fp = fp;
	fg = RB_FIND(aiost_file_tree, aiosp->fg_root, &find);
	if (fg) {
		have_active = fg->queue_size ? true : false;
	}

	/*
	 * if aiocbp_uptr != NULL, then just cancel the job associated with that
	 * uptr.
	 * if aiocbp_uptr == NULL, then cancel all jobs associated with fildes.
	 */
	if (aiocbp_uptr) {
		error = aiocbp_lookup_job(aiosp, aiocbp_uptr, &job);
		if (error || job == NULL) {
			*retval = AIO_ALLDONE;
			goto finish;
		}

		if (job->completed) {
			*retval = AIO_ALLDONE;
		} else {
			*retval = AIO_NOTCANCELED;
		}

		/*
		 * If the job is on sp->job (signified by job->on_queue)
		 * that means that it has been distribtued yet. And if
		 * it is not on the queue that means it is currently
		 * beign processed.
		 */
		if (job->on_queue) {
			aio_job_cancel(aiosp, job);
			aio_job_mark_complete(job);
			*retval = AIO_CANCELED;
		}

		mutex_exit(&job->mtx);
	} else {
		/*
		 * Cancel all queued jobs associated with this file descriptor
		 */
		struct aio_job *tmp;
		TAILQ_FOREACH_SAFE(job, &aiosp->jobs, list, tmp) {
			if (job->aiocbp.aio_fildes == (int)fildes) {
				aio_job_cancel(aiosp, job);
				aio_job_mark_complete(job);
				canceled++;
			}
		}

		if (canceled && !have_active) {
			*retval = AIO_CANCELED;
		} else if (!canceled) {
			*retval = have_active ? AIO_NOTCANCELED : AIO_ALLDONE;
		} else {
			*retval = AIO_NOTCANCELED;
		}
	}
finish:
	mutex_exit(&aiosp->mtx);
	mutex_exit(&aio->aio_mtx);
	
	return 0;
}

int
sys_aio_error(struct lwp *l, const struct sys_aio_error_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;

	if (aio == NULL) {
		return SET_ERROR(EINVAL);
	}

	const void *uptr = SCARG(uap, aiocbp);
	return aiosp_error(&aio->aiosp, uptr, retval);
}

int
sys_aio_fsync(struct lwp *l, const struct sys_aio_fsync_args *uap,
	register_t *retval)
{
	int op = SCARG(uap, op);

	if ((op != O_DSYNC) && (op != O_SYNC)) {
		return SET_ERROR(EINVAL);
	}

	op = (op == O_DSYNC) ? AIO_DSYNC : AIO_SYNC;

	return aio_enqueue_job(op, SCARG(uap, aiocbp), NULL);
}

int
sys_aio_read(struct lwp *l, const struct sys_aio_read_args *uap,
	register_t *retval)
{
	int error;

	error = aio_enqueue_job(AIO_READ, SCARG(uap, aiocbp), NULL);
	if (error) {
		return error;
	}

	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	KASSERT(aio);
	return aiosp_distribute_jobs(&aio->aiosp);
}

int
sys_aio_return(struct lwp *l, const struct sys_aio_return_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;

	if (aio == NULL) {
		return SET_ERROR(EINVAL);
	}

	const void *uptr = SCARG(uap, aiocbp);
	return aiosp_return(&aio->aiosp, uptr, retval);
}

int
sys___aio_suspend50(struct lwp *l, const struct sys___aio_suspend50_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	struct aiocb **list;
	struct timespec ts;
	int error, nent;

	nent = SCARG(uap, nent);
	if (nent <= 0 || nent > aio_listio_max) {
		return SET_ERROR(EAGAIN);
	}

	if (aio == NULL) {
		return SET_ERROR(EINVAL);
	}

	if (SCARG(uap, timeout)) {
		/* Convert timespec to ticks */
		error = copyin(SCARG(uap, timeout), &ts,
			sizeof(ts));
		if (error)
			return error;
	}

	list = kmem_alloc(nent * sizeof(*list), KM_SLEEP);
	error = copyin(SCARG(uap, list), list, nent * sizeof(*list));
	if (error) {
		goto out;
	}

	error = aiosp_suspend(&aio->aiosp, list, nent, SCARG(uap, timeout) ?
		&ts : NULL, AIOSP_SUSPEND_ANY);
out:
	kmem_free(list, nent * sizeof(*list));
	return error;
}

int
sys_aio_write(struct lwp *l, const struct sys_aio_write_args *uap,
	register_t *retval)
{
	int error;

	error = aio_enqueue_job(AIO_WRITE, SCARG(uap, aiocbp), NULL);
	if (error) {
		return error;
	}

	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	KASSERT(aio);
	return aiosp_distribute_jobs(&aio->aiosp);
}

int
sys_lio_listio(struct lwp *l, const struct sys_lio_listio_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aiocb **aiocbp_list;
	struct lio_req *lio;
	int i, error = 0, errcnt, mode, nent;

	mode = SCARG(uap, mode);
	nent = SCARG(uap, nent);

	/* Non-accurate checks for the limit and invalid values */
	if (nent < 1 || nent > aio_listio_max) {
		return SET_ERROR(EINVAL);
	}

	/* Check if AIO structure is initialized, if not initialize it */
	if (p->p_aio == NULL) {
		if (aio_procinit(p)) {
			return SET_ERROR(EAGAIN);
		}
	}
	aio = p->p_aio;

	/* Create a LIO structure */
	lio = pool_get(&aio_lio_pool, PR_WAITOK);
	lio->refcnt = 1;
	error = 0;

	switch (mode) {
	case LIO_WAIT:
		memset(&lio->sig, 0, sizeof(lio->sig));
		break;
	case LIO_NOWAIT:
		/* Check for signal, validate it */
		if (SCARG(uap, sig)) {
			struct sigevent *sig = &lio->sig;

			error = copyin(SCARG(uap, sig), &lio->sig,
				sizeof(lio->sig));
			if (error == 0 &&
				(sig->sigev_signo < 0 ||
				sig->sigev_signo >= NSIG ||
				sig->sigev_notify < SIGEV_NONE ||
				sig->sigev_notify > SIGEV_SA))
				error = SET_ERROR(EINVAL);
		} else {
			memset(&lio->sig, 0, sizeof(lio->sig));
		}
		break;
	default:
		error = SET_ERROR(EINVAL);
		break;
	}

	if (error != 0) {
		pool_put(&aio_lio_pool, lio);
		return error;
	}

	/* Get the list from user-space */
	aiocbp_list = kmem_alloc(nent * sizeof(*aiocbp_list), KM_SLEEP);
	error = copyin(SCARG(uap, list), aiocbp_list,
		nent * sizeof(*aiocbp_list));
	if (error) {
		mutex_enter(&aio->aio_mtx);
		goto err;
	}

	/* Enqueue all jobs */
	errcnt = 0;
	for (i = 0; i < nent; i++) {
		error = aio_enqueue_job(AIO_LIO, aiocbp_list[i], lio);
		/*
		 * According to POSIX, in such error case it may
		 * fail with other I/O operations initiated.
		 */
		if (error) {
			errcnt++;
		}
	}

	error = aiosp_distribute_jobs(&aio->aiosp);
	if (error) {
		goto err;
	}

	mutex_enter(&aio->aio_mtx);

	/* Return an error if any */
	if (errcnt) {
		error = SET_ERROR(EIO);
		goto err;
	}

	if (mode == LIO_WAIT) {
		error = aiosp_suspend(&aio->aiosp, aiocbp_list, nent,
			NULL, AIOSP_SUSPEND_ALL);
	}

err:
	if (--lio->refcnt != 0) {
		lio = NULL;
	}
	mutex_exit(&aio->aio_mtx);
	if (lio != NULL) {
		aiost_sigsend(p, &lio->sig);
		pool_put(&aio_lio_pool, lio);
	}
	kmem_free(aiocbp_list, nent * sizeof(*aiocbp_list));
	return error;
}

/*
 * SysCtl
 */
static int
sysctl_aio_listio_max(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, newsize;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = aio_listio_max;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return error;
	}

	if (newsize < 1 || newsize > aio_max) {
		return SET_ERROR(EINVAL);
	}
	aio_listio_max = newsize;

	return 0;
}

static int
sysctl_aio_max(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, newsize;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = aio_max;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return error;
	}

	if (newsize < 1 || newsize < aio_listio_max) {
		return SET_ERROR(EINVAL);
	}
	aio_max = newsize;

	return 0;
}

SYSCTL_SETUP(sysctl_aio_init, "aio sysctl")
{
	int rv;

	rv = sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "posix_aio",
		SYSCTL_DESCR("Version of IEEE Std 1003.1 and its "
		"Asynchronous I/O option to which the "
		"system attempts to conform"),
		NULL, _POSIX_ASYNCHRONOUS_IO, NULL, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (rv != 0) {
		return;
	}

	rv = sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "aio_listio_max",
		SYSCTL_DESCR("Maximum number of asynchronous I/O "
		"operations in a single list I/O call"),
		sysctl_aio_listio_max, 0, &aio_listio_max, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (rv != 0) {
		return;
	}

	rv = sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "aio_max",
		SYSCTL_DESCR("Maximum number of asynchronous I/O "
		"operations"),
		sysctl_aio_max, 0, &aio_max, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	return;
}

/*
 * Debugging
 */
#if defined(DDB)
void
aio_print_jobs(void (*pr)(const char *, ...))
{
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio;
	struct aiosp *sp;
	struct aiost *st;
	struct aio_job *job;

	if (p == NULL) {
		(*pr)("AIO: no current process context.\n");
		return;
	}

	aio = p->p_aio;
	if (aio == NULL) {
		(*pr)("AIO: not initialized (pid=%d).\n", p->p_pid);
		return;
	}

	sp = &aio->aiosp;

	(*pr)("AIO: pid=%d\n", p->p_pid);
	(*pr)("AIO: global jobs=%u, proc jobs=%u\n", aio_jobs_count,
		aio->jobs_count);
	(*pr)("AIO: sp{ total_threads=%zu active=%zu free=%zu pending=%zu\n"
		"         processing=%lu hash_buckets=%zu mask=%#x }\n",
		sp->nthreads_total, sp->nthreads_active, sp->nthreads_free,
		sp->jobs_pending, (u_long)sp->njobs_processing,
		sp->aio_hash_size, sp->aio_hash_mask);

	/* Pending queue */
	(*pr)("\nqueue (%zu pending):\n", sp->jobs_pending);
	TAILQ_FOREACH(job, &sp->jobs, list) {
		(*pr)("  op=%d err=%d state=%d uptr=%p completed=%d\n",
			job->aio_op, job->aiocbp._errno, job->aiocbp._state,
			job->aiocb_uptr, job->completed);
		(*pr)("    fd=%d off=%llu buf=%p nbytes=%zu lio=%p\n",
			job->aiocbp.aio_fildes,
			(unsigned long long)job->aiocbp.aio_offset,
			(void *)job->aiocbp.aio_buf,
			(size_t)job->aiocbp.aio_nbytes,
			job->lio);
	}

	/* Active service threads */
	(*pr)("\nactive threads (%zu):\n", sp->nthreads_active);
	TAILQ_FOREACH(st, &sp->active, list) {
		(*pr)("  lwp=%p state=%d freelist=%d\n",
		    st->lwp, st->state, st->freelist ? 1 : 0);

		if (st->job) {
			struct aio_job *j = st->job;
			(*pr)(
			    "    job: op=%d err=%d state=%d uptr=%p\n",
			    j->aio_op, j->aiocbp._errno,
			    j->aiocbp._state, j->aiocb_uptr);
			(*pr)(
			    "      fd=%d off=%llu buf=%p nbytes=%zu\n",
			    j->aiocbp.aio_fildes,
			    (unsigned long long)j->aiocbp.aio_offset,
			    j->aiocbp.aio_buf,
			    (size_t)j->aiocbp.aio_nbytes);
		}

		if (st->fg) {
			(*pr)("    file-group: fp=%p qlen=%zu\n",
			    st->fg->fp, st->fg->queue_size);
		}
	}

	/* Freelist summary */
	(*pr)("\nfree threads (%zu)\n", sp->nthreads_free);

	/* aiocbp hash maps user aiocbp to kernel job */
	(*pr)("\naiocbp hash: buckets=%zu\n", sp->aio_hash_size);
	if (sp->aio_hash != NULL && sp->aio_hash_size != 0) {
		size_t b;
		for (b = 0; b < sp->aio_hash_size; b++) {
			struct aiocbp *hc;
			if (TAILQ_EMPTY(&sp->aio_hash[b])) {
				continue;
			}

			(*pr)("  [%zu]:", b);
			TAILQ_FOREACH(hc, &sp->aio_hash[b], list) {
				(*pr)(" uptr=%p job=%p", hc->uptr, hc->job);
			}
			(*pr)("\n");
		}
	}
}
#endif /* defined(DDB) */
