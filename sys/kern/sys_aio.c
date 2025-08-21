/*	$NetBSD: sys_aio.c,v 0.00 2025/08/15 12:00:00 ethan4984 Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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
 * On process teardown, outstanding working is quiesced and threads are destroyed. 
 *
 * Job distribution:
 * Jobs are appended to aiosp->jobs which are then distributed to a worker thread.
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
 * 
 * io_read/io_write currently use fallback implementations
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_aio.c,v 1.50 2024/12/07 02:38:51 riastradh Exp $");

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

static int		io_write(struct aiost *, struct aio_job *);
static int		io_read(struct aiost *, struct aio_job *);
static int		io_sync(struct aiost *);
static int		uio_construct(struct aio_job *, struct file **,
				struct iovec *, struct uio *);
static int		io_write_fallback(struct aio_job *);
static int		io_read_fallback(struct aio_job *);

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
	if (error != 0)
		(void)aio_fini(false);
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
	aio = kmem_zalloc(sizeof(struct aioproc), KM_SLEEP);

	/* Initialize the service pool */
	error = aiosp_initialize(&aio->aiosp);
	if (error) {
		kmem_free(aio, sizeof(struct aioproc));
		return error;
	}

	error = aiocbp_init(&aio->aiosp, 256);
	if (error) {
		aiosp_destroy(&aio->aiosp, NULL);
		kmem_free(aio, sizeof(struct aioproc));
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

	if (cookie != NULL)
		aio = cookie;
	else if ((aio = p->p_aio) == NULL)
		return;

	aiocbp_destroy(&aio->aiosp);
	aiosp_destroy(&aio->aiosp, NULL);
	mutex_destroy(&aio->aio_mtx);
	kmem_free(aio, sizeof(struct aioproc));
}

/*
 * Group jobs by file descriptor and distribute to service threads.
 * Regular files are coalesced per-fp, others get individual threads.
 * Must be called with jobs queued in sp->jobs
 */
int
aiosp_distribute_jobs(struct aiosp *sp)
{
	struct aio_job *job;
	struct file *fp;
	int error = 0;

	mutex_enter(&sp->mtx);
	if (!sp->jobs_pending) {
		mutex_exit(&sp->mtx);
		return 0;
	}

	struct aio_job *tmp;
	TAILQ_FOREACH_SAFE(job, &sp->jobs, list, tmp) {
		fp = fd_getfile2(job->p, job->aiocbp.aio_fildes);
		if (fp == NULL) {
			mutex_exit(&sp->mtx);
			error = SET_ERROR(EBADF);
			return error;
		}

		struct aiost_file_group *fg = NULL;
		struct aiost *aiost = NULL;

		if (fp->f_vnode && fp->f_vnode->v_type == VREG) {
			struct aiost_file_group find = { 0 };
			find.fp = fp;
			fg = RB_FIND(aiost_file_tree, sp->fg_root, &find);

			if (fg == NULL) {
				fg = kmem_zalloc(sizeof(*fg), KM_SLEEP);
				fg->fp = fp;
				fg->vp = fp->f_vnode;
				fg->queue_size = 0;
				TAILQ_INIT(&fg->queue);

				error = aiosp_worker_extract(sp, &aiost);
				if (error) {
					kmem_free(fg, sizeof(*fg));
					closef(fp);
					mutex_exit(&sp->mtx);
					return error;
				}

				RB_INSERT(aiost_file_tree, sp->fg_root, fg);
				fg->aiost = aiost;
	
				aiost->fg = fg;
				aiost->job = NULL;
			} else {
				/*
				 * release fp as it already exists within fg
				 */
				closef(fp);
				aiost = fg->aiost;
			}
		} else {
			error = aiosp_worker_extract(sp, &aiost);
			if (error) {
				closef(fp);
				mutex_exit(&sp->mtx);
				return error;
			}

			aiost->fg = NULL;
			aiost->job = job;
		}

		/*
		 * Move from sp->jobs to fg->jobs
		 */
		TAILQ_REMOVE(&sp->jobs, job, list);
		sp->jobs_pending--;
		job->on_queue = false;

		if (fg) {
			TAILQ_INSERT_TAIL(&fg->queue, job, list);
			fg->queue_size++;
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
	int error = 0;
	int timo;
	size_t target = 0;
	size_t monitor = 0;

	if (ts) {
		timo = mstohz((ts->tv_sec * 1000) + (ts->tv_nsec / 1000000));
		if (timo == 0 && ts->tv_sec == 0 && ts->tv_nsec > 0) {
			timo = 1;
		}

		if (timo <= 0) {
			error = SET_ERROR(EAGAIN);
			return error;
		}
	} else {
		timo = 0;
	}

	struct aiowaitgroup *wg = kmem_zalloc(sizeof(*wg), KM_SLEEP);
	aiowaitgroup_init(wg);

	mutex_enter(&wg->mtx);
	for (int i = 0; i < nent; i++) {
		if (aiocbp_list[i] == NULL) {
			continue;
		}

		struct aiocbp *aiocbp = NULL;
		error = aiocbp_lookup(aiosp, &aiocbp, aiocbp_list[i]);
		if (error) {
			goto done;
		}
		if (aiocbp == NULL) {
			continue;
		}

		job = aiocbp->job;
		monitor++;

		mutex_enter(&job->mtx);
		if (job->completed) {
			wg->completed++;
			wg->total++;
		} else {
			aiowaitgroup_join(wg, &job->lk);
		}
		mutex_exit(&job->mtx);
	}

	if (!monitor) {
		goto done;
	}

	if (flags & AIOSP_SUSPEND_ANY) {
		target = 1;
	} else if (flags & AIOSP_SUSPEND_ALL) {
		target = monitor;
	}

	for (; wg->completed < target;) {
		error = aiowaitgroup_wait(wg, timo);
		if (error) {
			goto done;
		}
	}

done:
	wg->active = false;
	wg->refcnt--;

	if (wg->refcnt == 0) {
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
	struct aiost *tmp;
	int error = 0;
	int cnt = 0;

	mutex_enter(&sp->mtx);

	/*
	 * Terminate and destroy every service thread both free and active.
	 */
	TAILQ_FOREACH_SAFE(st, &sp->freelist, list, tmp) {
		error = aiost_terminate(st);
		if (error) {
			mutex_exit(&sp->mtx);
			return error;
		}

		cnt++;
		kmem_free(st, sizeof(*st));
	}

	TAILQ_FOREACH_SAFE(st, &sp->active, list, tmp) {
		error = aiost_terminate(st);
		if (error) {
			mutex_exit(&sp->mtx);
			return error;
		}

		cnt++;
		kmem_free(st, sizeof(*st));
	}

	if (cn) {
		*cn = cnt;
	}

	mutex_exit(&sp->mtx);
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

	int error = kthread_create(PRI_USER, 0, NULL, aiost_entry,
		st, &st->lwp, "aio_%d_%ld", p->p_pid, sp->nthreads_total);
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
aiost_process_singleton (struct aiost *st)
{
	struct aio_job *job;

	job = st->job;
	KASSERT(job != NULL);
	if (job->aio_op & AIO_READ) {
		io_read_fallback(job);
	} else if (job->aio_op & AIO_WRITE) {
		io_write_fallback(job);
	} else if (job->aio_op & AIO_SYNC) {
		io_sync(st);
	} else {
		panic("aio_process: invalid operation code\n");
	}

	mutex_enter(&job->mtx);
	aiowaitgrouplk_flush(&job->lk);
	job->completed = true;
	mutex_exit(&job->mtx);

	aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);
}

/*
 * Process all jobs in a file group.
 */
static void
aiost_process_fg (struct aiost *st)
{
	struct aiosp *sp = st->aiosp;
	struct aiost_file_group *fg = st->fg;
	struct aio_job *job;

	struct aio_job *tmp;
	TAILQ_FOREACH_SAFE(job, &fg->queue, list, tmp) {
		if (job->aio_op & AIO_READ) {
			io_read(st, job);
		} else if (job->aio_op & AIO_WRITE) {
			io_write(st, job);
		} else if (job->aio_op & AIO_SYNC) {
			io_sync(st);
		} else {
			panic("aio_process: invalid operation code\n");
		}

		mutex_enter(&job->mtx);
		aiowaitgrouplk_flush(&job->lk);
		job->completed = true;
		mutex_exit(&job->mtx);

		aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);
	}

	mutex_enter(&sp->mtx);
	RB_REMOVE(aiost_file_tree, sp->fg_root, fg);
	closef(fg->fp);
	kmem_free(fg, sizeof(*fg));
	mutex_exit(&sp->mtx);
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
			mutex_exit(&st->mtx);
			aiost_process_fg(st);
			mutex_enter(&st->mtx);
		} else {
			mutex_exit(&st->mtx);
			aiost_process_singleton(st);
			mutex_enter(&st->mtx);
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
		pool_put(&aio_job_pool, st->job);
		atomic_dec_uint(&aio_jobs_count);
	} else {
		struct aiost_file_group *fg = st->fg;
		KASSERT(fg);

		while (!TAILQ_EMPTY(&fg->queue)) {
			struct aio_job *job = TAILQ_FIRST(&fg->queue);
			TAILQ_REMOVE(&fg->queue, job, list);
			pool_put(&aio_job_pool, job);
			atomic_dec_uint(&aio_jobs_count);
		}
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
io_write(struct aiost *aiost, struct aio_job *job)
{
	return io_write_fallback(job);
}

/*
 * Process read operation for non-blocking jobs.
 */
static int
io_read(struct aiost *aiost, struct aio_job *job)
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
	int fd = aiocbp->aio_fildes;
	int error = 0;

	if (aiocbp->aio_nbytes > SSIZE_MAX) {
		error = SET_ERROR(EINVAL);
		return error;
	}
	
	*fp = fd_getfile2(job->p, fd);
	if (*fp == NULL) {
		error = SET_ERROR(EBADF);
		return error;
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
	struct file *fp;
	struct iovec aiov;
	struct uio auio;
	struct aiocb *aiocbp;
	int error;

	error = uio_construct(job, &fp, &aiov, &auio);
	if (error) {
		if (fp) {
			closef(fp);
		}

		goto done;
	}

	/*
	 * Perform write
	 */
	aiocbp = &job->aiocbp;
	KASSERT(job->aio_op & AIO_WRITE);

	if ((fp->f_flag & FWRITE) == 0) {
		closef(fp);
		error = SET_ERROR(EBADF);
		goto done;
	}
	auio.uio_rw = UIO_WRITE;
	error = (*fp->f_ops->fo_write)(fp, &aiocbp->aio_offset,
		&auio, fp->f_cred, FOF_UPDATE_OFFSET);

	closef(fp);
	
	/*
	 * Store the result value
	 */
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
	struct file *fp;
	struct iovec aiov;
	struct uio auio;
	struct aiocb *aiocbp;
	int error;

	error = uio_construct(job, &fp, &aiov, &auio);
	if (error) {
		if (fp) {
			closef(fp);
		}
		goto done;
	}

	/* 
	 * Perform read
	 */
	aiocbp = &job->aiocbp;
	KASSERT((job->aio_op & AIO_WRITE) == 0);

	if ((fp->f_flag & FREAD) == 0) {
		closef(fp);
		error = SET_ERROR(EBADF);
		goto done;
	}
	auio.uio_rw = UIO_READ;
	error = (*fp->f_ops->fo_read)(fp, &aiocbp->aio_offset,
		&auio, fp->f_cred, FOF_UPDATE_OFFSET);

	closef(fp);
	
	/*
	 * Store the result value
	 */
	job->aiocbp.aio_nbytes -= auio.uio_resid;
	job->aiocbp._retval = (error == 0) ? job->aiocbp.aio_nbytes : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;

	return 0;
}

/*
 * Flush file data to stable storage.
 */
static int
io_sync(struct aiost *aiost)
{
	struct aio_job *job = aiost->job;
	struct aiocb *aiocbp = &job->aiocbp;
	struct file *fp;
	int fd = aiocbp->aio_fildes;
	int error = 0;

	/*
	 * Perform a file sync operation
	 */
	struct vnode *vp;

	if ((error = fd_getvnode(fd, &fp)) != 0) {
		goto done;
	}

	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(fd);
		error = SET_ERROR(EBADF);
		goto done;
	}

	vp = fp->f_vnode;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (job->aio_op & AIO_DSYNC) {
		error = VOP_FSYNC(vp, fp->f_cred,
			FSYNC_WAIT | FSYNC_DATAONLY, 0, 0);
	} else if (job->aio_op & AIO_SYNC) {
		error = VOP_FSYNC(vp, fp->f_cred,
			FSYNC_WAIT, 0, 0);
	}
	VOP_UNLOCK(vp);
	fd_putfile(fd);

	/*
	 * Store the result value
	 */
	job->aiocbp._retval = (error == 0) ? 0 : -1;
done:
	job->aiocbp._errno = error;
	job->aiocbp._state = JOB_DONE;

	copyout(&job->aiocbp, job->aiocb_uptr, 
		sizeof(struct aiocb));

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

	mutex_enter(&aiosp->mtx);

	/* check active threads */
	TAILQ_FOREACH(st, &aiosp->active, list) {
		KASSERT(st->job);
		if (st->job->aiocb_uptr == uptr) {
			mutex_exit(&aiosp->mtx);
			return EINVAL;
		}
	}

	/* no need to check freelist threads as they have no jobs */

	mutex_exit(&aiosp->mtx);
	return 0;
}

/*
 * Get error status of async I/O operation
 */
int aiosp_error(struct aiosp *aiosp, const void *uptr, register_t *retval)
{
	struct aiocbp *aiocbp = NULL;
	struct aio_job *job;
	int error;

	error = aiocbp_lookup(aiosp, &aiocbp, uptr);
	if (error) {
		return error;
	}

	job = aiocbp->job;
	if (job->aiocbp._state == JOB_NONE) {
		return SET_ERROR(EINVAL);
	}

	*retval = job->aiocbp._errno;

	return error;
}

/*
 * Get return value of completed async I/O operation
 */
int aiosp_return(struct aiosp *aiosp, const void *uptr, register_t *retval)
{
	struct aiocbp *aiocbp = NULL;
	struct aio_job *job;
	int error;

	error = aiocbp_lookup(aiosp, &aiocbp, uptr);
	if (error) {
		return error;
	}
	job = aiocbp->job;

	if (job->aiocbp._errno == EINPROGRESS || job->aiocbp._state != JOB_DONE) {
		return SET_ERROR(EINVAL);
	}

	*retval = job->aiocbp._retval;

	job->aiocbp._errno = 0;
	job->aiocbp._retval = -1;
	job->aiocbp._state = JOB_NONE;

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
 * Find aiocb entry by user pointer.
 */
int
aiocbp_lookup(struct aiosp *aiosp, struct aiocbp **aiocbpp, const void *uptr)
{
	struct aiocbp *aiocbp;
	u_int hash;

	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;

	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH(aiocbp, &aiosp->aio_hash[hash], list) {
		if (aiocbp->uptr == uptr) {
			*aiocbpp = aiocbp;
			mutex_exit(&aiosp->aio_hash_mtx);
			return 0;
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);
	
	return ENOENT;
}

/*
 * Remove aiocb entry from hash table.
 */
int
aiocbp_remove(struct aiosp *aiosp, const void *uptr)
{
	struct aiocbp *aiocbp;
	u_int hash;

	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;

	struct aiocbp *tmp;
	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH_SAFE(aiocbp, &aiosp->aio_hash[hash], list, tmp) {
		if (aiocbp->uptr == uptr) {
			TAILQ_REMOVE(&aiosp->aio_hash[hash], aiocbp, list);
			mutex_exit(&aiosp->aio_hash_mtx);
			return 0;
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);
	
	return ENOENT;
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

	kmem_free(aiosp->aio_hash,
		aiosp->aio_hash_size * sizeof(*aiosp->aio_hash));
	aiosp->aio_hash = NULL;
	aiosp->aio_hash_mask = 0;
	aiosp->aio_hash_size = 0;
	mutex_exit(&aiosp->aio_hash_mtx);
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
 */
void
aiowaitgrouplk_fini(struct aiowaitgrouplk *lk)
{
	mutex_destroy(&lk->mtx);

	if (lk->s) {
		kmem_free(lk->wgs, sizeof(*lk->wgs) * lk->s);
	}
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

	/* Non-accurate check for the limit */
	if (aio_jobs_count + 1 > aio_max)
		return SET_ERROR(EAGAIN);

	/* Get the data structure from user-space */
	error = copyin(aiocb_uptr, &aiocb, sizeof(struct aiocb));
	if (error)
		return error;

	/* Check if signal is set, and validate it */
	sig = &aiocb.aio_sigevent;
	if (sig->sigev_signo < 0 || sig->sigev_signo >= NSIG ||
		sig->sigev_notify < SIGEV_NONE || sig->sigev_notify > SIGEV_SA)
		return SET_ERROR(EINVAL);

	/* Buffer and byte count */
	if (((AIO_SYNC | AIO_DSYNC) & op) == 0)
		if (aiocb.aio_buf == NULL || aiocb.aio_nbytes > SSIZE_MAX)
			return SET_ERROR(EINVAL);

	/* Check the opcode, if LIO_NOP - simply ignore */
	if (op == AIO_LIO) {
		KASSERT(lio != NULL);
		if (aiocb.aio_lio_opcode == LIO_WRITE)
			op = AIO_WRITE;
		else if (aiocb.aio_lio_opcode == LIO_READ)
			op = AIO_READ;
		else
			return (aiocb.aio_lio_opcode == LIO_NOP) ? 0 :
				SET_ERROR(EINVAL);
	} else {
		KASSERT(lio == NULL);
	}

	/*
	 * Look for already existing job.  If found - the job is in-progress.
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
	 * Check if AIO structure is initialized, if not - initialize it.
	 * In LIO case, we did that already.  We will recheck this with
	 * the lock in aio_procinit().
	 */
	if (lio == NULL && p->p_aio == NULL)
		if (aio_procinit(p))
			return SET_ERROR(EAGAIN);
	aio = p->p_aio;

	/*
	 * Set the state with errno, and copy data
	 * structure back to the user-space.
	 */
	aiocb._state = JOB_WIP;
	aiocb._errno = SET_ERROR(EINPROGRESS);
	aiocb._retval = -1;
	error = copyout(&aiocb, aiocb_uptr, sizeof(struct aiocb));
	if (error)
		return error;

	/* Allocate and initialize a new AIO job */
	a_job = pool_get(&aio_job_pool, PR_WAITOK | PR_ZERO);

	/*
	 * Set the data.
	 * Store the user-space pointer for searching.  Since we
	 * are storing only per proc pointers - it is safe.
	 */
	memcpy(&a_job->aiocbp, &aiocb, sizeof(struct aiocb));
	a_job->aiocb_uptr = aiocb_uptr;
	a_job->aio_op |= op;
	a_job->lio = lio;
	mutex_init(&a_job->mtx, MUTEX_DEFAULT, IPL_NONE);
	aiowaitgrouplk_init(&a_job->lk);

	/*
	 * Add the job to the queue, update the counters, and
	 * notify the AIO worker thread to handle the job.
	 */
	mutex_enter(&aio->aio_mtx);

	/* Fail, if the limit was reached */
	if (atomic_inc_uint_nv(&aio_jobs_count) > aio_max ||
		aio->jobs_count >= aio_listio_max) {
		atomic_dec_uint(&aio_jobs_count);
		mutex_exit(&aio->aio_mtx);
		pool_put(&aio_job_pool, a_job);
		return SET_ERROR(EAGAIN);
	}

	a_job->pri = PRI_KTHREAD;
	a_job->p = curlwp->l_proc;

	struct aiocbp *aiocbp = kmem_zalloc(sizeof(struct aiocbp), KM_SLEEP);
	aiocbp->job = a_job;
	aiocbp->uptr = aiocb_uptr;

	mutex_exit(&aio->aio_mtx);

	error = aiocbp_insert(&aio->aiosp, aiocbp);
	if (error) {
		return SET_ERROR(error);
	}

	error = aiosp_enqueue_job(&aio->aiosp, a_job);
	if (error) {
		return SET_ERROR(error);
	}

	mutex_enter(&aio->aio_mtx);
	aio->jobs_count++;
	if (lio)
		lio->refcnt++;
	mutex_exit(&aio->aio_mtx);

	/*
	 * One would handle the errors only with aio_error() function.
	 * This way is appropriate according to POSIX.
	 */
	return 0;
}

/*
 * Syscall functions.
 */

int
sys_aio_cancel(struct lwp *l, const struct sys_aio_cancel_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) fildes;
		syscallarg(struct aiocb *) aiocbp;
	} */

	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aiocb *aiocbp_uptr;
	struct filedesc	*fdp = p->p_fd;
	struct aiosp *aiosp;
	struct aio_job *job;
	unsigned int fildes;
	fdtab_t *dt;
	int error;

	fildes = (unsigned int)SCARG(uap, fildes);
	dt = atomic_load_consume(&fdp->fd_dt);
	if (fildes >= dt->dt_nfiles)
		return SET_ERROR(EBADF);
	if (dt->dt_ff[fildes] == NULL || dt->dt_ff[fildes]->ff_file == NULL)
		return SET_ERROR(EBADF);

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

	if (aiocbp_uptr) {
		struct aiocbp *aiocbp = NULL;
		error = aiocbp_lookup(aiosp, &aiocbp, aiocbp_uptr);
		if (error) {
			mutex_exit(&aiosp->mtx);
			mutex_exit(&aio->aio_mtx);
			return error;
		}
		if (aiocbp) {
			job = aiocbp->job;

			if (job->on_queue) {
				TAILQ_REMOVE(&aiosp->jobs, job, list);
				job->on_queue = false;

				mutex_enter(&job->mtx);
				aiowaitgrouplk_flush(&job->lk);
				job->completed = true;
				mutex_exit(&job->mtx);

				aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);

				*retval = AIO_CANCELED;
			} else {
				if (job->completed) {
					*retval = AIO_ALLDONE;
				} else {
					*retval = AIO_NOTCANCELED;
				}
			}

			mutex_exit(&aiosp->mtx);
			mutex_exit(&aio->aio_mtx);
			
			return 0;
		}
	}

	/* Cancel all jobs associated with this file handle */

	mutex_exit(&aiosp->mtx);
	mutex_exit(&aio->aio_mtx);

	return 0;
}

int
sys_aio_error(struct lwp *l, const struct sys_aio_error_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(const struct aiocb *) aiocbp;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;

	if (aio == NULL)
		return SET_ERROR(EINVAL);

	const void *uptr = SCARG(uap, aiocbp);
	return aiosp_error(&aio->aiosp, uptr, retval);
}

int
sys_aio_fsync(struct lwp *l, const struct sys_aio_fsync_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(struct aiocb *) aiocbp;
	} */
	int op = SCARG(uap, op);

	if ((op != O_DSYNC) && (op != O_SYNC))
		return SET_ERROR(EINVAL);

	op = O_DSYNC ? AIO_DSYNC : AIO_SYNC;

	return aio_enqueue_job(op, SCARG(uap, aiocbp), NULL);
}

int
sys_aio_read(struct lwp *l, const struct sys_aio_read_args *uap,
	register_t *retval)
{
	int error;
	error = aio_enqueue_job(AIO_READ, SCARG(uap, aiocbp), NULL);
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio = p->p_aio;
	error = aiosp_distribute_jobs(&aio->aiosp);
	return error;
}

int
sys_aio_return(struct lwp *l, const struct sys_aio_return_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(struct aiocb *) aiocbp;
	} */
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
	/* {
		syscallarg(const struct aiocb *const[]) list;
		syscallarg(int) nent;
		syscallarg(const struct timespec *) timeout;
	} */
	struct aiocb **list;
	struct timespec ts;
	int error, nent;

	nent = SCARG(uap, nent);
	if (nent <= 0 || nent > aio_listio_max)
		return SET_ERROR(EAGAIN);

	if (SCARG(uap, timeout)) {
		/* Convert timespec to ticks */
		error = copyin(SCARG(uap, timeout), &ts,
			sizeof(struct timespec));
		if (error)
			return error;
	}

	list = kmem_alloc(nent * sizeof(*list), KM_SLEEP);
	error = copyin(SCARG(uap, list), list, nent * sizeof(*list));
	if (error)
		goto out;

	struct proc *p = l->l_proc;
	struct aioproc *aio = p->p_aio;
	KASSERT(aio);
	error = aiosp_suspend(&aio->aiosp, list, nent, SCARG(uap, timeout) ?
		&ts : NULL, AIOSP_SUSPEND_ALL);
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
	struct proc *p = curlwp->l_proc;
	struct aioproc *aio = p->p_aio;
	KASSERT(aio);
	error = aiosp_distribute_jobs(&aio->aiosp);
	return error;
}

int
sys_lio_listio(struct lwp *l, const struct sys_lio_listio_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) mode;
		syscallarg(struct aiocb *const[]) list;
		syscallarg(int) nent;
		syscallarg(struct sigevent *) sig;
	} */
	struct proc *p = l->l_proc;
	struct aioproc *aio;
	struct aiocb **aiocbp_list;
	struct lio_req *lio;
	int i, error, errcnt, mode, nent;

	mode = SCARG(uap, mode);
	nent = SCARG(uap, nent);

	/* Non-accurate checks for the limit and invalid values */
	if (nent < 1 || nent > aio_listio_max)
		return SET_ERROR(EINVAL);
	if (aio_jobs_count + nent > aio_max)
		return SET_ERROR(EAGAIN);

	/* Check if AIO structure is initialized, if not - initialize it */
	if (p->p_aio == NULL)
		if (aio_procinit(p))
			return SET_ERROR(EAGAIN);
	aio = p->p_aio;

	/* Create a LIO structure */
	lio = pool_get(&aio_lio_pool, PR_WAITOK);
	lio->refcnt = 1;
	error = 0;

	switch (mode) {
	case LIO_WAIT:
		memset(&lio->sig, 0, sizeof(struct sigevent));
		break;
	case LIO_NOWAIT:
		/* Check for signal, validate it */
		if (SCARG(uap, sig)) {
			struct sigevent *sig = &lio->sig;

			error = copyin(SCARG(uap, sig), &lio->sig,
				sizeof(struct sigevent));
			if (error == 0 &&
				(sig->sigev_signo < 0 ||
				sig->sigev_signo >= NSIG ||
				sig->sigev_notify < SIGEV_NONE ||
				sig->sigev_notify > SIGEV_SA))
				error = SET_ERROR(EINVAL);
		} else
			memset(&lio->sig, 0, sizeof(struct sigevent));
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
		if (error)
			errcnt++;
	}

	error = aiosp_distribute_jobs(&aio->aiosp);
	if (error) {
		return error;
	}

	mutex_enter(&aio->aio_mtx);

	/* Return an error, if any */
	if (errcnt) {
		error = SET_ERROR(EIO);
		goto err;
	}

	if (mode == LIO_WAIT) {
		error = aiosp_suspend(&aio->aiosp, aiocbp_list, nent,
			NULL, AIOSP_SUSPEND_ALL);
	}

err:
	if (--lio->refcnt != 0)
		lio = NULL;
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
	if (error || newp == NULL)
		return error;

	if (newsize < 1 || newsize > aio_max)
		return SET_ERROR(EINVAL);
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
	if (error || newp == NULL)
		return error;

	if (newsize < 1 || newsize < aio_listio_max)
		return SET_ERROR(EINVAL);
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

	if (rv != 0)
		return;

	rv = sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "aio_listio_max",
		SYSCTL_DESCR("Maximum number of asynchronous I/O "
		"operations in a single list I/O call"),
		sysctl_aio_listio_max, 0, &aio_listio_max, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		return;

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
	(*pr)("AIO: sp{ total_threads=%zu active=%zu free=%zu pending=%zu processing=%lu hash_buckets=%zu mask=%#x }\n",
		sp->nthreads_total, sp->nthreads_active, sp->nthreads_free,
		sp->jobs_pending, (u_long)sp->njobs_processing,
		sp->aio_hash_size, sp->aio_hash_mask);

	/* Pending queue */
	(*pr)("\nqueue (%zu pending):\n", sp->jobs_pending);
	TAILQ_FOREACH(job, &sp->jobs, list) {
		(*pr)("  op=%d err=%d state=%d uptr=%p completed=%d\n",
			job->aio_op, job->aiocbp._errno, job->aiocbp._state,
			job->aiocb_uptr, job->completed);
		(*pr)("    fd=%d off=%llu buf=%p nbytes=%zu pri=%d lio=%p\n",
			job->aiocbp.aio_fildes,
			(unsigned long long)job->aiocbp.aio_offset,
			(void *)job->aiocbp.aio_buf,
			(size_t)job->aiocbp.aio_nbytes,
			(int)job->pri, job->lio);
	}

	/* Active service threads */
	(*pr)("\nactive threads (%zu):\n", sp->nthreads_active);
	{
		struct aiost *st;
		TAILQ_FOREACH(st, &sp->active, list) {
			(*pr)("  lwp=%p state=%d freelist=%d\n",
				(void *)st->lwp, st->state, st->freelist ? 1 : 0);

			if (st->job) {
				struct aio_job *j = st->job;
				(*pr)("    job: op=%d err=%d state=%d uptr=%p\n",
					j->aio_op, j->aiocbp._errno, j->aiocbp._state,
					j->aiocb_uptr);
				(*pr)("      fd=%d off=%llu buf=%p nbytes=%zu\n",
					j->aiocbp.aio_fildes,
					(unsigned long long)j->aiocbp.aio_offset,
					(void *)j->aiocbp.aio_buf,
					(size_t)j->aiocbp.aio_nbytes);
			}

			if (st->fg) {
				(*pr)("    file-group: vp=%p fp=%p qlen=%zu\n",
					(void *)st->fg->vp, (void *)st->fg->fp,
					st->fg->queue_size);
			}
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
				(*pr)(" uptr=%p job=%p", hc->uptr, (void *)hc->job);
			}
			(*pr)("\n");
		}
	}
}
#endif /* defined(DDB) */
