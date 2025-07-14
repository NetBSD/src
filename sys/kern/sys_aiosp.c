/*	$NetBSD: sys_aiosp.c,v 0.00 2025/05/18 12:00:00 ethan4984 Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * Implementation of service pools to support asynchronous I/O
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_aiosp.c,v 0.00 2025/05/18 12:00:00 ethan4984 Exp $");

#include <sys/param.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/hash.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/sdt.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/vnode.h>

MODULE(MODULE_CLASS_MISC, aiosp, NULL);

static size_t		aiosp_ops_expected(size_t);
static void		aiosp_ops_init(struct aiosp_ops *);
static void		aiosp_ops_fini(struct aiosp_ops *);
static int		aiosp_worker_extract(struct aiosp *, struct aiost **);

static int		aiost_create(struct aiosp *, struct aiost **);
static int		aiost_terminate(struct aiost *);
static int		aiost_process_rw(struct aiost *);
static int		aiost_process_sync(struct aiost *);
static void		aiost_entry(void *);
static void		aiost_sigsend(struct proc *, struct sigevent *);
static void		aiost_notify_ops (struct aio_job *);

/*
 * Module interface
 */
static int
aiosp_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	default:
		return SET_ERROR(ENOTTY);
	}
}

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
 * Group jobs by file handle for coalescing and distribute them among service
 * threads
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

		if (fg) {
			TAILQ_INSERT_TAIL(&fg->queue, job, list);
			fg->queue_size++;
		}

		aiost->freelist = false;
		aiost->state = AIOST_STATE_OPERATION;

		cv_signal(&aiost->service_cv);
	}

	mutex_exit(&sp->mtx);

	return error;
}

/*
 * aiosp_ops represent a collection of operations whose status should be
 * tracked. When the user invokes a suspend, we create a new collection, and
 * then for each aiost referenced within aiocbp_list, when those operations
 * are finished, every aiosp_ops appended to that thread (aiost->ops) gets
 * awoken and the completion count incremented. The completion counter can be
 * incremeneted posthumously as well.
 */
int
aiosp_suspend(struct aiosp *aiosp, struct aiocb **aiocbp_list, int nent,
	struct timespec *ts, uint32_t flags)
{
	struct aio_job *job;
	int error;
	int timo;
	size_t target = 0;

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

	if (flags & AIOSP_SUSPEND_ANY) {
		target = 1;
	} else if (flags & AIOSP_SUSPEND_ALL) {
		target = nent;
	} else if (flags & AIOSP_SUSPEND_N) {
		target = AIOSP_SUSPEND_NEXTRACT(flags);
	}

	struct aiosp_ops *ops = kmem_zalloc(sizeof(*ops), KM_SLEEP);
	aiosp_ops_init(ops);

	/*
	 * We want a hash table that tracks jobs, using uptr as a key. We use
	 * this to track job completion status. How do we handle the case where
	 * a job is completed with one aiost, then completed, then another job
	 * enqueued and assigned to that exact aiost. This makes it such that
	 * both aiosts are assigned to both threads.
	 */

	mutex_enter(&ops->mtx);
	for (int i = 0; i < nent; i++) {
		if (aiocbp_list[i] == NULL) {
			continue;
		}

		struct aiocbp *aiocbp = NULL;
		error = aiocbp_lookup(aiosp, &aiocbp, aiocbp_list[i]);
		if (error) {
			mutex_exit(&ops->mtx);
			aiosp_ops_fini(ops);
			kmem_free(ops, sizeof(*ops));
			return error;
		}
		if (aiocbp == NULL) {
			continue;
		}

		job = aiocbp->job;

		mutex_enter(&job->mtx);
		if (job->completed) {
			ops->completed++;
		}

		if (job->ops_total >= job->ops_size) {
			size_t old_size = job->ops_size;
			size_t new_size = job->ops_size == 0 ? 1 :
				(job->ops_size == 1 ? 2 :
				(job->ops_size * job->ops_size));

			struct aiosp_ops **new_ops = kmem_zalloc(new_size *
				sizeof(*new_ops), KM_SLEEP);

			if (job->ops && old_size) {
				memcpy(new_ops, job->ops, 
					job->ops_total * sizeof(*job->ops));
				kmem_free(job->ops, old_size *
					sizeof(*job->ops));
			}

			job->ops_size = new_size;
			job->ops = new_ops;
		}

		job->ops[job->ops_total] = ops;
		job->ops_total++;

		mutex_exit(&job->mtx);
		ops->total++;
	}

	for (; ops->completed < target;) {
		//printf("waiting on ops %ld %ld\n", ops->completed, target);
		error = cv_timedwait_sig(&ops->done_cv, &ops->mtx, timo);
		if (error) {
			if (error == EWOULDBLOCK) {
				error = SET_ERROR(EAGAIN);
			}

			mutex_exit(&ops->mtx);
			aiosp_ops_fini(ops);
			kmem_free(ops, sizeof(*ops));

			return error;
		}
	}

	mutex_exit(&ops->mtx);
	aiosp_ops_fini(ops);
	kmem_free(ops, sizeof(*ops));

	return error;
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
 * The size of aiost->ops scales with powers of two. The size of aiost->ops will
 * only either collapse to zero upon being terminated, or continue growing, so
 * scaling by a power of two is simple enough.
 */
static size_t
aiosp_ops_expected(size_t total)
{
	if (total <= 1) {
		return 1;
	}

	total -= 1;
	for (int j = 0; j < ilog2(sizeof(total) * 8); j++) {
		total |= total >> (1 << j);
	}
	total += 1;

	return total;
}

/*
 *
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
aiosp_destroy(struct aiosp *sp)
{
	struct aiost *st;
	struct aiost *tmp;
	int error = 0;

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

		kmem_free(st, sizeof(*st));
	}

	TAILQ_FOREACH_SAFE(st, &sp->active, list, tmp) {
		error = aiost_terminate(st);
		if (error) {
			mutex_exit(&sp->mtx);
			return error;
		}

		kmem_free(st, sizeof(*st));
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
 * wake up anyone waiting on the completion of this job
 */
static void
aiost_notify_ops (struct aio_job *job)
{
	for (int i = 0; i < job->ops_total; i++) {
		struct aiosp_ops *ops = job->ops[i];
		if (ops == NULL) {
			continue;
		}

		mutex_enter(&ops->mtx);
		KASSERT(ops->total > ops->completed);
		ops->completed++;
		mutex_exit(&ops->mtx);
		cv_signal(&ops->done_cv);
	}

	if (job->ops && job->ops_total) {
		size_t total = aiosp_ops_expected(job->ops_total);
		kmem_free(job->ops, total * sizeof(*job->ops));
		job->ops_total = 0;
		job->ops = NULL;
	}
}

/*
 * Servicing thread entry point. Process the operation. Notify all those
 * blocking on the completion of the operation. Send a signal if necessary. And
 * then mark the current servicing thread as free.
 */
static void
aiost_entry(void *arg)
{
	struct aiost *st = arg;
	struct aiosp *sp = st->aiosp;
	struct aio_job *job;
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

			struct aio_job *tmp;
			TAILQ_FOREACH_SAFE(job, &fg->queue, list, tmp) {
				if (job->aio_op & (AIO_READ | AIO_WRITE)) {
					// implement and call io_read/write
				} else if (job->aio_op & AIO_SYNC) {
					// implement and call io_sync
				}

				mutex_enter(&job->mtx);
				job->completed = true;
				mutex_exit(&job->mtx);

				aiost_notify_ops(job);
				aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);

				TAILQ_REMOVE(&fg->queue, job, list);
				fg->queue_size--;
			}

			mutex_enter(&sp->mtx);
			RB_REMOVE(aiost_file_tree, sp->fg_root, fg);
			closef(fg->fp);
			kmem_free(fg, sizeof(*fg));
			mutex_exit(&sp->mtx);
		} else {
			job = st->job;
			KASSERT(job != NULL);
			if (job->aio_op & (AIO_READ | AIO_WRITE)) {
				error = aiost_process_rw(st);
			} else if (job->aio_op & AIO_SYNC) {
				error = aiost_process_sync(st);
			} else {
				panic("aio_process: invalid operation code\n");
			}

			job->completed = true;

			aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);
			aiost_notify_ops(job);
		}

		st->state = AIOST_STATE_NONE;
		st->job = NULL;
		st->fg = NULL;

		/*
		 * Remove st from list of active service threads, append to
		 * freelist, dance around locks, then iterate loop and block on
		 * st->service_cv
		 */
		mutex_enter(&sp->mtx);

		st->freelist = true;

		TAILQ_REMOVE(&sp->active, st, list);
		sp->nthreads_active--;

		TAILQ_INSERT_TAIL(&sp->freelist, st, list);
		sp->nthreads_free++;

		mutex_exit(&sp->mtx);
	}

	if (st->fg) {
		struct aiost_file_group *fg = st->fg;

		struct aio_job *tmp;
		TAILQ_FOREACH_SAFE(job, &fg->queue, list, tmp) {
			mutex_enter(&job->mtx);
			job->completed = true;
			mutex_exit(&job->mtx);

			// CONFIRM WHETHER OR NOT THIS IS EXPECTED BEHAVIOUR
			aiost_notify_ops(job);
			aiost_sigsend(job->p, &job->aiocbp.aio_sigevent);

			TAILQ_REMOVE(&fg->queue, job, list);
			fg->queue_size--;
		}

		kmem_free(fg, sizeof(*fg)); 
	}

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
	mutex_exit(&st->mtx);
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
 * processes a read/write asynchronous operations
 */
static int
aiost_process_rw(struct aiost *aiost)
{
	struct aio_job *job = aiost->job;
	struct aiocb *aiocbp = &job->aiocbp;
	struct file *fp;
	int fd = aiocbp->aio_fildes;
	int error = 0;

	struct iovec aiov;
	struct uio auio;

	if (aiocbp->aio_nbytes > SSIZE_MAX) {
		error = SET_ERROR(EINVAL);
		goto done;
	}
	
	fp = fd_getfile2(job->p, fd);
	if (fp == NULL) {
		error = SET_ERROR(EBADF);
		goto done;
	}

	aiov.iov_base = aiocbp->aio_buf;
	aiov.iov_len = aiocbp->aio_nbytes;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = aiocbp->aio_nbytes;
	auio.uio_offset = aiocbp->aio_offset;
	auio.uio_vmspace = job->p->p_vmspace;

	if (job->aio_op & AIO_READ) {
		/*
		 * Perform a Read operation
		 */
		KASSERT((job->aio_op & AIO_WRITE) == 0);

		if ((fp->f_flag & FREAD) == 0) {
			closef(fp);
			error = SET_ERROR(EBADF);
			goto done;
		}
		auio.uio_rw = UIO_READ;
		error = (*fp->f_ops->fo_read)(fp, &aiocbp->aio_offset,
			&auio, fp->f_cred, FOF_UPDATE_OFFSET);
	} else {
		/*
		 * Perform a Write operation
		 */
		KASSERT(job->aio_op & AIO_WRITE);
	
		if ((fp->f_flag & FWRITE) == 0) {
			closef(fp);
			error = SET_ERROR(EBADF);
			goto done;
		}
		auio.uio_rw = UIO_WRITE;
		error = (*fp->f_ops->fo_write)(fp, &aiocbp->aio_offset,
			&auio, fp->f_cred, FOF_UPDATE_OFFSET);
	}
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
 * processes a sync/dsync asynchronous operations
 */
static int
aiost_process_sync(struct aiost *aiost)
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
 * Initialises aiosp_ops
 */
static void
aiosp_ops_init(struct aiosp_ops *ops)
{
	ops->completed = 0;
	ops->total = 0;
	cv_init(&ops->done_cv, "aiodone");
	mutex_init(&ops->mtx, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Cleans up aiosp_ops
 */
static void
aiosp_ops_fini(struct aiosp_ops *ops)
{
	cv_destroy(&ops->done_cv);
	mutex_destroy(&ops->mtx);
}

/*
 * Ensure that the same job can not be enqueued twice. 
 */
int
aiosp_validate_conflicts(struct aiosp *aiosp, void *uptr)
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
 * aiocbp hash function
 */
static inline u_int
aiocbp_hash(void *uptr)
{
	return hash32_buf(&uptr, sizeof(uptr), HASH32_BUF_INIT);
}

/*
 * aiocbp hash lookup
 */
int
aiocbp_lookup(struct aiosp *aiosp, struct aiocbp **aiocbpp, void *uptr)
{
	struct aiocbp *aiocbp;
	u_int hash;

	hash = aiocbp_hash(uptr) & aiosp->aio_hash_mask;

	//printf("searching element with key {%lx} and hash {%x}\n", (uintptr_t)uptr, hash);

	mutex_enter(&aiosp->aio_hash_mtx);
	TAILQ_FOREACH(aiocbp, &aiosp->aio_hash[hash], list) {
		if (aiocbp->uptr == uptr) {
			//printf("element found {%lx} and the job {%lx} {%lx}\n", (uintptr_t)aiocbp, (uintptr_t)aiocbp->job, (uintptr_t)aiocbp->job->aiost);

			*aiocbpp = aiocbp;
			mutex_exit(&aiosp->aio_hash_mtx);
			return 0;
		}
	}
	mutex_exit(&aiosp->aio_hash_mtx);
	
	return ENOENT;
}

/*
 * aiocbp hash removal
 */
int
aiocbp_remove(struct aiosp *aiosp, void *uptr)
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
 * aiocbp hash insertion
 */
int
aiocbp_insert(struct aiosp *aiosp, struct aiocbp *aiocbp)
{
	struct aiocbp *found;
	void *uptr;
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

	//printf("appending element with key {%x} onto hash {%lx} aiocbp {%lx}\n", hash, (uintptr_t)uptr, (uintptr_t)aiocbp);

	TAILQ_INSERT_HEAD(&aiosp->aio_hash[hash], aiocbp, list);
	mutex_exit(&aiosp->aio_hash_mtx);
	
	return 0;
}

/*
 * aiocbp initialise
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
 * aiocbp destroy
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
